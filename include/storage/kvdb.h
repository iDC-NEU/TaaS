//
// Created by user on 23-7-3.
//

#ifndef TAAS_KVDB_H
#define TAAS_KVDB_H
namespace Taas {
//    class KVDB {
//    public:
//        static Context ctx;
//        static tikv_client::TransactionClient* tikv_client_ptr;
//        static AtomicCounters_Cache
//                epoch_should_push_down_txn_num, epoch_pushed_down_txn_num;
//        static std::unique_ptr<moodycamel::BlockingConcurrentQueue<std::unique_ptr<proto::Transaction>>> task_queue, redo_log_queue;
//        static std::vector<std::unique_ptr<moodycamel::BlockingConcurrentQueue<std::unique_ptr<proto::Transaction>>>>
//        epoch_redo_log_queue; ///store transactions receive from clients, wait to push down
//        static std::vector<std::unique_ptr<std::atomic<bool>>> epoch_redo_log_complete;
//
//        static void StaticInit(const Context& ctx_);
//        static void StaticClear(uint64_t &epoch);
//
//        static bool GeneratePushDownTask(uint64_t &epoch);
//
//        static void SendTransactionToDB_Usleep();
//        static void SendTransactionToDB_Block();
//
//        bool CheckEpochPushDownComplete(uint64_t &epoch);
//        void DBRedoLogQueueEnqueue(uint64_t &epoch, std::unique_ptr<proto::Transaction> &&txn_ptr);
//        bool DBRedoLogQueueTryDequeue(uint64_t &epoch, std::unique_ptr<proto::Transaction> &txn_ptr);
//
//    };
}
#endif //TAAS_KVDB_H
