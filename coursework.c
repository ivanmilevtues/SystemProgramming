// Тема №29
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "sorting.h"

#define CHUNK_SIZE 10
 
int load_array(char * filename, int ** array);

 
void benchmark_sort(void (*f)(int *, int), int * array, int array_size);

int main() {
	int * array;
	int size = load_array("test_data_5", &array);
	
	benchmark_sort(&bubble_sort, array, size);

	return 0;
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