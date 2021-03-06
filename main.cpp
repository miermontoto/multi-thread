#include <stdio.h>
#include <math.h>
#include <CImg.h>
#include <time.h>
#include <fstream>
#include <iostream>     // std::cout
#include <algorithm>    // std::max

using namespace std; // this removes the need of writing "std::"" every time.
using namespace cimg_library;

#define R 17 // number of times the algorithm is repeated.
typedef struct {
	float* _SOURCE;
	float* _HELP;
	float* _DEST;
	int _width;
	int _height;
	int _startRow;
	int _numRows;
} ThreadParams;

const int numThreads = 8; // number of threads to use.

const char* SOURCE_IMG      = "bailarina.bmp"; // source image's file name.
const char* HELP_IMG        = "background_V.bmp"; // aid image's file name.
const char* DESTINATION_IMG = "result.bmp"; // resulting image's file name.

void* threadTask(void* param) {
	ThreadParams params = *(ThreadParams*) param;
	int startingPoint = params._startRow * params._width;
	// first pixel to be processed by the thread.

	// pointer initialization:

	// source image pointers
	float* pRsrc = params._SOURCE + startingPoint;  // red component
	float* pGsrc = pRsrc + params._height * params._width; // green component
	float* pBsrc = pGsrc + params._height * params._width; // blue component

	// help image pointers
	float* pRaid = params._HELP + startingPoint;    // red component
	float* pGaid = pRaid + params._height * params._width; // green component
	float* pBaid = pGaid + params._height * params._width; // blue component

	
	// destination image pointers
	float* pRdest = params._DEST + startingPoint;            // red component
	float* pGdest = pRdest + params._height * params._width; // green component
	float* pBdest = pGdest + params._height * params._width; // blue component


	for (int i = 0; i < params._width * params._numRows; i++) {
		
		int red, blue, green; // temporal component initialization

		// Algorithm
		// Blend: blacken mode #12
		red = 255 - ((256 * (255 - pRaid[i])) / (pRsrc[i] + 1));
		green = 255 - ((256 * (255 - pGaid[i])) / (pGsrc[i] + 1));
		blue = 255 - ((256 * (255 - pBaid[i])) / (pBsrc[i] + 1));

		// Values are checked and trimmed.
		red = max(min(red, 255), 0);
		green = max(min(green, 255), 0);
		blue = max(min(blue, 255), 0);

		// Results are added to the destination image.

		pRdest[i] = red;
		pGdest[i] = green;
		pBdest[i] = blue;
	}

	return NULL;
}


int main() {

	// Check to see if both files to process actually exist.
    std::ifstream file1(SOURCE_IMG);
    std::ifstream file2(HELP_IMG);

	if(!file1 || !file2) {
		printf("Couldn't locate source and help images.\n");
		exit(1);
	}

	// Both the source image and the aid image are loaded.
	CImg<float> srcImage(SOURCE_IMG);
	CImg<float> aidImage(HELP_IMG);

	// Pointers and variables initialization.
	float *pDstImage; // Resulting image's pointer.
	uint width, height;
	uint nComp;

	// Time variables are initialized
	struct timespec tStart, tEnd;
	double dElapsedTimeS;

	// The width and height of both images are checked to be equal.
	if(srcImage.width() != aidImage.width() && srcImage.height() != aidImage.height()) {
		perror("Images to blend don't have the same size.");
		exit(1);
	}

	// Certain information of the source image is stored:
	// width and height should be equal for both images.
	width  = srcImage.width();
	height = srcImage.height();
	nComp  = srcImage.spectrum(); // number of image's components
	/*
	 * (1) if the image is in black and white.
	 * (3) if it's a color image.
	 * (4) if it's a color image WITH transparency values.
	 */

	// Allocate memory for the resulting image.
	pDstImage = (float *) malloc (width * height * nComp * sizeof(float));
	if (pDstImage == NULL) {
		perror("Error: couldn't allocate memory for the destination image.");
		exit(1);
	}

	

	// Starting time
	if (clock_gettime(CLOCK_REALTIME, &tStart)==-1) {
		printf("Error: couldn't obtain starting time");
		exit(1);
	}

	// The algorithm repeats itself the same times as the single-threaded version
	for(int i = 0; i < R; i++) {
		pthread_t threads[numThreads];
		const int rowsPerThread = height / numThreads;

		ThreadParams params[numThreads];

		for(int i = 0; i < numThreads; i++) {
			params[i]._SOURCE = srcImage.data();
			params[i]._DEST = pDstImage;
			params[i]._HELP = aidImage.data();
			params[i]._height = height;
			params[i]._width = width;
			params[i]._numRows = rowsPerThread;
			params[i]._startRow = i * rowsPerThread;
			pthread_create(&(threads[i]), NULL, threadTask, &(params[i]));
		}

		// After all threads are finished, join the results in the resulting image.
		for(int i = 0; i < numThreads; i++) {
			pthread_join(threads[i], NULL);
		}
	}

	// Ending time 
	if (clock_gettime(CLOCK_REALTIME, &tEnd) == -1){
		printf("Error: couldn't obtain ending time");
		exit(1);
	}

	// Print total execution time
	dElapsedTimeS = (tEnd.tv_sec - tStart.tv_sec);
	dElapsedTimeS += (tEnd.tv_nsec - tStart.tv_nsec) / 1e+9;

	printf("Final execution time = %f\n", dElapsedTimeS);

	CImg<float> dstImage(pDstImage, width, height, 1, nComp);

	dstImage.save(DESTINATION_IMG);   // the image is saved to file.
	dstImage.display(); // the resulting image is shown on screen.
	free(pDstImage);    // the memory used by the pointers is freed.

	return 0;
}
