//
// Created by 40461 on 2021/4/29.
//

#include <arpa/inet.h>
#include <cstring>
#include "Utils.h"

union Change {
    float g;
    uint32_t d;
    char data[4];
};

std::string floatToBytes(const float& x) {
    Change t;
    t.g = x;
    t.d = htonl(t.d);
    return std::string(t.data, t.data + 4);
}

float bytesToFloat(const std::string& s) {
    Change t;
    std::memcpy(t.data, s.c_str(), 4);
    t.d = ntohl(t.d);
    return t.g;
}

std::string intToBytes(const int& x) {
    Change t;
    t.d = htonl(x);
    return std::string(t.data, t.data + 4);
}

int bytesToInt(const std::string& s) {
    Change t;
    std::memcpy(t.data, s.c_str(), 4);
    return ntohl(t.d);
}

std::vector<float> streamToFloatVec(const std::string& stream,
                                    int sz,
                                    int start) {
    std::vector<float> result;
    result.reserve(sz);
    int end = start + sz * 4;
    for (int i = start; i < end; i += 4)
        result.emplace_back(bytesToFloat(stream.substr(i, 4)));
    return result;
}

std::string floatVecToStream(const std::vector<float>& v) {
    std::string result;
    result.reserve(v.size() * 4);
    for (auto i : v)
        result += floatToBytes(i);
    return result;
}
