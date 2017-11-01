#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
//#include <unistd.h>

#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// Default order is 4.
#define DEFAULT_ORDER 4

// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_ORDER 20

#define LEAF_ORDER 31
#define INTERNAL_ORDER 248

// Size of each page
#define PAGE_SIZE 4096

// Constants for printing part or all of the GPL license.
#define LICENSE_FILE "LICENSE.txt"
#define LICENSE_WARRANTEE 0
#define LICENSE_WARRANTEE_START 592
#define LICENSE_WARRANTEE_END 624
#define LICENSE_CONDITIONS 1
#define LICENSE_CONDITIONS_START 70
#define LICENSE_CONDITIONS_END 625

// TYPES.

/* Type representing the record
* to which a given key refers.
* In a real B+ tree system, the
* record would hold data (in a database)
* or a file (in an operating system)
* or some other information.
* Users can rewrite this part of the code
* to change the type and content
* of the value field.
*/
typedef struct record {
	int64_t key;
	char value[120];
} record;
typedef struct internal_record {
	int64_t key;
	int64_t page_offset;
} internal_record;
/* Type representing a node in the B+ tree.
* This type is general enough to serve for both
* the leaf and the internal node.
* The heart of the node is the array
* of keys and the array of corresponding
* pointers.  The relation between keys
* and pointers differs between leaves and
* internal nodes.  In a leaf, the index
* of each key equals the index of its corresponding
* pointer, with a maximum of order - 1 key-pointer
* pairs.  The last pointer points to the
* leaf to the right (or NULL in the case
* of the rightmost leaf).
* In an internal node, the first pointer
* refers to lower nodes with keys less than
* the smallest key in the keys array.  Then,
* with indices i starting at 0, the pointer
* at i + 1 points to the subtree with keys
* greater than or equal to the key in this
* node at index i.
* The num_keys field is used to keep
* track of the number of valid keys.
* In an internal node, the number of valid
* pointers is always num_keys + 1.
* In a leaf, the number of valid pointers
* to data is always num_keys.  The
* last leaf pointer points to the next leaf.
*/
typedef struct node {
	void ** pointers;
	int * keys;
	struct node * parent;
	bool is_leaf;
	int num_keys;
	struct node * next; // Used for queue.
} node;

// GLOBALS.

/* The order determines the maximum and minimum
* number of entries (keys and pointers) in any
* node.  Every node has at most order - 1 keys and
* at least (roughly speaking) half that number.
* Every leaf has as many pointers to data as keys,
* and every internal node has one more pointer
* to a subtree than the number of keys.
* This global variable is initialized to the
* default value.
*/
int order;

/* The queue is used to print the tree in
* level order, starting from the root
* printing each entire rank on a separate
* line, finishing with the leaves.
*/
node * queue;

/* The user can toggle on and off the "verbose"
* property, which causes the pointer addresses
* to be printed out in hexadecimal notation
* next to their corresponding keys.
*/
bool verbose_output;


// FUNCTION PROTOTYPES.

// Output and utility.

void license_notice(void);
void print_license(int licence_part);
void usage_1(void);
void usage_2(void);
void usage_3(void);
int open_db(char *pathname);								// CREATED
off_t find_rootpage(void);									// CREATED
struct general_page create_newpage(void);					// CREATED
int make_findpage(int64_t page_address);					// CREATED


void enqueue(node * new_node);
node * dequeue(void);
int height(node * root);
int path_to_root(node * root, node * child);
void print_leaves(node * root);
void print_tree(node * root);
void find_and_print(node * root, int key, bool verbose);
void find_and_print_range(node * root, int range1, int range2, bool verbose);
int find_range(node * root, int key_start, int key_end, bool verbose,
	int returned_keys[], void * returned_pointers[]);

struct general_page find_leafpage(int64_t key);				// CREATED

char * leafpage_find_key(off_t leafpage_offset, int key);	// CREATED
char * find(int64_t key);									// CREATED

int cut(int length);										// CREATED

// Insertion.

struct record make_record(int key, char * value);			// CREATED
struct general_page insert_into_leafpage(struct general_page leafpage, 
	int64_t key, char *value);								// CREATED
int insert_into_new_rootpage(struct general_page leftpage,
	struct general_page rightpage, int64_t key);			// CREATED
int get_left_index(struct general_page parentpage, off_t leftpage_offset);	// CREATED
int insert_into_internalpage(struct general_page parentpage, int left_index,
	int64_t key, off_t rightpage_offset);					// CREATED
int insert_into_parentpage(struct general_page leftpage,
	int64_t key, struct general_page rightpage);			// CREATED
int insert_into_internalpage_after_splitting(struct general_page oldpage,
	int left_index, int64_t key, off_t rightpage_offset);	// CREATED
int insert_into_leafpage_after_splitting(struct general_page leafpage,
	int64_t key, char * value);								// CREATED
int start_new_tree(int64_t key, char * value);				// CREATED
int insert(int64_t key, char * value);						// CREATED

// Deletion.

int get_neighbor_index(node * n);
node * adjust_root(node * root);
node * coalesce_nodes(node * root, node * n, node * neighbor,
	int neighbor_index, int k_prime);
node * redistribute_nodes(node * root, node * n, node * neighbor,
	int neighbor_index,
	int k_prime_index, int k_prime);
node * delete_entry(node * root, node * n, int key, void * pointer);
node * delete(node * root, int key);

void destroy_tree_nodes(node * root);
node * destroy_tree(node * root);

#endif /* __BPT_H__*/
