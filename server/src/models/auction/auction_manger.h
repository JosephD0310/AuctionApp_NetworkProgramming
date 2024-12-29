#ifndef AUCTION_MANAGER_H
#define AUCTION_MANAGER_H

#include <stdbool.h>
#include <pthread.h>
#include <glib.h>

#define MAX_AUCTIONS 100
#define MAX_CLIENTS_IN_AUCTION 50

typedef struct AuctionSession {
    int room_id;                   // ID của phòng đấu giá
    int current_item_id;           // ID của vật phẩm hiện tại
    char current_item_name[20];         // Tên của vật phẩm hiện tại
    int current_bid;               // Giá đấu hiện tại
    int highest_bidder_id;         // ID của client đấu giá cao nhất
    char highest_bidder[20];            // Client đấu giá cao nhất
    int remaining_time;            // Thời gian còn lại (giây)
    int auction_state;             // Trạng thái (0: Chờ, 1: Đang đấu giá)
    pthread_mutex_t lock;          // Mutex bảo vệ dữ liệu
    int reset_timer;   // 1: cần reset timer, 0: không cần
    int participants[MAX_CLIENTS_IN_AUCTION]; // Danh sách client tham gia
    int participant_count;         // Số lượng client tham gia
    guint timeout_id;              // Lưu id callback countdown
} AuctionSession;

// Khởi tạo danh sách phiên đấu giá
void init_auctions();

// Thêm một phiên đấu giá
bool add_auction(int client_socket, int room_id);

// Xóa một phiên đấu giá
void remove_auction(int room_id);

// Tìm phiên đấu giá theo room_id
AuctionSession *find_auction_by_room_id(int room_id);

// Cập nhật thông tin của phiên đấu giá
bool update_auction(int room_id, int current_item_id, int current_bid, int highest_bidder_id, int remaining_time);

// In thông tin tất cả các phiên đấu giá
void print_auctions();


#endif
