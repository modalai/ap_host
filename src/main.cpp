
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "slpi_link.h"
#include "protocol.h"

volatile bool _running = false;
bool enable_debug = false;
bool enable_remote_debug = false;

int socket_fd;
struct sockaddr_in local_addr;
struct sockaddr_in remote_addr;

static void shutdown_signal_handler(int signo)
{
    switch(signo){
    case SIGINT: // normal ctrl-c shutdown interrupt
        _running = false;
        fprintf(stderr, "\nreceived SIGINT Ctrl-C\n");
        break;
    case SIGTERM: // catchable terminate signal
        _running = false;
        fprintf(stderr, "\nreceived SIGTERM\n");
        break;
    case SIGHUP:
        // terminal closed or disconnected, carry on anyway
        fprintf(stderr, "\nreceived SIGHUP, continuing anyway\n");
        break;
    default:
        fprintf(stderr, "\nreceived signal %d\n", signo);
        break;
    }
    return;
}

static uint32_t num_params = 0;

static void receive_callback(const uint8_t *data, uint32_t length_in_bytes) {
	if (enable_debug) printf("Got %u bytes in receive callback\n", length_in_bytes);

	if (length_in_bytes != 0) {
		uint8_t msg_id = data[0];
		switch (msg_id) {
		case QURT_MSG_ID_TEST_MSG:
		{
			printf("Got test message\n");
			struct qurt_test_msg msg;
			memcpy((void*) &msg, (void*) data, length_in_bytes);
			printf("Parsing struct test_msg\n");
			printf("msg_id = 0x%x\n", msg.msg_id);
			printf("byte_field = 0x%x\n", msg.byte_field);
			printf("word16_field = 0x%x\n", msg.word16_field);
			printf("word32_field = 0x%x\n", msg.word32_field);
			printf("word64_field = 0x%llx\n", msg.word64_field);
			printf("float_field = %f\n", msg.float_field);
			printf("double_field = %f\n", msg.double_field);
			break;
		}
		case QURT_MSG_ID_MAVLINK_MSG:
		{
			// printf("Got mavlink message\n");
			// printf("0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x 0x%.2x\n",
			// 		data[1], data[2], data[3], data[4], data[5], data[6], data[7], data[8], data[9], data[10], data[11], data[12]);
			if (_running) {
				int bytes_sent = sendto(socket_fd, &data[1], length_in_bytes - 1, MSG_CONFIRM,
					(const struct sockaddr*) &remote_addr, sizeof(remote_addr));
				if (bytes_sent > 0) {
					// printf("Send %d bytes to GCS\n", bytes_sent);
					// if (data[8] == 22) printf("PARAM_VALUE %u\n", num_params++);
				} else {
					fprintf(stderr, "Send to GCS failed\n");
				}
			}
			break;
		}
		default:
			fprintf(stderr, "Got unknown message id %d\n", msg_id);
			break;
		}
	}
}

static void send_test_message(void) {
	struct qurt_test_msg msg;
	
	msg.byte_field = 0x45;
	msg.word16_field = 0x1034;
	msg.word32_field = 0x456508;
	msg.word64_field = 0x3490000012221267;
	msg.float_field = 67.59008;
	msg.double_field = -329.099823;
	
	printf("Sending test message Length = %u\n", sizeof(msg));
    if (slpi_link_send((const uint8_t*) &msg, sizeof(msg))) {
        fprintf(stderr, "slpi_link_send_data failed\n");
	}
}

int main() {
	printf("About to call init\n");

    // make the sigaction struct for shutdown signals
    // sa_handler and sa_sigaction is a union, only set one
    struct sigaction action;
    action.sa_handler = shutdown_signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    // set actions
    if(sigaction(SIGINT, &action, NULL) < 0){
        fprintf(stderr, "ERROR: failed to set sigaction\n");
        return -1;
    }
    if(sigaction(SIGTERM, &action, NULL) < 0){
        fprintf(stderr, "ERROR: failed to set sigaction\n");
        return -1;
    }
    if(sigaction(SIGHUP, &action, NULL) < 0){
        fprintf(stderr, "ERROR: failed to set sigaction\n");
        return -1;
    }

	if (slpi_link_init(enable_remote_debug, &receive_callback, "libslpi_hello_world.so") != 0) {
        fprintf(stderr, "Error with init\n");
    	sleep(1);
    	slpi_link_reset();
        return -1;
    } else if (enable_debug) {
        printf("slpi_link_initialize succeeded\n");
    }

	send_test_message();

	//initialize socket and structure
    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
    	fprintf(stderr, "Could not create socket");
		return -1;
    }

    //assign values
    local_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(14552);

    if (bind(socket_fd, (struct sockaddr*) &local_addr, sizeof(local_addr)) == 0) {
		printf("Bind succeeded!\n");
	} else {
		fprintf(stderr, "Bind failed!\n");
	}

	remote_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(14559);

	printf("Waiting for receive\n");

	printf("Enter ctrl-c to exit\n");
	_running = true;
	struct qurt_mavlink_msg msg;
	while (_running) {
		uint32_t bytes_received = recvfrom(socket_fd, msg.mav_msg, MAVLINK_MAX_PAYLOAD_LEN,
			MSG_WAITALL, NULL, NULL);
		if (bytes_received < 0) {
			fprintf(stderr, "Received failed");
	    } else {
	    	// printf("Message received. %d bytes\n", bytes_received);
		    if (slpi_link_send((const uint8_t*) &msg, bytes_received + 1)) {
		        fprintf(stderr, "slpi_link_send_data failed\n");
			}
		}
	}

	printf("Reseting SLPI\n");
    // Send reset to SLPI
    slpi_link_reset();
    sleep(1);

	printf("Exiting\n");

    return 0;
}

