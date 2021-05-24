// Тема №29
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <float.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "sorting.h"
#include "utils.h"
#include "socket_utils.h"
#include "client.h"

// sort test_data_1 test_data_2 test_data_3 test_data_4 test_data_5 -a QuickSort BubbleSort SelectionSort HeapSort StableSelectionSort

#define CHUNK_SIZE 64

/*
 * Will load the contents of file with name filename in the giver array
 * char * filename - filename in which the data is stored
 * int ** array - pointer to the array in which the data will loaded
 * returns the size of the array in which the data was loaded
 */
int load_array(char * filename, int ** array);

/*
 * Will benchmark the passed algorithm with the given array
 * void (*f)(int *, int) - pointer to the method which will be used for sorting.
 * int * array - the array which will be sorted by the passed method
 * int array_size - the size of the passed array
 * returns the time taken for the benchmark to complete in seconds
 */
double benchmark_sort(void (*f)(int *, int), int * array, int array_size);

/*
 * Given the algorithm name the function will return pointer to the coresponding function
 * char * algorithm_name - the name of the algorithm
 * returns pointer to the function for the algorithm_name if not found void will be returned
 */
void * get_algorithm(char * algorithm_name);

/*
 * Execute the complete benchmark - times it and returns the results to the client
 * struct parsed_command * cmnd - command which will be used as parameters for the benchmark
 */
void execute_benchmark(struct parsed_command * cmnd);

/*
 * Starts the server
 */
void start_server();


/*
 * Will listen for the client commands and execute them(start the benchmark)
 * int sock_fd - socket file descriptor
 * struct sockaddr_in addr_con - destination address
 * int addrlen - the size of the addr
 */
void listen_for_client(int sock_fd, struct sockaddr_in addr_con, int addrlen);

/*
 * Will expect and read the contents of number of files from the socket. The read files will be save in files with names from filenames
 * For each file recieve_file will be called.
 * char ** filenames - the names of the files in which the received contents will be saved
 * int size - the size of filenames
 * int sock_fd - socket file descriptor
 * struct sockaddr_in addr_con - destination address
 * int addrlen - the size of the addr
 */
void receive_files(char ** filenames, int size, int sock_fd, struct sockaddr_in addr_con, int addrlen);


/*
 * Will send the contents of the benchmark file
 * int sock_fd - socket file descriptor
 * struct sockaddr_in addr_con - destination address
 * int addrlen - the size of the addr
 */
void send_benchmark_data(int sock_fd, struct sockaddr_in addr_con, int addrlen);

/* 
 * Will write struct sort_res result in the binary file with descriptor fd
 * int * fd - file descriptor
 * struct sort_res result - structure to be written
 */
void write_sort_result(int * fd, struct sort_res result);

/*
 * Will check if algoname is contained in algos. The check is done via POINTER ADDRESS NOT STRCMP.
 * char * algoname - the searched string
 * char ** algos - the collection in which it is searched
 * int size - the size of algos
 * returns 0 if algoname not found else 1
 */
int in_slowest(char * algoname, char ** algos, int size);

struct sorting_algorithm {
	char * algorithm_name;
	void (* sorting_algo_f) (int *, int);
};



int main(int argc, char ** argv) {
	if(argc == 1) {
		int pid = fork();
		if (pid != 0) {
			start_server();
		} else {
			start_client();
		}	
	} else {
		if(strcmp(argv[1], "-s") == 0) {
			start_server();
		} else {
			start_client();
		}
	}
	return 0;
}


void start_server() {
	printf("Server started\n");
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
			listen_for_client(sock_fd, addr_con, addrlen);
		}
	}
}


void write_sort_result(int * fd, struct sort_res result) {
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
	printf("benchmark started\n");
	int fd = open("test_file", O_CREAT | O_WRONLY, 0777);
	if(fd < 0) {
		perror("Error while opening file for benchmark results");
	}

	char ** slowest_algos = malloc(cmnd -> algos_size * sizeof(char*));
	for(k = 0; k < cmnd -> filenames_size; k++) {
		double slowest_time = DBL_MIN;
		for(i = 0; i < cmnd -> algos_size; i++) {
			if(in_slowest(cmnd -> algorithms[i], slowest_algos, k + 1 < cmnd -> algos_size ? k + 1: cmnd -> algos_size)) {
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

			write_sort_result(&fd, result);
		}
	}
	printf("benchmark finshed\n");
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


void listen_for_client(int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	struct parsed_command * cmnd = malloc(sizeof(struct parsed_command));
	int size = 0;

	if (recvfrom(sock_fd, &size, sizeof(int), 0, (struct sockaddr*) &addr_con, &addrlen) < 0) {
		perror("Error recieving data");
	}

	cmnd -> filenames_size = size;
	cmnd -> filenames = malloc(size * sizeof(char*));
	receive_string(cmnd -> filenames, size, sock_fd, addr_con, addrlen);

	if (recvfrom(sock_fd, &size, sizeof(int), 0, (struct sockaddr*) &addr_con, &addrlen) < 0) {
		perror("Error recieving data\n");
	}
	cmnd -> algos_size = size;
	cmnd -> algorithms = malloc(size * sizeof(char*));

	receive_string(cmnd -> algorithms, size, sock_fd, addr_con, addrlen);

	receive_files(cmnd -> filenames, cmnd -> filenames_size, sock_fd, addr_con, addrlen);

	execute_benchmark(cmnd);
	free_command_memory(cmnd);
	send_benchmark_data(sock_fd, addr_con, addrlen);

}


void receive_files(char ** filenames, int size, int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	int i, fd;
	for(i = 0; i < size; i++) {
		char filename[258] = "test/";
		strcat(filename, filenames[i]);
		receive_file(filename, sock_fd, addr_con, addrlen);
	}
}


void send_benchmark_data(int sock_fd, struct sockaddr_in addr_con, int addrlen) {
	send_file("test_file", sock_fd, addr_con, addrlen);
}