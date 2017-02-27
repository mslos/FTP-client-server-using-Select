#ifndef SERVER_H
#define SERVER_H

typedef struct {
	int usrFD;
	char * name;
	char * pass;
	int auth;
	char * current_directory;
	int transFD;
	FILE * incoming_file;
} user;

void get_command(int * file_transfer_sock, int * first_connection, 
	fd_set * file_transfer_fds, int * file_fd_range, struct sockaddr_in * file_transfer_addr, 
	char * path, int src);
int list_server_files(user * authorized_users, char * path, int fd);
int change_directory(char * current_directory, char * new_directory);
void put_command(int * file_transfer_sock, int * first_connection, 
	fd_set * file_transfer_fds, int * file_fd_range, struct sockaddr_in * file_transfer_addr, user * usr);
void pass_command(user * authorized_users, char * params, int fd);
void user_command(user * authorized_users, char * params, int fd);
void parse_command(char *command, char * params, char * buffer, int fd);
void set_up_authorized_list(user * usr);
void openTCPport(struct sockaddr_in * myaddr, int *port, char * ip_addr, int * sock);
int open_socket(struct sockaddr_in * myaddr, int * port, char * addr, int * sock);


#endif