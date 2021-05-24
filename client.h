#define LINE_SIZE 1000

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


/*
 * Will send the parsed command as well as the needed files for the benchmark to the server
 * struct parsed_command cmnd - the command which will be sent
 * int sock_fd - socket file descriptor
 * struct sockaddr_in addr_con - destination address
 * int addrlen - the size of the addr
 */
void send_data(struct parsed_command cmnd, int sock_fd, struct sockaddr_in addr_con, int addrlen);

/*
 * Will expect and read the contents of a the file with the benchmark results from the socket. 
 * The read file will be saved in benchmark_result
 * 
 * int sock_fd - socket file descriptor
 * struct sockaddr_in addr_con - destination address
 * int addrlen - the size of the addr
 */
void receive_results(int sock_fd, struct sockaddr_in addr_con, int addrlen);

/*
 * Will parse the command entered by the user into struct parsed_commadn
 * char * command - string which is the user's input line
 * returns the parsed result
 */
struct parsed_command * parse_input(char * command);

/*
 * Will print the results of the benchmark to stdout
 * char * filename - path to the file in which the results are held
 */
void show_results(char * filename);

void start_client() {
	printf("Client started\n");
	while(1) {
		int sock_fd;

		printf("Enter what kind of benchmark you want to run...:\n");
		printf("In order to start the benchmark enter command with the following format:\n");
		printf("sort <pathToFileWithData1> <pathToFileWithData2> -a <algorithmName1> <algorithmName2> <algorithmName3>\n");
		printf("You can pass as many alogrithms as you want from the following list:\n ");
		printf("BubbleSort, QuickSort, SelectionSort, HeapSort, StableSelectionSort\n");

		char buffer[LINE_SIZE];

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

		show_results("benchmark_results");
	}	
}

void show_results(char * filename) {
	int fd = open(filename, O_RDONLY);
	if(fd < 0) {
		perror("Error while reading benchmark results file");
		return;
	}

	struct sort_res res;
	char algorithm_name[BUFFER_SIZE];
	while(read(fd, &res, sizeof(res))) {
		memset(algorithm_name, '\0', BUFFER_SIZE);
		read(fd, algorithm_name, res.algorithm_size * sizeof(char));
		if(res.algorithm_size == 0) {
			break;
		}
		printf("%s took %f s to finsih sorting array of size %d\n", algorithm_name, res.execution_time_s, res.array_size);
	}

	printf("The last printed algorithm is the best performer!\n");

	close(fd);
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


void receive_bin_file(char * filename, int sock_fd, struct sockaddr_in addr_con, int addrlen) {
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