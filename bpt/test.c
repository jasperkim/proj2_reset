#include<stdio.h>
#include "bpt.h"


int main( int argc, char ** argv ) {
	char instruction;
	int key;
	printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'f':
				scanf("%d", &key);
				find(key);
