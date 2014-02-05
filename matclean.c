/*
  matclean.c: Provides some memory-cleanup routines
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

void matio_unlink(MATdata * list)
{
	MATdata *temp;
	
	while(list != NULL) {
		if(!(list -> queried) && list -> type != MATERROR) {
			free(list -> dimensions);
			free(list -> name);
			if(list -> type == MATREAL)
				free(list -> real);
			else 
				if(list -> type == MATCOMPLEX)
					free(list -> comp);
			temp = list -> next;
			free(list);
			list = temp;
		} else {
			temp = list -> next;
			list -> next = NULL;
			list = temp;
		}

	}
	return;
}

void matio_destroy(MATdata * data)
{
	MATdata *temp, *list;

	list = data;

	while(list != NULL) {
		free(list -> dimensions);
		free(list -> name);
		if(list -> type == MATREAL)
				free(list -> real);
			else 
				free(list -> comp);
		temp = list -> next;
		free(list);
		list = temp;
	}

	data = NULL;
	
	return;
}
