#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    int rows = 1000;
    int cols = 1000;
    srand(time(NULL));
    
    FILE *file = fopen("in.txt", "w");
    if (!file) {
        fprintf(stderr, "Ошибка создания файла\n");
        return 1;
    }
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int value = rand() % 256;  
            fprintf(file, "%d ", value);
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    return 0;
}
