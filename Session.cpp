//
// Created by max on 2021/4/23.
//

#include "Session.h"
#include <iostream>
#include "SessionManager.h"
#include "Utils.h"

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
    buf = intToBytes(clientId) + floatVecToStream(manager.params().getData());
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
    buf.resize(manager.params().size() * 4 + 1);
    asio::async_read(socket, asio::buffer(buf),
                     [this, self, func = std::move(func)](
                         const asio::error_code& err, std::size_t) {
                         stepDebug(func, err);
                         if (err)
                             return;
                         manager.params().update(
                             streamToFloatVec(buf, manager.params().size()));
                         if (buf.back() == 'C')
                             sendParams();
                         else
                             manager.triggerFinish();
                     });
}

void Session::sendParams() {
    string func = __func__;
    auto self(shared_from_this());
    buf = floatVecToStream(manager.params().getData());
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
    buf = floatVecToStream(manager.params().getData());
    asio::async_write(socket, asio::buffer(buf),
                      [this, self, func = std::move(func)](
                          const asio::error_code& err, std::size_t) {
                          stepDebug(func, err);
                          manager.stop(self);
                      });
}