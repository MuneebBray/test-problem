// packet_processor.c
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "packet_processor.h"

// Storage for received packets
static uint8_t last_processed_packet_id = 0;
static bool retrieving_missing_packets = false;

// Helper function to parse a packet from raw data
static void parse_packet(uint8_t* data, Packet* packet) {
    // Null check
    if (data == NULL) {
        return;
    }

    // Parse fixed fields
    packet->version = data[0];
    packet->packet_id = data[1];
    memcpy(&packet->base_epoch, data + 2, sizeof(uint32_t));

    // Parse first TSPV
    packet->tspvs[0].status1 = data[6];
    packet->tspvs[0].status2 = data[7];
    packet->tspvs[0].time_offset = (int8_t)data[8];
    memcpy(&packet->tspvs[0].present_value, data + 9, sizeof(float));

    // Parse second TSPV
    packet->tspvs[1].status1 = data[13];
    packet->tspvs[1].status2 = data[14];
    packet->tspvs[1].time_offset = (int8_t)data[15];
    memcpy(&packet->tspvs[1].present_value, data + 16, sizeof(float));
}

// Request missing packets from the source device
static void request_missing_packets(uint8_t expected_id, uint8_t received_id) {
    // Handle invalid input
    assert(received_id >= expected_id);

    // Only the last 4 packets are stored (Assumption: received packet included)
    uint8_t startId = expected_id;
    if (received_id - expected_id > MAX_STORED_PACKETS - 1)
    {
        startId = received_id - (MAX_STORED_PACKETS - 1);
    }

    retrieving_missing_packets = true;

    for (uint8_t id = startId; id < received_id; id++) {
        uint8_t* packet = requestOldPacket(id);
        if (packet != NULL) {
            receiveMSG(packet, PACKET_LENGTH);
        }
    }

    retrieving_missing_packets = false;
}

// Validate packet order and detect missing packets
static void validate_packet_order(Packet* new_packet) {
    if (new_packet->packet_id == 0) return;  // Reserved packet ID

    // Detect missing packets
    int numMissingPackets = new_packet->packet_id - last_processed_packet_id;

    if (numMissingPackets > 1 && new_packet->packet_id <= 255) {
        request_missing_packets(last_processed_packet_id + 1, new_packet->packet_id);
        return;
    }

    // Handle packet ID wrapping (from 255 back to 1)
    if (last_processed_packet_id == 255 && new_packet->packet_id != 1) {
        request_missing_packets(1, new_packet->packet_id);
        return;
    }
}

// Process a valid TSPV
static void process_tspv(Packet* packet, TSPV* tspv, uint32_t base_epoch) {
    uint32_t tspv_epoch = base_epoch + tspv->time_offset; // TODO: Use in post
    postTSPV(tspv->status1, tspv->status2, tspv->present_value);
}

// Main packet processing function
static void process_packet(Packet* packet) {
    // Process both TSPVs
    process_tspv(packet, &packet->tspvs[0], packet->base_epoch);
    process_tspv(packet, &packet->tspvs[1], packet->base_epoch);

    last_processed_packet_id = packet->packet_id;
}

// Reset variables
void resetProcessor() {
    last_processed_packet_id = 0;
    retrieving_missing_packets = false;
}

// Receive message function
void receiveMSG(uint8_t* data, uint8_t length) {
    if (length != PACKET_LENGTH) return;  // Invalid packet length

    Packet new_packet;
    parse_packet(data, &new_packet);

    if (!retrieving_missing_packets) {
        validate_packet_order(&new_packet);
    }

    // Process the packet
    process_packet(&new_packet);
}