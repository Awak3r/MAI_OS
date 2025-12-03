#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>

#define SHM_SIZE 4096

static char SERVER_PROGRAM_NAME[] = "lab3_server";
const char SHM_NAME[] = "lab_shm";
const char SEM_NAME[] = "lab_sem";

int main(int argc, char **argv) {
	shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);
    int shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0600);
    if (shm == -1) {
        const char msg[] = "error: failed to create SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        exit(EXIT_FAILURE);
    }
    if (ftruncate(shm, SHM_SIZE) == -1) {
        const char msg[] = "error: failed to resize SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        shm_unlink(SHM_NAME);
        close(shm);
        exit(EXIT_FAILURE);
    }
    char *shm_buf = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
    if (shm_buf == MAP_FAILED) {
        const char msg[] = "error: failed to map SHM\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        shm_unlink(SHM_NAME);
        close(shm);
        exit(EXIT_FAILURE);
    }
    uint32_t *length = (uint32_t *)shm_buf;
    *length = 0;
    sem_t *sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0600, 1);
    if (sem == SEM_FAILED) {
        const char msg[] = "error: failed to create semaphore\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        munmap(shm_buf, SHM_SIZE);
        shm_unlink(SHM_NAME);
        close(shm);
        exit(EXIT_FAILURE);
    }
    char filename[256];
    ssize_t bytes_read = read(STDIN_FILENO, filename, sizeof(filename) - 1);
    if (bytes_read <= 0) {
        const char msg[] = "error: failed to read filename\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        sem_unlink(SEM_NAME);
        sem_close(sem);
        munmap(shm_buf, SHM_SIZE);
        shm_unlink(SHM_NAME);
        close(shm);
        exit(EXIT_FAILURE);
    }
    filename[bytes_read] = '\0';
    filename[strcspn(filename, "\n")] = '\0';
    char progpath[1024];
    ssize_t len = readlink("/proc/self/exe", progpath, sizeof(progpath) - 1);
    if (len == -1) {
        const char msg[] = "error: failed to read full program path\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        sem_unlink(SEM_NAME);
        sem_close(sem);
        munmap(shm_buf, SHM_SIZE);
        shm_unlink(SHM_NAME);
        close(shm);
        exit(EXIT_FAILURE);
    }
    while (len > 0 && progpath[len] != '/')
        --len;
    
    if (len == 0) {
        const char msg[] = "error: invalid program path\n";
        write(STDERR_FILENO, msg, sizeof(msg) - 1);
        sem_unlink(SEM_NAME);
        sem_close(sem);
        munmap(shm_buf, SHM_SIZE);
        shm_unlink(SHM_NAME);
        close(shm);
        exit(EXIT_FAILURE);
    }
    progpath[len] = '\0';
    const pid_t child = fork();
    switch (child) {
        case -1: {
            const char msg[] = "error: failed to spawn new process\n";
            write(STDERR_FILENO, msg, sizeof(msg) - 1);
            sem_unlink(SEM_NAME);
            sem_close(sem);
            munmap(shm_buf, SHM_SIZE);
            shm_unlink(SHM_NAME);
            close(shm);
            exit(EXIT_FAILURE);
        } break;
        
        case 0: {
            char path[1024];
            strcpy(path, progpath);
            strcat(path, "/");
            strcat(path, SERVER_PROGRAM_NAME);
            
            char *const args[] = {SERVER_PROGRAM_NAME, filename, NULL};
            int32_t status = execv(path, args);
            
            if (status == -1) {
                const char msg[] = "error: failed to exec into new executable image\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
                exit(EXIT_FAILURE);
            }
        } break;
        
        default: { 
            char buf[SHM_SIZE - sizeof(uint32_t) - 1];
            ssize_t bytes;
            while ((bytes = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
                if (buf[0] == '\n') {
                    break;
                }
                sem_wait(sem);
                uint32_t *length = (uint32_t *)shm_buf;
                char *text = shm_buf + sizeof(uint32_t);
                
                *length = bytes;
                memcpy(text, buf, bytes);
                text[bytes] = '\0'; 
                sem_post(sem);
            }
            
            if (bytes < 0) {
                const char msg[] = "error: failed to read from stdin\n";
                write(STDERR_FILENO, msg, sizeof(msg) - 1);
            }
            sem_wait(sem);
            uint32_t *length_final = (uint32_t *)shm_buf;
            *length_final = UINT32_MAX;
            sem_post(sem);
            sem_unlink(SEM_NAME);
            sem_close(sem);
            munmap(shm_buf, SHM_SIZE);
            shm_unlink(SHM_NAME);
            close(shm);
        } break;
    }
    
    return 0;
}