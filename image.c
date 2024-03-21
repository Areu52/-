#include "lodepng.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int i, j;

//
char* load_png_file  (const char *filename, int *width, int *height) {
	unsigned char *image = NULL;
	int error = lodepng_decode32_file(&image, width, height, filename);
	if (error) {
		printf("error %u: %s\n", error, lodepng_error_text(error));
		return NULL;
	}

	return (image);
}

struct pixel { 
	char R;
	char G;
	char B; 
	char alpha;
};

int main() {

	int w = 0, h = 0;
	int k = 0;

	char *filename = "skull.png";
	char *picture = load_png_file(filename, &w, &h);

	if (picture == NULL) {
		printf("I can't read the picture %s. Error.\n", filename);
		return -1;
	}

	for (i = 0; i < h * w * 4; i += 4) {
		//This is an example how to operate with pixel 
		//encoded as RGBa / 4 bytes
		struct pixel P;
		P.R = picture[i+0];
		P.G = picture[i+1];
		P.B = picture[i+2];
		P.alpha = picture[i+3];

		k++;
	}
	// Here is the place for your experiments 
	// with image processing
	// ....
	// ....
	// ...
	//
	free(picture);

	return 0;
}
