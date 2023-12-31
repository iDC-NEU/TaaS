//
// Created by 周慰星 on 11/15/22.
//

#include <utility>
#include "transaction/merge.h"
#include "epoch/epoch_manager.h"
#include "tools/utilities.h"
#include "storage/redo_loger.h"

namespace Taas {
    Context Merger::ctx;
    AtomicCounters_Cache
            Merger::epoch_should_merge_txn_num(10, 2),
            Merger::epoch_merged_txn_num(10, 2),
            Merger::epoch_should_commit_txn_num(10, 2),
            Merger::epoch_committed_txn_num(10, 2),
            Merger::epoch_record_commit_txn_num(10, 2),
            Merger::epoch_record_committed_txn_num(10, 2);

    std::vector<std::unique_ptr<concurrent_crdt_unordered_map<std::string, std::string, std::string>>>
            Merger::epoch_merge_map, ///epoch merge   row_header
            Merger::local_epoch_abort_txn_set,
            Merger::epoch_abort_txn_set; /// for epoch final check


    concurrent_unordered_map<std::string, std::string>
            Merger::read_version_map_data, ///read validate for higher isolation
            Merger::read_version_map_csn, ///read validate for higher isolation
            Merger::insert_set;   ///插入集合，用于判断插入是否可以执行成功 check key exits?


    std::vector<std::unique_ptr<BlockingConcurrentQueue<std::shared_ptr<proto::Transaction>>>>
            Merger::epoch_merge_queue,///存放要进行merge的事务，分片
            Merger::epoch_commit_queue;///存放每个epoch要进行写日志的事务，分片写日志

    std::vector<std::unique_ptr<std::atomic<bool>>>
            Merger::epoch_merge_complete,
            Merger::epoch_commit_complete;

    std::atomic<uint64_t> Merger::total_merge_txn_num(0), Merger::total_merge_latency(0), Merger::total_commit_txn_num(0),
        Merger::total_commit_latency(0), Merger::success_commit_txn_num(0), Merger::success_commit_latency(0),
        Merger::total_read_version_check_failed_txn_num(0), Merger::total_failed_txn_num(0);

    std::condition_variable Merger::merge_cv, Merger::commit_cv;


    void Merger::StaticInit(const Context &ctx_) {
        ctx = ctx_;
        auto max_length = ctx.taasContext.kCacheMaxLength;
        auto pack_num = ctx.taasContext.kIndexNum;
        ///epoch merge state
        epoch_merge_map.resize(max_length);
        local_epoch_abort_txn_set.resize(max_length);
        epoch_abort_txn_set.resize(max_length);


        epoch_merge_queue.resize(max_length);
        epoch_commit_queue.resize(max_length);

        epoch_merge_complete.resize(max_length);
        epoch_commit_complete.resize(max_length);

        epoch_should_merge_txn_num.Init(max_length, pack_num);
        epoch_merged_txn_num.Init(max_length, pack_num);
        epoch_should_commit_txn_num.Init(max_length, pack_num);
        epoch_committed_txn_num.Init(max_length, pack_num);
        epoch_record_commit_txn_num.Init(max_length, pack_num);
        epoch_record_committed_txn_num.Init(max_length, pack_num);

        for(int i = 0; i < static_cast<int>(max_length); i ++) {
            epoch_merge_complete[i] = std::make_unique<std::atomic<bool>>(false);
            epoch_commit_complete[i] = std::make_unique<std::atomic<bool>>(false);
            epoch_merge_map[i] = std::make_unique<concurrent_crdt_unordered_map<std::string, std::string, std::string>>();
            local_epoch_abort_txn_set[i] = std::make_unique<concurrent_crdt_unordered_map<std::string, std::string, std::string>>();
            epoch_abort_txn_set[i] = std::make_unique<concurrent_crdt_unordered_map<std::string, std::string, std::string>>();

            epoch_merge_queue[i] = std::make_unique<BlockingConcurrentQueue<std::shared_ptr<proto::Transaction>>>();
            epoch_commit_queue[i] = std::make_unique<BlockingConcurrentQueue<std::shared_ptr<proto::Transaction>>>();
        }
    }

    void Merger::MergeQueueEnqueue(uint64_t &epoch, const std::shared_ptr<proto::Transaction>& txn_ptr) {
        auto epoch_mod = epoch % ctx.taasContext.kCacheMaxLength;
        epoch_should_merge_txn_num.IncCount(epoch, txn_ptr->server_id(), 1);
        epoch_merge_queue[epoch_mod]->enqueue(txn_ptr);
        epoch_merge_queue[epoch_mod]->enqueue(nullptr);
    }
    bool Merger::MergeQueueTryDequeue(uint64_t &epoch, const std::shared_ptr<proto::Transaction>& txn_ptr) {
        ///not use for now
        return false;
    }
    void Merger::CommitQueueEnqueue(uint64_t& epoch, const std::shared_ptr<proto::Transaction>& txn_ptr) {
        epoch_should_commit_txn_num.IncCount(epoch, ctx.taasContext.txn_node_ip_index, 1);
        auto epoch_mod = epoch % ctx.taasContext.kCacheMaxLength;
        epoch_commit_queue[epoch_mod]->enqueue(txn_ptr);
        epoch_commit_queue[epoch_mod]->enqueue(nullptr);
    }
    bool Merger::CommitQueueTryDequeue(uint64_t& epoch, std::shared_ptr<proto::Transaction> txn_ptr) {
        auto epoch_mod = epoch % ctx.taasContext.kCacheMaxLength;
        return epoch_commit_queue[epoch_mod]->try_dequeue(txn_ptr);
    }

    void Merger::ClearMergerEpochState(uint64_t& epoch) {
        auto epoch_mod = epoch % ctx.taasContext.kCacheMaxLength;
        epoch_merge_complete[epoch_mod]->store(false);
        epoch_commit_complete[epoch_mod]->store(false);
        epoch_merge_map[epoch_mod]->clear();
        epoch_abort_txn_set[epoch_mod]->clear();
        local_epoch_abort_txn_set[epoch_mod]->clear();
        epoch_should_merge_txn_num.Clear(epoch_mod), epoch_merged_txn_num.Clear(epoch_mod);
        epoch_should_commit_txn_num.Clear(epoch_mod), epoch_committed_txn_num.Clear(epoch_mod);
        epoch_record_commit_txn_num.Clear(epoch_mod), epoch_record_committed_txn_num.Clear(epoch_mod);

//        epoch_merge_queue[epoch_mod] = std::make_unique<BlockingConcurrentQueue<std::shared_ptr<proto::Transaction>>>();
//        epoch_commit_queue[epoch_mod] = std::make_unique<BlockingConcurrentQueue<std::shared_ptr<proto::Transaction>>>();
    }

    void Merger::Init(uint64_t id_) {
        txn_ptr.reset();
        thread_id = id_;
        message_handler.Init(thread_id);
    }

    void Merger::Merge() {
        auto time1 = now_to_us();
        epoch = txn_ptr->commit_epoch();
        if (!CRDTMerge::ValidateReadSet(txn_ptr)) {
            total_read_version_check_failed_txn_num.fetch_add(1);
            goto end;
        }
        if (!CRDTMerge::MultiMasterCRDTMerge(txn_ptr)) {
            goto end;
        }
        end:
        total_merge_txn_num.fetch_add(1);
        total_merge_latency.fetch_add(now_to_us() - time1);
        epoch_merged_txn_num.IncCount(epoch, txn_server_id, 1);
    }

    void Merger::Commit() {
        auto time1 = now_to_us();
        ///validation phase
        if (!CRDTMerge::ValidateWriteSet(txn_ptr)) {
            auto key = std::to_string(txn_ptr->client_txn_id());
            total_failed_txn_num.fetch_add(1);
            EpochMessageSendHandler::SendTxnCommitResultToClient(txn_ptr, proto::TxnState::Abort);
        } else {
            epoch_record_commit_txn_num.IncCount(epoch, txn_ptr->server_id(), 1);
            CRDTMerge::Commit(txn_ptr);
            if(txn_ptr->server_id() == ctx.taasContext.txn_node_ip_index) { /// only local txn do redo log
                RedoLoger::RedoLog(txn_ptr);
            }
            epoch_record_committed_txn_num.IncCount(epoch, txn_ptr->server_id(), 1);
            success_commit_txn_num.fetch_add(1);
            success_commit_latency.fetch_add(now_to_us() - time1);
            EpochMessageSendHandler::SendTxnCommitResultToClient(txn_ptr, proto::TxnState::Commit);
        }
        total_commit_txn_num.fetch_add(1);
        total_commit_latency.fetch_add(now_to_us() - time1);
        epoch_committed_txn_num.IncCount(epoch, txn_ptr->server_id(), 1);
    }

    void Merger::EpochMerge() {
        epoch = EpochManager::GetLogicalEpoch();
        std::mutex mtx;
        std::unique_lock lck(mtx);
        while (!EpochManager::IsTimerStop()) {
            sleep_flag = true;
            epoch = EpochManager::GetLogicalEpoch();
            epoch_mod = epoch % ctx.taasContext.kCacheMaxLength;
            if(!EpochManager::IsShardingMergeComplete(epoch)) {
                while (epoch_merge_queue[epoch_mod]->try_dequeue(txn_ptr)) {
                    if (txn_ptr != nullptr && txn_ptr->txn_type() != proto::TxnType::NullMark) {
                        Merge();
                        txn_ptr.reset();
                        sleep_flag = false;
                    }
                }
            }

            if(EpochManager::IsAbortSetMergeComplete(epoch) && !EpochManager::IsCommitComplete(epoch)) {
                while (!EpochManager::IsCommitComplete(epoch) && epoch_commit_queue[epoch_mod]->try_dequeue(txn_ptr)) {
                    if (txn_ptr != nullptr && txn_ptr->txn_type() != proto::TxnType::NullMark) {
                        Commit();
                        txn_ptr.reset();
                        sleep_flag = false;
                    }
                }
            }
            if(sleep_flag)
                usleep(merge_sleep_time);
        }
    }
}