//
// Created by 40461 on 2021/4/29.
//

#ifndef ASGD_PARAMS_H
#define ASGD_PARAMS_H

#include <torch/torch.h>
//#include <shared_mutex>

/**
 * @note for multi-thread
 * if we use multi-thread for the IO-context, we need to use read-write mutex.
 */

class Params {
   public:
    Params() = default;

    std::vector<at::Tensor> getData(
        const std::vector<std::vector<int64_t>>& dims) const;

    std::vector<float> getData() const;

    void setData(std::vector<float>&& _params);

    void setData(std::vector<at::Tensor>&& _params);

    void update(const std::vector<float>& delta);

    int size() const;

   private:
    //    mutable std::shared_mutex mutex;

    std::vector<float> params;
};

#endif  // ASGD_PARAMS_H
