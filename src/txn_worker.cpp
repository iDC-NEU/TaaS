//
// Created by 周慰星 on 2022/9/14.
//

#include <queue>
#include <utility>
#include "epoch/merge.h"
#include "epoch/epoch_manager.h"
#include "tools/utilities.h"
#include "tikv_client.h"

namespace Taas {

/**
 * @brief do local_merge remote_merge and commit
 *
 * @param ctx XML中的配置相关信息
 * @return true
 * @return false
 */
    void MergeWorkerThreadMain(uint64_t id, Context ctx) {
        Merger merger;
        merger.Run(id, ctx);
        merger.Init(id, std::move(ctx));

        auto sleep_flag = false;
        while(!EpochManager::IsTimerStop()) {
            sleep_flag = false;

            sleep_flag = sleep_flag | merger.EpochMerge();

            sleep_flag = sleep_flag | merger.EpochCommit_RedoLog_TxnMode();

//            sleep_flag = sleep_flag | merger.EpochCommit_RedoLog_ShardingMode();

            if(!sleep_flag) usleep(200);
        }
    }

    void SendTiKVThreadMain(uint64_t id, Context ctx) {
        auto sleep_flag = false;
        auto txn_ptr = std::make_unique<proto::Transaction>();
//        tikv_client::TransactionClient client({"172.19.215.168:2379"});
        tikv_client::TransactionClient client({ctx.kTiKVIP});
        while(!EpochManager::IsTimerStop()) {
            sleep_flag = false;
            if(redo_log_queue.try_dequeue(txn_ptr) && txn_ptr != nullptr) {
                auto tikv_txn = client.begin();
                for (auto i = 0; i < txn_ptr->row_size(); i++) {
                    const auto& row = txn_ptr->row(i);
                    if (row.op_type() == proto::OpType::Insert || row.op_type() == proto::OpType::Update) {
                        tikv_txn.put(row.key(), row.data());
                    }
                }
                while(redo_log_queue.try_dequeue(txn_ptr)) {
                    if(txn_ptr == nullptr) continue;
                    for (auto i = 0; i < txn_ptr->row_size(); i++) {
                        const auto& row = txn_ptr->row(i);
                        if (row.op_type() == proto::OpType::Insert || row.op_type() == proto::OpType::Update) {
                            tikv_txn.put(row.key(), row.data());
//                            printf("put key: %s, put value: %s\n", row.key().c_str(), row.data().c_str());
                        }
                    }
                }
                tikv_txn.commit();
                sleep_flag = true;
            }
        }
        if(!sleep_flag) {
            usleep(200);
        }
    }
}

