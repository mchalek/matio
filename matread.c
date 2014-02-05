/*
  matread.c: Routines for inputting data from Level 5 .mat files
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
#include "matio.h"
#include "matio_internal_types.h"


MATdata * matio_read_force_complex(const char *filename)
{
	return matio_read_driver(filename, 1);
}

MATdata * matio_read(const char *filename)
{
	return matio_read_driver(filename, 0);
}

MATdata * matio_read_driver(const char * filename, int force_complex) 
{
	FILE * infile;
	char buffer[117], *bigbuffer, *augmented_filename;
	int byteswap, datatype, datasize, endian_test;
	MATdata *ret, *temp;
	
	// We'll first try to open the file with exactly the name specified. If that file does not exist,
	// we'll try to open that file with the extension .mat appended to it.  This follows the convention
	// used in matlab for specifying input filenames.
	infile = fopen(filename, "r");
	if(!infile) {
		augmented_filename = (char *) malloc((5 + strlen(filename))*sizeof(char));
		strcpy(augmented_filename, filename);
		strcat(augmented_filename, ".mat");
		infile = fopen(augmented_filename, "r");
		free(augmented_filename);
	}
	
	if(!infile) { // We couldn't open the file.  We'll give the system default error message
		perror(filename);
		return NULL;
	}
	
	endian_test = 256 * (short int) 'K' + (short int) 'M'; 
	
	if(((char *) &endian_test)[0] == 'M')
		byteswap = 1;
	else 
		byteswap = 0;

	ret = (MATdata *) malloc(sizeof(MATdata));
	ret -> name = NULL;
	temp = ret;
	
	fread(buffer, 1, 116, infile); // read header text
#ifdef __VERBOSE
	printf("Reading ");
	printf(filename);
	buffer[116] = '\0';
	printf(":\nHeader information: \n%s\n", buffer);
	fflush(stdout);
#endif

	fread(buffer, 1, 8, infile); // read subsystem data offset
	
	fread(buffer, 1, 2, infile); // read version
	if(*((short int *) buffer) != 0x0100) 
		printf("%s: Non-native MAT file version detected.  Results may be incorrect.\n", filename);

	fread(buffer, 1, 2, infile); // read endian indicator
	
	if(buffer[0] == 'I') // Check byte-swapping requirement
		byteswap ^= 1;
	else
		if(buffer[0] == 'M')
			byteswap ^= 0;
		else {
			printf("%s: invalid file format detected. Only Level 5 MAT files are supported.\n", filename);
			return NULL;
		}
	
#ifdef __VERBOSE
	if(byteswap)
		printf("Non-native endian format. Byte-swapping required to read this file.\n");
	else
		printf("Native endian format detected.\n");
#endif
		
	fread(&datatype, 4, 1, infile); // Read data type
	while(!feof(infile)) {
		fread(&datasize, 4, 1, infile); // Read data size
		if(byteswap) {
			matio_swap_bytes(&datatype, 4);
			matio_swap_bytes(&datasize, 4);
		}
		
		datasize += datasize % 8 && datatype != 15 ? 8 - datasize % 8 : 0;
		bigbuffer = (char *) calloc(datasize, 1);
		fread(bigbuffer, datasize, 1, infile); // Read data

		
		if(temp -> name != NULL) {
			temp -> next = (MATdata *) malloc(sizeof(MATdata));
			temp = temp -> next;
		}
	

		*temp = matio_process(datatype, datasize, bigbuffer, byteswap, force_complex); // Process data
		temp -> queried = 0;
		
		free(bigbuffer);
		fread(&datatype, 4, 1, infile); // Read data type
	}

	fclose(infile);
	return ret;
}
 
matio_element matio_interpret_element(char *buff, int *offset, int byteswap)
{
	int j, temp;
	short int *sibuffer = (short int *) (buff + *offset);
	char *cbuffer = buff + *offset;
	int *ibuffer = (int *)(buff + *offset);

	matio_element ret;
	if(sibuffer[0] && sibuffer[1]) {
		if(byteswap) 
			matio_swap_bytes(ibuffer, 4);

		ret.size = ((*ibuffer) & 0xffff0000) >> 16;
		ret.type = (*ibuffer) & 0xffff;

		ret.data = (char *)malloc(4);
	
		memcpy(ret.data, cbuffer + 4, 4);
		
		if(byteswap && ret.size > 0) 
			for(j = 0; j < 4; j += matio_sizeof(ret.type))
				matio_swap_bytes(ret.data + j, matio_sizeof(ret.type));		
		
		*offset += 8;
	} else { 
		if(byteswap) {
			matio_swap_bytes(ibuffer, 4);
			matio_swap_bytes(ibuffer + 1, 4);
		}
		ret.type = ibuffer[0];
		ret.size = ibuffer[1];
		ret.data = (char *) malloc(ret.size);
	

		memcpy(ret.data, cbuffer + 8, ret.size);
		
		if(byteswap) 
			for(j = 0; j < ret.size; j += matio_sizeof(ret.type))
				matio_swap_bytes(ret.data + j, matio_sizeof(ret.type));
		*offset += ret.size + (ret.size % 8 ? 16 - ret.size % 8 : 8);
	}
	return ret;
}

MATdata matio_process(int datatype, int datasize, char *buffer, int byteswap, int force_complex)
{
	char *unzip_buffer;
	int i, j, status, offset = 0, *dimensions, numel = 1;
	unsigned long unzip_buffer_size;
	matio_element array_flags, dim, varname, pr, pi;
	MATdata ret, temp;
	ret.next = NULL;
		
#ifdef __DEBUG
	fprintf(stderr, "Processing data type %d, size %d\n", datatype, datasize);
#endif
	switch(datatype) {
		case miCOMPRESSED: 
			unzip_buffer_size = 2 * datasize; //estimate a 50% compression ratio
			unzip_buffer = (char *) malloc(unzip_buffer_size);
			
			status = uncompress((unsigned char *)unzip_buffer, &unzip_buffer_size, (unsigned char *)buffer, datasize);

			i = 2;
			while(status == Z_BUF_ERROR) {
				free(unzip_buffer);
				i++;
				unzip_buffer_size = i * datasize;
				unzip_buffer = (char *) malloc(unzip_buffer_size);
				if(!unzip_buffer)
					status = Z_MEM_ERROR;
				else
					status = uncompress((unsigned char *)unzip_buffer, &unzip_buffer_size, (unsigned char *) buffer, datasize);
			}

			switch(status) {
				case Z_OK: 
					if(byteswap) {
						matio_swap_bytes(unzip_buffer, 4);
						matio_swap_bytes(unzip_buffer + 4, 4);
					}
					
					datatype = ((int *) unzip_buffer)[0];
					datasize = ((int *) unzip_buffer)[1];
					
					datasize += datasize % 8 ? 8 - datasize % 8 : 0;
					ret = matio_process(datatype, datasize, unzip_buffer + 8, byteswap, force_complex);
					free(unzip_buffer);
					break;
				case Z_MEM_ERROR : printf("uncompress() failed: not enough memory.\n"); break;
				case Z_DATA_ERROR : printf("uncompress() failed: input data corrupted.\n"); break;
			}
		break;

		case miMATRIX: 
			array_flags = matio_interpret_element(buffer, &offset, byteswap);
			ret.type = *((int *)array_flags.data) & 0x800 ? MATCOMPLEX : MATREAL;

			if((*((int *) array_flags.data) & 0xff) < 6) {
#ifdef __VERBOSE		
				printf("MATIO Error: Only dense numerical arrays presently supported.\n");
				printf("You asked to read type %d\n", array_flags.data[0]);
#endif				
				ret.type = MATERROR;
				ret.name = (char *) malloc(sizeof(char));
				*ret.name = '\0';

				return ret;
			}
			free(array_flags.data);

			dim = matio_interpret_element(buffer, &offset, byteswap);
			ret.dimensions = (int *) malloc(sizeof(int) * dim.size);
			ret.num_dim = dim.size / 4;
			for(i = 0; i < dim.size / 4; i++) {
                		ret.dimensions[i] = ((int *)dim.data)[i];
				numel *= ret.dimensions[i];
			}
			free(dim.data);			
			
			varname = matio_interpret_element(buffer, &offset, byteswap);
			ret.name = (char *) malloc(varname.size + 1);
			strncpy(ret.name, varname.data, varname.size);
			ret.name[varname.size] = '\0';
			free(varname.data);
			
			pr = matio_interpret_element(buffer, &offset, byteswap);
			temp = matio_process(pr.type, pr.size, pr.data, byteswap, force_complex);
			free(pr.data);

			ret.real = temp.real;

			if(ret.type == MATCOMPLEX ) {
				pi = matio_interpret_element(buffer, &offset, byteswap);
				temp = matio_process(pi.type, pi.size, pi.data, byteswap, 0);
				free(pi.data);
				ret.comp = (complex16 *) malloc(numel * sizeof(complex16));
				for(i = 0; i < numel; i++) {
#ifdef __USE_COMPLEX_H
					ret.comp[i] = ret.real[i] + I * temp.real[i];
#else
					ret.comp[i].re = ret.real[i];	
					ret.comp[i].im = temp.real[i];
#endif
				}
					
				free(temp.real);
				free(ret.real);
				ret.real = NULL;
			}
			
			if((ret.type == MATREAL) && force_complex) {
				ret.comp = (complex16 *) calloc(numel, sizeof(complex16));
				
				free(ret.real);
				ret.real = NULL;
				ret.type = MATCOMPLEX;
			}
			break;
		case miDOUBLE:
			ret.real = (double *) malloc(datasize);
			memcpy(ret.real, buffer, datasize);
			break;
		case miINT8:
			ret.real = (double *) malloc(datasize * 8);
			for(i = 0; i < datasize; i++)
				ret.real[i] = (double) buffer[i];
			break;
		case miUINT8:
			ret.real = (double *) malloc(datasize * 8);
			for(i = 0; i < datasize; i++)
				ret.real[i] = (double) ((unsigned char *)buffer)[i];
			break;
		case miINT16:
			ret.real = (double *) malloc(datasize * 4);
			for(i = 0; i < datasize / 2; i++)
				ret.real[i] = (double) ((short int *)buffer)[i];
			break;
		case miUINT16:
			ret.real = (double *) malloc(datasize * 4);
			for(i = 0; i < datasize / 2; i++)
				ret.real[i] = (double) ((unsigned short int *)buffer)[i];
			break;
		case miINT32:
			ret.real = (double *) malloc(datasize * 2);
			for(i = 0; i < datasize / 4; i++)
				ret.real[i] = (double) ((int *)buffer)[i];
			break;
		case miSINGLE:
			ret.real = (double *) malloc(datasize * 2);
			for(i = 0; i < datasize / 4; i++)
				ret.real[i] = (double) ((short int *)buffer)[i];
			break;
		default : printf("Data type %d is not yet supported.\n", datatype); break;
	}
	return ret;
}

void matio_swap_bytes(void *input_data, int N) 
{
	char temp, *data;
	int i;
	data = (char *) input_data;
	if(N == 1) 
		return;
	
	for(i = 0; i < N / 2; i ++) {
		temp = data[N - i - 1];
		data[N - i - 1] = data[i];
		data[i] = temp; 
	}
	return;
}

int matio_sizeof(int type) 
{
	switch(type) {
		case miINT8: return 1; break;
		case miUINT8: return 1; break;
		case miINT16: return 2; break;
		case miUINT16: return 2; break;
		case miINT32: return 4; break;
		case miUINT32: return 4; break;
		case miSINGLE: return 4; break;
		case miDOUBLE: return 8; break;
	}

	return 0;
}

int matio_array_size(MATdata *M)
{
	int i, ret = 1; 
	for(i = 0; i < M -> num_dim; i++)
		ret *= M -> dimensions[i];

	return ret;
}
