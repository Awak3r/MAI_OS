#include <stdint.h>
#include <stdbool.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_LENGTH 256


int cmp(const void *a, const void *b) {
    int x = *(const int*)a, y = *(const int*)b;
    return (x > y) - (x < y); 
}

int main(int argc, char ** argv){
    int max_threads = 0;
    char input_file[MAX_LENGTH];
    char output_file[MAX_LENGTH];
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--max-threads") == 0 && i+1 < argc) {
            max_threads = atoi(argv[i+1]);
     
        }
    }
    int x,y;
    fprintf(stdout, "Введите размер марицы\nx: ");
    scanf("%d", &x);
    fprintf(stdout, "y: ");
    scanf("%d", &y);
    int matrix[x*y];
    fprintf(stdout, "Введите имя файла c матрицей: ");
    scanf("%s", input_file);
    fprintf(stdout, "Введите имя файла для вывода: ");
    scanf("%s", output_file);
    FILE *f_in = fopen(input_file, "r");
    if (!f_in){
        fprintf(stderr, "error: cannot open file %s\n", input_file);
        return -1;
    }
    for (int i = 0; i < x * y; i++){
        if (fscanf(f_in, "%d", &matrix[i]) != 1){
            fprintf(stderr, "error: failed to read element %d\n", i);
            fclose(f_in);
            return -1;
        }
    }
    fclose(f_in);
    fprintf(stdout, "Матрица успешно прочитана\n");
    int K, window_x, window_y;
    fprintf(stdout, "\nВведите размер окна фильтра\nx: ");
    scanf("%d", &window_x);
    fprintf(stdout, "y: ");
    scanf("%d", &window_y);
    fprintf(stdout, "\nКол-во обходов матрицы (K): "); 
    scanf("%d", &K);


    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);


    for (int iteration = 0; iteration < K; iteration++) {
        int result[x * y];  
        for (int k = 0; k < x * y ; k++) {
            int i = k / y;
            int j = k % y;
            int window[window_x * window_y];
            int index = 0;
            int half_wx = window_x / 2, half_wy = window_y / 2;
            for (int di = -half_wx; di <= half_wx; di++) {
                for (int dj = -half_wy; dj <= half_wy; dj++) {
                    int ni = i + di, nj = j + dj;
                    if (ni >= 0 && ni < x && nj >= 0 && nj < y) {
                        window[index++] = matrix[ni * y + nj];
                    }
                }
            }
            qsort(window, index, sizeof(int), cmp);
            result[i * y + j] = window[index / 2];
        }    
        memcpy(matrix, result, sizeof(matrix));
    }
    FILE *f_out = fopen(output_file, "w");
    for (int k = 0; k<x*y; k++){
        if (k > 0 && k % y == 0){
            fprintf(f_out, "\n");
        }
        fprintf(f_out, "%d ", matrix[k]);
    }
    fclose(f_out);


    clock_gettime(CLOCK_MONOTONIC, &end);
    double time_taken;
    time_taken = (end.tv_sec - start.tv_sec) * 1e9;
    time_taken = (time_taken + (end.tv_nsec - start.tv_nsec)) * 1e-9;
    printf("Время выполнения: %.9f секунд\n", time_taken);
    return 0;
}


