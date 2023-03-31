#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

int is_block_zero(char *block, int size) {
	for (int i = 0; i < size; i++){
		if (block[i] != 0)
			return 0;
	}
	return 1;
}

void close_files(int input, int output){
	close(input);
	close(output);
}

int handle_error(int input, int output, char *message){
	perror(message);
	close_files(input, output);
	return 1;
}


void create_sparse_file(int input, int output, int block_size){
	char *block = malloc(block_size);
	if (block == NULL){
		handle_error(input, output, "Memory allocation error");
		return;
	}
	
	while (block_size = read(input, block, block_size)) {
		if (errno){
			handle_error(input, output, "Reading from input error");
			break;
		}
		if (is_block_zero(block, block_size)){
			lseek(output, block_size, SEEK_CUR);
			if (errno){
				handle_error(input, output, "lseek error");
				break;
			}
		}
		else {
			write(output, block, block_size);
			if (errno){
				handle_error(input, output, "Writing in output file error");
				break;
			}
		}
	}
	
	free(block);
}

int main(int argc, char *argv[]){
	int input = 0;
	int output;
	int block_size = 4096;
	
	if (argc > 2) {
		input = open(argv[1], O_RDONLY);
		output = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
		if (argc == 4){
			block_size = atoi(argv[3]);
			if (block_size < 0)
				return handle_error(input, output, "Negative block size, you should use block_size > 0");
		}
	}
	else {
		output = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
		if (argc < 2) {
			printf("Not enough arguments\n");
			return 0;
		}
	}
	if (errno)
		return handle_error(input, output, "Open file error");
		
	create_sparse_file(input, output, block_size);
	close_files(input, output);
	
	return 0;
}


