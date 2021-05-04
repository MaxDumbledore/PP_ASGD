//
// Created by max on 2021/4/23.
//

#ifndef ASGD_SESSION_H
#define ASGD_SESSION_H

#include <asio.hpp>
#include <asio/ssl.hpp>

class SessionManager;

class Session : public std::enable_shared_from_this<Session> {
   public:
    Session(int clientId,
            asio::ssl::stream<asio::ip::tcp::socket> socket,
            SessionManager& manager);

    void start();

    void stop();

    void sendFinalParams();

   private:
    int clientId;
    asio::ssl::stream<asio::ip::tcp::socket> socket;
    SessionManager& manager;
    std::string buf;

    void handshake();

    void sendIdAndInitialParams();

    void receiveUpdate();
    
    void sendParams();

    void stepDebug(const std::string& func, const asio::error_code& err);
};

using SessionPtr = std::shared_ptr<Session>;

#endif  // ASGD_SESSION_H
