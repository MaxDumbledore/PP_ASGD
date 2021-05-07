#include <asio.hpp>
#include <iostream>
#include "Client.h"
#include "Constants.h"
#include "Server.h"

int main(int argc, char* argv[]) {
    try {
        if (argc <= 1 || (strcmp(argv[1], "-c") && strcmp(argv[1], "-s"))) {
            std::cerr << argv[1];
            std::cerr << "Options: -s(server) or -c(client)" << std::endl;
            return 1;
        }
        asio::io_context ioContext;
        if (argv[1][1] == 'c') {
            if (argc != 4) {
                std::cerr << "Usage: -c <host> <port>\n";
                return 1;
            }
            auto trainSet(
                torch::data::datasets::MNIST(DATA_PATH)
                    .map(torch::data::transforms::Normalize<>(0.1307, 0.3081))
                    .map(torch::data::transforms::Stack<>()));
            auto testSet(
                torch::data::datasets::MNIST(
                    DATA_PATH, torch::data::datasets::MNIST::Mode::kTest)
                    .map(torch::data::transforms::Normalize<>(0.1307, 0.3081))
                    .map(torch::data::transforms::Stack<>()));

            std::cout << "Train Data Size: " << trainSet.size().value()
                      << std::endl;
            std::cout << "Test Data Size: " << testSet.size().value()
                      << std::endl;

            asio::ip::tcp::resolver resolver(ioContext);
            auto endpoints = resolver.resolve(argv[2], argv[3]);
            auto client = new Client(trainSet, ioContext);
            client->connect(endpoints);

            auto startTime = std::clock();

            client->start(&testSet);
            std::clog << "Final Correctness: " << client->test(testSet)
                      << std::endl;
            std::clog << "Used Time:"
                      << (double)(std::clock() - startTime) / CLOCKS_PER_SEC
                      << std::endl;
        } else {
            if (argc != 3) {
                std::cerr << "Usage: -s <port>\n";
                return 1;
            }
            new Server(ioContext, std::stoi(argv[2]));
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
