//Checkpoint 1
//Hnalde multiple clients with select, accept basic authentication
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */

#define BUFFER_SIZE 500

int main (int argc, char ** argv) {
	for (int i = 0; i<argc; i++) {
		printf("Argument %d is %s\n", i, argv[i]);
	}
	 //tcp protocol,, same as opening a file
	int port, sock, len, client_fd;
	char * ip_addr;
	struct sockaddr_in server_addr, client_addr;

	char buffer[BUFFER_SIZE];

	open_socket(&server_addr, port, ip_addr, &sock);

	bind(sock, &server_addr, sizeof(server_addr));
	listen(sock,5);
	len = sizeof(client_addr);
	while(1) {
		client_fd = accept(sock,(const struct sockaddr *) &client_addr,&len);
		//will block if there are 0 clients

		int size = read(client_fd, buffer, sizeof(buffer));
		printf("%s",buffer);
	}
}

int open_socket(struct sockaddr_in * myaddr, int port, char * addr, int * sock) {
	*sock = socket(AF_INET, SOCK_STREAM,0);
	myaddr->sin_family = AF_INET;
	myaddr->sin_port = htons(port);
	
	if( inet_aton(addr, &(myaddr->sin_addr.s_addr))==0 )
		fprintf(stderr, "Error, cannot translate IP into binary");
}