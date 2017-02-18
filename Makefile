CC = gcc
OBJ = ./obj
CLIENT = FTPclient
SERVER = FTPserver

target: $(OBJ) $(OBJ)/$(CLIENT) $(OBJ)/$(SERVER)

$(OBJ)/$(CLIENT): $(OBJ)/$(CLIENT).o
	@echo link $(CLIENT).o
	$(CC) $< -o $@

$(OBJ)/$(SERVER): $(OBJ)/$(SERVER).o
	@echo link $(SERVER).c
	$(CC) $< -o $@

$(OBJ)/%.o: %.c
	@echo compile
	$(CC) -c $< -o $@

$(OBJ):
	mkdir $@

clean:
	rm $(CLIENT) $(SERVER) $(CLIENT).o $(SERVER).o
