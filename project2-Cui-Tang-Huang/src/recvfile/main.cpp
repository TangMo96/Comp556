#include <iostream>
#include <climits>
#include <cstring>

#include "Runner.h"


int main(int argc, char** argv) {
	if (argc < 3) {
		printf("Please provide enough params.\n");

		return 0;
	}

	// check param
	char *param = argv[1];
	if (strncmp("-p", param, 2) != 0) {
		printf("Invalid param. Try \"-p\".\n");

		return 0;
	}

	// the port on which the receiver is listening
	long port = strtol(argv[2], nullptr, 0);
	if (!port
		|| port == LONG_MAX || port == LONG_MIN
		|| port < 18000 || port > 18200) {
		printf("Invalid port number: (18000, 18200)\n");

		return 0;
	}

	// create a new Runner and start it
	Runner runner((uint16_t) port);
	runner.start();

	return 0;
}