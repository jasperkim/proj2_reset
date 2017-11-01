#define Version "1.14"

#include "bpt.h"
#include "page.h"

int fd = 0;

int order = DEFAULT_ORDER;

node * queue = NULL;

bool verbose_output = false;

header_page headerpage;
general_page generalpage;
general_page findpage;
off_t leafpage_offset, leftpage_offset, rightpage_offset, newpage_offset;

void license_notice(void) {
	printf("bpt version %s -- Copyright (C) 2010  Amittai Aviram "
		"http://www.amittai.com\n", Version);
	printf("This program comes with ABSOLUTELY NO WARRANTY; for details "
		"type `show w'.\n"
		"This is free software, and you are welcome to redistribute it\n"
		"under certain conditions; type `show c' for details.\n\n");
}
void print_license(int license_part) {
	int start, end, line;
	FILE * fp;
	char buffer[0x100];

	switch (license_part) {
	case LICENSE_WARRANTEE:
		start = LICENSE_WARRANTEE_START;
		end = LICENSE_WARRANTEE_END;
		break;
	case LICENSE_CONDITIONS:
		start = LICENSE_CONDITIONS_START;
		end = LICENSE_CONDITIONS_END;
		break;
	default:
		return;
	}

	fp = fopen(LICENSE_FILE, "r");
	if (fp == NULL) {
		perror("print_license: fopen");
		exit(EXIT_FAILURE);
	}
	for (line = 0; line < start; line++)
		fgets(buffer, sizeof(buffer), fp);
	for (; line < end; line++) {
		fgets(buffer, sizeof(buffer), fp);
		printf("%s", buffer);
	}
	fclose(fp);
}
void usage_1(void) {
	printf("B+ Tree of Order %d.\n", order);
	printf("Following Silberschatz, Korth, Sidarshan, Database Concepts, "
		"5th ed.\n\n"
		"To build a B+ tree of a different order, start again and enter "
		"the order\n"
		"as an integer argument:  bpt <order>  ");
	printf("(%d <= order <= %d).\n", MIN_ORDER, MAX_ORDER);
	printf("To start with input from a file of newline-delimited integers, \n"
		"start again and enter the order followed by the filename:\n"
		"bpt <order> <inputfile> .\n");
}
void usage_2(void) {
	printf("Enter any of the following commands after the prompt > :\n"
		"\ti <k>  -- Insert <k> (an integer) as both key and value).\n"
		"\tf <k>  -- Find the value under key <k>.\n"
		"\tp <k> -- Print the path from the root to key k and its associated "
		"value.\n"
		"\tr <k1> <k2> -- Print the keys and values found in the range "
		"[<k1>, <k2>\n"
		"\td <k>  -- Delete key <k> and its associated value.\n"
		"\tx -- Destroy the whole tree.  Start again with an empty tree of the "
		"same order.\n"
		"\tt -- Print the B+ tree.\n"
		"\tl -- Print the keys of the leaves (bottom row of the tree).\n"
		"\tv -- Toggle output of pointer addresses (\"verbose\") in tree and "
		"leaves.\n"
		"\tq -- Quit. (Or use Ctl-D.)\n"
		"\t? -- Print this help message.\n");
}
void usage_3(void) {
	printf("Usage: ./bpt [<order>]\n");
	printf("\twhere %d <= order <= %d .\n", MIN_ORDER, MAX_ORDER);
}

/* 	A helper function for opening an DB file
int open_db (char *pathname);	from the main.c, ( newfile == pathname )
*/
int open_db(char *pathname) {
	int a, b, c;
	//	header_page headerpage;
	//	long int free_page_offset_buffer[1];
	//	long int root_page_offset_buffer[1];
	//	long int number_of_pages_buffer[1];
	long int init_offset_buffer[1] = { 0 };
	long int init_number_of_pages[1] = { 1 };
	if ((fd = open(pathname, O_RDWR)) >= 0) {
		printf("File already exists and is now opened.\n");
	}
	else {
		printf("%d\n", fd);
		if ((fd = open(pathname, O_CREAT | O_EXCL | O_RDWR, 0777)) == -1) {
			perror("File already exists. Failure creating a file.\n");
			exit(EXIT_FAILURE);
		}
		else {
			//			printf("before calloc %d\n", sizeof(headerpage));
			printf("File is created and is now opened.\n");
			//			headerpage = (*header_page)calloc(1, sizeof(PAGE_SIZE));
			//			printf("after calloc %d\n", sizeof(headerpage));
			if (pwrite(fd, init_offset_buffer, sizeof(headerpage.free_page_offset), 0) == -1)
				perror("Failed to init header_page -> free_page_offset.\n");
			if (pwrite(fd, init_offset_buffer, sizeof(headerpage.root_page_offset), 8) == -1)
				perror("Failed to init header_page -> root_page_offset.\n");
			if (pwrite(fd, init_number_of_pages, sizeof(headerpage.number_of_pages), 16) == -1)
				perror("Failed to init header_page -> number_of_pages.\n");
			if (pwrite(fd, headerpage.reserved, sizeof(headerpage.reserved), 24) == -1)
				perror("Failed to achieve reserved space header_page -> reserved[4072]\n");
		}
	}
	// from here, we read the header_page
	if ((a = pread(fd, headerpage.free_page_offset, sizeof(headerpage.free_page_offset), 0)) == -1)
		perror("Failed to read header_page -> free_page_offset.\n");
	if ((b = pread(fd, headerpage.root_page_offset, sizeof(headerpage.root_page_offset), 8)) == -1)
		perror("Failed to read header_page -> root_page_offset.\n");
	if ((c = pread(fd, headerpage.number_of_pages, sizeof(headerpage.number_of_pages), 16)) == -1)
		perror("Failed to read header_page -> number_of_pages.\n");

	printf("Unntil now, file creation or opening succeeded.\n");
	printf("File descriptor value : %d\n", fd);
	if ((a != -1) && (b != -1) && (c != -1))	// when reading all
		return 0;
	else
		return -1;
}

/* Helper function for printing the
* tree out.  See print_tree.
*/
void enqueue(node * new_node) {
	node * c;
	if (queue == NULL) {
		queue = new_node;
		queue->next = NULL;
	}
	else {
		c = queue;
		while (c->next != NULL) {
			c = c->next;
		}
		c->next = new_node;
		new_node->next = NULL;
	}
}
node * dequeue(void) {
	node * n = queue;
	queue = queue->next;
	n->next = NULL;
	return n;
}
/* Prints the bottom row of keys
* of the tree (with their respective
* pointers, if the verbose_output flag is set.
*/
void print_leaves(node * root) {
	int i;
	node * c = root;
	if (root == NULL) {
		printf("Empty tree.\n");
		return;
	}
	while (!c->is_leaf)
		c = c->pointers[0];
	while (true) {
		for (i = 0; i < c->num_keys; i++) {
			if (verbose_output)
				printf("%lx ", (unsigned long)c->pointers[i]);
			printf("%d ", c->keys[i]);
		}
		if (verbose_output)
			printf("%lx ", (unsigned long)c->pointers[order - 1]);
		if (c->pointers[order - 1] != NULL) {
			printf(" | ");
			c = c->pointers[order - 1];
		}
		else
			break;
	}
	printf("\n");
}

/* Utility function to give the height
* of the tree, which length in number of edges
* of the path from the root to any leaf.
*/
int height(node * root) {
	int h = 0;
	node * c = root;
	while (!c->is_leaf) {
		c = c->pointers[0];
		h++;
	}
	return h;
}

/* Utility function to give the length in edges
* of the path from any node to the root.
*/
int path_to_root(node * root, node * child) {
	int length = 0;
	node * c = child;
	while (c != root) {
		c = c->parent;
		length++;
	}
	return length;
}

// Utility function for finding the offset_address of rootpage.
off_t find_rootpage(void) {

	off_t rootpage_offset;
	rootpage_offset = headerpage.root_page_offset[1];
	if (rootpage_offset == 0) {
		printf("The tree is empty!\n");
		return 0;
	}
	else
		return rootpage_offset;
}

/* Utility function to decide whether use freepage or create page
*  Return, global structure, findpage.
*/
struct general_page create_newpage(void) {
	off_t newpage_offset;
	printf("Function : create_newpage.\n");
	int temp[1024];
	if (headerpage.free_page_offset[0]) {
		printf("When freepage exist, use freepage first.\n");
		newpage_offset = headerpage.free_page_offset[0];
		make_findpage(newpage_offset);
		printf("First freepage copied to findpage.\n");
		headerpage.free_page_offset[0] = findpage.page_header.parent_page_offset[0];
		printf("Headerpage -> freepage_offset is moved to the next one.\n");
		findpage.page_header.num_of_keys[0] = 0;
		return findpage;
	}
	else if (!headerpage.free_page_offset[0]) {
		pwrite(fd, temp, sizeof(general_page), 4096 * headerpage.number_of_pages[0]);
		printf("When freepage doesn't exist, create new page. Using pwrite.\n");
		make_findpage(4096 * headerpage.number_of_pages[0]);
		printf("End address of last page copied to findpage.\n");
		newpage_offset = 4096 * headerpage.number_of_pages[0];
		findpage.page_header.num_of_keys[0] = 0;
		return findpage;
	}
}

// Helper function : INPUT is PAGE_ADDRESS, OUTPUT is 0, -1
// Findpage can be both internal_paage or leaf_page
// When successfully copied, return 0, else -1
int make_findpage(int64_t page_address) {

	printf("Copy page_header to Findpage : Start.\n");
	if (pread(fd, (findpage.page_header.parent_page_offset), sizeof(int64_t), page_address) == -1) {
		printf("find_generalpage -> find_page_header -> parent_page_offset ERROR.\n");
		return -1;	}
	if (pread(fd, (findpage.page_header.is_leaf), sizeof(int), (page_address + 8)) == -1) {
		printf("find_generalpage -> find_page_header -> is_leaf ERROR.\n");
		return -1;	}
	if (pread(fd, (findpage.page_header.num_of_keys), sizeof(int), (page_address + 12)) == -1) {
		printf("find_generalpage -> find_page_header -> #pages ERROR.\n");
		return -1;	}
	if (pread(fd, (findpage.page_header.reserved), sizeof(char)*104, (page_address + 16)) == -1) {
		printf("find_generalpage -> find_page_header -> reserved ERROR.\n");
		return -1;	}
	if (pread(fd, (findpage.page_header.next_page_offset), sizeof(int64_t), page_address + 120) == -1) {
		printf("find_generalpage -> find_page_header -> next_page_offset ERROR.\n");
		return -1;	}
	printf("Copy page_header to Findpage : End.\nNow copy pagedata to Findpage.\n");

	if (findpage.page_header.is_leaf[0] == 1) {		// if it is leaf_page
		for (int i = 0; i < 31; i++) {
			if (pread(fd, findpage.pagetype.record[i].key, sizeof(int64_t),
				page_address + 128 * (i + 1)) == -1) return -1;
			if (pread(fd, findpage.pagetype.record[i].value, sizeof(char) * 120,
				page_address + 128 * (i + 1) + sizeof(int64_t)) == -1) return -1;
		}
		return 0; // successfully copied
	}
	else if (findpage.page_header.is_leaf[0] == 0) {	// if it is internal_page
		for (int i = 0; i < 248; i++) {
			if (pread(fd, findpage.pagetype.internal_record[i].key, sizeof(int64_t),
				page_address + 128 + (16 * i)) == -1) return -1;
			if (pread(fd, findpage.pagetype.internal_record[i].page_offset, sizeof(int64_t),
				page_address + 128 + (16 * i) + sizeof(int64_t)) == -1) return -1;
		}
		return 0; // successfully copied
	}
}

char * leafpage_find_key(off_t leafpage_offset, int key) {

	int i;
	char * value;
	if (make_findpage(leafpage_offset) == -1)
		printf("FIND LEAF PAGE KEY COPY ERROR.\n");
	printf("Findpage is Leafpage");
	// findpage == leafpage

	i = 0;
	while ((i < findpage.page_header.num_of_keys[0]) && (key<findpage.pagetype.record[i].key))
		i++;
	if (findpage.pagetype.record[i].key == key) {
		printf("i moved to corresponding index and found correct key.\n");
		return findpage.pagetype.record[i].value;
	}
	else {
		printf("No corresponding key in the Leafpage.\n");
		return NULL;
	}
}

// Master find function.
char * find(int64_t key) {

	off_t rootpage_offset;
	off_t internalpage_offset;
	off_t leafpage_offset;
	int i;
	rootpage_offset = find_root_page();
	if (rootpage_offset == 0) {			//There is no ROOT in the TREE.
		printf("TREE IS EMPTY.\n");
		return NULL;
	}
	else { // we found the root page
		lseek(fd, rootpage_offset, 0);
		if (make_findpage(rootpage_offset) == -1) 
			printf("DATA COPY ERROR FOR ROOTPAGE.\n");
		printf("Rootpage data is copied to Findpage.\n");
	}
	while (!(findpage.page_header.is_leaf[0])) {

		if (!(findpage.page_header.is_leaf[0]) && (key<findpage.pagetype.internal_record[0].key))
			internalpage_offset = findpage.page_header.next_page_offset[0];	// When page is located on the very left.
		else {
			int i = 0;
			while (i < findpage.page_header.num_of_keys[0] && key <= findpage.pagetype.internal_record[i].key)
				i++;
			internalpage_offset = findpage.pagetype.internal_record[i].page_offset;
		}
		// next page address copied to internalpaage_offset;
		if (make_findpage(internalpage_offset) == -1)
				printf("DATA COPY ERROR FOR INTERNALPAGE.\n");
	}
	printf("Leafpage data is copied to Findpage.\n");
	printf("Using Leafpage_address, find key in the address.\n");
	return leafpage_find_key(internalpage_offset, key);
}

int cut(int length) {
	if (length % 2 == 0)
		return length / 2;
	else
		return length / 2 + 1;
}


// INSERTION

/* Creates a new record to hold the value
* to which a key refers.
*/
struct record make_record(int key, char * value) {
	record newrecord;
	newrecord.key = key;
	*(newrecord.value) = value;
	return newrecord;
}


struct general_page find_leafpage(int64_t key) {
	off_t rootpage_offset;
	off_t internalpage_offset;
	int i;

	printf("Function : find_leafpage.\n");
	rootpage_offset = find_root_page();

	lseek(fd, rootpage_offset, 0);
	if (make_findpage(rootpage_offset) == -1)
		printf("DATA COPY ERROR FOR ROOTPAGE.\n");

	printf("Rootpage data is copied to Findpage.\n");

	while (!(findpage.page_header.is_leaf[0])) {
		if (!(findpage.page_header.is_leaf[0]) && (key<findpage.pagetype.internal_record[0].key))
			internalpage_offset = findpage.page_header.next_page_offset[0];	// When page is located on the very left.
		else {
			int i = 0;
			while (i < findpage.page_header.num_of_keys[0] && key <= findpage.pagetype.internal_record[i].key)
				i++;
			internalpage_offset = findpage.pagetype.internal_record[i].page_offset;
		}
		// next page address copied to internalpaage_offset;
		if (make_findpage(internalpage_offset) == -1)
			printf("DATA COPY ERROR FOR INTERNALPAGE.\n");
	}

	leafpage_offset = internalpage_offset;
	printf("Leafpage data is copied to Findpage.\n");
	printf("Using Leafpage_address, find key in the address.\n");
	printf("Global variable, leafpage_offset is set.\n");
	return findpage;
}

struct general_page insert_into_leafpage(struct general_page leafpage, int64_t key, char *value) {
	int i, insertion_point;
	insertion_point = -1;
	while (insertion_point < leafpage.page_header.num_of_keys[0]
		&& leafpage.pagetype.record[insertion_point].key < key)
		insertion_point++;
	// if insertion_point is -1, 
	for (i = leafpage.page_header.num_of_keys[0]; i > insertion_point; i--) {
		leafpage.pagetype.record[i].key = leafpage.pagetype.record[i - 1].key;
		*(leafpage.pagetype.record[i].value) = *(leafpage.pagetype.record[i - 1].value);
	}
	printf("Function : insert_into_leafpage.\nExisting records are pushed to side.\n");
	leafpage.pagetype.record[insertion_point].key = key;
	*(leafpage.pagetype.record[insertion_point].value) = value;
	leafpage.page_header.num_of_keys[0]++;
	printf("Function : insert_into_leafpage.\nNew record is inserted to insertion_point.\n");
	return leafpage;
}

int insert_into_new_rootpage(struct general_page leftpage,
	struct general_page rightpage, int64_t key) {
	int leftpage_offset, rightpage_offset;
	general_page new_rootpage;
	printf("Function : insert_into_new_rootpage.\n");
	new_rootpage = create_newpage();
	new_rootpage.page_header.parent_page_offset[0] = 0;
	new_rootpage.page_header.is_leaf[0] = 0;
	new_rootpage.page_header.num_of_keys[0] = 1;
	//	new_rootpage.page_header.next_page_offset = leftpage_offset;
	new_rootpage.pagetype.internal_record[0].key = key;
	//	new_rootpage.pagetype.internal_record[0].page_offset = rightpage_offset;

	return 0;
}

/* Helper function used in insert_into_parentpage
* to find the index of the parent's pointer to
* the node to the left of the key to be inserted.
*/
int get_left_index(struct general_page parentpage, off_t leftpage_offset) {
	int left_index = 0;	// unlike leaf, it starts from 0
	printf("Function : get_left_index.\n");
	while (left_index <= parentpage.page_header.num_of_keys[0] &&
		parentpage.pagetype.internal_record[0].page_offset != leftpage_offset)
		left_index++;
	return left_index;
}

int insert_into_internalpage(struct general_page parentpage, int left_index,
	int64_t key, off_t rightpage_offset) {
	int i;
	printf("Function : insert_into_node.\n");
	for (i = parentpage.page_header.num_of_keys[0]; i > left_index; i--) {
		parentpage.pagetype.internal_record[i + 1].page_offset
			= parentpage.pagetype.internal_record[i].page_offset;
		parentpage.pagetype.internal_record[i + 1].key
			= parentpage.pagetype.internal_record[i].key;
	}
	printf("Parentpage internal_record moved up by 1.\n");
	parentpage.pagetype.internal_record[left_index].key = key;
	parentpage.pagetype.internal_record[left_index].page_offset = rightpage_offset;
	parentpage.page_header.num_of_keys[0]++;
	printf("Inserted pushed up value to the parentpage.\n");
	return 0;
}

int insert_into_internalpage_after_splitting(struct general_page oldpage,
	int left_index, int64_t key, off_t rightpage_offset) {
	// oldpage == leftpage, 
	int i, j, split;
	int64_t k_prime;
	off_t leftmost_offset;
	// oldpage's leftmost_page_offset is saved to leftmost_offset;
	general_page newpage;
	int64_t temp_keys[INTERNAL_ORDER + 1];
	off_t temp_page_offset[INTERNAL_ORDER + 1];

	printf("Function : insert_into_node_after_splitting\n");

	leftmost_offset = oldpage.page_header.next_page_offset[0];
	// oldpage's leftmost_page_offset is saved to leftmost_offset;
	for (i = 0, j = 0; i < (oldpage.page_header.num_of_keys[0] + 1); i++, j++) {
		if (j == left_index + 1)j++;
		temp_page_offset[j] = oldpage.pagetype.internal_record[i].key;
		temp_keys[j] = oldpage.pagetype.internal_record[i].page_offset;
	}
	temp_page_offset[left_index] = rightpage_offset;
	temp_keys[left_index] = key;
	printf("Data of original Parentpage is copied to temp_array.\n");
	split = cut(INTERNAL_ORDER + 1);

	//	split == 125[124]
	newpage = create_newpage();
	//	newpage_offset
	oldpage.page_header.num_of_keys[0] = 0, newpage.page_header.num_of_keys[0] = 0;
	oldpage.page_header.parent_page_offset[0] = leftmost_offset; // 
	for (i = 0; i < split; i++) {
		oldpage.pagetype.internal_record[i].page_offset = temp_page_offset[i];
		oldpage.pagetype.internal_record[i].key = temp_keys[i];
		oldpage.page_header.num_of_keys[0]++;
	}

	k_prime = temp_keys[split];
	newpage.page_header.next_page_offset[0] = temp_page_offset[split];

	for (++i; i <= INTERNAL_ORDER; i++) {
		newpage.pagetype.internal_record[i].page_offset = temp_page_offset[i];
		newpage.pagetype.internal_record[i].key = temp_keys[i];
		newpage.page_header.num_of_keys[0]++;
	}
	printf("Data of Leftpage and Rightpage of internalpage is copied.\n");

	newpage.page_header.parent_page_offset[0]
		= oldpage.page_header.parent_page_offset[0];
	printf("Leftpage and Rightpage has the same Parentpage_offset.\n");

	rightpage_offset = newpage_offset;

	for (i = 0; i <= newpage.page_header.num_of_keys[0]; i++) {
		if ((make_findpage(newpage.pagetype.internal_record[i].page_offset)) == 0);
		// Findpage is the Childpage of Rightpage.
		findpage.page_header.parent_page_offset[0] = newpage_offset;
		// Childpage's parentpage_offset is set to Rightpage.
	}
	printf("The parentpage_offset of Childpages of Rightpage is set to Rightpage.\n");
	return insert_into_parentpage(oldpage, k_prime, newpage);
}


int insert_into_parentpage(struct general_page leftpage, int64_t key,
	struct general_page rightpage) {

	int left_index;
	general_page parentpage;
	//	leftpage_offset, rightpage_offset;
	printf("Function : insert_into_parentpage.\n");

	if (make_findpage(leftpage.page_header.parent_page_offset[0]) == -1)
		printf("Error when copying the parentpage to the findpage.\n");
	parentpage = findpage;

	// Case : The level is 1, now create a new root.
	if (parentpage.page_header.parent_page_offset[0] == 0) {
		printf("Case : Only had a leafpage, creat the rootpage.\n");
		return insert_into_new_rootpage(leftpage, rightpage, key);
	}

	// 
	left_index = get_left_index(parentpage, leftpage_offset);

	// Case : The newkey fits into the node.
	if (parentpage.page_header.num_of_keys[0] < INTERNAL_ORDER) {
		printf("Case : The newkey fits into the parent page. Just insert the value and offset directly.\n");
		return insert_into_internalpage(parentpage, left_index, key, rightpage_offset);
	}

	// Case : Split the parent page to preserve the B+Tree property.
	printf("Case : Split the parent page to preserve the B+Tree property.\n");
	return insert_into_internalpage_after_splitting(parentpage, left_index, key, rightpage_offset);
}

int insert_into_leafpage_after_splitting(struct general_page leafpage,
	int64_t key, char * value) {

	general_page newpage;
	int temp_keys[32];
	char temp_value[32][120];
	int insertion_index, split, newkey, i, j;
	printf("Function : insert_into_leafpage_after_splitting\n");
	newpage = create_newpage();
	rightpage_offset = newpage_offset;	// from create_newpage function, 
										// newpage_offset is set.
	insertion_index = 0;

	while (insertion_index < order - 1 &&
		leafpage.pagetype.record[insertion_index].key < key)
		insertion_index++;

	for (i = 0, j = 0; i < leafpage.page_header.num_of_keys[0]; i++, j++) {
		if (j == insertion_index) j++;
		temp_keys[j] = leafpage.pagetype.record[i].key;
		strcpy(leafpage.pagetype.record[i].value, temp_value[j]);
	}
	temp_keys[j] = leafpage.pagetype.record[i].key;
	strcpy(leafpage.pagetype.record[j].value, temp_value[i]);
	printf("All data is saved to temp_array.\n");

	leafpage.page_header.num_of_keys[0] = 0;
	split = cut(order - 1);
	printf("Temp_arrays are splitted into half.\n");

	for (i = 0; i < split; i++) {
		strcpy(temp_value[i], leafpage.pagetype.record[i].value);
		leafpage.pagetype.record[i].key = temp_keys[i];
		leafpage.page_header.num_of_keys[0]++;
	}
	for (i = split, j = 0; i < order; i++, j++) {
		strcpy(temp_value[i], newpage.pagetype.record[i].value);
		newpage.pagetype.record[i].key = temp_keys[i];
		newpage.page_header.num_of_keys[0]++;
	}
	printf("Save the data in separate leftpage and rightpage.\n");
	// leafpage == leftpage, newpage == rightpage
	leafpage.page_header.is_leaf[0] = 1;
	newpage.page_header.is_leaf[0] = 1;
	newpage.page_header.next_page_offset[0] = leafpage.page_header.next_page_offset[0];
	leafpage.page_header.next_page_offset[0] = rightpage_offset;
	newpage.page_header.parent_page_offset[0] = leafpage.page_header.parent_page_offset[0];
	printf("Features of each pages ae given.\n");
	// 

	newkey = newpage.pagetype.record[0].key;
	return insert_into_parentpage(leafpage, newkey, newpage);
}

//  Utility function for starting new tree.
int start_new_tree(int64_t key, char * value) {
	general_page newpage;
	newpage = create_newpage();
	newpage.page_header.parent_page_offset[0] = 0;	// parent is the headerpage
	newpage.page_header.is_leaf[0] = 1;				// newpage of newtree is always leaf
	newpage.page_header.num_of_keys[0] = 1;			// only one key exists, <key>
	newpage.pagetype.record[0].key = key;
	*(newpage.pagetype.record[0].value) = value;
	printf("Data inserted to the root page.\n");
	printf("key : %d\tvalue : %s", newpage.pagetype.record[0].key, *(newpage.pagetype.record[0].value));
	return 0;
}

/* Master insertion function.
* Inserts a key and an associated value into
* the B+ tree, causing the tree to be adjusted
* however necessary to maintain the B+ tree
* properties.
*/
// Master insertion function
int insert(int64_t key, char * value) {
	general_page newpage;
	general_page leafpage;
	record newrecord;

	/* Duplicate keys are not allowed.
	*  Cannot insert to a tree and return -1.
	*/
	if (find(key) != NULL) {
		printf("Duplicate key was inserted.\n");
		return -1;
	}
	newrecord = make_record(key, value);

	printf("Case : The root page does not exist.\nStart a new tree.\n");

	if (headerpage.root_page_offset == 0) {
		printf("Create rootpage(leaf node) and insert data.\n");
		return start_new_tree(key, value);
	}

	printf("Case : The tree already exists.\nInclude to the tree.\n");
	leafpage = find_leafpage(key);
	// leafpage_offset is set
	printf("Find_leafpage has the data of Leaftree, and returned to Leafpage.\n");
	if (leafpage.page_header.num_of_keys[0] < order - 1) {
		printf("Case : Leaf page has space to insert a new record.\n");
		leafpage = insert_into_leafpage(leafpage, key, value); // Input leafpage -> Output leafpage
		printf("New record is inserted to a leafpage.\n");
		return 1; // How can I tell, it was inserted correctly?
	}

	printf("Case : Leaf page has NO space to insert a new record.\n");
	return insert_into_leafpage_after_splitting(leafpage, key, value);
}




// DELETION.

/* Utility function for deletion.  Retrieves
* the index of a node's nearest neighbor (sibling)
* to the left if one exists.  If not (the node
* is the leftmost child), returns -1 to signify
* this special case.
*/

int get_neighbor_index(node * n) {

	int i;

	/* Return the index of the key to the left
	* of the pointer in the parent pointing
	* to n.
	* If n is the leftmost child, this means
	* return -1.
	*/
	for (i = 0; i <= n->parent->num_keys; i++)
		if (n->parent->pointers[i] == n)
			return i - 1;

	// Error state.
	printf("Search for nonexistent pointer to node in parent.\n");
	printf("Node:  %#lx\n", (unsigned long)n);
	exit(EXIT_FAILURE);
}


node * remove_entry_from_node(node * n, int key, node * pointer) {

	int i, num_pointers;

	// Remove the key and shift other keys accordingly.
	i = 0;
	while (n->keys[i] != key)
		i++;
	for (++i; i < n->num_keys; i++)
		n->keys[i - 1] = n->keys[i];

	// Remove the pointer and shift other pointers accordingly.
	// First determine number of pointers.
	num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
	i = 0;
	while (n->pointers[i] != pointer)
		i++;
	for (++i; i < num_pointers; i++)
		n->pointers[i - 1] = n->pointers[i];


	// One key fewer.
	n->num_keys--;

	// Set the other pointers to NULL for tidiness.
	// A leaf uses the last pointer to point to the next leaf.
	if (n->is_leaf)
		for (i = n->num_keys; i < order - 1; i++)
			n->pointers[i] = NULL;
	else
		for (i = n->num_keys + 1; i < order; i++)
			n->pointers[i] = NULL;

	return n;
}


node * adjust_root(node * root) {

	node * new_root;

	/* Case: nonempty root.
	* Key and pointer have already been deleted,
	* so nothing to be done.
	*/

	if (root->num_keys > 0)
		return root;

	/* Case: empty root.
	*/

	// If it has a child, promote 
	// the first (only) child
	// as the new root.

	if (!root->is_leaf) {
		new_root = root->pointers[0];
		new_root->parent = NULL;
	}

	// If it is a leaf (has no children),
	// then the whole tree is empty.

	else
		new_root = NULL;

	free(root->keys);
	free(root->pointers);
	free(root);

	return new_root;
}


/* Coalesces a node that has become
* too small after deletion
* with a neighboring node that
* can accept the additional entries
* without exceeding the maximum.
*/
node * coalesce_nodes(node * root, node * n, node * neighbor, int neighbor_index, int k_prime) {

	int i, j, neighbor_insertion_index, n_end;
	node * tmp;

	/* Swap neighbor with node if node is on the
	* extreme left and neighbor is to its right.
	*/

	if (neighbor_index == -1) {
		tmp = n;
		n = neighbor;
		neighbor = tmp;
	}

	/* Starting point in the neighbor for copying
	* keys and pointers from n.
	* Recall that n and neighbor have swapped places
	* in the special case of n being a leftmost child.
	*/

	neighbor_insertion_index = neighbor->num_keys;

	/* Case:  nonleaf node.
	* Append k_prime and the following pointer.
	* Append all pointers and keys from the neighbor.
	*/

	if (!n->is_leaf) {

		/* Append k_prime.
		*/

		neighbor->keys[neighbor_insertion_index] = k_prime;
		neighbor->num_keys++;


		n_end = n->num_keys;

		for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
			neighbor->keys[i] = n->keys[j];
			neighbor->pointers[i] = n->pointers[j];
			neighbor->num_keys++;
			n->num_keys--;
		}

		/* The number of pointers is always
		* one more than the number of keys.
		*/

		neighbor->pointers[i] = n->pointers[j];

		/* All children must now point up to the same parent.
		*/

		for (i = 0; i < neighbor->num_keys + 1; i++) {
			tmp = (node *)neighbor->pointers[i];
			tmp->parent = neighbor;
		}
	}

	/* In a leaf, append the keys and pointers of
	* n to the neighbor.
	* Set the neighbor's last pointer to point to
	* what had been n's right neighbor.
	*/

	else {
		for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
			neighbor->keys[i] = n->keys[j];
			neighbor->pointers[i] = n->pointers[j];
			neighbor->num_keys++;
		}
		neighbor->pointers[order - 1] = n->pointers[order - 1];
	}

	root = delete_entry(root, n->parent, k_prime, n);
	free(n->keys);
	free(n->pointers);
	free(n);
	return root;
}


/* Redistributes entries between two nodes when
* one has become too small after deletion
* but its neighbor is too big to append the
* small node's entries without exceeding the
* maximum
*/
node * redistribute_nodes(node * root, node * n, node * neighbor, int neighbor_index,
	int k_prime_index, int k_prime) {

	int i;
	node * tmp;

	/* Case: n has a neighbor to the left.
	* Pull the neighbor's last key-pointer pair over
	* from the neighbor's right end to n's left end.
	*/

	if (neighbor_index != -1) {
		if (!n->is_leaf)
			n->pointers[n->num_keys + 1] = n->pointers[n->num_keys];
		for (i = n->num_keys; i > 0; i--) {
			n->keys[i] = n->keys[i - 1];
			n->pointers[i] = n->pointers[i - 1];
		}
		if (!n->is_leaf) {
			n->pointers[0] = neighbor->pointers[neighbor->num_keys];
			tmp = (node *)n->pointers[0];
			tmp->parent = n;
			neighbor->pointers[neighbor->num_keys] = NULL;
			n->keys[0] = k_prime;
			n->parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
		}
		else {
			n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
			neighbor->pointers[neighbor->num_keys - 1] = NULL;
			n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
			n->parent->keys[k_prime_index] = n->keys[0];
		}
	}

	/* Case: n is the leftmost child.
	* Take a key-pointer pair from the neighbor to the right.
	* Move the neighbor's leftmost key-pointer pair
	* to n's rightmost position.
	*/

	else {
		if (n->is_leaf) {
			n->keys[n->num_keys] = neighbor->keys[0];
			n->pointers[n->num_keys] = neighbor->pointers[0];
			n->parent->keys[k_prime_index] = neighbor->keys[1];
		}
		else {
			n->keys[n->num_keys] = k_prime;
			n->pointers[n->num_keys + 1] = neighbor->pointers[0];
			tmp = (node *)n->pointers[n->num_keys + 1];
			tmp->parent = n;
			n->parent->keys[k_prime_index] = neighbor->keys[0];
		}
		for (i = 0; i < neighbor->num_keys - 1; i++) {
			neighbor->keys[i] = neighbor->keys[i + 1];
			neighbor->pointers[i] = neighbor->pointers[i + 1];
		}
		if (!n->is_leaf)
			neighbor->pointers[i] = neighbor->pointers[i + 1];
	}

	/* n now has one more key and one more pointer;
	* the neighbor has one fewer of each.
	*/

	n->num_keys++;
	neighbor->num_keys--;

	return root;
}


/* Deletes an entry from the B+ tree.
* Removes the record and its key and pointer
* from the leaf, and then makes all appropriate
* changes to preserve the B+ tree properties.
*/
node * delete_entry(node * root, node * n, int key, void * pointer) {

	int min_keys;
	node * neighbor;
	int neighbor_index;
	int k_prime_index, k_prime;
	int capacity;

	// Remove key and pointer from node.

	n = remove_entry_from_node(n, key, pointer);

	/* Case:  deletion from the root.
	*/

	if (n == root)
		return adjust_root(root);


	/* Case:  deletion from a node below the root.
	* (Rest of function body.)
	*/

	/* Determine minimum allowable size of node,
	* to be preserved after deletion.
	*/

	min_keys = n->is_leaf ? cut(order - 1) : cut(order) - 1;

	/* Case:  node stays at or above minimum.
	* (The simple case.)
	*/

	if (n->num_keys >= min_keys)
		return root;

	/* Case:  node falls below minimum.
	* Either coalescence or redistribution
	* is needed.
	*/

	/* Find the appropriate neighbor node with which
	* to coalesce.
	* Also find the key (k_prime) in the parent
	* between the pointer to node n and the pointer
	* to the neighbor.
	*/

	neighbor_index = get_neighbor_index(n);
	k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
	k_prime = n->parent->keys[k_prime_index];
	neighbor = neighbor_index == -1 ? n->parent->pointers[1] :
		n->parent->pointers[neighbor_index];

	capacity = n->is_leaf ? order : order - 1;

	/* Coalescence. */

	if (neighbor->num_keys + n->num_keys < capacity)
		return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

	/* Redistribution. */

	else
		return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}



/* Master deletion function.
*/
node * delete(node * root, int key) {

	node * key_leaf;
	record * key_record;

	key_record = find(key);
	key_leaf = find_leaf(root, key, false);
	if (key_record != NULL && key_leaf != NULL) {
		root = delete_entry(root, key_leaf, key, key_record);
		free(key_record);
	}
	return root;
}


void destroy_tree_nodes(node * root) {
	int i;
	if (root->is_leaf)
		for (i = 0; i < root->num_keys; i++)
			free(root->pointers[i]);
	else
		for (i = 0; i < root->num_keys + 1; i++)
			destroy_tree_nodes(root->pointers[i]);
	free(root->pointers);
	free(root->keys);
	free(root);
}


node * destroy_tree(node * root) {
	destroy_tree_nodes(root);
	return NULL;
}
