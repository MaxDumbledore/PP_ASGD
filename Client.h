//
// Created by max on 2021/4/23.
//

#ifndef ASGD_CLIENT_H
#define ASGD_CLIENT_H

#include <asio.hpp>
#include <asio/ssl.hpp>
#include "Model.h"
#include "Params.h"

class Client {
   public:
    Client(const Dataset& trainSet,
           asio::io_context& ioContext);

    void connect(const asio::ip::tcp::resolver::results_type& endpoints);

    void start(Dataset* testSet = nullptr);

    double test(const Dataset& testSet);

   private:
    int id;
    const Dataset& trainSet;
    TrainLoader trainLoader;
    std::pair<int, torch::data::Iterator<Batch>> iter;
    Model model;
    Params builder;
    std::vector<at::Tensor> lastParams;

    asio::ssl::context sslContext;
    std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket>> socket;
    std::string buf;
    std::vector<std::vector<int64_t>> dims;

    bool makeTrainLoader();

    bool iterateOneBatch();

    void nextIter();

    bool finished() const;

    std::vector<at::Tensor> getCurrentUpdate() const;

    bool handshake();

    bool receiveIdAndInitialParams();

    bool sendUpdate();

    bool receiveParams();
};

#endif  // ASGD_CLIENT_H
