#ifndef AUCTION_SESSION_H
#define AUCTION_SESSION_H
#define BUFFER_SIZE 100000
#define MAX_ROOMS 100

typedef struct AuctionSession {
    int room_id;                   // ID của phòng đấu giá
    int current_item_id;           // ID của vật phẩm hiện tại
    int current_bid;               // Giá đấu hiện tại
    int highest_bidder_id;         // ID của client đấu giá cao nhất
    int remaining_time;            // Thời gian còn lại cho vật phẩm hiện tại (giây)
    int auction_state;             // Trạng thái (0: Chờ, 1: Đang đấu giá)
    pthread_mutex_t lock;          // Mutex bảo vệ dữ liệu
    int participants[MAX_CLIENTS]; // Danh sách client tham gia
    int participant_count;         // Số lượng client tham gia
} AuctionSession;

AuctionSession sessions[MAX_ROOMS];

void start_auction(AuctionSession *session, int room_id, int item_id, int start_price);
void update_auction_time(AuctionSession *session);



#endif 
