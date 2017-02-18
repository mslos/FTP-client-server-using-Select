//Checkpoint 1
//Hnalde multiple clients with select, accept basic authentication
#include <stdio.h>

int main (int argc, char ** argv) {
	for (int i = 0; i<argc; i++) {
		printf("Argument %d is %s\n", i, argv[i]);
	}
}