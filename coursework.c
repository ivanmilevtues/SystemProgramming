// Тема №29
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "sorting.h"
#include "client.h"

#define CHUNK_SIZE 10

int load_array(char * filename, int ** array);

void benchmark_sort(void (*f)(int *, int), int * array, int array_size);

void * get_algorithm(char * algorithm_name);

void start_benchmark(struct parsed_command cmnd);

void start_console_dialog();

void start_server();

struct parsed_command * receive_command(int sock_fd, struct sockaddr_in addr_con, int addrlen);

void receive_string(char ** filenames, int size, int sock_fd, struct sockaddr_in addr_con, int addrlen);

void receive_files(char ** filenames, int size, int sock_fd, struct sockaddr_in addr_con, int addrlen);

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

void start_benchmark(struct parsed_command cmnd) {
	int i, k;
	for(k = 0; k < cmnd.filenames_size; k++) {
		for(i = 0; i < cmnd.algos_size; i++) {
			int * data_array;
			printf("%s\n", cmnd.filenames[k]);
			printf("%d\n", cmnd.filenames[k]);
			int size = load_array(cmnd.filenames[k], &data_array);
			void * algorithm = get_algorithm(cmnd.algorithms[i]);
			benchmark_sort(algorithm, data_array, size);
		}
	}
}

void * get_algorithm(char * algorithm_name) {
	struct sorting_algorithm sorting_algos[] = {
		{.algorithm_name = "BubbleSort", .sorting_algo_f = &bubble_sort},
		{.algorithm_name = "QuickSort", .sorting_algo_f = &quick_sort},
		{.algorithm_name = "SelectionSort", .sorting_algo_f = &selection_sort},
		{.algorithm_name = "HeapSort", .sorting_algo_f = &heap_sort},
		{.algorithm_name = "StableSelectionSort", .sorting_algo_f = &stable_selection_sort},
	};
	int i;
	for(i = 0; i < 5; i++) {
		if(strcmp(sorting_algos[i].algorithm_name, algorithm_name) == 0) {
			return sorting_algos[i].sorting_algo_f;
		}
	}
}

int load_array(char * filename, int ** result_array) {
	printf("fname - %s\n", filename);
	printf("%d\n", filename);
	FILE * fd = fopen(filename, "r");
	if(fd == NULL) {
		perror("Error on file opening");
	}
	int allocated_size = CHUNK_SIZE;
	int size = 0;
	int * array = malloc(sizeof(int) * allocated_size);
	while (fscanf(fd, "%d", array + size) >= 0) {
		if (++size > allocated_size - 1) {
			allocated_size += CHUNK_SIZE;
			array = realloc(array, sizeof(int) * allocated_size);
		}
		printf("Reading %d %d\n", size, allocated_size);

	}
	*(result_array) = array;

	fclose(fd);
	return size;
}


void benchmark_sort(void (*sorting_algo) (int *, int), int * array, int array_size) {
	clock_t begin = clock();

	(*sorting_algo)(array, array_size);
	
	clock_t end = clock();

	printf("%ld, %ld, %ld, %f\n", begin, end, end - begin, (double)(end - begin) / CLOCKS_PER_SEC );
	free(array);
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


void start_server() {
	int i, size = BUFFER_SIZE;
	struct sockaddr_in addr_con;
	char filename[128] = "test/";
	char file[BUFFER_SIZE];

	int addrlen = sizeof(addr_con);

	addr_con.sin_family = AF_INET;
	addr_con.sin_port = htons(PORT);
	addr_con.sin_addr.s_addr = INADDR_ANY;


	int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sock_fd < 0) {
		perror("Error on socket connection");
	} else if(bind(sock_fd, (struct sockaddr*)&addr_con, sizeof(addr_con))) {
		perror("Error while binding");
	} else {
		while (1) {
			struct parsed_command * cmnd = receive_command(sock_fd, addr_con, addrlen);
			start_benchmark(*cmnd);
		}
	}
}

struct parsed_command * receive_command(int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	struct parsed_command * cmnd = malloc(sizeof(struct parsed_command));
	int size = 0;
	if (recvfrom(sock_fd, &size, sizeof(int), 0, (struct sockaddr*) &addr_con, &addrlen) < 0) {
		perror("Error recieving data");
	}

	printf("File size recieved %d\n", size);

	cmnd->filenames_size = size;
	cmnd->filenames = malloc(size * sizeof(char*));
	receive_string(cmnd->filenames, size, sock_fd, addr_con, addrlen);

	if (recvfrom(sock_fd, &size, sizeof(int), 0, (struct sockaddr*) &addr_con, &addrlen) < 0) {
		perror("Error recieving data\n");
	}

	printf("Algorithm size recieved %d\n", size);

	cmnd->algos_size = size;
	cmnd->algorithms = malloc(size * sizeof(char*));
	receive_string(cmnd->algorithms, size, sock_fd, addr_con, addrlen);

	receive_files(cmnd->filenames, cmnd->filenames_size, sock_fd, addr_con, addrlen);

	return cmnd;
}

int read_from_buffer(int indx, char * buffer, char * array) {
	int i;
	for(i = 0; i < BUFFER_SIZE; i++, indx++) {
		if(buffer[i] == '\0') {
			return -1;
		}
		array[indx] = buffer[i];
	}

	return indx;
}


void receive_string(char ** array_to_hold, int size, int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	char buffer[BUFFER_SIZE];
	memset(buffer, '\0', BUFFER_SIZE);
	int i, indx;
	
	for(i = 0; i < size; i++) {
		array_to_hold[i] = malloc(sizeof(char) * size);
		recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, &addrlen);
		indx = 0;
		indx = read_from_buffer(indx, buffer, array_to_hold[i]);
		
		while(indx != -1)  {
			size += BUFFER_SIZE;
			array_to_hold[i] = realloc(array_to_hold[i], sizeof(char) * size);

			memset(buffer, '\0', BUFFER_SIZE);
			recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, &addrlen);
			indx = read_from_buffer(indx, buffer, array_to_hold[i]);
		}	
	}
}

void receive_files(char ** filenames, int size, int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	int i, fd;
	char buffer[BUFFER_SIZE];
	printf("Expecting %d files\n", size);
	for(i = 0; i < size; i++) {
		char filename[256] = "test/";
		strcat(filename, filenames[i]);
		free(filenames[i]);

		filenames[i] = filename;

		printf("filename: %s\n", filename);

		if((fd = open(filename, O_CREAT | O_WRONLY, 0777)) < 0) {
			perror("Error on file open");
		}

		while(1) {
			memset(buffer, '\0', BUFFER_SIZE);
			recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, &addrlen);
			if(write_to_file(&fd, buffer)) {
				break;
			}
		}
		if(close(fd) < 0) {
			perror("Error on file close");
		}
	}
}