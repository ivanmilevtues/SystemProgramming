#define PORT 8080

/*
 * Will send the string_to_send throw the sock_fd with addr_con
 * char * string_to_send - string which will be sent
 * int sock_fd - socket file descriptor
 * struct sockaddr_in addr_con - destination address
 * int addrlen - the size of the addr
 */
void send_string(char * string_to_send, int sock_fd, struct sockaddr_in addr_con, int addrlen);


/*
 * Will send the contents of the file with filename
 * char * filename - the name of the file which will be sent
 * int sock_fd - socket file descriptor
 * struct sockaddr_in addr_con - destination address
 * int addrlen - the size of the addr
 */
void send_file(char * filename, int sock_fd, struct sockaddr_in addr_con, int addrlen);


/*
 * Will expect and read the contents of a file from the socket. The read file will be save in file with name filename
 * char * filename - the name of the file in which the received contents will be saved
 * int sock_fd - socket file descriptor
 * struct sockaddr_in addr_con - destination address
 * int addrlen - the size of the addr
 */
void receive_file(char * filename, int sock_fd, struct sockaddr_in addr_con, int addrlen);


/*
 * Will expect and read string from the socket.
 * char ** array_to_hold - pointer to the char array in which the received data will be saved.
 * int sock_fd - socket file descriptor
 * struct sockaddr_in addr_con - destination address
 * int addrlen - the size of the addr
 */
void receive_string(char ** array_to_hold, int size, int sock_fd, struct sockaddr_in addr_con, int addrlen);


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
		if(recvfrom(sock_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr*) &addr_con, &addrlen) < 0) {
			perror("Error while receiving file content");
		}
		if(write_to_file(&fd, buffer)) {
			break;
		}
	}
	if(close(fd) < 0) {
		perror("Error on file close");
	}
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

