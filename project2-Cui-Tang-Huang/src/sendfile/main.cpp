#include <iostream>
#include <cstring>
#include <climits>
#include <sys/time.h>
#include <sys/stat.h>
#include <unistd.h>

#include "helper.h"
#include "Runner.h"


bool check_file_existence_and_folder(char *path) {
	/* check existence */
	if( (access(path, 0)) == -1) return false;

	/* check folder */
	struct stat buf;
	stat(path, &buf);

	return !(S_IFDIR & buf.st_mode);
}

int main(int argc, char** argv) {
	if (argc < 5) {
		printf("Please provide enough params.\n");

		return 0;
	}

	// check param
	char *param = argv[1];
	if (strncmp("-r", param, 2) != 0) {
		printf("Invalid param. Try \"-r\".\n");

		return 0;
	}

	// get receiver ip and port number
	char *receiver_ip = strtok(argv[2], ":");
	char *receiver_port_str = strtok(nullptr, ":");
	if (!receiver_port_str) {
		printf("Please provide a port number.\n");

		return 0;
	}

	long receiver_port = strtol(receiver_port_str, nullptr, 0);
	if (!receiver_port
		|| receiver_port == LONG_MAX || receiver_port == LONG_MIN
		|| receiver_port < 18000 || receiver_port > 18200) {
		printf("Invalid port number: (18000, 18200).\n");

		return 0;
	}

	// check param
	param = argv[3];
	if (strncmp("-f", param, 2) != 0) {
		printf("Invalid param. Try \"-f\".\n");

		return 0;
	}

	// check file_path
	char *file_path = argv[4];
	if (!file_path) {
		printf("Please input a file path.\n");

		return 0;
	} else if (!check_file_existence_and_folder(file_path)) {
		printf("File does not exist.\n");

		return 0;
	}

	// check subdir
	char *subdir = strrchr(argv[4], '/');

	if (subdir) {
		char origin = *subdir;

		*subdir = '\0';
		mkdir(argv[4], 0755);
		*subdir = origin;
	}

	// create a Runner and start it
	Runner runner(receiver_ip, (uint16_t) receiver_port, file_path);

	timeval tv;
	__time_t start_time;
	__time_t end_time;

	if (DEBUG) {
		gettimeofday(&tv, nullptr);
		start_time = tv.tv_sec * 1000000 + tv.tv_usec;
	}

	runner.start();

	if (DEBUG) {
		gettimeofday(&tv, nullptr);
		end_time = tv.tv_sec * 1000000 + tv.tv_usec;

		printf("%.3f\n", (float) (end_time - start_time) / 1000000);
	}

	return 0;
}