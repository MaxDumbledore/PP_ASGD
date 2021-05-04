//
// Created by max on 2021/4/23.
//

#include "Server.h"
#include <iostream>
#include "Constants.h"

using namespace std;

Server::Server(asio::io_context& ioContext, uint16_t port)
    : tcpAcceptor(ioContext,
                  asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
      sslContext(asio::ssl::context::sslv23),
      startedCount() {
    sslContext.set_options(asio::ssl::context::default_workarounds |
                           asio::ssl::context::no_sslv2);
    sslContext.use_certificate_chain_file(CERT_PATH + "server.pem");
    sslContext.use_private_key_file(CERT_PATH + "server.pem",
                                    asio::ssl::context::pem);
    sslContext.set_verify_mode(asio::ssl::verify_peer |
                               asio::ssl::verify_fail_if_no_peer_cert);
    sslContext.load_verify_file(CERT_PATH + "ca.pem");
    doAccept();
}

void Server::doAccept() {
    for (int curId = 0; curId < PARTICIPATE_COUNT; curId++) {
        tcpAcceptor.async_accept([this, curId](const asio::error_code& err,
                                               asio::ip::tcp::socket socket) {
            if (!err) {
                sessionManager.insert(std::make_shared<Session>(
                    curId,
                    asio::ssl::stream<asio::ip::tcp::socket>(std::move(socket),
                                                             sslContext),
                    sessionManager));
                if (++startedCount == PARTICIPATE_COUNT)
                    sessionManager.startAll();
            } else
                std::cerr << err.message() << std::endl;
        });
    }
}