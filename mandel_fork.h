/**
 * File: Mandel.h
 * Brief: .h file for mandel_fork.c
 * Author: Juliang Bolan
 * Date: 11/24/2026
 * Section: 131
 * Assignment name: Lab11_Multiprocessing
 * Compile: use provided Makefile i.e. 1. make clean 2. make 
 *  or with: gcc -g -o mandel_fork jpegrw.c mandel_fork.c -ljpeg
 */
#ifndef MANDEL_FORK_H
#define MANDEL_FORK_H

typedef struct {
    char outfile[64]; // might have to change back to pointer and use strdup()
    double xcenter;
    double ycenter;
    double xscale;
    double yscale;
    int image_width;
    int image_height;
    int max;
} imageSpecs;
#endif