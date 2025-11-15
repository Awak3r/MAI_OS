#include <linux/limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#define MAX_LENGTH 256

typedef struct {
    int * input;
    int * output;
    size_t i_start;
    size_t i_end;
    size_t window_x;
    size_t window_y;
    size_t x;
    size_t y;
} ThreadArgs;


int cmp(const void *a, const void *b) {
    int x = *(const int*)a, y = *(const int*)b;
    return (x > y) - (x < y); 
}


static void *task(void *_args)
{
    ThreadArgs *args = (ThreadArgs *)_args;    
    int *window = malloc( sizeof(int) * (args->window_x * args->window_y));
    if (!window) return NULL;
    for (size_t k = args->i_start; k <= args->i_end; k++) {
        int i = k / args->y;
        int j = k % args->y;
        int index = 0;
        int half_wx = args->window_x / 2, half_wy = args->window_y / 2;
        for (int di = -half_wx; di <= half_wx; di++) {
            for (int dj = -half_wy; dj <= half_wy; dj++) {
                int ni = i + di;
                int nj = j + dj;  
                if (ni >= 0 && ni < (int)args->x && 
                    nj >= 0 && nj < (int)args->y) {
                    window[index++] = args->input[ni * args->y + nj];
                }
            }
        }
        qsort(window, index, sizeof(int), cmp);
        args->output[i * args->y + j] = window[index / 2];
    }
    free(window);
    return NULL;
}



int main(int argc, char ** argv){
    int max_threads = 0;
    int flag = 0;
    char input_file[MAX_LENGTH];
    char output_file[MAX_LENGTH];
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--max-threads") == 0 && i + 1 < argc) {
            max_threads = atoi(argv[i+1]);
            flag = 1;
        }
        if (strcmp(argv[i], "--force-threads") == 0 && i + 1 < argc) {
            max_threads = atoi(argv[i+1]);
            flag = 2;
        }
    }
    if (flag == 0){
        fprintf(stderr, "error: specify --max-threads or --force-threads\n");
        return -1;
    }
    if (max_threads < 1){
        fprintf(stderr, "error: need at least 1 thread\n");
        return -1;
    }
    size_t n_threads;
    if (flag == 2){
        n_threads = max_threads;
    } else{
        n_threads = sysconf(_SC_NPROCESSORS_ONLN);
        if (n_threads > (size_t)max_threads) n_threads = max_threads;
    }
    if (n_threads == 0){
        fprintf(stderr, "error: cant realize multithreading with 0 threads\n");
        return -1;
    } 
    size_t x, y;
    fprintf(stdout, "Введите размер матрицы\nx: ");
    scanf("%zu", &x);
    fprintf(stdout, "y: ");
    scanf("%zu", &y);
    int *matrix = (int *)malloc(sizeof(int) * (x * y));
    if (!matrix) return -1;
    int *result = (int *)malloc(sizeof(int) * (x * y));
    if (!result) {
        free(matrix);
        return -1;
    } 
    fprintf(stdout, "Введите имя файла c матрицей: ");
    scanf("%s", input_file);
    fprintf(stdout, "Введите имя файла для вывода: ");
    scanf("%s", output_file);
    FILE *f_in = fopen(input_file, "r");
    if (!f_in){
        fprintf(stderr, "error: cannot open file %s\n", input_file);
        free(matrix);
        free(result);
        return -1;
    }
    for (int i = 0; i < x * y; i++){
        if (fscanf(f_in, "%d", &matrix[i]) != 1){
            fprintf(stderr, "error: failed to read element %d\n", i);
            fclose(f_in);
            free(matrix);
            free(result);
            return -1;
        }
    }
    fclose(f_in);
    fprintf(stdout, "Матрица успешно прочитана\n");
    size_t K, window_x, window_y;
    fprintf(stdout, "\nВведите размер окна фильтра\nx: ");
    scanf("%zu", &window_x);
    fprintf(stdout, "y: ");
    scanf("%zu", &window_y);
    fprintf(stdout, "\nКол-во обходов матрицы (K): "); 
    scanf("%zu", &K);



    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);



    pthread_t *threads = malloc(n_threads * sizeof(pthread_t));
    if (!threads) {
        free(matrix);
        free(result);
        return -1;
    }
    ThreadArgs *thread_args = malloc(n_threads * sizeof(ThreadArgs));
    if (!thread_args) 
    {
        free(matrix);
        free(result);
        free(threads);
        return -1;
    }
    int max_len = (x * y) / n_threads; 
    for (int iteration = 0; iteration < K; iteration++) {
        for (int k = 0; k< x * y; k++){
            result[k] = matrix[k];
        }
        for (size_t i = 0; i < n_threads; ++i) {
            thread_args[i] = (ThreadArgs){
                .i_start = max_len * i,
                .input = matrix,
                .output = result,
                .window_x = window_x,
                .window_y = window_y,
                .x = x,
                .y = y
            };
            if (i == n_threads -1){
                thread_args[i].i_end = x * y - 1;
            } else {
                thread_args[i].i_end = max_len * (i+1) - 1;
            }
            pthread_create(&threads[i], NULL, task, &thread_args[i]);
        }
        for (size_t i = 0; i < n_threads; ++i) {
            pthread_join(threads[i], NULL);
        }
        memcpy(matrix, result, x * y * sizeof(int));
    }
    free(thread_args);
    free(threads);
    free(result);
    FILE *f_out = fopen(output_file, "w");
    if (!f_out){
        fprintf(stderr, "error: cannot create output file %s\n", output_file);
        free(matrix);
        return -1;
    }
    for (int k = 0; k<x*y; k++){
        if (k > 0 && k % y == 0){
            fprintf(f_out, "\n");
        }
        fprintf(f_out, "%d ", matrix[k]);
    }
    fclose(f_out);
    fprintf(stdout, "Результат записан в файл %s\n", output_file);
    free(matrix);




    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken;
    time_taken = (end.tv_sec - start.tv_sec) * 1e9;
    time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
    printf("Время выполнения: %.9f секунд\n", time_taken);
    return 0;
}
