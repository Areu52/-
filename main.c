#include "lodepng.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Node
{
    unsigned char r, g, b, a;
    struct Node *up, *down, *left, *right, *parent;
    int rank;
} Node;

// функция поиска для системы непересекающихся множеств
Node* find(Node* x)
{
    if (x->parent != x)  x->parent = find(x->parent);

    return x->parent;
}

struct pixel
{
    unsigned char R, G, B, alpha;
};

// функция, которая читает изображение и возвращает массив пикселей. Функция из файла load.png
char* load_png_file(const char *filename, int *width, int *height) {
    unsigned char *image = NULL;
    int error = lodepng_decode32_file(&image, width, height, filename);
    if (error) {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        return NULL;
    }

    return (image);
}

void applySobelFilter(unsigned char *image, int width, int height)
{
    // создаем две матрицы(ядра свертки), которые будем применять к каждому пикселю(числа взяты из википедии)
    int gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int gy[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};

    // массив итоговых цветов каждого пикселя
    unsigned char *result = malloc(width * height * 4 * sizeof(unsigned char));

    // проходимся по каждому пикселю, начиная не с нулевого, а с первого
    // ( так как в алгоритме для каждого пикселя должны быть все соседи:
    // и сверху, и снизу, и справа, и слева
    // заканчиваем также на предпоследнем
    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int sumX = 0, sumY = 0;
            // в алгоритме нам нужно найти градиент,
            // так что находим производные по X и по Y
            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dx = -1; dx <= 1; dx++)
                {
                    // так как у нас не матрица пикселей, а массив, в котором пиксели лежат сверху
                    // (причем сначала в массиве лежат все пиксели с одинаковой высотой),
                    // то нам нужно вычислять, зная координату пикселя, положение его в массиве
                    // ( это будет index)
                    int index = ((y+dy) * width + (x+dx)) * 4;

                    // заменить название gray на цвет
                    int gray = (image[index] + image[index + 1] + image[index + 2]) / 3;

                    // c помощью матрицы находим производные функции яркости пикселя
                    sumX += gx[dy + 1][dx + 1] * gray;
                    sumY += gy[dy + 1][dx + 1] * gray;
                }
            }

            // длина градиента
            int magnitude = sqrt(sumX * sumX + sumY * sumY);
            // нормировка длины
            if (magnitude > 255) magnitude = 255;

            int resultIndex = (y * width + x) * 4; // определили индекс в массиве

            result[resultIndex] = (unsigned char)magnitude;
            result[resultIndex + 1] = (unsigned char)magnitude;
            result[resultIndex + 2] = (unsigned char)magnitude;
            result[resultIndex + 3] = image[resultIndex + 3]; // прозрачность не меняем
        }
    }

    //преобразование массива цветов в уже новое, выделенное изображение
    for (int i = 0; i < width * height * 4; i++)
    {
        image[i] = result[i];
    }

    free(result);
}

// функция объединения для системы непересекающихся множеств(будет использоваться в поиске компонент связности)
void union_set(Node* x, Node* y, double epsilon)
{
    if (x->r < 40 && y->r < 40)  return;

    Node* px = find(x);
    Node* py = find(y);

    double color_difference = sqrt(pow(x->r - y->r, 2) + pow(x->g - y->g, 2) + pow(x->b - y->b, 2));
    if (px != py && color_difference < epsilon)
    {
        if (px->rank > py->rank)
        {
            py->parent = px;
        } else
        {
            px->parent = py;
            if (px->rank == py->rank)
            {
                py->rank++;
            }
        }
    }
}

Node* create_graph(const char *filename, int *width, int *height)
{
    unsigned char *image = NULL;
//    char *picture = load_png_file(filename, &w, &h);
    int error = lodepng_decode32_file(&image, width, height, filename);
    printf("%d", error);
    // если не смогли считать выводим ошибку
    if (error)
    {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        return NULL;
    }

    Node* nodes = malloc(*width * *height * sizeof(Node));

    // проходимся по каждому пикселю и его засовываем в граф
    for (unsigned y = 0; y < *height; ++y)
    {
        for (unsigned x = 0; x < *width; ++x)
        {
            Node* node = &nodes[y * *width + x];
            unsigned char* pixel = &image[(y * *width + x) * 4];
            node->r = pixel[0];
            node->g = pixel[1];
            node->b = pixel[2];
            node->a = pixel[3];
            node->up = y > 0 ? &nodes[(y - 1) * *width + x] : NULL;
            node->down = y < *height - 1 ? &nodes[(y + 1) * *width + x] : NULL;
            node->left = x > 0 ? &nodes[y * *width + (x - 1)] : NULL;
            node->right = x < *width - 1 ? &nodes[y * *width + (x + 1)] : NULL;
            node->parent = node;
            node->rank = 0;
        }
    }

    free(image);

    return nodes;
}

// поиск компонент связности
void find_components(Node* nodes, int width, int height, double epsilon)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            Node* node = &nodes[y * width + x];
            if (node->up)
            {
                union_set(node, node->up, epsilon);
            }
            if (node->down)
            {
                union_set(node, node->down, epsilon);
            }
            if (node->left)
            {
                union_set(node, node->left, epsilon);
            }
            if (node->right)
            {
                union_set(node, node->right, epsilon);
            }
        }
    }
}

void free_graph(Node* nodes)
{
    free(nodes);
}

// покраска компонент свзяности
void color_components_and_count(Node* nodes, int width, int height)
{
    unsigned char* output_image = malloc(width * height * 4 * sizeof(unsigned char));

    // изначально каждая точка - отдельная компонента связности
    int* component_sizes = calloc(width * height, sizeof(int));

    // кол-во компонент
    int total_components = 0;

    // проходимся по каждому пикселю и говорим, если компонента маленькая то удаляем ее
    for (int i = 0; i < width * height; i++)
    {
        Node* p = find(&nodes[i]); // ищем компоненты для каждого пикселя
        if (p == &nodes[i])
        {
            // если маленькая компонента не учитываем ее
            if (component_sizes[i] < 3)
            {
                p->r = 0;
                p->g = 0;
                p->b = 0;
            }
            else
            {
                // рандомно красим цвета пикселя
                p->r = rand() % 256;
                p->g = rand() % 256;
                p->b = rand() % 256;
            }
            total_components++;
        }
        output_image[4 * i + 0] = p->r;
        output_image[4 * i + 1] = p->g;
        output_image[4 * i + 2] = p->b;
        output_image[4 * i + 3] = 255;
        component_sizes[p - nodes]++;
    }

    //(сюда нужно вставить либо ссылку на Рука_output_2.png, либо на Голова_output_2.png)
    char *output_filename = "Palm_output_2.png";
    // сохраняем итоговую картинку
//    lodepng_encode32_file(output_filename, output_image, width, height);
    free(output_image);
    free(component_sizes);
}



void floodFillRecursive(unsigned char* image, int x, int y, int newColor1, int newColor2, int newColor3, int oldColor, int width, int height) {
    if(x < 0 || x >= width || y < 0 || y >= height)
        return;

    int resultIndex = (y * width + x) * 4;
    if(image[resultIndex] > oldColor)
        return;

    image[resultIndex] = newColor1;
    image[resultIndex + 1] = newColor2;
    image[resultIndex + 2] = newColor3;

    floodFillRecursive(image, x+1, y, newColor1, newColor2, newColor3, oldColor, width, height);
    floodFillRecursive(image, x-1, y, newColor1, newColor2, newColor3, oldColor, width, height);
    floodFillRecursive(image, x, y+1, newColor1, newColor2, newColor3, oldColor, width, height);
    floodFillRecursive(image, x, y-1, newColor1, newColor2, newColor3, oldColor, width, height);
}


void colorComponents(unsigned char* image, int width, int height, int epsilon) {
    int color1, color2, color3;
    for(int y = 1; y < height - 1; y++) {
        for(int x = 1; x < width - 1; x++) {
            if(image[4 * (y * width + x)] < epsilon) {
                color1 = rand() % (255 - epsilon * 2) + epsilon * 2;
                color2 = rand() % (255 - epsilon * 2) + epsilon * 2;
                color3 = rand() % (255 - epsilon * 2) + epsilon * 2;
                floodFillRecursive(image, x, y, color1, color2, color3, epsilon, width, height);

            }

        }
    }
}



int main()
{
    // эта часть кода выделяет контуры

    int w = 0, h = 0; // // w - ширина, h - высота

    // читаем изначальную картинку и засовываем в файл(сюда нужно вставить либо ссылку на Рука_input.png, либо на Голова_input.png)
    char *filename = "Palm_input.png";

    // в массиве picture лежат пикслели RGB и прозрачность(на каждый пиксель 4 эл-та массива)
    char *picture = load_png_file(filename, &w, &h);
    // фильтр, который как раз и выделяет границы
    applySobelFilter(picture, w, h);

    // засовываем итоговую картинку с выделением в output_filename(сюда нужно вставить либо ссылку на Рука_output_1.png, либо на Голова_output_1.png)

    // сохраняем в файл output_filename с помощью функции из lodepng.h
    char *output_filename = "Palm_output_1.png";
    lodepng_encode32_file(output_filename, picture, w, h);


    // эта часть кода красит компоненты связности

    // мы создаем граф, состоящий из всех пикселей
    Node* nodes = create_graph(output_filename, &w, &h);

    double epsilon = 50.0; // Любое не такое большое число. Чем больше эпсилон, тем более разные по цвету пикслели будут засунуты в одну компоненту

    // функция поиска компонент
    find_components(nodes, w, h, epsilon);

    // покраска компонент в рандомные цвета
    color_components_and_count(nodes, w, h);
    colorComponents(picture, w, h, 20);
    lodepng_encode32_file("Palm_output_3.png", picture, w, h);
    free_graph(nodes);
    // чистим массив цветов
    free(picture);

    return 0;
}
