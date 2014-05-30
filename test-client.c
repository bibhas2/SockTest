#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>

#define PORT_NUMBER 8080
#define MAX_CLIENTS 5
#define DIE(value, message) if (value < 0) {perror(message); exit(value);}

void
_info(const char* fmt, ...) {
        va_list ap;

        va_start(ap, fmt);

        printf("INFO: ");
        vprintf(fmt, ap);
        printf("\n");
}

void
zero_fd_list(int *list) {
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		list[i] = 0;
	}
}


void
populate_fd_set(int *file_desc_list, fd_set *pFdSet) {
	FD_ZERO(pFdSet);

	for (int i = 0; i < MAX_CLIENTS; ++i) {
		int fd = file_desc_list[i];

		if (fd != 0) {
			FD_SET(fd, pFdSet);
		}
	}
}

int
main(int argc, char **argv) {
	_info("Connecting to %s:%s", argv[1], argv[2]);

	struct addrinfo hints, *res;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_INET;
	hints.ai_socktype = SOCK_STREAM;
	_info("Resolving name...");
	int status = getaddrinfo(argv[1], argv[2], &hints, &res);
	DIE(status, "Failed to resolve address.");
	if (res == NULL) {
		_info("Failed to resolve address: %s", argv[1]);
		exit(-1);
	}

	int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	DIE(sock, "Failed to open socket.");

	status = connect(sock, res->ai_addr, res->ai_addrlen);
	DIE(status, "Failed to connect to port.");

	char *req = "GET / HTTP/1.1\r\n"
		"Host: *\r\n"
		"Accept: */*\r\n"
		"\r\n";

	_info("Writing request...");
	status = write(sock, req, strlen(req));
	DIE(status, "Write failed.");

	char buff[256];

	_info("Reading response");
	while (1) {
		status = read(sock, buff, sizeof(buff) - 1);
		if (status <= 0) {
			break;
		}
		buff[status] = '\0';
		printf("%s", buff);	
	}

	close(sock);
	freeaddrinfo(res);

	return 0;
}
