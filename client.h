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

#define BUFFER_SIZE 256


struct parsed_command {
	int filenames_size;
	char ** filenames;
	int algos_size;
	char ** algorithms;
};

void send_data(struct parsed_command cmnd);
struct parsed_command * parse_input(char * command);

void send_string(char * string_to_send, int sock_fd, struct sockaddr_in addr_con, int addrlen);
void send_file(char * filename, int sock_fd, struct sockaddr_in addr_con, int addrlen);

void start_console_dialog() {
	printf("Enter what kind of benchmark you want to run...:\n");
	printf("In order to start the benchmark enter command with the following format:\n");
	printf("sort <pathToFileWithData1> <pathToFileWithData2> -a <algorithmName1> <algorithmName2> <algorithmName3>\n");
	printf("You can pass as many alogrithms as you want from the following list:\n ");
	printf("BubbleSort, QuickSort, SelectionSort, HeapSort, StableSelectionSort\n");

	char buffer[100];

	scanf("%[^\n]", buffer);

	struct parsed_command * cmnd = parse_input(buffer);

	send_data(*cmnd);
}


struct parsed_command * parse_input(char * command) {
	struct parsed_command * parsed_cmnd = malloc(sizeof(struct parsed_command));

	int file_indx = 0, algo_indx = 0, algo_flag = 0, filenames_size = 2, algos_size = 2;

	char * token = strtok(command, " ");
	token = strtok(NULL, " "); // skip the token of "sort"

	(*parsed_cmnd).filenames =  malloc(filenames_size * sizeof(char *));
	(*parsed_cmnd).algorithms =  malloc(algos_size * sizeof(char *));

	while(token != NULL) {
		if(strcmp(token, "-a") == 0) {
			token = strtok(NULL, " ");
			algo_flag = 1;
			continue;
		}

		if(!algo_flag) {
			if(file_indx >= filenames_size) {
				filenames_size += 2;
				(*parsed_cmnd).filenames =  realloc((*parsed_cmnd).filenames, filenames_size * sizeof(char *));
			}
			(*parsed_cmnd).filenames[file_indx++] = token;

		} else {
			if(algo_indx >= algos_size) {
				algos_size += 2;
				(*parsed_cmnd).algorithms =  realloc((*parsed_cmnd).algorithms, algos_size * sizeof(char *));
			}
			(*parsed_cmnd).algorithms[algo_indx++] = token;
		}

		token = strtok(NULL, " ");
	}

	(*parsed_cmnd).algos_size = algo_indx;
	(*parsed_cmnd).filenames_size = file_indx;
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
	int sock_fd, i, indx = 0;
	struct sockaddr_in addr_con;
	int addrlen = sizeof(addr_con);

	addr_con.sin_family = AF_INET;
	addr_con.sin_port = htons(PORT);
	addr_con.sin_addr.s_addr = INADDR_ANY;

	char buffer[BUFFER_SIZE];

	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

	sendto(sock_fd, &cmnd.filenames_size, sizeof(int), 0, (struct sockaddr *) &addr_con, addrlen);

	for(i = 0; i < cmnd.filenames_size; i++) {
		send_string(cmnd.filenames[i], sock_fd, addr_con, addrlen);
	}

	sendto(sock_fd, &cmnd.algos_size, sizeof(int), 0, (struct sockaddr *) &addr_con, addrlen);

	for(i = 0; i < cmnd.algos_size; i++) {
		send_string(cmnd.algorithms[i], sock_fd, addr_con, addrlen);
	}

	for(i = 0; i < cmnd.filenames_size; i++) {
		send_file(cmnd.filenames[i], sock_fd, addr_con, addrlen);
	}
}


void send_string(char * string_to_send, int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	char buffer[BUFFER_SIZE];
	int indx = 0;
	while(indx != -1) {
		memset(buffer, '\0', BUFFER_SIZE);
		indx = write_to_buffer(indx, string_to_send, buffer);
		sendto(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, addrlen);
	}
}


void send_file(char * filename, int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	int fd = open(filename, O_RDONLY);
	char buffer[BUFFER_SIZE];

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