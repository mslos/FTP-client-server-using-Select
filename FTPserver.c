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

#include <dirent.h>

#include <wordexp.h>

#include <fcntl.h>
#include <pwd.h>
#include <sys/sendfile.h>



#define BUFFER_SIZE 500
#define MAX_NUM_OF_CLIENTS 5
#define MAX_NAME_SIZE 10
#define MAX_PASS_SIZE 10
#define NUM_OF_USERS 3
#define FILE_TRANSFER_PORT 7000
#define MAX_PATH_SIZE 500

typedef struct {
	int usrFD;
	char * name;
	char * pass;
	int auth;
	char * current_directory;
	int transFD;
	FILE * incoming_file;
} user;


void set_up_authorized_list(user * usr);

int open_socket(struct sockaddr_in * myaddr, int *port, char * addr, int * sock);

int main (int argc, char ** argv) {

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

	file_port = FILE_TRANSFER_PORT;
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

	struct timeval tv;
	tv.tv_sec = 1;

	while(1) {

		// Select() with error checking
		// Add connection to temp_fd before validating 
		temp_fds = master_fds;
		if( select(connection_fd_range+1, &temp_fds, NULL, NULL, &tv) == -1) {
			perror("Select() failed");
		}
		tv.tv_sec = 1;

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
						close(fd);
					}
					else {
						char command[100]; 
						char params[100]; 
						memset(command,0,sizeof(command));
						memset(params,0,sizeof(params));
						parse_command(command, params, buffer, fd);

						if (strcmp(command, "USER") == 0) {
							user_command(authorized_users, params, fd);
						}

					    else if (strcmp(command, "PASS") == 0) {
					    	pass_command(authorized_users, params, fd);
						}

						else if(strcmp(command, "PUT")==0){
							int j;
							char msg1[] = "File upload request received.\n";
							write(fd, msg1, strlen(msg1) +1);
							for (j = 0; j<NUM_OF_USERS; j++) {
								if(authorized_users[j].usrFD == fd && authorized_users[j].auth == 1) {
									put_command(&file_transfer_sock, &first_connection, &file_transfer_fds, 
										&file_fd_range, &file_transfer_addr, &(authorized_users[j]));
									printf("returned to while loop\n");
									char path [MAX_PATH_SIZE];
									memset(path,0,sizeof(path));
									strcat(path,authorized_users[j].current_directory);
									strcat(path,"/");
									strcat(path,params);
									if ((authorized_users[j].incoming_file = fopen(path,"a"))==NULL)
									{
										perror("Cannot create file");
									}
									memset(command,0,sizeof(command));
									memset(params,0,sizeof(params));
									break;
								}
							}
							if (j == NUM_OF_USERS) {
								char msg5[] = "File upload request: Authenticate first!\n";
								printf("%s",msg5);
								// write(fd, msg5, strlen(msg5) +1);
							}
						}


						else if (strcmp(command, "LS") == 0) {
							list_server_files(authorized_users, params, fd);


							// list_server_files(current_directory, params);
							// change_directory(current_directory, params);
							// user_command(authorized_users, params, fd);
						}
						else if (strcmp(command, "CD") == 0) {
							char tmp_cur[2000];
							int j;
							for (j = 0; j<NUM_OF_USERS; j++) {
								if(authorized_users[j].usrFD == fd) {
									strcpy(tmp_cur, authorized_users[j].current_directory);
									break;
								}

							}

							// User is not authenticated
							if (strcmp(tmp_cur,"")==0) {
								char msg5[] = "Authenticate yourself please\n";
								printf("%s",msg5);
								write(fd, msg5, strlen(msg5) +1);
							}
							change_directory(tmp_cur, params);
							// change_directory(current_directory, params);
							// user_command(authorized_users, params, fd);
						}

						else if(strcmp(command, "GET")==0){
							int j;
							char msg1[] = "File download request received.\n";
							write(fd, msg1, strlen(msg1) +1);
							for (j = 0; j<NUM_OF_USERS; j++) {
								if(authorized_users[j].usrFD == fd && authorized_users[j].auth == 1) {
									char path [MAX_PATH_SIZE];
									memset(path,0,sizeof(path));
									strcat(path,authorized_users[j].current_directory);
									strcat(path,"/");
									strcat(path,params);

									get_command(&file_transfer_sock, &first_connection, &file_transfer_fds, 
										&file_fd_range, &file_transfer_addr, path);
									printf("returned from get_command into main\n");
									memset(command,0,sizeof(command));
									memset(params,0,sizeof(params));
									break;
								}
							}
							if (j == NUM_OF_USERS) {
								char msg5[] = "File download request: Authenticate first!\n";
								printf("%s",msg5);
								// write(fd, msg5, strlen(msg5) +1);
							}

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
			if( select(file_fd_range+1, &temp_file_fds, NULL, NULL, &tv) == -1) {
				perror("Select() failed\n");
			}
			tv.tv_sec = 1;

			// int fd, new_connection;
			// Iterate through all fds
			for(fd=3; fd<= file_fd_range; fd++){

				// If there is data on a connection to read
				if((int)( FD_ISSET(fd, &temp_file_fds) )) {

					// Check for new connections
					if (fd==file_transfer_sock){
					}
					// Event not on listener
					else {
						//memset(buffer,0,BUFFER_SIZE);
						printf("in else\n");
						// I think we shoud read here, need to handshake to anticipate number of bytes in the file
						// open file descriptor (create if not exist), and write into the file
						int num_of_bytes = read(fd, buffer, BUFFER_SIZE);

						printf("Buffer is %s\n", buffer);
						int j;

						if( num_of_bytes < 0)
							perror("Error reading incoming stream\n");
						else if (num_of_bytes == 0) {
							printf("Socket %d closed\n",fd);
							FD_CLR(fd, &file_transfer_fds);
							close(fd);
							for (j = 0; j<NUM_OF_USERS; j++) {
								if(authorized_users[j].transFD == fd) {
									printf("Closing the file transfer\n");
									fclose(authorized_users[j].incoming_file);
								}
							}						
						}
						else { 
							for (j = 0; j<NUM_OF_USERS; j++) {
								if(authorized_users[j].transFD == fd) {
									printf("TransFD found\n");
									fputs(buffer,authorized_users[j].incoming_file);
								}
							}						
						}
						

					}
					printf("After else statement\n");

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
		usr[i].current_directory = malloc(MAX_PATH_SIZE);
		strcpy(usr[i].current_directory, "/home/cs217/Desktop");
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

void accept_connection(int new_connection, int * listener_sock, struct sockaddr_in * client_addr, 
	fd_set master_fds, int connection_fd_range, int * len){
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

void put_command(int * file_transfer_sock, int * first_connection, 
	fd_set * file_transfer_fds, int * file_fd_range, struct sockaddr_in * file_transfer_addr, user * usr){
	int new_connection, len;
	len = sizeof(file_transfer_addr);
	// Listen for clients, max number specified by MAX_NUM_OF_CLIENTS, block until first connection
	if(*first_connection){
		printf("file connection starting\n");
		if ( listen(*file_transfer_sock,MAX_NUM_OF_CLIENTS) < 0)
			perror("Error in listening on file transfer socket\n");
		else{
			printf("First connection\n");
			*first_connection = 0;
		}
			// Add listener into fd set for select
		FD_SET(*file_transfer_sock, file_transfer_fds);

		*file_fd_range = *file_transfer_sock;

	}

	if((new_connection = accept(*file_transfer_sock,(const struct sockaddr *) file_transfer_addr,&len))<0){
		perror("Server cannot accept file transfer connection");
	} 
	else {
		printf("Accepted connection\n");
		usr->transFD = new_connection;
		// Add to master set
		FD_SET(new_connection, file_transfer_fds);
		if (new_connection>*file_fd_range){
			*file_fd_range=new_connection;
			printf("New file transfer connection successfuly added into fd set\n");
		}
	}

}


int change_directory(char * current_directory, char * new_directory){
	char new_path[2000]; 
	if(new_directory[0] == '/') {
		strcpy(new_path,new_directory);
	}
	else if (new_directory[0]=='~') {
	   	wordexp_t p;
	   	wordexp(new_directory, &p, 0);
	    strcpy(new_path,p.we_wordv[0]);
	    wordfree(&p);
	}
	else
		strcat(strcat(strcpy(new_path,current_directory),"/"),new_directory);
		// printf("Trying to resolve %s\n", new_path);
	DIR* dir = opendir(new_path);

	if (dir){
	    // Directory exists.
	    // TODO: Check if the buffer has space for new file name
		// strcat(strcat(current_directory,"/"),new_directory);
		realpath(new_path,current_directory);
		printf("Changed directory to %s\n", new_directory);
	    closedir(dir);
	    return 0;
	}
	else if (ENOENT == errno){
	    //Directory does not exist.
	    printf("Directory does not exist. \n");
	    return 1;
	}
	else {
	    printf("CD failed.\n");
	    return 2;
	}
}

int list_server_files(user * authorized_users, char * path, int fd){
	// char msg1[] = "List command called\n";
	// write(fd, msg1, strlen(msg1) +1);
	printf("List command called\n");
	char tmp_cur[2000];

	// Check if user is authenticated
	int j;
	for (j = 0; j<NUM_OF_USERS; j++) {
		if(authorized_users[j].usrFD == fd) {
			strcpy(tmp_cur, authorized_users[j].current_directory);
			break;
		}

	}

	// User is not authenticated
	if (strcmp(tmp_cur,"")==0) {
		char msg5[] = "Authenticate yourself please\n";
		printf("%s",msg5);
		write(fd, msg5, strlen(msg5) +1);
	}
	else{
		printf("current path%s\n", tmp_cur);

		if(strcmp(path,"")) {
			if(change_directory(tmp_cur, path) != 0) {
				
				char msg2[] = "Error, directory does not exist!\n";
				printf("%s",msg2);
				write(fd, msg2, strlen(msg2) +1);
				return -1;
			}
		}

		DIR *directory_path;
		struct dirent *file_pointer;     
		directory_path = opendir (tmp_cur);

		char files[2000];
		memset(files,0,sizeof(files));


		if (directory_path != NULL){
			while (file_pointer = readdir (directory_path)){
			  	strcat(files,file_pointer->d_name);
				strcat(files,"\n");
			}
			(void) closedir (directory_path);
			
		}
		else{
			char msg5[] = "Couldn't open the directory\n";
			printf("%s",msg5);
			write(fd, msg5, strlen(msg5) +1);
			// perror ("Couldn't open the directory\n");
			return -1;
		}

		write(fd, files, strlen(files) +1);
	}
}


void get_command(int * file_transfer_sock, int * first_connection, 
	fd_set * file_transfer_fds, int * file_fd_range, struct sockaddr_in * file_transfer_addr, char * path) {

	struct stat st; // Information aobut the file
	int new_connection, len;
	len = sizeof(file_transfer_addr);
	int src = open(path, O_RDONLY);
	fstat(src, &st);
	printf("Size of file is %d\n",st.st_size);
	int bytes_sent;

	if(*first_connection){
			printf("file connection starting\n");
			if ( listen(*file_transfer_sock,MAX_NUM_OF_CLIENTS) < 0)
				perror("Error in listening on file transfer socket\n");
			else{
				printf("First connection\n");
				*first_connection = 0;
			}
					// Add listener into fd set for select
		FD_SET(*file_transfer_sock, file_transfer_fds);

		*file_fd_range = *file_transfer_sock;
	}
	
		if((new_connection = accept(*file_transfer_sock,(const struct sockaddr *) file_transfer_addr,&len))<0){
		perror("Server cannot accept file transfer connection");
	} 
	else {

		if( src < 0) {
			printf("Error opening file\n");
			return;
		} // Open the file
		else {
			printf("Opened file successfully\n");
			bytes_sent = sendfile(new_connection,src,NULL,st.st_size);

			if(bytes_sent <= 0) {
				printf("Error, send file failed.\n");
			}
			else if (bytes_sent < st.st_size) {
				printf("Warning: Did not PUT all bytes of the file.\n");
			}

		close(new_connection);
		close(src);
		}
	}
	printf("returned to while loop\n");
}