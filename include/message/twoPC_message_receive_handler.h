//
// Created by zwx on 23-7-17.
//

#ifndef TAAS_TWOPCMESSAGERECEIVEHANDLER_H
#define TAAS_TWOPCMESSAGERECEIVEHANDLER_H

#pragma once

#include "message/message.h"
#include "queue"
#include "tools/utilities.h"
#include "zmq.hpp"

namespace Taas {
  
  enum TwoPCTxnState {
    client_txn = 0,         // 2pl+2pc client send to txn
    remote_server_txn = 1,  // 2pl+2pc txn sends to txn, transactions sharding
    lock_ok = 2,          // 2pl result remote to local
    lock_abort = 3,         // 2pl abort local to remote
    prepare_req = 4,        // 2pc prepare local to remote
    prepare_ok = 5,       // redo log result remote to local
    prepare_abort = 6,      // redo log abort local to remote
    commit_req = 7,         // commit local to remote
    commit_ok = 8,        // commit remote to local
    commit_done = 9,        // commit has done completely
    commit_abort = 10,      // commit abort
    abort_txn = 11,
  };

  struct TwoPCTxnStateStruct {
    uint64_t txn_sharding_num;         /// 有多少个分片子事务
    std::atomic<uint64_t> two_pl_num;  /// 用来记录有多少个子事务执行完成
    std::atomic<uint64_t> two_pc_prepare_num,
        two_pc_commit_num;  /// 2pc准备阶段子事务个数，提交阶段子事务个数
    std::atomic<uint64_t> two_pl_reply;
    std::atomic<uint64_t> two_pc_prepare_reply, two_pc_commit_reply;
    TwoPCTxnState txn_state;

    TwoPCTxnStateStruct& operator=(const struct TwoPCTxnStateStruct& value) {
      txn_sharding_num = value.txn_sharding_num;
      // 接收到reply数量
      two_pl_reply.store(value.two_pl_reply);
      two_pc_prepare_reply.store(value.two_pc_prepare_reply);
      two_pc_commit_reply.store(value.two_pc_commit_reply);
      // 完成数量
      two_pl_num.store(value.two_pl_num);
      two_pc_prepare_num.store(value.two_pc_prepare_num);
      two_pc_commit_num.store(value.two_pc_commit_num);
      txn_state = value.txn_state;
      return *this;
    }
  };

  class TwoPCMessageReceiveHandler {
  public:
    bool Init(const Context& ctx_, uint64_t id);

    void HandleReceivedMessage();
    bool SetMessageRelatedCountersInfo();
    bool HandleReceivedTxn();
    bool HandleClientTxn();

    uint64_t GetHashValue(const std::string& key) const { return _hash(key) % sharding_num; }

    static bool StaticInit(const Context& context);
    static bool StaticClear(const Context& context, uint64_t& epoch);

  private:
    std::unique_ptr<zmq::message_t> message_ptr;
    std::unique_ptr<std::string> message_string_ptr;
    std::unique_ptr<proto::Message> msg_ptr;
    std::unique_ptr<proto::Transaction> txn_ptr;
    std::unique_ptr<pack_params> pack_param;
    std::string csn_temp, key_temp, key_str, table_name, csn_result;
    uint64_t thread_id = 0, server_dequeue_id = 0, max_length = 0,
             sharding_num = 0,                           /// cache check
        message_sharding_id = 0, message_server_id = 0,  /// message epoch info

        server_reply_ack_id = 0;

    bool res, sleep_flag;

    Context ctx;
    proto::Transaction empty_txn;
    std::hash<std::string> _hash;


  public:
    static std::vector<uint64_t> sharding_send_ack_epoch_num, backup_send_ack_epoch_num,
        backup_insert_set_send_ack_epoch_num,
        abort_set_send_ack_epoch_num;  /// check and reply ack

    static std::vector<
        std::unique_ptr<BlockingConcurrentQueue<std::unique_ptr<proto::Transaction>>>>
        epoch_remote_sharding_txn, epoch_local_sharding_txn, epoch_local_txn, epoch_backup_txn,
        epoch_insert_set, epoch_abort_set;
  };
}  // namespace Taas

#endif  // TAAS_TWOPCMESSAGERECEIVEHANDLER_H
