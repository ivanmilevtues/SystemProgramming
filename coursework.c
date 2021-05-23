// Тема №29
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sorting.h"
#include "client.h"

#define CHUNK_SIZE 10

int load_array(char * filename, int ** array);

void benchmark_sort(void (*f)(int *, int), int * array, int array_size);

void init_benchmark();

void start_console_dialog();

void start_server();

struct sorting_algorithm {
	char * algorithm_name;
	void (* sorting_algo_f) (int *, int);
};


struct sort_res
{
	int array_size;
	char * filename;
	char * sorting_algo;
};


int main(int argc, char ** argv) {
	int pid = fork();

	if (pid != 0) {
		start_server();
	} else {
		start_console_dialog();
	}
	return 0;
}


void init_benchmark() {
	struct sorting_algorithm sorting_algos[] = {
		{.algorithm_name = "BubbleSort", .sorting_algo_f = &bubble_sort},
		{.algorithm_name = "QuickSort", .sorting_algo_f = &quick_sort},
		{.algorithm_name = "SelectionSort", .sorting_algo_f = &selection_sort},
		{.algorithm_name = "HeapSort", .sorting_algo_f = &heap_sort},
		{.algorithm_name = "StableSelectionSort", .sorting_algo_f = &stable_selection_sort},
	};
}


int load_array(char * filename, int ** result_array) {
	FILE * fd = fopen(filename, "r");
	int allocated_size = CHUNK_SIZE;
	int size = 0;

	int * array = malloc(sizeof(int) * allocated_size);

	while (fscanf(fd, "%d", array + size) >= 0) {
		if (++size > allocated_size - 1) {
			allocated_size += CHUNK_SIZE;
			array = realloc(array, sizeof(int) * allocated_size);
		}

	}
	*(result_array) = array;

	fclose(fd);
	return size;
}


void benchmark_sort(void (*sorting_algo) (int *, int), int * array, int array_size) {
	time_t start, end;
	start = time(NULL);
	(*sorting_algo)(array, array_size);
	end = time(NULL);

	printf("%ld, %ld, %ld\n", start, end, end - start);
	free(array);
}


int write_to_file(int * fd, char * buffer) {
	int i;
	for(i = 0; i < BUFFER_SIZE; i++) {
		printf("%c\n", buffer[i]);
		if(buffer[i] == '\0') {
			return 1;
		}
		if(buffer[i] == EOF) {
			return 1;
		}
	}
	int size = write(*fd, buffer, i);
	if(size < 0) {
		perror("Error while writing");
	}
	return 0;
}


void start_server() {
	int i, size = BUFFER_SIZE;
	struct sockaddr_in addr_con;
	char filename[128] = "test/";
	char file[BUFFER_SIZE];

	int addrlen = sizeof(addr_con);

	addr_con.sin_family = AF_INET;
	addr_con.sin_port = htons(PORT);
	addr_con.sin_addr.s_addr = INADDR_ANY;

	char buffer[BUFFER_SIZE];

	int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0) {
		perror("Error on socket connection");
	} else if(bind(sock_fd, (struct sockaddr*)&addr_con, sizeof(addr_con))) {
		perror("Error while binding");
	} else {
		while (1) {
			struct parsed_command cmnd;
			int indx;

			if (recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, &addrlen) < 0) {
				perror("Error recieving data");
			}

			cmnd.filename = malloc(sizeof(char) * size);
			while((indx = write_to_buffer(indx, buffer, cmnd.filename)) != -1)  {
				memset(buffer, '\0', BUFFER_SIZE);
				recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, &addrlen);
				size += BUFFER_SIZE;
				cmnd.filename = realloc(cmnd.filename, sizeof(char) * size);
			}
			strcat(filename, cmnd.filename);
			free(cmnd.filename);
			cmnd.filename = filename;

			int fd = open(filename, O_CREAT | O_WRONLY, 0777);
			if (fd < 0) {
				perror("Error opening file");
			}

			int number_of_algos;
			recvfrom(sock_fd, &number_of_algos, sizeof(int), 0, (struct sockaddr*) &addr_con, &addrlen);

			for(i = 0; i < number_of_algos; i++) {
				memset(buffer, '\0', BUFFER_SIZE);
				recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, &addrlen);
				cmnd.algorithms[i] = malloc(sizeof(char) * BUFFER_SIZE);
				strcpy(cmnd.algorithms[i], buffer);
			}

			while(1) {
				memset(buffer, '\0', BUFFER_SIZE);
				recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, &addrlen);
				if(write_to_file(&fd, buffer)) {
					break;
				}
			}
			close(fd);
		}
	}
}