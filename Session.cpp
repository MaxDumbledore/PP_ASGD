//
// Created by max on 2021/4/23.
//

#include "Session.h"
#include <iostream>
#include "Constants.h"
#include "SessionManager.h"
#include "Utils.h"
//#define DEBUG

using namespace std;

Session::Session(int clientId,
                 asio::ssl::stream<asio::ip::tcp::socket> socket,
                 SessionManager& manager)
    : clientId(clientId), socket(std::move(socket)), manager(manager) {}

void Session::stepDebug(const std::string& func, const asio::error_code& err) {
#ifdef DEBUG
    std::clog << "With Client " << clientId << ": " << func;
    if (err)
        std::clog << " failed because " << err.message() << std::endl;
    else
        std::clog << " succeeded!" << std::endl;
#endif
}

void Session::start() {
    handshake();
}

void Session::handshake() {
    string func = __func__;
    auto self(shared_from_this());
    socket.async_handshake(
        asio::ssl::stream_base::server,
        [this, self, func = std::move(func)](const asio::error_code& err) {
            stepDebug(func, err);
            if (!err)
                sendIdAndInitialParams();
        });
}

void Session::stop() {
    socket.shutdown();
}

void Session::sendIdAndInitialParams() {
    string func = __func__;
    auto self(shared_from_this());
    buf = intToBytes(clientId) +
          floatVecToStream(manager.getFormatter().getData());
    asio::async_write(socket, asio::buffer(buf),
                      [this, self, func = std::move(func)](
                          const asio::error_code& err, std::size_t) {
                          stepDebug(func, err);
                          if (!err)
                              receiveUpdate();
                      });
}

void Session::receiveUpdate() {
    string func = __func__;
    auto self(shared_from_this());
    buf.clear();
    asio::async_read_until(
        socket, asio::dynamic_buffer(buf), FIN_FLAG,
        [this, self, func = std::move(func)](const asio::error_code& err,
                                             std::size_t) {
            stepDebug(func, err);
            if (err)
                return;
            stringstream ss(buf.substr(0, buf.size() - FIN_FLAG.size() - 1));
            std::vector<seal::Ciphertext> ciphers;
            while (ss.rdbuf()->in_avail()) {
                ciphers.emplace_back();
                ciphers.back().load(manager.getContext(), ss);
            }

                // ifstream ifs(CKKS_PATH+"secKey.txt",ios::binary);
                // seal::SecretKey secKey;
                // seal::Plaintext plain;
                // secKey.load(manager.getContext(),ifs);
                // seal::Decryptor decryptor(manager.getContext(),secKey);
                // decryptor.decrypt(ciphers[0],plain);
                // seal::CKKSEncoder encoder(manager.getContext());
                // vector<double> db;
                // encoder.decode(plain,db);
                // std::clog<<"rcv:"<<db[0]<<std::endl;

            asyncUpdate(std::move(ciphers));
            if (buf[buf.size() - FIN_FLAG.size() - 1] == 'C')
                sendParams();
        });
}

void Session::asyncUpdate(std::vector<seal::Ciphertext>&& ciphers) {
    string func = __func__;
    auto self(shared_from_this());
    asio::post(socket.get_executor(), [this, self, func = std::move(func),
                                       ciphers = std::move(ciphers)] {
        manager.getAggregator().update(manager.getEvaluator(), ciphers);

            // ifstream ifs(CKKS_PATH+"secKey.txt",ios::binary);
            // seal::SecretKey secKey;
            // seal::Plaintext plain;
            // secKey.load(manager.getContext(),ifs);
            // seal::Decryptor decryptor(manager.getContext(),secKey);
            // decryptor.decrypt(manager.getAggregator().getData()[0],plain);
            // seal::CKKSEncoder encoder(manager.getContext());
            // vector<double> db;
            // encoder.decode(plain,db);
            // std::clog<<"sum:"<<db[0]<<std::endl;

        stepDebug(func, asio::error_code());
    });
}

void Session::sendParams() {
    string func = __func__;
    auto self(shared_from_this());
    stringstream ss;
    for (auto& i : manager.getAggregator().getData())
        i.save(ss);
    buf = ss.str() + FIN_FLAG;
    asio::async_write(socket, asio::buffer(buf),
                      [this, self, func = std::move(func)](
                          const asio::error_code& err, std::size_t) {
                          stepDebug(func, err);
                          if (!err)
                              receiveUpdate();
                      });
}

void Session::sendFinalParams() {
    string func = __func__;
    auto self(shared_from_this());
    stringstream ss;
    for (auto& i : manager.getAggregator().getData())
        i.save(ss);
    buf = ss.str() + FIN_FLAG;
    asio::async_write(socket, asio::buffer(buf),
                      [this, self, func = std::move(func)](
                          const asio::error_code& err, std::size_t) {
                          stepDebug(func, err);
                          manager.stop(self);
                      });
}