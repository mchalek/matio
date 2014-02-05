/*
  matsearch.c: Searches a list of MATdata objects for an object with a specified name.
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

MATdata * matio_search(MATdata * list, char * name)
{
	MATdata *temp;
	int found = 0;
	
	temp = list;
	
	while(temp != NULL && !found) {
		if(strcmp(temp -> name, name) == 0)
			found = 1;
		else
			temp = temp -> next;
	}
	if(found) 
		temp -> queried = 1;
	return temp;
}
