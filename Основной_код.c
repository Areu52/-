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

// ôóíêöèÿ ïîèñêà äëÿ ñèñòåìû íåïåðåñåêàþùèõñÿ ìíîæåñòâ
Node* find(Node* x)
{
    if (x->parent != x)  x->parent = find(x->parent);

    return x->parent;
}

struct pixel
{
    unsigned char R, G, B, alpha;
};

// ôóíêöèÿ, êîòîðàÿ ÷èòàåò èçîáðàæåíèå è âîçâðàùàåò ìàññèâ ïèêñåëåé. Ôóíêöèÿ èç ôàéëà load.png
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
    // ñîçäàåì äâå ìàòðèöû(ÿäðà ñâåðòêè), êîòîðûå áóäåì ïðèìåíÿòü ê êàæäîìó ïèêñåëþ(÷èñëà âçÿòû èç âèêèïåäèè)
    int gx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    int gy[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};

    // ìàññèâ èòîãîâûõ öâåòîâ êàæäîãî ïèêñåëÿ
    unsigned char *result = malloc(width * height * 4 * sizeof(unsigned char));

    // ïðîõîäèìñÿ ïî êàæäîìó ïèêñåëþ, íà÷èíàÿ íå ñ íóëåâîãî, à ñ ïåðâîãî
    // ( òàê êàê â àëãîðèòìå äëÿ êàæäîãî ïèêñåëÿ äîëæíû áûòü âñå ñîñåäè:
    // è ñâåðõó, è ñíèçó, è ñïðàâà, è ñëåâà
    // çàêàí÷èâàåì òàêæå íà ïðåäïîñëåäíåì
    for (int y = 1; y < height - 1; y++)
    {
        for (int x = 1; x < width - 1; x++)
        {
            int sumX = 0, sumY = 0;
            // â àëãîðèòìå íàì íóæíî íàéòè ãðàäèåíò,
            // òàê ÷òî íàõîäèì ïðîèçâîäíûå ïî X è ïî Y
            for (int dy = -1; dy <= 1; dy++)
            {
                for (int dx = -1; dx <= 1; dx++)
                {
                    // òàê êàê ó íàñ íå ìàòðèöà ïèêñåëåé, à ìàññèâ, â êîòîðîì ïèêñåëè ëåæàò ñâåðõó
                    // (ïðè÷åì ñíà÷àëà â ìàññèâå ëåæàò âñå ïèêñåëè ñ îäèíàêîâîé âûñîòîé),
                    // òî íàì íóæíî âû÷èñëÿòü, çíàÿ êîîðäèíàòó ïèêñåëÿ, ïîëîæåíèå åãî â ìàññèâå
                    // ( ýòî áóäåò index)
                    int index = ((y+dy) * width + (x+dx)) * 4;

                    // çàìåíèòü íàçâàíèå gray íà öâåò
                    int gray = (image[index] + image[index + 1] + image[index + 2]) / 3;

                    // c ïîìîùüþ ìàòðèöû íàõîäèì ïðîèçâîäíûå ôóíêöèè ÿðêîñòè ïèêñåëÿ
                    sumX += gx[dy + 1][dx + 1] * gray;
                    sumY += gy[dy + 1][dx + 1] * gray;
                }
            }

            // äëèíà ãðàäèåíòà
            int magnitude = sqrt(sumX * sumX + sumY * sumY);
            // íîðìèðîâêà äëèíû
            if (magnitude > 255) magnitude = 255;

            int resultIndex = (y * width + x) * 4; // îïðåäåëèëè èíäåêñ â ìàññèâå

            result[resultIndex] = (unsigned char)magnitude;
            result[resultIndex + 1] = (unsigned char)magnitude;
            result[resultIndex + 2] = (unsigned char)magnitude;
            result[resultIndex + 3] = image[resultIndex + 3]; // ïðîçðà÷íîñòü íå ìåíÿåì
        }
    }

    //ïðåîáðàçîâàíèå ìàññèâà öâåòîâ â óæå íîâîå, âûäåëåííîå èçîáðàæåíèå
    for (int i = 0; i < width * height * 4; i++)
    {
        image[i] = result[i];
    }
    // î÷èñòêà ìàññèâà
    free(result);
}

// ôóíêöèÿ îáúåäèíåíèÿ äëÿ ñèñòåìû íåïåðåñåêàþùèõñÿ ìíîæåñòâ(áóäåò èñïîëüçîâàòüñÿ â ïîèñêå êîìïîíåíò ñâÿçíîñòè)
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

    // åñëè íå ñìîãëè ñ÷èòàòü âûâîäèì îøèáêó
    if (error)
    {
        printf("error %u: %s\n", error, lodepng_error_text(error));
        return NULL;
    }

    Node* nodes = malloc(*width * *height * sizeof(Node));

    // ïðîõîäèìñÿ ïî êàæäîìó ïèêñåëþ è åãî çàñîâûâàåì â ãðàô
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

// ïîèñê êîìïîíåíò ñâÿçíîñòè
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

// ïîêðàñêà êîìïîíåíò ñâçÿíîñòè
void color_components_and_count(Node* nodes, int width, int height)
{
    unsigned char* output_image = malloc(width * height * 4 * sizeof(unsigned char));

    // èçíà÷àëüíî êàæäàÿ òî÷êà - îòäåëüíàÿ êîìïîíåíòà ñâÿçíîñòè
    int* component_sizes = calloc(width * height, sizeof(int));

    // êîë-âî êîìïîíåíò
    int total_components = 0;

    // ïðîõîäèìñÿ ïî êàæäîìó ïèêñåëþ è ãîâîðèì, åñëè êîìïîíåíòà ìàëåíüêàÿ òî óäàëÿåì åå
    for (int i = 0; i < width * height; i++)
    {
        Node* p = find(&nodes[i]); // èùåì êîìïîíåíòû äëÿ êàæäîãî ïèêñåëÿ
        if (p == &nodes[i])
        {
            // åñëè ìàëåíüêàÿ êîìïîíåíòà íå ó÷èòûâàåì åå
            if (component_sizes[i] < 3)
            {
                p->r = 0;
                p->g = 0;
                p->b = 0;
            }
            else
            {
                // ðàíäîìíî êðàñèì öâåòà ïèêñåëÿ
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

    char *output_filename = "C:\\Users\\user\\OneDrive\\Ðàáî÷èé ñòîë\\Áîëüøàÿ çàäà÷à\\Ðóêà_output_2.png";
    // ñîõðàíÿåì èòîãîâóþ êàðòèíêó
    lodepng_encode32_file(output_filename, output_image, width, height);
    free(output_image);
    free(component_sizes);
}




int main()
{
    // ýòà ÷àñòü êîäà âûäåëÿåò êîíòóðû

    int w = 0, h = 0; // w - øèðèíà, h - âûñîòà

    // ÷èòàåì èçíà÷àëüíóþ êàðòèíêó è çàñîâûâàåì â ôàéë
    char *filename = "C:\\Users\\user\\OneDrive\\Ðàáî÷èé ñòîë\\Áîëüøàÿ çàäà÷à\\Ðóêà_input.png";

    // â ìàññèâå picture ëåæàò ïèêñëåëè RGB è ïðîçðà÷íîñòü(íà êàæäûé ïèêñåëü 4 ýë-òà ìàññèâà)
    char *picture = load_png_file(filename, &w, &h);

    // ôèëüòð, êîòîðûé êàê ðàç è âûäåëÿåò ãðàíèöû
    applySobelFilter(picture, w, h);

    // çàñîâûâàåì èòîãîâóþ êàðòèíêó ñ âûäåëåíèåì â output_filename
    char *output_filename = "C:\\Users\\user\\OneDrive\\Ðàáî÷èé ñòîë\\Áîëüøàÿ çàäà÷à\\Ðóêà_output_1.png";

    // ñîõðàíÿåì â ôàéë output_filename ñ ïîìîùüþ ôóíêöèè èç lodepng.h
    lodepng_encode32_file(output_filename, picture, w, h);

    // ÷èñòèì ìàññèâ öâåòîâ
    free(picture);

    // ýòà ÷àñòü êîäà êðàñèò êîìïîíåíòû ñâÿçíîñòè

    // ìû ñîçäàåì ãðàô, ñîñòîÿùèé èç âñåõ ïèêñåëåé
    Node* nodes = create_graph(output_filename, &w, &h);

    double epsilon = 50.0; // Ëþáîå íå òàêîå áîëüøîå ÷èñëî. ×åì áîëüøå ýïñèëîí, òåì áîëåå ðàçíûå ïî öâåòó ïèêñëåëè áóäóò çàñóíóòû â îäíó êîìïîíåíòó

    // ôóíêöèÿ ïîèñêà êîìïîíåíò
    find_components(nodes, w, h, epsilon);

    // ïîêðàñêà êîìïîíåíò â ðàíäîìíûå öâåòà
    color_components_and_count(nodes, w, h);

    // î÷èñòêà ãðàôà
    free_graph(nodes);

    return 0;
}
