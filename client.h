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


struct sort_res
{
	int array_size;
	int algorithm_size;
	char * algorithm;
	double execution_time_s;
};


void send_data(struct parsed_command cmnd, int sock_fd, struct sockaddr_in addr_con, int addrlen);
void receive_results(int sock_fd, struct sockaddr_in addr_con, int addrlen);

struct parsed_command * parse_input(char * command);

void send_string(char * string_to_send, int sock_fd, struct sockaddr_in addr_con, int addrlen);
void send_file(char * filename, int sock_fd, struct sockaddr_in addr_con, int addrlen);
void receive_file(char * filename, int sock_fd, struct sockaddr_in addr_con, int addrlen);

void start_console_dialog() {
	int sock_fd;

	printf("Enter what kind of benchmark you want to run...:\n");
	printf("In order to start the benchmark enter command with the following format:\n");
	printf("sort <pathToFileWithData1> <pathToFileWithData2> -a <algorithmName1> <algorithmName2> <algorithmName3>\n");
	printf("You can pass as many alogrithms as you want from the following list:\n ");
	printf("BubbleSort, QuickSort, SelectionSort, HeapSort, StableSelectionSort\n");

	char buffer[100];

	scanf("%[^\n]", buffer);

	struct parsed_command * cmnd = parse_input(buffer);

	struct sockaddr_in addr_con;
	int addrlen = sizeof(addr_con);

	addr_con.sin_family = AF_INET;
	addr_con.sin_port = htons(PORT);
	addr_con.sin_addr.s_addr = INADDR_ANY;
	sock_fd = socket(AF_INET, SOCK_DGRAM, 0);

	send_data(*cmnd, sock_fd, addr_con, addrlen);

	receive_results(sock_fd, addr_con, addrlen);
}


int write_to_file(int * fd, char * buffer) {
	int i;
	for(i = 0; i < BUFFER_SIZE; i++) {
		if(buffer[i] == '\0') {
			break;
		}
		if(buffer[i] == EOF) {
			break;
		}
	}
	int size = write(*fd, buffer, i);
	if(size < 0) {
		perror("Error while writing");
	}
	return i != BUFFER_SIZE;
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
		return 0;
	}
	if(sizing != BUFFER_SIZE) {
		buf[sizing] = EOF;
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


void send_data(struct parsed_command cmnd, int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	int i, indx = 0;

	char buffer[BUFFER_SIZE];

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
	printf("%s sending file\n", filename);
	int fd = open(filename, O_RDONLY);
	if(fd < 0) {
		perror("Error on file to send opening");
	}
	char buffer[BUFFER_SIZE];

	if (sock_fd < 0) {
		perror("socket initialization failed\n");
	} else {
		while(1) {
			int read = read_file_buf(&fd, buffer);
			if(sendto(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, addrlen) < 0) {
				perror("Error sending file content");
			}
			if(!read) {
				break;
			}
			memset(buffer, '\0', BUFFER_SIZE);
		}
	}

	if (close(fd) < 0)  {
		perror("Error on file to send closing");
	}
}


void receive_file(char * filename, int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	char buffer[BUFFER_SIZE];
	int fd;

	if((fd = open(filename, O_CREAT | O_WRONLY, 0777)) < 0) {
		perror("Error on file open");
	}

	while(1) {
		memset(buffer, '\0', BUFFER_SIZE);
		printf("Waiting for data...\n");
		if(recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, &addrlen) < 0) {
			perror("Error while receiving file content");
		}
		printf("Data Recevide: %s\n", filename);
		if(write_to_file(&fd, buffer)) {
			break;
		}
	}
	if(close(fd) < 0) {
		perror("Error on file close");
	}
}

void receive_bin_file(char * filename, int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	char buffer[BUFFER_SIZE];
	int fd;

	if((fd = open(filename, O_CREAT | O_WRONLY, 0777)) < 0) {
		perror("Error on file open");
	}

	while(1) {
		memset(buffer, '\0', BUFFER_SIZE);
		printf("Waiting for data...\n");
		if(recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, &addrlen) < 0) {
			perror("Error while receiving file content");
		}
		printf("Data Recevide: %s\n", filename);
		int i;
		for(i = 0; i < BUFFER_SIZE; i++) {
			if(buffer[i] == EOF) {
				break;
			}
		}
		int size = write(fd, buffer, i);
		if(size != BUFFER_SIZE) {
			break;
		}
	}
	if(close(fd) < 0) {
		perror("Error on file close");
	}
}

void receive_results(int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	receive_bin_file("benchmark_results", sock_fd, addr_con, addrlen);
}