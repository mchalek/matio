#include "matio.h"
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define OUTFILE_NAME "matio_test_out.mat"

int main(int argc, char **argv)
{
	MATdata *list, *data;
	int i, j;
	
	if(argc < 2) {
		printf("You must specify an input file name!\n");
		exit(-1);
	}
		
	list = matio_read(argv[1]);
	data = list; 
	
#ifndef __WRITE
	while(data != NULL) {
		if(data -> type != MATERROR) {
			printf("%s = \n[", data -> name);
			for(i = 0; i < data -> dimensions[0]; i++) {
				for(j = 0; j < data -> dimensions[1]; j++)
					if(data -> type == MATREAL)
						printf("%g\t\t" , data -> real[i + j * data -> dimensions[0]]);
					else
#ifdef __USE_COMPLEX_H
						printf("%g + %gi\t\t", creal(data -> comp[i + j * data -> dimensions[0]]), cimag(data -> comp[i + j * data -> dimensions[0]]));
#else
						printf("%g + %gi\t\t", (data -> comp[i + j * data -> dimensions[0]]).re, (data -> comp[i + j * data -> dimensions[0]]).im);
#endif
				printf(";\n[");
			}
			printf("\n");
		}
		data = data -> next;
	}
#else
	fprintf(stderr, "Copying %s to %s...", argv[1], OUTFILE_NAME);
	matio_write(OUTFILE_NAME, list);
	fprintf(stderr, "Done.\n");
#endif

	matio_destroy(list);
	return 0;
}
