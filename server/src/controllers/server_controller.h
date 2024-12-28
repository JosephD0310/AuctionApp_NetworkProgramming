#ifndef SERVER_CONTROLLER_H
#define SERVER_CONTROLLER_H
#define BUFFER_SIZE 100000

#include "auction_session.h"

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

void start_auction(AuctionSession *session, int room_id, int item_id, int start_price);
void update_auction_time(AuctionSession *session);
void handle_bid_request(int client_id, AuctionSession *session, int bid_amount);

#endif 
