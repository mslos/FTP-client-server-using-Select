#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <wordexp.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <pwd.h>
#include <sys/sendfile.h>
#include <stdio.h>
#include <arpa/inet.h>


#include "FTPclient.h"

#define BUFFER_SIZE 500
#define FILE_TRANSFER_PORT 7000
#define MAX_COMMAND_LEN 20
#define MAX_PARAMETER_LEN 200
#define MAX_INPUT_LEN 250

int main (int argc, char ** argv) {

	int port, sock_fd, len, client_fd, file_transfer_fd, file_port;
	char * ip_addr;
	struct sockaddr_in server_addr, file_transfer_addr;
	char buffer[BUFFER_SIZE];
	memset(buffer,0,BUFFER_SIZE);

	// Read in port number and address from args
	ip_addr = argv[1];
	port = atoi(argv[2]); // Convert string to int 

	openTCP(&server_addr, &port, ip_addr, &sock_fd);


	file_port = FILE_TRANSFER_PORT;

	// Read in commands from client
	char command[MAX_COMMAND_LEN]; 
	char params[MAX_PARAMETER_LEN]; 
	char line[MAX_INPUT_LEN];
	// Initialize current directory
	char current_directory[1000]; //TODO: is this too small
	getcwd(current_directory, sizeof(current_directory));

	while(1) {
		printf("ftp> ");
		// gets(line);
		fgets(line, MAX_INPUT_LEN, stdin);
		memset(command,0,sizeof(command));
		memset(params,0,sizeof(params));
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

		// Upload file to server
		else if (strcmp(command, "PUT") == 0) {
			int src = open(params, O_RDONLY);
			if( src < 0) {
				printf("Error opening file\n");
			} else {
				parse_arg_to_buffer(command, params, sock_fd, buffer);
				char err [] = "File upload request: Authenticate first!";
				if (strcmp(buffer,err) > 10 || strcmp(buffer,err) < -10) {
					put_file(params, &file_transfer_addr, &file_port, ip_addr, &file_transfer_fd, src);
				}
			}
			// memset(command,0,sizeof(command));
		} 

		// Download file to server
		else if (strcmp(command, "GET") == 0) {
			parse_arg_to_buffer(command, params, sock_fd, buffer);
			char err [] = "Error opening file / File not found";
			if (strcmp(buffer,err) > 10 || strcmp(buffer,err) < -10) {
				get_file(current_directory, params, &file_transfer_addr, &file_port, ip_addr, &file_transfer_fd);
			}
		
			// memset(command,0,sizeof(command));
		} 

		// List Files
		else if (strcmp(command, "!LS") == 0) {
			list_client_files(current_directory, params);
		} 

		// Current working path
		else if (strcmp(command, "!PWD") == 0) {
			printf("%s\n",current_directory);
		} 

		// Change directory
		else if (strcmp(command, "!CD") == 0) {
			change_directory(current_directory, params);
		} 

		// Server List Files
		else if (strcmp(command, "LS") == 0) {
			parse_arg_to_buffer(command, params, sock_fd, buffer);
		} 

		// Server change directory
		else if (strcmp(command, "CD") == 0) {
			parse_arg_to_buffer(command, params, sock_fd, buffer);
		} 

		// Server path of working directory
		else if (strcmp(command, "PWD") == 0) {
			parse_arg_to_buffer(command, params, sock_fd, buffer);
		} 

		// Exit
		else if (strcmp(command, "QUIT") == 0) {
			close(sock_fd);
			printf("Goodbye. \n");
			return 0;
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
      perror("Error opening socket\n");
   }

	myaddr->sin_family = AF_INET;
	myaddr->sin_port = htons(*port);
	
	if( inet_aton(addr, &(myaddr->sin_addr))==0 ) {
		perror("Error, cannot translate IP into binary\n");
	}

	return 0;
}


void parse_arg_to_buffer(char * command, char * params, int sock_fd, char * buffer) {
	memset(buffer,0,BUFFER_SIZE);
	int bytes_read = BUFFER_SIZE;
	strcpy(buffer, command);
	strcat(buffer, " ");
	strcat(buffer, params);
	if( write(sock_fd, buffer, strlen(buffer)+1) < 0)
		perror("Writing failed\n");

	memset(buffer,0,BUFFER_SIZE);

	while( (bytes_read == BUFFER_SIZE) ) {
		bytes_read = read(sock_fd, buffer, BUFFER_SIZE);
		if ( bytes_read < 0 ) {
			close(sock_fd);
			perror("Could not read from socket.\n");
		}
		printf("%s",buffer);
		// memset(buffer,0,BUFFER_SIZE);
	}
}



void put_file(char * filename, struct sockaddr_in * server_addr, int * port, char * ip_addr, int * sock_fd, int src){

	
	struct stat st; // Information aobut the file
	

	printf("Opened file successfully\n");
	// Open a new TCP connection
	openTCP(server_addr, port, ip_addr, sock_fd);

	fstat(src, &st);
	printf("Size of file is %d\n",(int)st.st_size);
	int bytes_sent;
	int total_bytes_sent = 0;

	//must send server information about file size
	// read until program 
	// if sock closed, return 0
	//must confirm that we sent complete file (sendfile returns bytes transferred)
	bytes_sent = sendfile(*sock_fd,src,NULL,st.st_size);

	if(bytes_sent < 0) {
		printf("Error, send file failed.\n");
	}
	else if (bytes_sent < st.st_size) {
		printf("Warning: Did not PUT all bytes of the file.\n");
	}

	close(*sock_fd);
	close(src);
}

void get_file(char * cur_dir, char * filename, struct sockaddr_in * server_addr, int * port, char * ip_addr, int * sock_fd) {
	char path[2000];
	strcpy(path,cur_dir);
	strcat(path,"/");
	strcat(path,filename);
	int num_of_bytes = -1;
	char buffer[BUFFER_SIZE];

	memset(buffer,0,BUFFER_SIZE);
	// Open a new TCP connection
	openTCP(server_addr, port, ip_addr, sock_fd);
	write(*sock_fd,"Hello there!", 20);

	FILE * fp;
	fp = fopen (path,"a");
	if (fp==NULL) {
	    perror("Could not open file to write into");
	}
	while(num_of_bytes != 0) {
		num_of_bytes = read(*sock_fd, buffer, BUFFER_SIZE);
		if (num_of_bytes < 0) {
			perror("Problem reading file from the server.");
			break;
		}
		fputs(buffer,fp);
	}
	fclose(fp);
	close(*sock_fd);
}


int list_client_files(char * current_directory, char * path){
	char tmp_cur[2000];
	strcpy(tmp_cur, current_directory);

	if(strcmp(path,"")) {
		if(change_directory(tmp_cur, path) != 0) {
			printf("Error, directory does not exist!\n");
			return 1;
		}
	}


	DIR *directory_path;
	struct dirent *file_pointer;     
	directory_path = opendir (tmp_cur);

	if (directory_path != NULL){
		while (file_pointer = readdir (directory_path))
		  puts (file_pointer->d_name);

		(void) closedir (directory_path);
		return 0;
	}
	else
		perror ("Couldn't open the directory\n");
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


void openTCP(struct sockaddr_in * server_addr, 
	int * port, char * ip_addr, int * sock_fd){
	// printf("Entered openTCP.");
	open_socket(server_addr, port, ip_addr, sock_fd);
	// printf("Seems like connection went through.");

	int ret = connect(*sock_fd,(const struct sockaddr *) server_addr, sizeof(*server_addr));
	if (ret < 0) {
		perror("Error happened while connecting in client\n");
		return;
	}

}