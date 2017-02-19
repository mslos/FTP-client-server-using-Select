//Checkpoint 1
//Hnalde multiple clients with select, accept basic authentication
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 500

int open_socket(struct sockaddr_in * myaddr, int * port, char * addr, int * sock);

int main (int argc, char ** argv) {
	for (int i = 0; i<argc; i++) {
		printf("Argument %d is %s\n", i, argv[i]);
	}
	
	int port, sock_fd, len, client_fd;
	char * ip_addr;
	struct sockaddr_in server_addr;
	char buffer[BUFFER_SIZE];
	bcopy(argv[3], buffer, sizeof(argv[3]));


	//read in port number and address from args
	ip_addr = argv[1];
	port = atoi(argv[2]);

	open_socket(&server_addr, &port, ip_addr, &sock_fd);

	int ret = connect(sock_fd,(const struct sockaddr *) &server_addr, sizeof(server_addr));

	if (ret < 0) {
		perror("Error happened while connecting in client\n");
		return 1;
	}

	printf("Buffer writes %s\n", buffer);
	if( write(sock_fd, buffer, strlen(buffer)+1) < 0)
		perror("Writing failed");

	close(sock_fd);
}

int open_socket(struct sockaddr_in * myaddr, int * port, char * addr, int * sock) {
	*sock = socket(AF_INET, SOCK_STREAM,0);

	if (*sock < 0) {
      printf(stderr,"Error opening socket");
   }

	myaddr->sin_family = AF_INET;
	myaddr->sin_port = htons(*port);
	
	if( inet_aton(addr, &(myaddr->sin_addr))==0 ) {
		fprintf(stderr, "Error, cannot translate IP into binary");
	}

	return 0;
}