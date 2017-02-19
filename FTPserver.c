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
#include <sys/select.h>
#include <sys/time.h>
#include <stdlib.h>


#define BUFFER_SIZE 500
#define MAX_NUM_OF_CLIENTS 5
#define MAX_NAME_SIZE 10
#define MAX_PASS_SIZE 10
#define NUM_OF_USERS 1

typedef struct {
	int usrFD;
	char * name;
	char * pass;
	int auth;
} user;

void set_up_authorized_list(user * usr);

int open_socket(struct sockaddr_in * myaddr, int *port, char * addr, int * sock);

int main (int argc, char ** argv) {
	for (int i = 0; i<argc; i++) {
		printf("Argument %d is %s\n", i, argv[i]);
	}
	 //tcp protocol,, same as opening a file
	int port, listener_sock, len, client_fd;
	char * ip_addr;
	struct sockaddr_in server_addr, client_addr;
	char buffer[BUFFER_SIZE];
	len = sizeof(client_addr); //necessary since accept requires lvalue

	//define array of users and initiate
	user authorizedUsers[NUM_OF_USERS];
	set_up_authorized_list(authorizedUsers);

	//slect related variables
	fd_set master_fds;
	fd_set temp_fds;
	int max_fd_num;
	FD_ZERO(&master_fds);
	FD_ZERO(&temp_fds);


	//read in port number and address from args
	ip_addr = argv[1];
	port = atoi(argv[2]);

	//create socket descriptor
	open_socket(&server_addr, &port, ip_addr, &listener_sock);
	//bind socket to port and address
	if (bind(listener_sock, &server_addr, sizeof(server_addr)) < 0) {
      perror("Could not bind socket");
   	}

	//listen for clients, max number specified by MAX_NUM_OF_CLIENTS, block until first connection
	if ( listen(listener_sock,MAX_NUM_OF_CLIENTS) < 0)
		perror("Error in listening on  listener socket");
	
	memset(buffer,0,sizeof(buffer));

	//add listener into fd set for select
	FD_SET(listener_sock, &master_fds);
	max_fd_num = listener_sock;


	while(1) {

		//select with error checking
		temp_fds = master_fds;
		if( select(max_fd_num+1, &temp_fds, NULL, NULL, NULL) == -1) {
			perror("Select() failed");
		}
		int fd, new_connection;
		for(fd=3; fd<= max_fd_num; fd++){
			// If there is data on a connection to read
			if((int)( FD_ISSET(fd, &temp_fds) )) {
				// Check for new connections
				if (fd==listener_sock){
					len = sizeof(client_addr);
					// Accept new connection
					if((new_connection =accept(listener_sock,(const struct sockaddr *) &client_addr,&len))<0){
						perror("Server cannot accept connection");
					} else {
						FD_SET(new_connection, &master_fds);
						if (new_connection>max_fd_num){
							max_fd_num=new_connection;
							printf("New connection successfuly added into fd set\n");
						}
					}					
				}
				//event not on listener
				else {
					int num_of_bytes = read(fd, buffer, BUFFER_SIZE);
					if( num_of_bytes < 0)
						perror("Error reading incoming stream");
					else if (num_of_bytes == 0) {
						printf("Socket %d closed\n",fd);
						FD_CLR(fd, &master_fds);
					}
					else {
						char command[100]; 
						char params[100]; 

						printf("Client fd %d, says: %s\n",fd,buffer);
						// char msg1[] = "Username OK, password required\n";
						// write(fd, msg1, strlen(msg1) +1);
						sscanf(buffer,"%s %s", command , params);
						printf("command is %s, params are %s\n", command, params);
						if (strcmp(command, "USER") == 0) {
							printf("USER command activated.");
							for (int j = 0; j<NUM_OF_USERS; j++) {
								printf("Names check: %d", (authorizedUsers[j].name));
								if(strcmp(authorizedUsers[j].name, params) == 0) {
									printf("User found in database.");
									authorizedUsers[j].usrFD = fd;
									char msg1[] = "Username OK, password required\n";
									write(fd, msg1, strlen(msg1) +1);
									break;
								}
							}
							char msg3[] = "Username does not exist\n";
							write(fd, msg3, strlen(msg3) +1);
						}
					    else if (strcmp(command, "PASS") == 0) {
							for (int j = 0; j<NUM_OF_USERS; j++) {
								if(authorizedUsers[j].usrFD == fd) {
									if(strcmp(authorizedUsers[j].pass, params) == 0) {
										char msg2[] = "Authentication complete";
										write(fd, msg2, strlen(msg2) +1);
								}
								}
								
							}
						}
						else {
						  	printf("An invalid FTP command.\n");
						}

					}
					

				}

			}
		}
	}
}

		// // fflush(stdout);
		// client_fd = accept(listener_sock,(const struct sockaddr *) &client_addr,&len);
		// //will block if there are 0 clients

		// int size = read(client_fd, buffer, BUFFER_SIZE);
		// printf("Server read from client: %s\n",buffer);
		// // fflush(stdout);


		//Select server
			//listener will be listening for new connections
			//if new connection alert
				//add to the set of fds
				//remove from the set of fds
				//read from the input
					//process USER - add state
					//process PASS - add state

						



	// }


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

void set_up_authorized_list(user * usr) {
	for (int i = 0; i < NUM_OF_USERS; i++) {
		usr[i].name = (char *) malloc(MAX_NAME_SIZE);
		usr[i].pass = (char *) malloc(MAX_PASS_SIZE);
		usr[i].auth = 0;
		usr[i].usrFD = -1;
	}
	strcpy(usr[0].name, "Nabil");
	strcpy(usr[0].pass, "1234");

	printf("Init auth usesr, test user 1 is %s\n", (usr[0].name) );

	// usr[0].name = "Nabil";
	// usr[0].pass = "1234";
	// usr[1].name = "Martin";
	// usr[1].pass = "nyuad";
	// usr[2].name = "Yasir";
	// usr[2].pass = "1234";
	// usr[3].name = "Brooke";
	// usr[3].pass = "nyuad";
	// usr[4].name = "John";
	// usr[4].pass = "1234";


}