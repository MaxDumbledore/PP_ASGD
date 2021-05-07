//
// Created by max on 2021/4/23.
//

#include "Constants.h"
#include <cmath>

const std::string DATA_PATH = "/home/max/MNIST_DATA";
const std::string CERT_PATH = "/home/max/ssl/";
const int PARTICIPATE_COUNT = 3;
const int TRAIN_BATCH_SIZE = 64;
const int TEST_BATCH_SIZE = 1024;
const int EPOCHS = 1;
const double LEARNING_RATE = 0.01;
const double MOMENTUM = 0.5;

const size_t POLY_MODULUS_DEGREE = 32768;
const size_t SLOTS=POLY_MODULUS_DEGREE/2;
const double SCALE = std::pow(2.0, 30);
const std::string CKKS_PATH="/home/max/ckks/";
const std::string FIN_FLAG="!!!!!!";