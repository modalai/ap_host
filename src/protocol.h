
#include <stdint.h>

// #include <GCS_MAVLink/GCS_MAVLink.h>
#define MAVLINK_MAX_PAYLOAD_LEN 255

#pragma once

#define QURT_MSG_ID_TEST_MSG 1
struct qurt_test_msg {
	uint8_t msg_id{QURT_MSG_ID_TEST_MSG};
	uint8_t byte_field;
	uint16_t word16_field;
	uint32_t word32_field;
	uint64_t word64_field;
	float float_field;
	double double_field;
};

#define QURT_MSG_ID_MAVLINK_MSG 2
struct qurt_mavlink_msg {
	uint8_t msg_id{QURT_MSG_ID_MAVLINK_MSG};
    uint8_t mav_msg[MAVLINK_MAX_PAYLOAD_LEN];
};
