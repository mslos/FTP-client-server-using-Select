//Checkpoint 1
//Hnalde multiple clients with select, accept basic authentication
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */



int main (int argc, char ** argv) {
	for (int i = 0; i<argc; i++) {
		printf("Argument %d is %s\n", i, argv[i]);
	}
	 //tcp protocol,, same as opening a file
	int port, sock;
	char * ip_addr;
	struct sockaddr_in server_addr;
	open_socket(&server_addr, port, ip_addr, &sock);

	
}

int open_socket(struct sockaddr_in * myaddr, int port, char * addr, int * sock) {
	*sock = socket(AF_INET, SOCK_STREAM,0);
	myaddr->sin_family = AF_INET;
	myaddr->sin_port = htons(port);
	
	if( inet_aton(addr, &(myaddr->sin_addr.s_addr))==0 )
		fprintf(stderr, "Error, cannot translate IP into binary");
}