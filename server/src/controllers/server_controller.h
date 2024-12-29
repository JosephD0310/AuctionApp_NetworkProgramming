#ifndef SERVER_CONTROLLER_H
#define SERVER_CONTROLLER_H
#define BUFFER_SIZE 100000

void handle_login(int client_socket, char buffer[BUFFER_SIZE]);
void handle_register(int client_socket, char buffer[BUFFER_SIZE]);
void handleCreateRoom(int client_socket, char buffer[BUFFER_SIZE]);
void handleDeleteRoom(int sockfd, char buffer[BUFFER_SIZE]);
void handleFetchAllRoom(int client_socket);
void handleFetchOwnRoom(int client_socket);
void handleJoinRoom(int client_socket, int room_id);
void handleExitRoom(int client_socket, int room_id);
void handleStartAuction(int client_socket, int room_id);
void handleFetchItems(int client_socket, char buffer[BUFFER_SIZE]);
void handleCreateItem(int client_socket, char buffer[BUFFER_SIZE]);
void handleDeleteItem(int sockfd, char buffer[BUFFER_SIZE]);
void handleBidRequest(int client_socket, char buffer[BUFFER_SIZE]);

#endif 
