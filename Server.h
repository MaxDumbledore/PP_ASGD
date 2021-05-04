//
// Created by max on 2021/4/23.
//

#ifndef ASGD_SERVER_H
#define ASGD_SERVER_H

#include <asio.hpp>
#include <asio/ssl.hpp>
#include "SessionManager.h"

/**
 * @note this class is not thread-safe
 * Attention! if we use multi-thread for the IO-context, we need to and make
 * startedCount an automic Integer.
 */

class Server {
   public:
    Server(asio::io_context& ioContext, uint16_t port);

   private:
    asio::ip::tcp::acceptor tcpAcceptor;
    asio::ssl::context sslContext;
    SessionManager sessionManager;
    int startedCount;
    void doAccept();
};

#endif  // ASGD_SERVER_H
