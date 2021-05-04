//
// Created by max on 2021/4/23.
//

#ifndef ASGD_MODEL_H
#define ASGD_MODEL_H


#include "MnistCNN.h"
#include "cxxabi.h"

template<typename T>
void getType(const T &x) {
#define quote(x) #x
    int status;
    char *temp = abi::__cxa_demangle(typeid(x).name(), nullptr, nullptr, &status);
    std::cerr << temp << "\t" << quote(x) << std::endl;
    free(temp);
}

using Batch = torch::data::Example<at::Tensor, at::Tensor>;
using Dataset = torch::data::datasets::MapDataset<torch::data::datasets::MapDataset<torch::data::datasets::MNIST, torch::data::transforms::Normalize<at::Tensor> >, torch::data::transforms::Stack<Batch> >;
using TrainSampler = torch::data::samplers::DistributedRandomSampler;
using TestSampler = torch::data::samplers::RandomSampler;
using TrainLoader = std::unique_ptr<torch::data::StatelessDataLoader<Dataset, TrainSampler>, std::default_delete<torch::data::StatelessDataLoader<Dataset, TrainSampler> > >;
using TestLoader = std::unique_ptr<torch::data::StatelessDataLoader<Dataset, TestSampler>, std::default_delete<torch::data::StatelessDataLoader<Dataset, TestSampler> > >;

class Model {
public:
    Model();

    void train(std::pair<int, torch::data::Iterator<Batch>> iter);

    std::vector<at::Tensor> getNoGradParams() const;

    void setParams(const std::vector<at::Tensor> &_params);

    int matchCount(const TestLoader &testLoader);

private:
    MnistCNN net;
    torch::optim::SGD optimizer;
};



#endif //ASGD_MODEL_H
