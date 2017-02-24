#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <string.h>
#include <errno.h>
#include <dirent.h>


#define BUFFER_SIZE 500


int open_socket(struct sockaddr_in * myaddr, int * port, char * addr, int * sock);

void parse_arg_to_buffer(char * command, char * params, int sock_fd, char * buffer);


int main (int argc, char ** argv) {

	// for (int i = 0; i<argc; i++) {
	// 	printf("Argument %d is %s\n", i, argv[i]);
	// }
	
	int port, sock_fd, len, client_fd;
	char * ip_addr;
	struct sockaddr_in server_addr;
	char buffer[BUFFER_SIZE];
	memset(buffer,0,BUFFER_SIZE);

	// Read in port number and address from args
	ip_addr = argv[1];
	port = atoi(argv[2]); // Convert string to int 


	open_socket(&server_addr, &port, ip_addr, &sock_fd);

	int ret = connect(sock_fd,(const struct sockaddr *) &server_addr, sizeof(server_addr));

	if (ret < 0) {
		perror("Error happened while connecting in client\n");
		return 1;
	}

	// Read in commands from client
	char command[100]; 
	char params[100]; 
	char line[200];
	while(1) {
		printf("ftp> ");
		gets(line);
		
		// Parse input on space
		sscanf(line,"%s %s", command, params);
		
		// Username
		if (strcmp(command, "USER") == 0) {
			parse_arg_to_buffer(command, params, sock_fd, buffer);
		} 

		// Password
		else if (strcmp(command, "PASS") == 0) {
			parse_arg_to_buffer(command, params, sock_fd, buffer);
		} 

		// List Files
		if (strcmp(command, "LS") == 0) {
			ls();
		} 


		// Exit
		// TODO: Quit FTP connection?
		else if (strcmp(command, "QUIT") == 0) {
			close(sock_fd);
			printf("Goodbye. \n");
		}

		// Command not found
		else {
		  	printf("Invalid FTP command.\n");
		}
	}
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


void parse_arg_to_buffer(char * command, char * params, int sock_fd, char * buffer){
	memset(buffer,0,BUFFER_SIZE);
	strcpy(buffer, command);
	strcat(buffer, " ");
	strcat(buffer, params);
	if( write(sock_fd, buffer, strlen(buffer)+1) < 0)
		perror("Writing failed");
	memset(buffer,0,BUFFER_SIZE);
	if ( read(sock_fd, buffer, 40) < 0 )
		perror("Could not read from socket.");
	printf("%s",buffer);
}

void ls(){
	DIR *dp;
	struct dirent *ep;     
	dp = opendir ("./");

	if (dp != NULL)
	{
	while (ep = readdir (dp))
	  puts (ep->d_name);

	(void) closedir (dp);
	}
	else
	perror ("Couldn't open the directory");
}