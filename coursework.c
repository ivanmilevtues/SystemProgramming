// Тема №29
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <float.h>

#include "sorting.h"
#include "client.h"

#define CHUNK_SIZE 64

int load_array(char * filename, int ** array);

double benchmark_sort(void (*f)(int *, int), int * array, int array_size);

void * get_algorithm(char * algorithm_name);

void execute_benchmark(struct parsed_command * cmnd);

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
	int algorithm_size;
	char * algorithm;
	double execution_time_s;
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


void write_res(int * fd, struct sort_res result) {
	write(*fd, &result, sizeof(struct sort_res));
	write(*fd, result.algorithm, sizeof(char) * result.algorithm_size);
}

int in_slowest(char * algoname, char ** algos, int size) {
	int i;
	for (i = 0; i < size; i++) {
		if (algos[i] == algoname) {
			return 1;
		}
	}
	return 0;
}


void execute_benchmark(struct parsed_command * cmnd) {
	int i, k;

	int fd = open("test_file", O_CREAT | O_WRONLY);

	char ** slowest_algos = malloc(cmnd -> algos_size * sizeof(char*));
	for(k = 0; k < cmnd -> filenames_size; k++) {
		double slowest_time = DBL_MIN;
		for(i = 0; i < cmnd -> algos_size; i++) {
			if(in_slowest(cmnd -> algorithms[i], slowest_algos, k + 1)) {
				continue;
			}

			struct sort_res result;
			int * data_array;

			int size = load_array(cmnd -> filenames[k], &data_array);
			result.array_size = size;

			void * algorithm = get_algorithm(cmnd -> algorithms[i]);
			result.algorithm = cmnd -> algorithms[i];
			result.algorithm_size = strlen(cmnd -> algorithms[i]);

			result.execution_time_s = benchmark_sort(algorithm, data_array, size);

			if(result.execution_time_s > slowest_time) {
				slowest_time = result.execution_time_s;
				slowest_algos[k] = cmnd -> algorithms[i];
			}

			write_res(&fd, result);
		}
	}

	free(slowest_algos);
	close(fd);
}


void * get_algorithm(char * algorithm_name) {
	struct sorting_algorithm sorting_algos[] = {
		{.algorithm_name = "BubbleSort\0", .sorting_algo_f = &bubble_sort},
		{.algorithm_name = "QuickSort\0", .sorting_algo_f = &quick_sort},
		{.algorithm_name = "SelectionSort\0", .sorting_algo_f = &selection_sort},
		{.algorithm_name = "HeapSort\0", .sorting_algo_f = &heap_sort},
		{.algorithm_name = "StableSelectionSort\0", .sorting_algo_f = &stable_selection_sort},
	};
	int i;
	for(i = 0; i < 5; i++) {
		if(strcmp(sorting_algos[i].algorithm_name, algorithm_name) == 0) {
			return sorting_algos[i].sorting_algo_f;
		}
	}
}


int load_array(char * filename, int ** result_array) {
	char fname[256] = "test/";
	strcat(fname, filename);

	FILE * fd = fopen(fname, "r");
	if(fd == NULL) {
		perror("Error on file opening");
	}

	int allocated_size = CHUNK_SIZE, size = 0;
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


double benchmark_sort(void (*sorting_algo) (int *, int), int * array, int array_size) {

	clock_t begin = clock();
	(*sorting_algo)(array, array_size);	
	clock_t end = clock();

	free(array);

	return (double)(end - begin) / CLOCKS_PER_SEC;
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
			execute_benchmark(cmnd);
		}
	}
}


struct parsed_command * receive_command(int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	struct parsed_command * cmnd = malloc(sizeof(struct parsed_command));
	int size = 0;
	if (recvfrom(sock_fd, &size, sizeof(int), 0, (struct sockaddr*) &addr_con, &addrlen) < 0) {
		perror("Error recieving data");
	}

	cmnd->filenames_size = size;
	cmnd->filenames = malloc(size * sizeof(char*));
	receive_string(cmnd->filenames, size, sock_fd, addr_con, addrlen);

	if (recvfrom(sock_fd, &size, sizeof(int), 0, (struct sockaddr*) &addr_con, &addrlen) < 0) {
		perror("Error recieving data\n");
	}
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
	for(i = 0; i < size; i++) {
		char filename[258] = "test/";
		strcat(filename, filenames[i]);

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