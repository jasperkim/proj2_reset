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
	int64_t parent_page_offset[1];
	int is_leaf[1];
	int num_of_keys[1];
	char reserved[104];
	int64_t next_page_offset[1];
} page_header;

typedef union pagetype {
	record record[31];
	internal_record internal_record[248];
} pagetype;

typedef struct general_page {
	page_header page_header;
	pagetype pagetype;
} general_page;

typedef struct find_general_page {
	page_header find_page_header;
	pagetype find_pagetype;
} find_general_page;  // use this find_general_page to find page

//
/* Using pages as global structure

*/
//header_page headerpage;
//general_page generalpage;
//find_general_page find_generalpage;


	

	

