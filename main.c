#include "lodepng.c"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Node
{
    unsigned char r, g, b, a;
    struct Node *up, *down, *left, *right, *parent;
    int rank;
} Node;

// ������� ������ ��� ������� ���������������� ��������
Node* find(Node* x)
{
    if (x->parent != x)  x->parent = find(x->parent);

    return x->parent;
}

struct pixel
{
    unsigned char R, G, B, alpha;
};

// �������, ������� ������ ����������� � ���������� ������ ��������. ������� �� ����� load.png
char* load_png_file(const char *filename, int *width, int *height)
{
    unsigned char *image = NULL;
    int error = lodepng_decode32_file(&image, width, height, filename);
    if (error)
    {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        return NULL;
    }

    return image;
}


void applySobelFilter(unsigned char *image, int width, int height)
{
    // ������� ��� �������(���� �������), ������� ����� ��������� � ������� �������(����� ����� �� ���������)
    int gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int gy[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};

    // ������ �������� ������ ������� �������
    unsigned char *result = malloc(width * height * 4 * sizeof(unsigned char));

    // ���������� �� ������� �������, ������� �� � ��������, � � �������
    // ( ��� ��� � ��������� ��� ������� ������� ������ ���� ��� ������:
    // � ������, � �����, � ������, � �����
    // ����������� ����� �� �������������
    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int sumX = 0, sumY = 0;
            // � ��������� ��� ����� ����� ��������,
            // ��� ��� ������� ����������� �� X � �� Y
            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dx = -1; dx <= 1; dx++)
                {
                    // ��� ��� � ��� �� ������� ��������, � ������, � ������� ������� ����� ������
                    // (������ ������� � ������� ����� ��� ������� � ���������� �������),
                    // �� ��� ����� ���������, ���� ���������� �������, ��������� ��� � �������
                    // ( ��� ����� index)
                    int index = ((y+dy) * width + (x+dx)) * 4;

                    // �������� �������� gray �� ����
                    int gray = (image[index] + image[index + 1] + image[index + 2]) / 3;

                    // c ������� ������� ������� ����������� ������� ������� �������
                    sumX += gx[dy + 1][dx + 1] * gray;
                    sumY += gy[dy + 1][dx + 1] * gray;
                }
            }

            // ����� ���������
            int magnitude = sqrt(sumX * sumX + sumY * sumY);
            // ���������� �����
            if (magnitude > 255) magnitude = 255;

            int resultIndex = (y * width + x) * 4; // ���������� ������ � �������

            result[resultIndex] = (unsigned char)magnitude;
            result[resultIndex + 1] = (unsigned char)magnitude;
            result[resultIndex + 2] = (unsigned char)magnitude;
            result[resultIndex + 3] = image[resultIndex + 3]; // ������������ �� ������
        }
    }

    //�������������� ������� ������ � ��� �����, ���������� �����������
    for (int i = 0; i < width * height * 4; i++)
    {
        image[i] = result[i];
    }
    // ������� �������
    free(result);
}

// ������� ����������� ��� ������� ���������������� ��������(����� �������������� � ������ ��������� ���������)
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

    int error = lodepng_decode32_file(&image, width, height, filename);

    // ���� �� ������ ������� ������� ������
    if (error)
    {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        return NULL;
    }

    Node* nodes = malloc(*width * *height * sizeof(Node));

    // ���������� �� ������� ������� � ��� ���������� � ����
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

// ����� ��������� ���������
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

// �������� ��������� ���������
void color_components_and_count(Node* nodes, int width, int height)
{
    unsigned char* output_image = malloc(width * height * 4 * sizeof(unsigned char));

    // ���������� ������ ����� - ��������� ���������� ���������
    int* component_sizes = calloc(width * height, sizeof(int));

    // ���-�� ���������
    int total_components = 0;

    // ���������� �� ������� ������� � �������, ���� ���������� ��������� �� ������� ��
    for (int i = 0; i < width * height; i++)
    {
        Node* p = find(&nodes[i]); // ���� ���������� ��� ������� �������
        if (p == &nodes[i])
        {
            // ���� ��������� ���������� �� ��������� ��
            if (component_sizes[i] < 3)
            {
                p->r = 0;
                p->g = 0;
                p->b = 0;
            }
            else
            {
                // �������� ������ ����� �������
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

    char *output_filename = "C:\\Users\\user\\OneDrive\\������� ����\\������� ������\\����_output_2.png";
    // ��������� �������� ��������
    lodepng_encode32_file(output_filename, output_image, width, height);
    free(output_image);
    free(component_sizes);
}




int main()
{
    // ��� ����� ���� �������� �������

    int w = 0, h = 0; // w - ������, h - ������

    // ������ ����������� �������� � ���������� � ����
    char *filename = "C:\\Users\\user\\OneDrive\\������� ����\\������� ������\\����_input.png";

    // � ������� picture ����� �������� RGB � ������������(�� ������ ������� 4 ��-�� �������)
    char *picture = load_png_file(filename, &w, &h);

    // ������, ������� ��� ��� � �������� �������
    applySobelFilter(picture, w, h);

    // ���������� �������� �������� � ���������� � output_filename
    char *output_filename = "C:\\Users\\user\\OneDrive\\������� ����\\������� ������\\����_output_1.png";

    // ��������� � ���� output_filename � ������� ������� �� lodepng.h
    lodepng_encode32_file(output_filename, picture, w, h);

    // ������ ������ ������
    free(picture);

    // ��� ����� ���� ������ ���������� ���������

    // �� ������� ����, ��������� �� ���� ��������
    Node* nodes = create_graph(output_filename, &w, &h);

    double epsilon = 50.0; // ����� �� ����� ������� �����. ��� ������ �������, ��� ����� ������ �� ����� �������� ����� �������� � ���� ����������

    // ������� ������ ���������
    find_components(nodes, w, h, epsilon);

    // �������� ��������� � ��������� �����
    color_components_and_count(nodes, w, h);

    // ������� �����
    free_graph(nodes);

    return 0;
}
