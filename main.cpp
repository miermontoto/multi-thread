#include <stdio.h>
#include <math.h>
#include <CImg.h>
#include <time.h>
#include <iostream>     // std::cout
#include <algorithm>    // std::max

using namespace std; // this removes the need of writing "std::"" every time.
using namespace cimg_library;

#define R 17 // number of times the algorithm is repeated.
typedef float data_t;
typedef struct {
	CImg<float> _SOURCE;
	CImg<float> _HELP;
	float* _DEST;
	int _width;
	int _height;
	int _startRow;
	int _numRows;
} ThreadParams;

const int numThreads = 4; // number of threads to use.

const char* SOURCE_IMG      = "bailarina.bmp"; // source image's file name.
const char* HELP_IMG        = "background_V.bmp"; // aid image's file name.
const char* DESTINATION_IMG = "result.bmp"; // resulting image's file name.

void threadTask(ThreadParams params) {
	//ThreadParams params = *((ThreadParams*) param);
	for (uint i = params._startRow; i < params._width * params._height; i++) {
		// pointer initialization:

		// source image pointers
		float* pRsrc = params._SOURCE.data();                  // red component
		float* pGsrc = pRsrc + params._height * params._width; // green component
		float* pBsrc = pGsrc + params._height * params._width; // blue component

		// help image pointers
		float* pRaid = params._HELP.data();                    // red component
		float* pGaid = pRaid + params._height * params._width; // green component
		float* pBaid = pGaid + params._height * params._width; // blue component

		
		// destination image pointers
		float* pRdest = params._SOURCE.data();                   // red component
		float* pGdest = pRdest + params._height * params._width; // green component
		float* pBdest = pGdest + params._height * params._width; // blue component

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
}


int main() {
	// Both the source image and the aid image are loaded.
	CImg<data_t> srcImage(SOURCE_IMG);
	CImg<data_t> aidImage(HELP_IMG);

	// Pointers and variables initialization.
	data_t *pDstImage; // Resulting image's pointer.
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
	pDstImage = (data_t *) malloc (width * height * nComp * sizeof(data_t));
	if (pDstImage == NULL) {
		perror("Error: couldn't allocate memory for the destination image.");
		exit(1);
	}

	

	// Starting time
	if (clock_gettime(CLOCK_REALTIME, &tStart)==-1) {
		printf("Error: couldn't obtain starting time");
		exit(1);
	}

	// The algorithm repeats itself to be within time completition valid margins
	// It should last between 5 and 10 seconds with this configuration.
	for(int i = 0; i < R; i++) {
		pthread_t threads[numThreads];
		const int rowsPerThread = height / numThreads;

		ThreadParams params[numThreads];

		for(int i = 0; i < numThreads; i++) {
			params[i]._SOURCE = srcImage;
			params[i]._DEST = pDstImage;
			params[i]._HELP = aidImage;
			params[i]._height = height;
			params[i]._width = width;
			params[i]._numRows = rowsPerThread;
			params[i]._startRow = i * rowsPerThread;
			pthread_create(&(threads[i]), NULL, threadTask(), &(params[i]));
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

	printf ("Final execution time = %f\n", dElapsedTimeS);

	CImg<data_t> dstImage(pDstImage, width, height, 1, nComp);

	dstImage.save(DESTINATION_IMG);   // the image is saved to file.
	dstImage.display(); // the resulting image is shown on screen.
	free(pDstImage);    // the memory used by the pointers is freed.


	return 0;
}
