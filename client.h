#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define PORT 8080

#define BUFFER_SIZE 64


struct parsed_command {
	char * filename;
	int number_of_algos;
	char * algorithms[5];
};

void send_data(struct parsed_command cmnd);
struct parsed_command * algorithms(char * command);


void start_console_dialog() {
	printf("Enter what kind of benchmark you want to run...:\n");
	printf("In order to start the benchmark enter command with the following format:\n");
	printf("sort <pathToFileWithBenchmarkData> <algorithmName1> <algorithmName2> <algorithmName3>\n");
	printf("You can pass as many alogrithms as you want from the following list:\n ");
	printf("BubbleSort, QuickSort, SelectionSort, HeapSort, StableSelectionSort\n");

	char buffer[100];

	scanf("%[^\n]", buffer);

	struct parsed_command * cmnd = algorithms(buffer);

	send_data(*cmnd);
}


// TODO: Add validation...
struct parsed_command * algorithms(char * command) {
	struct parsed_command * parsed_cmnd = malloc(sizeof(struct parsed_command));
	int indx = 0;

	char first_try = 1;
	char * token = strtok(command, " ");
	token = strtok(NULL, " "); // skip the token of "sort"
	while(token != NULL) {
		if(first_try) {
			first_try = 0;
			(*parsed_cmnd).filename = token;
		} else {
			(*parsed_cmnd).algorithms[indx] = token;
			indx++;
		}

		token = strtok(NULL, " ");
	}
	parsed_cmnd.number_of_algos = indx;
	return parsed_cmnd;
}


int read_file_buf(int* fd, char * buf) {
	int sizing = read(*fd, buf, BUFFER_SIZE);
	if(sizing < 0) {
		perror("An error occured while reading file");
	}
	return sizing != 0;
}


int write_to_buffer(int indx, char * read_from, char * buffer) {
	int i;
	for(i = 0; i < BUFFER_SIZE; i++, indx++) {
		if(read_from[indx] == '\0') {
			return -1;
		}
		buffer[i] = read_from[indx];
	}

	return indx;
}


void send_data(struct parsed_command cmnd) {
	int sock_fd, i;
	struct sockaddr_in addr_con;
	int addrlen = sizeof(addr_con);

	addr_con.sin_family = AF_INET;
	addr_con.sin_port = htons(PORT);
	addr_con.sin_addr.s_addr = INADDR_ANY;

	char buffer[BUFFER_SIZE];

	memset(buffer, '\0', BUFFER_SIZE);

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

	int fd = open(cmnd.filename, O_RDONLY);

	sendto(sock_fd, cmnd.filename, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, addrlen);
	sendto(sock_fd, cmnd.number_of_algos, sizeof(int), 0, (struct sockaddr *) &addr_con, addrlen);

	for(i = 0; i < cmnd.number_of_algos; i++) {
		int indx = 0;
		while(indx != -1) {
			memset(buffer, '\0', BUFFER_SIZE);
			indx = write_to_buffer(indx, cmnd.alogrithms[i], buffer);
			sendto(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, addrlen);
		}
	}

	if (sock_fd < 0) {
		perror("socket initialization failed\n");
	} else {
		while(read_file_buf(&fd, buffer)) {
			sendto(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, addrlen);
			memset(buffer, '\0', BUFFER_SIZE);
		}
	}

	close(fd);
}