//
// Created by 周慰星 on 23-3-30.
//

#ifndef TAAS_WORKER_H
#define TAAS_WORKER_H

#include "tools/context.h"

namespace Taas {
    extern void StateChecker(const Context& ctx);
    extern void WorkerThreadMain(const Context& ctx, uint64_t id);
}


#endif //TAAS_WORKER_H
