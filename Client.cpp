//
// Created by max on 2021/4/23.
//

#include "Client.h"
#include "Constants.h"
#include "Utils.h"
//#define DEBUG

Client::Client(const Dataset& trainSet, asio::io_context& ioContext)
    : trainSet(trainSet),
      id(-1),
      sslContext(asio::ssl::context::sslv23),
      iter(-1, nullptr) {
    builder.setData(model.getNoGradParams());
    for (auto& i : model.getNoGradParams())
        dims.emplace_back(i.sizes().vec());
    sslContext.use_certificate_chain_file(CERT_PATH + "client.pem");
    sslContext.use_private_key_file(CERT_PATH + "client.pem",
                                    asio::ssl::context::pem);
    sslContext.load_verify_file(CERT_PATH + "ca.pem");
    socket = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket>>(
        ioContext, sslContext);
    socket->set_verify_mode(asio::ssl::verify_peer);
}

bool Client::makeTrainLoader() {
    if (id < 0 || id >= PARTICIPATE_COUNT)
        return false;
    trainLoader = torch::data::make_data_loader<Dataset, TrainSampler>(
        trainSet, TrainSampler(trainSet.size().value(), PARTICIPATE_COUNT, id),
        TRAIN_BATCH_SIZE);
    iter = {0, trainLoader->begin()};
    return true;
}

bool Client::iterateOneBatch() {
    if (finished())
        return false;
    lastParams = model.getNoGradParams();
    model.train(iter);

    //    std::clog << __func__ << ":Epoch " << iter.first << std::endl;

    nextIter();
    return true;
}

void Client::nextIter() {
    if (++iter.second == trainLoader->end()) {
        iter.second = trainLoader->begin();
        ++iter.first;
    }
}

bool Client::finished() const {
    return iter.first == EPOCHS;
}

std::vector<at::Tensor> Client::getCurrentUpdate() const {
    auto params = model.getNoGradParams();
    std::vector<at::Tensor> result(params.size());
    for (int i = 0; i < result.size(); i++)
        result[i] = params[i] - lastParams[i];
    return result;
}

void stepDebug(const std::string& func, const asio::error_code& err) {
#ifdef DEBUG
    if (err)
        std::cerr << func + " failed: " << err.message() << std::endl;
    else
        std::clog << func + " succeeded!" << std::endl;
#endif
}

void Client::connect(
    const asio::ip::basic_resolver<asio::ip::tcp,
                                   asio::any_io_executor>::results_type&
        endpoints) {
    asio::error_code err;
    asio::connect(socket->lowest_layer(), endpoints, err);
    stepDebug(__func__, err);
}

void Client::start(Dataset* testSet) {
    if (handshake() && receiveIdAndInitialParams()) {
        int cnt = 0;
        do {
            if (++cnt % 100 == 0) {
                std::clog << "------" << cnt << "------" << std::endl;
                if (testSet)
                    std::clog << "Correctness: " << test(*testSet) << std::endl;
            }
        } while (iterateOneBatch() && sendUpdate() && receiveParams());
    }
    socket->shutdown();
}

double Client::test(const Dataset& testSet) {
    TestLoader testLoader(
        torch::data::make_data_loader(testSet, TEST_BATCH_SIZE));
    return (double)model.matchCount(testLoader) / testSet.size().value();
}

bool Client::handshake() {
    asio::error_code err;
    socket->handshake(asio::ssl::stream_base::client, err);
    stepDebug(__func__, err);
    return !err.operator bool();
}

bool Client::receiveIdAndInitialParams() {
    asio::error_code err;
    buf.resize(4 + builder.size() * 4);
    asio::read(*socket, asio::buffer(buf), err);
    stepDebug(__func__, err);
    id = bytesToInt(buf.substr(0, 4));
    makeTrainLoader();
    builder.setData(streamToFloatVec(buf, builder.size(), 4));
    model.setParams(builder.getData(dims));
    return !err.operator bool();
}

bool Client::sendUpdate() {
    asio::error_code err;
    builder.setData(getCurrentUpdate());
    buf = floatVecToStream(builder.getData()) + (finished() ? 'S' : 'C');
    asio::write(*socket, asio::buffer(buf), err);
    stepDebug(__func__, err);
    return !err.operator bool();
}

bool Client::receiveParams() {
    asio::error_code err;
    buf.resize(builder.size() * 4);
    asio::read(*socket, asio::buffer(buf), err);
    stepDebug(__func__, err);
    builder.setData(streamToFloatVec(buf, builder.size()));
    model.setParams(builder.getData(dims));
    return !err.operator bool();
}