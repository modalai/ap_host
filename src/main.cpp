
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include "slpi_link.h"

bool _running = true;
bool enable_debug = false;

struct test_msg {
	uint8_t msg_id;
	uint8_t byte_field;
	uint16_t word16_field;
	uint32_t word32_field;
	uint64_t word64_field;
	float float_field;
	double double_field;
};

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

static void receive_callback(const uint8_t *data, uint32_t length_in_bytes) {
	printf("Got %u bytes in receive callback\n", length_in_bytes);

	struct test_msg msg;
	memcpy((void*) &msg, (void*) data, length_in_bytes);
	printf("Parsing struct test_msg\n");
	printf("msg_id = 0x%x\n", msg.msg_id);
	printf("byte_field = 0x%x\n", msg.byte_field);
	printf("word16_field = 0x%x\n", msg.word16_field);
	printf("word32_field = 0x%x\n", msg.word32_field);
	printf("word64_field = 0x%llx\n", msg.word64_field);
	printf("float_field = %f\n", msg.float_field);
	printf("double_field = %f\n", msg.double_field);
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

	if (slpi_link_init(enable_debug, &receive_callback, "libslpi_hello_world.so") != 0) {
        fprintf(stderr, "Error with init\n");
    	sleep(1);
    	slpi_link_reset();
        return -1;
    } else if (enable_debug) {
        printf("slpi_link_initialize succeeded\n");
    }

	struct test_msg msg;

	msg.msg_id = 0x10;
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

	printf("Enter ctrl-c to exit\n");
	while (_running) {
		sleep(1);
	}

	printf("Reseting SLPI\n");
    // Send reset to SLPI
    slpi_link_reset();
    sleep(1);

	printf("Exiting\n");

    return 0;
}

