/**
 * File: mandel_fork.c
 * Brief: Program the creates mandelbrot images that will be stitched together into a movie
 * 	using a number processes to attempt to speed up / increase the efficency of the image generation code.
 * 	exact speed-up results can be found in the README.md file.
 * Note: mandel.c was the base code for this program
 * Author: Julian Bolan
 * Date: 11/24/2026
 * Section: 131
 * Assignment name: Lab11_Multiprocessing
 * Compile: use provided Makefile i.e. 1. make clean 2. make
 * 	or with: gcc -g -o mandel_fork jpegrw.c mandel_fork.c -ljpeg
 * 
 * Based on example code found here:
 * https://users.cs.fiu.edu/~cpoellab/teaching/cop4610_fall22/project3.html
 *
 * Converted to use jpg instead of BMP and other minor changes
 * 
 * Make movie via cmd: ffmpeg -framerate 10 -i mandel%02d.jpg -c:v libx264 -pix_fmt yuv420p mandel.mp4
 * Play movie via cmd: ffplay mandel.mp4  
 *
 */

#include <stdlib.h> // exit()
#include <stdio.h> 
#include <unistd.h> // fork(), _exit()
#include "jpegrw.h"
#include <sys/wait.h> // wait(), waitpid(), WEXITSTATUS
#include <sys/types.h> // pid_t
#include "mandel_fork.h"
#include <string.h>
#include <ctype.h>
#include <errno.h>

// local routines
static int iteration_to_color( int i, int max );
static int iterations_at_point( double x, double y, int max );
static void compute_image( imgRawImage *img, double xmin, double xmax,
									double ymin, double ymax, int max );

static void generateFrame(imageSpecs* image, int i);
// static void configureImage(int argc, char* argv[]);
static void configureImage(imageSpecs* image);


static void show_help();

#define NUM_FRAMES 50

int main( int argc, char *argv[] )
{
	// CHANGE TO POINTERS if need be
    // char c;

	// These are the default configuration values used
	// if no command line arguments are given.

	char c;
	while ((c = getopt(argc, argv, "h")) != -1) {
		if (c == 'h') {
			show_help();
			exit(1);
		}
		// switch(c) {
		// 	case 'h':
		// 	showhelp();
		// 	exit(1);
		// 	break;
		// }
	}

	imageSpecs* image = calloc(50, sizeof(*image));
	if (image == NULL) {
		perror("Couldn't allocate memory for imageSpecs structs");
		exit(1);
	} 
	configureImage(image);
	// image[0].outfile = "mandel.jpg";
	// image[0].xcenter = 0;
	// image[0].ycenter = 0;
	// image[0].xscale = 4;
	// image[0].yscale = 0; // calc later
	// image[0].image_width = 1000;
	// image[0].image_height = 1000;
	// image[0].max = 1000;

	// don't forget free! free(image);
	// while (argv[1] !])
	// if (!isdigit(argv[1])) {
	// 	perror("Specified value for number of children is not a number\n");
	// 	exit(1);
	// }

	char* end;
	errno = 0;
	int childCount = 1;
	int childCountTemp;
	if (argc > 2) { // subject to change
		perror("Too many arguments\n");
		exit(1);
	}

	if (argc > 1) { // could check for datatype size limits
		// using > 1 since argument number is subject to change
		childCountTemp = strtol(argv[1], &end, 10);
		if (end != argv[1] && *end == '\0') {
			childCount = childCountTemp;
		} else {
			perror("Specified value for number of children is not a number\n");
			exit(1);
		}
	} else {
		printf("No amount of children specified, using default value of 1\n");
	}

    const int max_child_count = childCount; // can predfine for testing
    int children = 0; // children active
	// fflush(stdout);
    for (int f = 0; f < NUM_FRAMES; f++) 
    {
		fflush(stdout); // flushing buffer before fork() that prints to console so children don't print messages of parent.
        pid_t p = fork(); // parent will create child here

        if (p == 0) { // if we are the child, generate frame f
            generateFrame(image, f); // ignore: replace args with proper args
            exit(0); // child exits successfuly

        } else if (p > 0) { // if parent, wait on child / children to finish, then after loop ends, create more children to continue processing frames
            
            if (++children == max_child_count) {
                for (int i = 0; i < max_child_count; i++) {
                    wait(NULL); // parent waits for children to finish
                }
				children = 0; // children have finished, no active children
            } 

			
        } else {
			// f--; // try to generate frame again if earlier code is unsuccessful
			perror("branch failure\n");
			exit(1);
		}
    }

	

	

	while (wait(NULL) > 0) {}; // terminates / reaps remaining children
	free(image);
	return 0;
}




/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iter;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max"
*/

void compute_image(imgRawImage* img, double xmin, double xmax, double ymin, double ymax, int max )
{
	int i,j;

	int width = img->width;
	int height = img->height;

	// For every pixel in the image...

	for(j=0;j<height;j++) {

		for(i=0;i<width;i++) {

			// Determine the point in x,y space for that pixel.
			double x = xmin + i*(xmax-xmin)/width;
			double y = ymin + j*(ymax-ymin)/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,max);

			// Set the pixel in the bitmap.
			setPixelCOLOR(img,i,j,iteration_to_color(iters,max));
		}
	}
}


/*
Convert a iteration number to a color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/
int iteration_to_color( int iters, int max )
{
	int color = 0xFFFFFF*iters/(double)max;
	return color;
}


// Show help message
void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates (X-axis). (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=1000)\n");
	printf("-H <pixels> Height of the image in pixels. (default=1000)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

// void configureImage(int argc, char* argv[]) {
//     // For each command line argument given,
// 	// override the appropriate configuration value.
// 	char c;
// 	while((c = getopt(argc,argv,"x:y:s:W:H:m:o:h"))!=-1) {
// 		switch(c) 
// 		{
// 			case 'x':
// 				xcenter = atof(optarg);
// 				break;
// 			case 'y':
// 				ycenter = atof(optarg);
// 				break;
// 			case 's':
// 				xscale = atof(optarg);
// 				break;
// 			case 'W':
// 				image_width = atoi(optarg);
// 				break;
// 			case 'H':
// 				image_height = atoi(optarg);
// 				break;
// 			case 'm':
// 				max = atoi(optarg);
// 				break;
// 			case 'o':
// 				outfile = optarg;
// 				break;
// 			case 'h':
// 				show_help();
// 				exit(1);
// 				break;
// 		}
// 	}
// }

void configureImage(imageSpecs* image) { // use NUM_FRAMES for frame count
	double xscale = 4;
	double yscale = 0;
	char name[64];
	for (int f = 0; f < NUM_FRAMES; f++) {
		// Calculate y scale based on x scale (settable) and image sizes in X and Y (settable)
		// yscale = image[f].xscale / image[f].image_width * image[f].image_height;
		snprintf(name, sizeof(name), "mandel%02d.jpg\n", f); // creating jpg names
		
		printf("mandel%02d.jpg", f); // for debugging

		// image[f].outfile = "mandel.jpg";
		strcpy(image[f].outfile, name);
		image[f].xcenter = -1; // seemingly exists from approx -2 to 0.5
		image[f].ycenter = 0; // seemingly exists from -1.15 to 1.15
		image[f].xscale = xscale;
		// image[f].yscale = yscale; // calc later
		image[f].image_width = 1000;
		image[f].image_height = 1000;
		image[f].max = 1000;

		xscale *= .95;
		yscale = image[f].xscale / image[f].image_width * image[f].image_height;
		image[f].yscale = yscale;
	}
}

void generateFrame(imageSpecs* image, int i) {
    // Display the configuration of the image.

    // // Calculate y scale based on x scale (settable) and image sizes in X and Y (settable)
	// yscale = image[i].xscale / image[i].image_width * image[i].image_height;

	printf("mandel: x=%lf y=%lf xscale=%lf yscale=%1f max=%d outfile=%s\n",image[i].xcenter,image[i].ycenter,image[i].xscale,image[i].yscale,image[i].max,image[i].outfile);

	// Create a raw image of the appropriate size.
	imgRawImage* img = initRawImage(image[i].image_width,image[i].image_height);

	// Fill it with a black
	setImageCOLOR(img,0);

	// Compute the Mandelbrot image
	compute_image(img,image[i].xcenter-image[i].xscale/2,image[i].xcenter+image[i].xscale/2,image[i].ycenter-image[i].yscale/2,image[i].ycenter+image[i].yscale/2,image[i].max);

	// Save the image in the stated file.
	storeJpegImageFile(img,image[i].outfile);

	// free the mallocs
	freeRawImage(img);
}

