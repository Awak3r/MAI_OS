#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
float  buf_to_numbers(char buf[]) {
    float n = 10, num2 = 0, result;
    int num_path, i = 0, flag = 0, min_flag = 0, first_flag = 0;
    while (true){
        if (buf[i] != '1' && buf[i] != '2' && buf[i] != '3' && buf[i] != '4' && buf[i] != '5' \
            && buf[i] != '6' && buf[i] != '7' && buf[i] != '8'&& buf[i] != '9' && buf[i] != '0' && buf[i] != '.' && buf[i] != ' ' \
            && buf[i] != '-' && buf[i] !='\n'){
                const char msg[] = "error: invalid data\n";
                write(STDOUT_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
        }
        if (buf[i] == ' '){
            n = 10;
            flag = 0;
            if (min_flag == 1){
                num2 *= -1;
            }
            min_flag = 0;
            if (first_flag == 0){
                result = num2;
                first_flag = 1;
            }
            else if (num2 != 0 && first_flag == 1){
                result /= num2;
            }
            else if (num2 == 0 && first_flag == 1){
                const char msg[] = "error: can`t devide by zero\n";
                write(STDOUT_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
            num2 = 0;
        }
        else if (buf[i] == '\n' || buf[i] == '\0'){
            n = 10;
            flag = 0;
            if (min_flag == 1){
                num2 *= -1;
            }
            min_flag = 0;
            if (first_flag == 0){
                result = num2;
                first_flag = 1;
            }
            else if (num2 != 0 && first_flag == 1){
                result /= num2;
            }
            else if (num2 == 0 && first_flag == 1){
                const char msg[] = "error: can`t devide by zero\n";
                write(STDOUT_FILENO, msg, sizeof(msg));
                exit(EXIT_FAILURE);
            }
            num2 = 0;
            break;
        }
        else if (buf[i] == '.'){
            flag = 1;
            n = 0.1;
        }
        else if (buf[i] == '-'){
            min_flag = 1;
        }
        else if (flag == 0) {
            num2 = (buf[i] - '0') + num2 * n;
        }
        else if (flag == 1) {
            num2 += (buf[i] - '0') * n;
            n/=10;
        }
        ++i;
    }
    return result;
}



int main(int argc, char **argv) {
	char buf[4096];
	ssize_t bytes;
	int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
	if (file == -1) {
		const char msg[] = "error: failed to open requested file\n";
	    write(STDOUT_FILENO, msg, sizeof(msg));
		exit(EXIT_FAILURE);
	}

	while ((bytes = read(STDIN_FILENO, buf, sizeof(buf)))) {
		if (bytes < 0) {
			const char msg[] = "error: failed to read from stdin\n";
			write(STDOUT_FILENO, msg, sizeof(msg));
			exit(EXIT_FAILURE);
		}

        float result = buf_to_numbers(buf);
        bytes = sprintf(buf, "%.4f\n", result);
		{
			int32_t written = write(file, buf, bytes);
			if (written != bytes) {
				const char msg[] = "error: failed to write to file\n";
				write(STDOUT_FILENO, msg, sizeof(msg));
				exit(EXIT_FAILURE);
			}       
		}
        const char msg[] = "success";
        write(STDOUT_FILENO, msg, sizeof(msg));
	}
	if (bytes == 0) {
		const char term = '\0';
		write(file, &term, sizeof(term));
	}

	close(file);
}