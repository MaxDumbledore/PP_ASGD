#include "Client.h"
#include "Constants.h"
#include "Utils.h"
#include <random>
//#define DEBUG

Client::Client(const Dataset& trainSet, asio::io_context& ioContext)
    : trainSet(trainSet),
      id(-1),
      sslContext(asio::ssl::context::sslv23),
      iter(-1, nullptr) {
    formatter.setData(model.getNoGradParams());
    for (auto& i : model.getNoGradParams())
        dims.emplace_back(i.sizes().vec());

    sslContext.use_certificate_chain_file(CERT_PATH + "client.pem");
    sslContext.use_private_key_file(CERT_PATH + "client.pem",
                                    asio::ssl::context::pem);
    sslContext.load_verify_file(CERT_PATH + "ca.pem");
    socket = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket>>(
        ioContext, sslContext);
    socket->set_verify_mode(asio::ssl::verify_peer);

    auto ifs = std::ifstream(CKKS_PATH + "params.txt", std::ios::binary);
    ckksParams.load(ifs);
    context = std::make_unique<seal::SEALContext>(ckksParams);
    ifs = std::ifstream(CKKS_PATH + "pubKey.txt", std::ios::binary);
    pubKey.load(*context, ifs);
    ifs = std::ifstream(CKKS_PATH + "secKey.txt", std::ios::binary);
    secKey.load(*context, ifs);
    encryptor = std::make_unique<seal::Encryptor>(*context, pubKey, secKey);
    decryptor = std::make_unique<seal::Decryptor>(*context, secKey);
    encoder = std::make_unique<seal::CKKSEncoder>(*context);
}

std::vector<float> Client::convertCipherToFloatVec(std::string&& s) {
    std::stringstream ss(s);
    std::vector<float> ans;
    while (ss.rdbuf()->in_avail()) {
        cipher.load(*context, ss);
        decryptor->decrypt(cipher, plain);
        encoder->decode(plain, result);
        ans.reserve(ans.size() + result.size());
        for (auto& i : result)
            ans.emplace_back((float)i);
    }
    return ans;
}

std::string Client::convertFloatVecToCipher(std::vector<float>&& t) {
    std::stringstream ss;
    // std::clog<<std::max_element(t.begin(),t.end())<<std::endl;
    for (int i = 0; i < t.size(); i += SLOTS) {
        encoder->encode(
            std::vector<double>(t.begin() + i,
                                std::min(t.end(), t.begin() + i + SLOTS)),
            SCALE, plain);
        encryptor->encrypt_symmetric(plain).save(ss);
    }
    return ss.str();
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

//#define RANDOM_DELAY

void Client::start(Dataset* testSet) {
    clock_t start = clock(), g = 0;
    if (handshake() && receiveIdAndInitialParams()) {
        int cnt = 0;
        #ifdef RANDOM_DELAY
        static std::random_device rd;
        static std::mt19937 e(rd());
        std::uniform_int_distribution<int> dist(0, 100);
        std::this_thread::sleep_for(
            std::chrono::milliseconds(dist(e)));  //休眠100毫秒
#endif
        do {
            if (++cnt % 50 == 0) {
                std::clog << "------" << cnt << "------" << std::endl;
                if (testSet) {
                    auto d = clock();
                    std::clog << "Correctness: " << test(*testSet) << std::endl;
                    g += (clock() - d);
                }
                std::clog << "TimeCost: "
                          << (double)(clock() - start - g) / CLOCKS_PER_SEC
                          << std::endl;
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
    buf.resize(4 + formatter.size() * 4);
    //   asio::read_until(*socket, asio::dynamic_buffer(buf), FIN_FLAG, err);

    asio::read(*socket, asio::buffer(buf), err);
    stepDebug(__func__, err);
    id = bytesToInt(buf.substr(0, 4));
    makeTrainLoader();
    formatter.setData(streamToFloatVec(buf, formatter.size(), 4));
    model.setParams(formatter.getData(dims));
    return !err.operator bool();
}

bool Client::sendUpdate() {
    asio::error_code err;
    formatter.setData(getCurrentUpdate());
    // std::clog<<"cur:"<<formatter.getData()[0]<<std::endl;
    buf = convertFloatVecToCipher(formatter.getData()) +
          (finished() ? 'S' : 'C') + FIN_FLAG;
    asio::write(*socket, asio::buffer(buf), err);
    stepDebug(__func__, err);
    return !err.operator bool();
}

bool Client::receiveParams() {
    asio::error_code err;
    buf.clear();
    asio::read_until(*socket, asio::dynamic_buffer(buf), FIN_FLAG, err);
    stepDebug(__func__, err);
    formatter.setData(
        convertCipherToFloatVec(buf.substr(0, buf.size() - FIN_FLAG.size())));
    model.setParams(formatter.getData(dims));
    //   std::clog<<"new:"<<model.getNoGradParams()[0][0][0]<<std::endl;

    return !err.operator bool();
}