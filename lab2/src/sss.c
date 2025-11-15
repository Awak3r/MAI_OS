#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
    int rows = 1000;
    int cols = 1000;
    
    // Инициализация генератора случайных чисел
    srand(time(NULL));
    
    FILE *file = fopen("in.txt", "w");
    if (!file) {
        fprintf(stderr, "Ошибка создания файла\n");
        return 1;
    }
    
    // Генерация случайных чисел от 0 до 255 (как в изображениях)
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            int value = rand() % 256;  // 0-255
            fprintf(file, "%d ", value);
        }
        fprintf(file, "\n");
    }
    
    fclose(file);
    return 0;
}
