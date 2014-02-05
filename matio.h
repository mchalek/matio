/*
  matio.h: Provides include files and function prototypes 
  Copyright (C) 2005  Kevin McHale
  
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef __MATIO_H
#define __MATIO_H

#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>

#define __USE_COMPLEX_H

#ifdef __USE_COMPLEX_H

#ifdef __cplusplus
#include <complex>
#else
#include <complex.h>
#endif
#define complex16 double complex

#else

typedef struct _complex {
	double re;
	double im;
} complex16; 


#endif

//#define __VERBOSE
//#define __DEBUG
#define __ENABLE_WRITE_COMPRESSION // Bugs have been fixed, this is a good option to enable.

#define MATREAL 100 
#define MATCOMPLEX 300
#define MATERROR 500

#define MATIO_FORCE_COMPLEX 10

typedef struct _matio_element { 
	int type; // matlab data type
	int size; // size in bytes
	char *data;
} matio_element;

struct _MATdata;
typedef struct _MATdata {
	int *dimensions, num_dim; //Array dimensions and number of dimensions
	char *name; //variable name
	int type;  //to indicate real or complex
	double *real;
	complex16 *comp;
	char queried; // Used to mark whether a particular variable has been used, so that we may free unused variables.
	struct _MATdata *next;
} MATdata;

MATdata * matio_read( const char * );
MATdata * matio_read_force_complex( const char * );
MATdata * matio_search( MATdata *, char * );
MATdata * matio_complex_MATdata( MATdata *, int, int, char *, complex16 * );
MATdata * matio_real_MATdata( MATdata *, int, int, char *, double * );
int matio_write( const char *, MATdata * );
void matio_unlink( MATdata * );
void matio_destroy( MATdata * );
int matio_array_size( MATdata * );

// Internal functions here...  move to a separate file so that users may not
// call these (not easily, at least)
MATdata * matio_read_driver( const char *, int );
matio_element matio_interpret_element( char *, int *, int );
MATdata matio_process( int, int, char *, int, int );
void matio_swap_bytes( void *, int );
int matio_sizeof( int );

#endif
