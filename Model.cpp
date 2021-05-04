//
// Created by max on 2021/4/23.
//

#include "Model.h"
#include "Constants.h"

std::vector<at::Tensor> Model::getNoGradParams() const {
    std::vector<at::Tensor> result;
    for (auto& i : net.parameters())
        result.emplace_back(i.detach().clone());
    return result;
}

void Model::setParams(const std::vector<at::Tensor>& _params) {
    torch::NoGradGuard ngg;
    int cur = 0;
    for (auto& m : net.parameters())
        m.copy_(_params[cur++]);
}

void Model::train(std::pair<int, torch::data::Iterator<Batch>> iter) {
    net.train();
    auto &data = iter.second->data, &target = iter.second->target;
    auto output = net.forward(data);
    auto loss = torch::nn::functional::cross_entropy(output, target);
    loss.backward();
    optimizer.step();
    optimizer.zero_grad();
}

Model::Model()
    : optimizer(net.parameters(),
                torch::optim::SGDOptions(LEARNING_RATE).momentum(MOMENTUM)) {}

int Model::matchCount(const TestLoader& testLoader) {
    torch::NoGradGuard noGrad;
    net.eval();
    int correct = 0;
    for (const auto& batch : *testLoader) {
        auto data = batch.data, target = batch.target;
        auto output = net.forward(data);
        correct +=
            torch::argmax(output, 1).eq(target).sum().template item<int>();
    }
    return correct;
}
