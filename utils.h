#define BUFFER_SIZE 256

/*
 * Writes buffer conenten until '\0' or EOF to the fd.
 * int * fd - file descriptior - not closed within the method
 * char * buffer - buffer's content will be writtein to the file
 * return 0 if the written conten is less than the buffer size else 1
 */
int write_to_file(int * fd, char * buffer);


/*
 * int * fd - file descriptior - not closed within the method
 * char * buffer - the buffer in which the file contents will be writter
 * return 0 if EOF or '\0' reached within reading else 1
 */
int read_file_buf(int* fd, char * buf);


/*
 * Writes until '\0' or EOF in buffer from read_from or BUFFER_SIZE is reached
 * int indx - indx from which to start iterating the read_from parameter
 * char * read_from - array from which data will be transfered
 * char * buffer - array in which data will be transfered
 * return new value for indx(in order to know to which index we've read read_from) or -1 if EOF or '\0' was reached
 */
int write_to_buffer(int indx, char * read_from, char * buffer);

/*
 * Writes until '\0' or EOF in array from buffer or BUFFER_SIZE is reached
 * int indx - indx from which data will be written in array
 * char * buffer - array from which data will be transfered
 * char * array - array in which data will be transfered
 * return new value for indx in order to know to which index we've written in array or -1 if EOF or '\0' was reached
 */
int read_from_buffer(int indx, char * buffer, char * array);


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
