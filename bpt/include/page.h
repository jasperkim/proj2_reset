/* The following are structure of pages
* Including header_page, general_page, page_header
*/

#include <stdio.h>
#include <stdlib.h>
#include "bpt.h"

typedef struct header_page {	// only for header_page
	long unsigned int free_page_offset[1];
	long unsigned int root_page_offset[1];
	long unsigned int number_of_pages[1];
	char reserved[4072];
} header_page;

typedef struct page_header {
	long unsigned int parent_page_offset[1];
	int is_leaf;
	int num_of_keys;
	char reserved[104];
	long unsigned int next_page_offset;
} page_header;

typedef union pagetype {
	record record[31];
	internal_record internal_record[248];
} pagetype;

typedef struct general_page {
	page_header page_header;
	pagetype pagetype;
} general_page;
	

	

