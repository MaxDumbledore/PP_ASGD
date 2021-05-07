#include "SessionManager.h"
#include "Constants.h"

void SessionManager::insert(const SessionPtr& s) {
    sessions.insert(s);
}

void SessionManager::startAll() {
    for (auto& i : sessions)
        i->start();
}

void SessionManager::finishAll() {
    for (auto& i : sessions)
        i->sendFinalParams();
}

void SessionManager::stop(const SessionPtr& s) {
    s->stop();
    sessions.erase(s);
}

ParamsFormatter& SessionManager::getFormatter() {
    return formatter;
}

const seal::SEALContext& SessionManager::getContext() const {
    return *context;
}

seal::Evaluator& SessionManager::getEvaluator() {
    return *evaluator;
}

Aggregator& SessionManager::getAggregator() {
    return aggregator;
}

SessionManager::SessionManager() {
    formatter.setData(MnistCNN().parameters());

    auto ifs = std::ifstream(CKKS_PATH + "params.txt", std::ios::binary);
    ckksParams.load(ifs);
    context = std::make_unique<seal::SEALContext>(ckksParams);
    evaluator = std::make_unique<seal::Evaluator>(*context);

    auto t = formatter.getData();

    seal::PublicKey pubKey;
    ifs = std::ifstream(CKKS_PATH + "pubKey.txt", std::ios::binary);
    pubKey.load(*context, ifs);
    seal::CKKSEncoder encoder(*context);
    seal::Encryptor encryptor(*context,pubKey);
    std::vector<seal::Ciphertext> ciphers;
    for (int i = 0; i < t.size(); i += SLOTS) {
        seal::Plaintext plain;
        ciphers.emplace_back();
        encoder.encode(
            std::vector<double>(t.begin() + i,
                                std::min(t.end(), t.begin() + i + SLOTS)),
            SCALE, plain);
        encryptor.encrypt(plain,ciphers.back());
    }
    aggregator.setData(std::move(ciphers));
}
