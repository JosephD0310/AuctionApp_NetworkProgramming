#ifndef AUCTION_SERVICE_H
#define AUCTION_SERVICE_H

#include <stdbool.h>
#include "room.h"
#include "item.h"

#define BUFFER_SIZE 100000

int handle_create_room(int sockfd, const char *room_name);
int handle_create_item(int sockfd, const char *item_name, int startingPrice, int buyNowPrice, int room_id);
int handle_delete_item(int sockfd, int itemId);
int handle_fetch_all_rooms(int sockfd, Room *rooms);
int handle_fetch_own_rooms(int sockfd, Room *rooms);
int handle_fetch_purchased_item(int sockfd, Item *items);
int handle_fetch_items(int sockfd, int room_id, Item *items);
int handle_join_room(int sockfd, int room_id, Room *room);
int handle_exit_room(int sockfd, int room_id);
void handle_start_auction(int sockfd, int room_id);
void handle_bid_request(int sockfd, int room_id, int bid_amount);
int handle_buy_now(int sockfd, int item_id);

#endif 