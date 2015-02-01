#ifndef PACKET_PROCESSOR_H
#define PACKET_PROCESSOR_H

#include "dpif.h"
#include "measure_data_type.h"
#include "debug_output.h"
#include "condition_map.h"
#include "sampled_map.h"
#include "global_operation.h"
#include "sample_model.h"

const uint32_t CONDITION_PACKET_DIP = 167837954; //10.1.1.2
const uint16_t CONDITION_PACKET_DPORT = 65535;   //65535

void process(struct dpif_execute *execute);
void process_normal_packet(packet_t* pkt_ptr);
void process_condition_packet(packet_t* pkt_ptr);
bool capture_the_packet(flow_key_t flow_key, packet_t* pkt_ptr);


void process(struct dpif_execute *execute){
    //out_port
    int out_port;
    //pkt_len, allocated_len
    uint32_t pkt_len;  //bytes in use
    uint32_t allocated_len;  //allocated
    //sip, dip
    char* l3_start;
    uint32_t srcip;
    uint32_t dstip;
    //sport, dport
    char* l4_start;
    uint16_t sport;
    uint16_t dport;
    uint16_t plen;  //payload len
    char* payload_ptr;
    //seqid 
    uint32_t seqid;
    //time interval id
    uint32_t ith_interval = 0;
    uint64_t timestamp;

    packet_t pkt;
    condition_payload_t condition_payload;

    //for debug
    char buffer[1000];
    
    out_port=*(int*)((char*)execute->actions+4)-1;
    //pkt_len, allocated_len
    pkt_len = execute->packet->size_;  //bytes in use
    allocated_len = execute->packet->allocated;  //allocated
    //sip, dip
    l3_start = ofpbuf_l3(execute->packet);
    srcip = ntohl(*(int*)(l3_start+12));
    dstip = ntohl(*(int*)(l3_start+16));
    //sport, dport
    l4_start = ofpbuf_l4(execute->packet); 
    sport = ntohs(*(uint16_t*)l4_start);
    dport = ntohs(*(uint16_t*)(l4_start+2));
    plen = ntohs(*(uint16_t*)(l4_start+4));  //payload len 

    snprintf(buffer, 1000, "sip:%u dip:%u sport:%u dport:%u payload_len:%u len_in_use:%u allo_len:%u out_port:%d", 
        srcip, dstip, sport, dport, plen, pkt_len, allocated_len, out_port);
    DEBUG(buffer);

    //data pointer, frame pointer
    payload_ptr = l4_start + 8;

    //get the packet
    pkt.srcip = srcip;
    pkt.dstip = dstip;
    pkt.sport = sport;
    pkt.dport = dport;
    pkt.out_port = out_port;
    //TODO: check which len to use
    pkt.len = pkt_len;

    if (dstip == CONDITION_PACKET_DIP && dport == CONDITION_PACKET_DPORT) {
        DEBUG("condition packet");
        //construct condition rate packet
        //memcpy(&condition_payload, payload_ptr, sizeof(condition_payload));
        condition_payload = *((condition_payload_t*)payload_ptr);

        if (plen < sizeof(condition_payload)) {
             ERROR("error: payload size < sizeof(condition_payload_t)");
            return;
        }
        snprintf(buffer, 1000, "condition info: srcip:%u, lossRate:%f, volume:%lu",
            condition_payload.srcip, condition_payload.loss_rate, condition_payload.volume);
        DEBUG(buffer);
        //set condition of the pkt
        pkt.condition.loss_rate = condition_payload.loss_rate;
        pkt.condition.volume = condition_payload.volume;
        //handle condition packet
        process_condition_packet(&pkt);
    } else {
        seqid = ntohl(*(uint32_t*)payload_ptr);

        snprintf(buffer, 1000, "normal packet, seqid:%u", seqid);
        DEBUG(buffer);
        //construct normal packet 
        //TODO: in normal packets, there should be a field telling the time interval id
        check_switch_buffers(ith_interval);

        //process the packet
        process_normal_packet(&pkt);
    }
}

void process_normal_packet(packet_t* pkt_ptr) {
    flow_key_t flow_key;
    flow_key.srcip = pkt_ptr->srcip;
    if (!capture_the_packet(flow_key, pkt_ptr)) {
        return;
    }
    add_sampled_volume(flow_key, pkt_ptr->out_port, pkt_ptr->len);
}

void process_condition_packet(packet_t* pkt_ptr) {
    //record the condition information of the flow in condition_flow_map
    flow_key_t flow_key;
    flow_key.srcip = pkt_ptr->srcip;
    put_condition(flow_key, pkt_ptr->condition);
}

bool capture_the_packet(flow_key_t flow_key, packet_t* pkt_ptr) {
    if (flow_key_exist(flow_key)) {
        return true;
    }
    //packet sample rate
    double pkt_sample_rate = get_pkt_sample_rate(flow_key, pkt_ptr);
    int pkt_sample_num = (int)(pkt_sample_rate * RAND_MOD_NUMBER);
    //rand number
    int rand_num = rand() % RAND_MOD_NUMBER + 1;

    if (rand_num <= pkt_sample_num) {
        return true;
    }
    return false;
}

#endif
