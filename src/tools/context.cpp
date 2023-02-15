//
// Created by 周慰星 on 11/8/22.
//

#include "tools/context.h"
#include "tools/tinyxml2.h"

namespace Taas {

    void Context::GetServerInfo(){
        tinyxml2::XMLDocument doc;
        doc.LoadFile("../config.xml");
        tinyxml2::XMLElement *root=doc.RootElement();
        tinyxml2::XMLElement *index_element=root->FirstChildElement("txn_node_ip");
        int symbol_local_or_remote=0;
        while (index_element){
            tinyxml2::XMLElement *ip_port=index_element->FirstChildElement("txn_ip");
            const char* content;
            while(ip_port){
                content=ip_port->GetText();
                std::string temp(content);
                kServerIp.push_back(temp);
                ip_port=ip_port->NextSiblingElement();

            }
            index_element=index_element->NextSiblingElement();
            symbol_local_or_remote++;
        }

        tinyxml2::XMLElement* server_num = root->FirstChildElement("txn_node_num");
        kTxnNodeNum= std::stoull(server_num->GetText());

        tinyxml2::XMLElement* txn_node_ip_index_xml = root->FirstChildElement("txn_node_ip_index");
        txn_node_ip_index=std::stoull(txn_node_ip_index_xml->GetText()) ;

        tinyxml2::XMLElement* epoch_size_us = root->FirstChildElement("epoch_size_us");
        kEpochSize_us= std::stoull(epoch_size_us->GetText());

        tinyxml2::XMLElement* cachemaxlength = root->FirstChildElement("cache_max_length");
        kCacheMaxLength = std::stoull(cachemaxlength->GetText());

        tinyxml2::XMLElement* package_num = root->FirstChildElement("index_num");
        kIndexNum = std::stoull(package_num->GetText());

        tinyxml2::XMLElement* pack_thread_num = root->FirstChildElement("pack_thread_num");
        kPackThreadNum = std::stoull(pack_thread_num->GetText());

        tinyxml2::XMLElement* commit_thread_num = root->FirstChildElement("cache_thread_num");
        kMessageCacheThreadNum = std::stoull(commit_thread_num->GetText());

        tinyxml2::XMLElement* merge_thread_num = root->FirstChildElement("worker_thread_num");
        kWorkerThreadNum = std::stoull(merge_thread_num->GetText());

        tinyxml2::XMLElement* send_client_thread_num = root->FirstChildElement("send_client_thread_num");
        kSendClientThreadNum = std::stoull(send_client_thread_num->GetText());

        tinyxml2::XMLElement* duration_time = root->FirstChildElement("duration_time_us");
        kDurationTime_us = std::stoull(duration_time->GetText());

        tinyxml2::XMLElement* client_num = root->FirstChildElement("test_client_num");
        kTestClientNum = std::stoull(client_num->GetText());

        tinyxml2::XMLElement* key_range = root->FirstChildElement("test_key_range");
        kTestKeyRange = std::stoull(key_range->GetText());

        tinyxml2::XMLElement* test_txn_op_num = root->FirstChildElement("test_txn_op_num");
        kTestTxnOpNum = std::stoull(test_txn_op_num->GetText());

        tinyxml2::XMLElement* sync_start = root->FirstChildElement("sync_start");
        is_sync_start = std::stoull(sync_start->GetText());

        printf("Config Info:\n \tServerIp:\n");

        int cnt = 0;
        for(auto i : kServerIp) {
            printf("\t \t ID: %d, IP: %s\n", cnt++, i.c_str());
        }

        printf("\t ServerNum: %lu\n\t txn_node_ip_index: %lu\n\t EpochSize_us: %lu\n", kTxnNodeNum, txn_node_ip_index, kEpochSize_us);
        printf("\t CacheLength: %lu\n\t IndexNum: %lu\n\t PackThreadNum: %lu\n", kCacheMaxLength, kIndexNum, kPackThreadNum);
        printf("\t CacheThreadNum: %lu\n\t WorkerNum: %lu\n\t DurationTime_us: %lu\n", kMessageCacheThreadNum, kWorkerThreadNum, kDurationTime_us);
        printf("\t TestClientNum: %lu\n\t TestKeyRange: %lu\n\t TestTxnOpNum: %lu\n", kTestClientNum, kTestKeyRange, kTestTxnOpNum);
        printf("\t SycnStart: %d\n", is_sync_start);

    }
}