#include <stdio.h>
#include <string.h>
#include "auction_manger.h"

AuctionSession auctions[MAX_AUCTIONS];

void init_auctions()
{
    for (int i = 0; i < MAX_AUCTIONS; i++)
    {
        auctions[i].room_id = -1; // Phiên chưa được sử dụng
        auctions[i].auction_state = 0;
        auctions[i].participant_count = 0;
        pthread_mutex_init(&auctions[i].lock, NULL);
    }
}

bool add_auction(int client_socket, int room_id)
{
    for (int i = 0; i < MAX_AUCTIONS; i++)
    {
        if (auctions[i].room_id == -1)
        { // Tìm một slot trống
            auctions[i].room_id = room_id;
            auctions[i].current_item_id = -1;
            auctions[i].current_bid = 0;
            auctions[i].highest_bidder_id = -1;
            auctions[i].remaining_time = 0;
            auctions[i].auction_state = 0;
            auctions[i].participants[0] = client_socket;
            auctions[i].participant_count = 1;
            printf("Auction added: room_id=%d\n", room_id);
            return true;
        }
    }
    printf("Failed to add auction: Max auctions reached.\n");
    return false;
}

void remove_auction(int room_id)
{
    for (int i = 0; i < MAX_AUCTIONS; i++)
    {
        if (auctions[i].room_id == room_id)
        {
            printf("Auction removed: room_id=%d\n", room_id);
            auctions[i].room_id = -1;
            auctions[i].current_item_id = -1;
            auctions[i].current_bid = 0;
            auctions[i].highest_bidder_id = -1;
            auctions[i].remaining_time = 0;
            auctions[i].auction_state = 0;
            auctions[i].participant_count = 0;
            memset(auctions[i].participants, 0, sizeof(auctions[i].participants));
            return;
        }
    }
    printf("Auction not found: room_id=%d\n", room_id);
}

AuctionSession *find_auction_by_room_id(int room_id)
{
    for (int i = 0; i < MAX_AUCTIONS; i++)
    {
        if (auctions[i].room_id == room_id)
        {
            return &auctions[i];
        }
    }
    return NULL;
}

bool update_auction(int room_id, int current_item_id, int current_bid, int highest_bidder_id, int remaining_time)
{
    AuctionSession *auction = find_auction_by_room_id(room_id);
    if (auction)
    {
        pthread_mutex_lock(&auction->lock);
        auction->current_item_id = current_item_id;
        auction->current_bid = current_bid;
        auction->highest_bidder_id = highest_bidder_id;
        auction->remaining_time = remaining_time;
        pthread_mutex_unlock(&auction->lock);
        printf("Auction updated: room_id=%d, current_item_id=%d, current_bid=%d, highest_bidder_id=%d, remaining_time=%d\n",
               room_id, current_item_id, current_bid, highest_bidder_id, remaining_time);
        return true;
    }
    return false;
}

void print_auctions()
{
    printf("Current Auctions:\n");
    for (int i = 0; i < MAX_AUCTIONS; i++)
    {
        if (auctions[i].room_id != -1)
        {
            printf("Room ID: %d, Current Item ID: %d, Current Bid: %d, Highest Bidder ID: %d, Remaining Time: %d, State: %d, Participants: %d\n",
                   auctions[i].room_id, auctions[i].current_item_id, auctions[i].current_bid,
                   auctions[i].highest_bidder_id, auctions[i].remaining_time, auctions[i].auction_state,
                   auctions[i].participant_count);
        }
    }
}


