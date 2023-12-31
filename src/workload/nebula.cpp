//
// Created by zwx on 23-8-29.
//

#include "workload/nebula.h"
#include "tools/utilities.h"
#include "workload/multi_model_workload.h"

#include<glog/logging.h>

namespace workload {

    nebula::SessionPoolConfig Nebula::nebulaSessionPoolConfig;
    std::unique_ptr<nebula::SessionPool> Nebula::nebulaSessionPool;

    void Nebula::Init(const Taas::Context& ctx) {
        nebula::ConnectionPool pool;
        auto connectConfig = nebula::Config{};
        connectConfig.maxConnectionPoolSize_ = ctx.multiModelContext.kClientNum ;
        pool.init({ctx.multiModelContext.kNebulaIP}, connectConfig);
        auto session = pool.getSession(ctx.multiModelContext.kNebulaUser, ctx.multiModelContext.kNebulaPwd);
        assert(session.valid());
        auto resp = session.execute(
                "CREATE SPACE IF NOT EXISTS " + ctx.multiModelContext.kNebulaSpace +" (vid_type = FIXED_STRING(30));");
        assert(resp.errorCode == nebula::ErrorCode::SUCCEEDED);

        nebulaSessionPoolConfig.username_ = ctx.multiModelContext.kNebulaUser;
        nebulaSessionPoolConfig.password_ = ctx.multiModelContext.kNebulaPwd;
        nebulaSessionPoolConfig.addrs_ = {ctx.multiModelContext.kNebulaIP};
        nebulaSessionPoolConfig.spaceName_ = ctx.multiModelContext.kNebulaSpace;
        nebulaSessionPoolConfig.maxSize_ = ctx.multiModelContext.kClientNum;
        nebulaSessionPool = std::make_unique<nebula::SessionPool>(nebulaSessionPoolConfig);
        nebulaSessionPool->init();
        resp = nebulaSessionPool->execute("CREATE TAG IF NOT EXISTS usertable (key string, filed string, tid string);");
        assert(resp.errorCode == nebula::ErrorCode::SUCCEEDED);
        LOG(INFO) << "Nebula Exec:" << "CREATE TAG IF NOT EXISTS usertable (key string, filed string, tid string);";
        resp = nebulaSessionPool->execute("CREATE TAG INDEX IF NOT EXISTS usertable_index on usertable(key(10));");
        assert(resp.errorCode == nebula::ErrorCode::SUCCEEDED);
        LOG(INFO) << "Nebula Exec:" << "CREATE TAG INDEX IF NOT EXISTS usertable_index on usertable(key(10));";
        LOG(INFO) << "=== connect to nebula done ===";
    }

    void Nebula::InsertData(const uint64_t& tid) {
        if(tid > MultiModelWorkload::ctx.multiModelContext.kRecordCount) return;
//        char genKey[100], gql[5000];
//        std::string data1 = Taas::RandomString(256);
//        std::string data2 = Taas::RandomString(256);
//        sprintf(genKey, "taas_nebula_key:%064lu", tid);
//        auto keyName = std::string(genKey);
//        utils::ByteIteratorMap values;
//        MultiModelWorkload::buildValues(values);
//        std::string value;
//        for (const auto &it: values) {
//            value += it.second + ",";
//        }
//        sprintf(gql, R"(INSERT VERTEX IF NOT EXISTS usertable(key, filed, txnid) VALUES "%s" :("%s","%s","%s");)",
//                                genKey, genKey, value.c_str(), ("tid:" + std::to_string(tid)).c_str());

//        auto resp = nebulaSessionPool->execute(gql);
//        LOG(INFO) << "Nebula Exec:" << gql;
    }

    void Nebula::RunTxn(const uint64_t& tid, const std::shared_ptr<std::atomic<uint64_t>>& sunTxnNum, std::shared_ptr<std::atomic<uint64_t>>& txn_num) {///single op txn
        char genKey[100], gql[5000];
        std::string value;
        int cnt, i;
        if(MultiModelWorkload::ctx.multiModelContext.kTestMode == Taas::MultiModelTest) {
            cnt = 1;
        }
        else if(MultiModelWorkload::ctx.multiModelContext.kTestMode == Taas::GQL) {
            cnt = 9;
        }
        else {
            sunTxnNum->fetch_add(1);
            return ;
        }
        {
            for (i = 0; i < cnt; i++) {
                auto opType = MultiModelWorkload::operationChooser->nextValue();
                auto id = MultiModelWorkload::keyChooser[2]->nextValue();
                sprintf(genKey, "usertable_key:%064lu", id);
                auto keyName = std::string(genKey);
                if (opType == Operation::READ) {
                    sprintf(gql, R"(FETCH PROP ON usertable "%s" YIELD properties(VERTEX);)",genKey);
                    auto resp = nebulaSessionPool->execute(gql);
                    LOG(INFO) << "Nebula Exec:" << gql;
                } else {
                    std::string data1 = Taas::RandomString(256);
                    std::string data2 = Taas::RandomString(256);
                    sprintf(genKey, "taas_nebula_key:%064lu", tid);
                    utils::ByteIteratorMap values;
                    value = "";
                    MultiModelWorkload::buildValues(values);
                    for (const auto &it: values) {
                        value += it.second + ",";
                    }
                    sprintf(gql, R"(INSERT VERTEX IF NOT EXISTS usertable(key, filed, txnid) VALUES "%s" :("%s","%s","%s");)",
                            genKey, genKey, value.c_str(), ("tid:" + std::to_string(tid)).c_str());
//                    sprintf(gql, R"(UPDATE VERTEX on usertable "%s" set filed = "%s", txnid = "%s";)",
//                            genKey, value.c_str(), ("tid:" + std::to_string(tid)).c_str());
                    txn_num->fetch_add(1);
                    /// todo : add a counter to notify RunMultiTxn sub txn gql send
                    auto resp = nebulaSessionPool->execute(gql);
                    LOG(INFO) << "Nebula Exec:" << gql;
                }
            }
        }
        sunTxnNum->fetch_add(1);
    }
}

