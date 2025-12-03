#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>
#include <ctype.h>

#define SHM_SIZE 4096
#define MIN_LEN 256

const char SHM_NAME[] = "lab_shm";
const char SEM_NAME[] = "lab_sem";
float  buf_to_numbers(char * buf) {
    float n = 10, num2 = 0, result;
    int num_path, i = 0, flag = 0, min_flag = 0, first_flag = 0;
    while (true){
        if (buf[i] != '1' && buf[i] != '2' && buf[i] != '3' && buf[i] != '4' && buf[i] != '5' \
            && buf[i] != '6' && buf[i] != '7' && buf[i] != '8'&& buf[i] != '9' && buf[i] != '0' && buf[i] != '.' && buf[i] != ' ' \
            && buf[i] != '-' && buf[i] !='\n'){
                return -1;
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
                return -1;
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
                return -1;
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
    if (argc < 2) {
        const char msg[] = "error: no filename provided\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    int shm = shm_open(SHM_NAME, O_RDWR, 0);
    if (shm == -1) {
        const char msg[] = "error: failed to open SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    char *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (shm_buf == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        close(shm);
        exit(EXIT_FAILURE);
    }
    sem_t *sem = sem_open(SEM_NAME, 0);
    if (sem == SEM_FAILED) {
        const char msg[] = "error: failed to open semaphore\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        munmap(shm_buf, SHM_SIZE);
        close(shm);
        exit(EXIT_FAILURE);
    }
    int32_t file = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (file == -1) {
        const char msg[] = "error: failed to open requested file\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        sem_close(sem);
        munmap(shm_buf, SHM_SIZE);
        close(shm);
        exit(EXIT_FAILURE);
    }
    bool running = true;
    float res = 0;
    char output_buf[MIN_LEN];
    while (running) {
        sem_wait(sem);
        uint32_t *length = (uint32_t *)shm_buf;
        char *text = shm_buf + sizeof(uint32_t);
        if (*length == UINT32_MAX) {
            sem_post(sem);
            running = false;
        } else if (*length > 0) {
            res = buf_to_numbers(text);
            if (res != -1) {
                int len = snprintf(output_buf, MIN_LEN, "%f\n", res);
                write(file, output_buf, len);
            } else {
                const char error_msg[] = "error: invalid input or division by zero\n";
                write(file, error_msg, sizeof(error_msg) - 1);
            }
            
            *length = 0;
            sem_post(sem);
        } else {
            sem_post(sem);
        }
    }
    sem_close(sem);
    munmap(shm_buf, SHM_SIZE);
    close(shm);
    close(file);
    
    return 0;
}