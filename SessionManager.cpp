//
// Created by max on 2021/4/23.
//

#include "SessionManager.h"

void SessionManager::insert(const SessionPtr& s) {
    sessions.insert(s);
}

void SessionManager::startAll() {
    for (auto& i : sessions)
        i->start();
}

void SessionManager::stop(const SessionPtr& s) {
    s->stop();
    sessions.erase(s);
}

SessionManager::SessionManager() : finishedCount() {
    globalParams.setData(MnistCNN().parameters());
}

Params& SessionManager::params() {
    return globalParams;
}

void SessionManager::triggerFinish(){
    if(++finishedCount==sessions.size())
        for(auto &i:sessions)
            i->sendFinalParams();
}