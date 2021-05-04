cmake_minimum_required(VERSION 3.16)
project(ASGD)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH /home/max/libtorch/ /home/max/SEAL/build)
include_directories(/home/max/asio-1.18.1/include)
find_package(Torch REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(SEAL REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")

add_executable(ASGD main.cpp Constants.cpp Constants.h MnistCNN.cpp MnistCNN.h Model.cpp Model.h Server.cpp Server.h Client.cpp Client.h SessionManager.cpp SessionManager.h Session.cpp Session.h Params.cpp Params.h Utils.cpp Utils.h)
target_link_libraries(ASGD PUBLIC "${TORCH_LIBRARIES}"  OpenSSL::SSL SEAL::seal)