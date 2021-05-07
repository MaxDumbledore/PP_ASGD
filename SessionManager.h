//
// Created by max on 2021/4/23.
//

#ifndef ASGD_SESSIONMANAGER_H
#define ASGD_SESSIONMANAGER_H

#include <seal/seal.h>
#include <set>
#include "Aggregator.h"
#include "Model.h"
#include "ParamsFormatter.h"
#include "Session.h"

/**
 * @note this class is not thread-safe
 * Attention! if we use multi-thread for the IO-context, we need to make
 * sessions thread-safe, and make finishedCount an automic Integer.
 */

class SessionManager {
   public:
    SessionManager();

    void insert(const SessionPtr& s);

    void startAll();

    void finishAll();

    void stop(const SessionPtr& s);

    ParamsFormatter& getFormatter();

    const seal::SEALContext& getContext() const;

    seal::Evaluator& getEvaluator();

    Aggregator& getAggregator();

   private:
    std::set<SessionPtr> sessions;
    ParamsFormatter formatter;
    Aggregator aggregator;

    seal::EncryptionParameters ckksParams;
    std::unique_ptr<seal::SEALContext> context;
    std::unique_ptr<seal::Evaluator> evaluator;
};

#endif  // ASGD_SESSIONMANAGER_H
