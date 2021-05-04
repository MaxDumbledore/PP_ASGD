//
// Created by 40461 on 2021/4/29.
//

#include "Params.h"

void Params::update(const std::vector<float> &delta) {
//    std::unique_lock<std::shared_mutex> lock(mutex);
    for (int i = 0; i < params.size(); i++)
        params[i] += delta[i];
}

std::vector<float> Params::getData() const {
//    std::shared_lock<std::shared_mutex> lock(mutex);
    return params;
}

std::vector<at::Tensor> Params::getData(const std::vector<std::vector<int64_t>> &dims) const {
    auto temp = getData();
    std::vector<at::Tensor> result;
    result.reserve(dims.size());
    int cur = 0;
    for (auto &dim:dims) {
        result.emplace_back(at::from_blob(temp.data() + cur, dim).clone().detach());
        cur += (int) result.back().numel();
    }
    return result;
}

void Params::setData(std::vector<at::Tensor> &&_params) {
    std::vector<float> temp;
    for (auto &t:_params)
        temp.insert(temp.end(), t.data_ptr<float>(), t.data_ptr<float>() + t.numel());
    setData(std::move(temp));
}

void Params::setData(std::vector<float> &&_params) {
//    std::unique_lock<std::shared_mutex> lock(mutex);
    params = std::move(_params);
}

int Params::size() const{
    return params.size();
}
