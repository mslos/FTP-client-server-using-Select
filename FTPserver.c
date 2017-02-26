#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <string.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdlib.h>


#define BUFFER_SIZE 500
#define MAX_NUM_OF_CLIENTS 5
#define MAX_NAME_SIZE 10
#define MAX_PASS_SIZE 10
#define NUM_OF_USERS 3
#define FILE_TRANSFER_PORT 2000

typedef struct {
	int usrFD;
	char * name;
	char * pass;
	int auth;
	// char * current_directory;
} user;

void set_up_authorized_list(user * usr);

int open_socket(struct sockaddr_in * myaddr, int *port, char * addr, int * sock);

int main (int argc, char ** argv) {
	for (int i = 0; i<argc; i++) {
		printf("Argument %d is %s\n", i, argv[i]);
	}

	 // TCP protocol, same as opening a file
	int port, listener_sock, file_port, file_transfer_sock, len, client_fd;
	char * ip_addr;
	struct sockaddr_in server_addr, client_addr, file_transfer_addr;
	char buffer[BUFFER_SIZE];
	len = sizeof(client_addr); //necessary since accept requires lvalue - why?

	// Define array of users and initiate
	user authorized_users[NUM_OF_USERS];
	set_up_authorized_list(authorized_users);

	// Select() variables
	fd_set master_fds;
	fd_set temp_fds;
	int connection_fd_range;
	FD_ZERO(&master_fds);
	FD_ZERO(&temp_fds);


	// Read in port number and address from args
	ip_addr = argv[1];
	port = atoi(argv[2]);

	file_port = 7000;
	// // Create socket descriptor
	// open_socket(&server_addr, &port, ip_addr, &listener_sock);
	

	// // Bind socket to port and address
	// if (bind(listener_sock, &server_addr, sizeof(server_addr)) < 0) {
 //      perror("Could not bind socket");
 //   	}

	// Opens Port for clients
	openTCPport(&server_addr, &port, ip_addr, &listener_sock);

	// Open port for file transfer
	openTCPport(&server_addr, &file_port, ip_addr, &file_transfer_sock);


	// Listen for clients, max number specified by MAX_NUM_OF_CLIENTS, block until first connection
	if ( listen(listener_sock,MAX_NUM_OF_CLIENTS) < 0)
		perror("Error in listening on  listener socket");
	

	memset(buffer,0,sizeof(buffer));

	// Add listener into fd set for select
	FD_SET(listener_sock, &master_fds);
	connection_fd_range = listener_sock;

	// Keep track of weather a file connection has been opened
	int first_connection = 1;

	// File descriptor list for file transfer
		// Select() variables
	fd_set file_transfer_fds;
	fd_set temp_file_fds;
	int file_fd_range;
	FD_ZERO(&file_transfer_fds);
	FD_ZERO(&temp_file_fds);

	while(1) {

		// Select() with error checking
		// Add connection to temp_fd before validating 
		temp_fds = master_fds;
		if( select(connection_fd_range+1, &temp_fds, NULL, NULL, NULL) == -1) {
			perror("Select() failed");
		}

		int fd, new_connection;
		// Iterate through all fds
		for(fd=3; fd<= connection_fd_range; fd++){

			// If there is data on a connection to read
			if((int)( FD_ISSET(fd, &temp_fds) )) {

				// Check for new connections
				if (fd==listener_sock){
					// accept_connection(new_connection, listener_sock, client_addr, master_fds, connection_fd_range,len);			
					len = sizeof(client_addr);
					// Accept new connection
					if((new_connection =accept(listener_sock,(const struct sockaddr *) &client_addr,&len))<0){
						perror("Server cannot accept connection\n");
					} else {
						// Add to master set
						FD_SET(new_connection, &master_fds);
						if (new_connection>connection_fd_range){
							connection_fd_range=new_connection;
							printf("New connection successfuly added into fd set\n");
						}
					}
				}
				// Event not on listener
				else {
					memset(buffer,0,BUFFER_SIZE);
					int num_of_bytes = read(fd, buffer, BUFFER_SIZE);

					if( num_of_bytes < 0)
						perror("Error reading incoming stream\n");
					else if (num_of_bytes == 0) {
						printf("Socket %d closed\n",fd);
						FD_CLR(fd, &master_fds);
					}
					else {
						char command[100]; 
						char params[100]; 
						parse_command(command, params, buffer, fd);

						if (strcmp(command, "USER") == 0) {
							user_command(authorized_users, params, fd);
						}

					    else if (strcmp(command, "PASS") == 0) {
					    	pass_command(authorized_users, params, fd);
						}

						else if(strcmp(command, "PUT")==0){
							char msg1[] = "File upload request recieved\n";
							write(fd, msg1, strlen(msg1) +1);
							put_command(&file_transfer_sock, &first_connection, &file_transfer_fds, &file_fd_range);
							printf("returned to while loop\n");
						}
						else {
						  	printf("An invalid FTP command.\n");
						}

					}
					

				}

			}
		}
		/////////////////////////////////////
		// printf("First connection is %d\n",first_connection);
		if(!first_connection){
			printf("in file transfer loop\n");
			// Select for file transfers
			temp_file_fds = file_transfer_fds;
			if( select(file_fd_range+1, &temp_file_fds, NULL, NULL, NULL) == -1) {
				perror("Select() failed");
			}

			// int fd, new_connection;
			// Iterate through all fds
			for(fd=3; fd<= file_fd_range; fd++){

				// If there is data on a connection to read
				if((int)( FD_ISSET(fd, &temp_file_fds) )) {

					// Check for new connections
					if (fd==file_transfer_sock){
						printf("file_transfer_sock");
						// accept_connection(new_connection, listener_sock, client_addr, master_fds, connection_fd_range,len);			
						len = sizeof(file_transfer_addr);
						// Accept new connection
						if((new_connection =accept(file_transfer_sock,(const struct sockaddr *) &file_transfer_addr,&len))<0){
							perror("Server cannot accept file transfer connection");
						} else {
							// Add to master set
							FD_SET(new_connection, &file_transfer_fds);
							if (new_connection>file_fd_range){
								file_fd_range=new_connection;
								printf("New file transfer connection successfuly added into fd set\n");
							}
						}
					}
					// Event not on listener
					else {
						memset(buffer,0,BUFFER_SIZE);
						int num_of_bytes = read(fd, buffer, BUFFER_SIZE);

						if( num_of_bytes < 0)
							perror("Error reading incoming stream");
						else if (num_of_bytes == 0) {
							printf("Socket %d closed\n",fd);
							FD_CLR(fd, &master_fds);
						}
						else {
							printf("Error with something else on listener\n");
							
						}
						

					}

				}
			}
		}
		/////////////////////////////////
	}
}


int open_socket(struct sockaddr_in * myaddr, int * port, char * addr, int * sock) {
	*sock = socket(AF_INET, SOCK_STREAM,0);

	if (*sock < 0) {
      printf(stderr,"Error opening socket\n");
   }

	myaddr->sin_family = AF_INET;
	myaddr->sin_port = htons(*port);
	
	if( inet_aton(addr, &(myaddr->sin_addr))==0 ) {
		fprintf(stderr, "Error, cannot translate IP into binary\n");
	}

	return 0;
}

void openTCPport(struct sockaddr_in * myaddr, int *port, char * ip_addr, int * sock){
	// Create filetransfer socket descriptor
	open_socket(myaddr, port, ip_addr, sock);

	if (bind(*sock, myaddr, sizeof(*myaddr)) < 0) {
	      perror("Could not bind socket");
	}
}

void set_up_authorized_list(user * usr) {
	for (int i = 0; i < NUM_OF_USERS; i++) {
		usr[i].name = (char *) malloc(MAX_NAME_SIZE);
		usr[i].pass = (char *) malloc(MAX_PASS_SIZE);
		usr[i].auth = 0;
		usr[i].usrFD = -1;
	}
	strcpy(usr[0].name, "Nabil");
	strcpy(usr[0].pass, "1234");
	strcpy(usr[1].name, "Brooke");
	strcpy(usr[1].pass, "qwer");
	strcpy(usr[2].name, "Martin");
	strcpy(usr[2].pass, "iluvnet");
	printf("Init auth usesr, test user 1 is %s\n", (usr[0].name) );
}

void parse_command(char *command, char * params, char * buffer, int fd){
	// char command[100]; 
	// char params[100]; 
	printf("Client fd %d, says: %s\n",fd,buffer);
	sscanf(buffer,"%s %s", command , params);
	printf("Command is %s, params are %s\n", command, params);
}


void user_command(user * authorized_users, char * params, int fd){

	printf("USER command activated.\n");
	for (int j = 0; j<NUM_OF_USERS; j++) {

		printf("Username check: %d \n", (authorized_users[j].name));

		if(strcmp(authorized_users[j].name, params) == 0) {
			printf("User found in database.\n");
			authorized_users[j].usrFD = fd;
			char msg1[] = "Username OK, password required\n";
			write(fd, msg1, strlen(msg1) +1);
			break; // Why this break?
		}

		else if (j == NUM_OF_USERS-1) {
			char msg2[] = "Username does not exist\n";
			printf("%s",msg2);
			write(fd, msg2, strlen(msg2) +1);
		}
	}
}

void pass_command(user * authorized_users, char * params, int fd){
	int j;

	for (j = 0; j<NUM_OF_USERS; j++) {
		if(authorized_users[j].usrFD == fd) {
			if(strcmp(authorized_users[j].pass, params) == 0) {
				char msg3[] = "Authentication complete\n";
				printf("%s",msg3);
				write(fd, msg3, strlen(msg3) +1);
				authorized_users[j].auth = 1;
				break;
			}
			else {
				char msg4[] = "Password incorrect, try again!\n";
				printf("%s",msg4);
				write(fd, msg4, strlen(msg4) +1);
				break;
			}

		}

	}

	if (j == NUM_OF_USERS) {
		char msg5[] = "Set USER first\n";
		printf("%s",msg5);
		write(fd, msg5, strlen(msg5) +1);
	}

}

void accept_connection(int new_connection, int * listener_sock, struct sockaddr_in * client_addr, fd_set master_fds, int connection_fd_range, int * len){
	len = sizeof(client_addr);
	// Accept new connection
	if((new_connection =accept(listener_sock,(const struct sockaddr *) &client_addr,&len))<0){
		perror("Server cannot accept connection\n");
	} else {
		// Add to master set
		FD_SET(new_connection, &master_fds);
		if (new_connection>connection_fd_range){
			connection_fd_range=new_connection;
			printf("New connection successfuly added into fd set\n");
		}
	}
}

void put_command(int * file_transfer_sock, int * first_connection, fd_set * file_transfer_fds, int * file_fd_range){
	
	// Listen for clients, max number specified by MAX_NUM_OF_CLIENTS, block until first connection
	if(*first_connection){
		printf("file connection starting\n");
		if ( listen(*file_transfer_sock,MAX_NUM_OF_CLIENTS) < 0)
			perror("Error in listening on file transfer socket\n");
		else{
			printf("First connection\n");
			*first_connection = 0;
		}
	}
	// Add listener into fd set for select
	FD_SET(*file_transfer_sock, file_transfer_fds);

	*file_fd_range = *file_transfer_sock;

}
