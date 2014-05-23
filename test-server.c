#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

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

void
disconnect_clients(int *file_desc_list) {
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		int fd = file_desc_list[i];

		if (fd != 0) {
			close(fd);
			file_desc_list[i] = 0;
		}
	}
}

int
add_client_fd(int *file_desc_list, int fd) {
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (file_desc_list[i] == 0) {
			//We have a free slot
			file_desc_list[i] = fd;

			return i;
		}
	}

	return -1;
}

int
remove_client_fd(int *file_desc_list, int fd) {
	for (int i = 0; i < MAX_CLIENTS; ++i) {
		if (file_desc_list[i] == fd) {
			//We found it!
			file_desc_list[i] = 0;

			return i;
		}
	}

	return -1;
}

int
handle_client_write(int fd) {
	char buff[256];

	int bytesRead = read(fd, buff, sizeof(buff));

	if (bytesRead < 0) {
		return -1;
	}
	if (bytesRead == 0) {
		return 0;
	}
	//Dump the written data
	write(1, buff, bytesRead);

	return bytesRead;
}

void
server_loop(int sock) {
	int file_desc_list[MAX_CLIENTS];

	zero_fd_list(file_desc_list);

	//Save the server socket in list
	file_desc_list[0] = sock;

	fd_set fdSet;
	struct timeval timeout;

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;

	while (1) {
		populate_fd_set(file_desc_list, &fdSet);

		int numEvents = select(FD_SETSIZE, &fdSet, NULL, NULL, &timeout);
		DIE(numEvents, "select() failed.");

		if (numEvents == 0) {
			_info("select() timed out.");

			break;
		}
		
		//Make sense out of the event
		if (FD_ISSET(sock, &fdSet)) {
			_info("Client is connecting...");
			int clientFd = accept(sock, NULL, NULL);

			DIE(clientFd, "accept() failed.");

			int position = add_client_fd(file_desc_list, clientFd);

			if (position < 0) {
				_info("Too many clients. Disconnecting...");
				remove_client_fd(file_desc_list, clientFd);
				close(clientFd);
			}
		} else {
			//Client wrote something or disconnected
			for (int i = 0; i < MAX_CLIENTS; ++i) {
				if (FD_ISSET(file_desc_list[i], &fdSet)) {
					int fd = file_desc_list[i];
					int status = handle_client_write(fd);
					if (status < 1) {
						_info("Client is finished. Status: %d", status);
						remove_client_fd(file_desc_list, fd);
						close(fd);
					}
				}
			}
		}
	}

	disconnect_clients(file_desc_list);
}

int
main() {
	int sock = socket(PF_INET, SOCK_STREAM, 0);

	DIE(sock, "Failed to open socket.");

	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_port = htons(PORT_NUMBER);

	int status = bind(sock, (struct sockaddr*) &addr, sizeof(addr));

	DIE(status, "Failed to bind to port.");

	_info("Calling listen.");
	status = listen(sock, 10);
	_info("listen returned.");

	DIE(status, "Failed to listen.");

	server_loop(sock);

	close(sock);

	return 0;
}
