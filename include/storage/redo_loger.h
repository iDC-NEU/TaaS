//
// Created by 周慰星 on 23-3-30.
//

#ifndef TAAS_REDO_LOGER_H
#define TAAS_REDO_LOGER_H

#pragma once

#include "tools/context.h"
#include "tools/concurrent_hash_map.h"
#include "tools/atomic_counters.h"
#include "tools/blocking_concurrent_queue.hpp"

#include "proto/message.pb.h"
#include "tikv_client.h"
#include "tools/thread_pool.h"

namespace Taas {
    class RedoLoger {
    public:
        static std::unique_ptr<util::thread_pool_light> workers;
        static AtomicCounters epoch_log_lsn;///epoch, value        for epoch log (each epoch has single one counter)
        static std::vector<std::unique_ptr<concurrent_unordered_map<std::string, std::shared_ptr<proto::Transaction>>>> committed_txn_cache;
        static std::vector<std::unique_ptr<BlockingConcurrentQueue<std::shared_ptr<proto::Transaction>>>> epoch_redo_log_queue;
        static void StaticInit(const Context& ctx);
        static void ClearRedoLog(const Context& ctx, uint64_t& epoch_mod);
        static bool RedoLog(const Context& ctx, std::shared_ptr<proto::Transaction> txn_ptr);

        static bool GeneratePushDownTask(const Context& ctx, const uint64_t& epoch);

        static bool CheckPushDownComplete(const Context& ctx, const uint64_t& epoch);

        static bool SendTransactionToDB_Usleep(const Context &ctx);
    };
}



#endif //TAAS_REDO_LOGER_H
