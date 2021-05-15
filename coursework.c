// Тема №29
#include <stdio.h> 
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 10

 
int load_array(char * filename, int ** array);

 
int main() {
	int * array;
	int size = load_array("test_data_1", &array);
	int i;
	for(i = 0; i < size; i++) {
		printf("%d\n", array[i]);
	}
	return 0;
}
 
int load_array(char * filename, int ** result_array) {
	FILE * fd = fopen(filename, "r");
	int allocated_size = CHUNK_SIZE;
	int size = 0;
	
	int * array = malloc(sizeof(int) * allocated_size);

	while(fscanf(fd, "%d", array + size) >= 0) {
		if (++size > allocated_size) {
			allocated_size += CHUNK_SIZE;
			array = realloc(array, sizeof(int) * allocated_size);
		}

	}
	*(result_array) = array;
	return size;
}