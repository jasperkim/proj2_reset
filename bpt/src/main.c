#include "bpt.h"
#include "page.h"
// MAIN

int main(int argc, char ** argv) {

	int db_file, insertion_check;
	char * input_file;
	char newfile[100];
	char record_value[120];
	char *newfile_str;
	char *recval_str;
	FILE * fp;
	node * root;
	header_page headerpage;
	general_page in_use_page;
	int input, range2;
	char instruction;
	char license_part;

	// page notice
	header_page HP;
	general_page GP;
	page_header PH;


	root = NULL;
	verbose_output = false;

	if (argc > 1) {
		order = atoi(argv[1]);
		if (order < MIN_ORDER || order > MAX_ORDER) {
			fprintf(stderr, "Invalid order: %d .\n\n", order);
			usage_3();
			exit(EXIT_FAILURE);
		}
	}

	license_notice();
	usage_1();
	usage_2();

	if (argc > 2) {
		input_file = argv[2];
		fp = fopen(input_file, "r");
		if (fp == NULL) {
			perror("Failure  open input file.");
			exit(EXIT_FAILURE);
		}
		while (!feof(fp)) {
			fscanf(fp, "%d\n", &input);
			root = insert(root, input, input);
		}
		fclose(fp);
		print_tree(root);
	}

	printf("size of Header_page :%d\nsize of General_page :%d\nsize of Page_header :%d\n",
		sizeof(HP), sizeof(GP), sizeof(PH));

	printf("> ");
	while (scanf("%c", &instruction) != EOF) {
		switch (instruction) {
		case 'o':
			scanf("%s", newfile);
			//			getchar();
			//			fgets(newfile, sizeof(newfile), stdin);	// remodify
			//			newfile[strlen(newfile)-1]='\0';
			//			newfile_str=newfile;
			db_file = open_db(newfile);	//CREATED
			if (db_file == 0)
				printf("Open_db succeeded\n");
			break;
		case 'd':
			scanf("%d", &input);
			root = delete(root, input);
			print_tree(root);
			break;
		case 'i':
			scanf("%d", &input);
			getchar();
			fgets(record_value, sizeof(record_value), stdin);
			record_value[strlen(record_value) - 1] = '\0';
			recval_str = record_value;
			//			insertion_check = insert(input, recval_str); // prototype insert
			// int insert (int key, char * value);
			//			print_tree(root); 	// modify
			break;


		case 'f':
			break;


		case 'p':
			//            scanf("%d", &input);
			//            find_and_print(root, input, instruction == 'p');
			break;
		case 'r':
			scanf("%d %d", &input, &range2);
			if (input > range2) {
				int tmp = range2;
				range2 = input;
				input = tmp;
			}
			find_and_print_range(root, input, range2, instruction == 'p');
			break;
		case 'l':
			print_leaves(root);
			break;
		case 'q':
			while (getchar() != (int)'\n');
			return EXIT_SUCCESS;
			break;
		case 't':
			print_tree(root);
			break;
		case 'v':
			verbose_output = !verbose_output;
			break;
		case 'x':
			if (root)
				root = destroy_tree(root);
			print_tree(root);
			break;
		default:
			usage_2();
			break;
		}
		while (getchar() != (int)'\n');
		printf("> ");
	}
	printf("\n");

	return EXIT_SUCCESS;
}
