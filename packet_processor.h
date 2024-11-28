#pragma once
#include <stdint.h>
#include <stdbool.h>

#define MAX_STORED_PACKETS 4
#define PACKET_LENGTH 20

typedef struct {
    uint8_t status1;
    uint8_t status2;
    int8_t time_offset;
    float present_value;
} TSPV;

typedef struct {
    uint8_t version;
    uint8_t packet_id;
    uint32_t base_epoch;
    TSPV tspvs[2];
} Packet;

// Function prototypes
uint8_t* requestOldPacket(uint8_t packetid);
void postTSPV(uint8_t stat1, uint8_t stat2, float pv);
void receiveMSG(uint8_t* data, uint8_t length);
void resetProcessor();