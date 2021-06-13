#include <asio.hpp>
#include <iostream>
#include "Client.h"
#include "Constants.h"
#include "Server.h"

int main(int argc, char* argv[]) {
    torch::set_num_threads(1);
    try {
        if (argc <= 1 || (strcmp(argv[1], "-c") && strcmp(argv[1], "-s") &&
                          strcmp(argv[1], "-k"))) {
            std::cerr << argv[1];
            std::cerr << "Options: -s(server) or -c(client)" << std::endl;
            return 1;
        }
        if (argv[1][1] == 'k') {
            seal::EncryptionParameters ckksParams(seal::scheme_type::ckks);
            ckksParams.set_poly_modulus_degree(POLY_MODULUS_DEGREE);
            ckksParams.set_coeff_modulus(
                seal::CoeffModulus::Create(POLY_MODULUS_DEGREE, {SCALE_P}));
            std::ofstream ofsP(CKKS_PATH + "params.txt", std::ios::binary);
            std::ofstream ofsSK(CKKS_PATH + "secKey.txt", std::ios::binary);
            std::ofstream ofsPK(CKKS_PATH + "pubKey.txt", std::ios::binary);
            seal::SEALContext context(ckksParams);
            seal::KeyGenerator keygen(context);
            ckksParams.save(ofsP);
            const auto& sk = keygen.secret_key();
            sk.save(ofsSK);
            auto pk = keygen.create_public_key();
            pk.save(ofsPK);
        } else {
            asio::io_context ioContext;
            if (argv[1][1] == 'c') {
                if (argc != 4) {
                    std::cerr << "Usage: -c <host> <port>\n";
                    return 1;
                }
                auto trainSet(torch::data::datasets::MNIST(DATA_PATH)
                                  .map(torch::data::transforms::Normalize<>(
                                      0.1307, 0.3081))
                                  .map(torch::data::transforms::Stack<>()));
                auto testSet(
                    torch::data::datasets::MNIST(
                        DATA_PATH, torch::data::datasets::MNIST::Mode::kTest)
                        .map(torch::data::transforms::Normalize<>(0.1307,
                                                                  0.3081))
                        .map(torch::data::transforms::Stack<>()));

                std::cout << "Train Data Size: " << trainSet.size().value()
                          << std::endl;
                std::cout << "Test Data Size: " << testSet.size().value()
                          << std::endl;

                asio::ip::tcp::resolver resolver(ioContext);
                auto endpoints = resolver.resolve(argv[2], argv[3]);
                Client client(trainSet, ioContext);
                client.connect(endpoints);

                auto startTime = std::clock();

                client.start(&testSet);
                std::clog << "Final Correctness: " << client.test(testSet)
                          << std::endl;
                std::clog << "Used Time:"
                          << (double)(std::clock() - startTime) / CLOCKS_PER_SEC
                          << std::endl;
            } else {
                if (argc != 3) {
                    std::cerr << "Usage: -s <port>\n";
                    return 1;
                }
                Server(ioContext, std::stoi(argv[2]));
            }
        }
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
