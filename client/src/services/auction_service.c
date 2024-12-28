#include "auction_service.h"
#include "session_manager.h"
#include "message_type.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include "room.h"
#include "item.h"

// Client gửi yêu cầu tạo phòng đấu giá
int handle_create_room(int sockfd, const char *room_name)
{

    char buffer[BUFFER_SIZE];
    buffer[0] = CREATE_ROOM;
    memcpy(&buffer[1], room_name, MAX_LENGTH);

    // Gửi dữ liệu qua socket
    if (send(sockfd, buffer, MAX_LENGTH + 1, 0) < 0)
    {
        perror("Failed to send create room request");
        return 0;
    }

    // Nhận phản hồi từ server
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Failed to receive response");
        return -1;
    }
    printf("%d\n", buffer[0]);
    return buffer[0]; // Trả về ID phòng được tạo, hoặc -1 nếu lỗi
}

// Client gửi yêu cầu xoá phòng đấu giá
int handle_delete_room(int sockfd, int item_id)
{
    char buffer[BUFFER_SIZE];

    // Đóng gói dữ liệu
    buffer[0] = DELETE_ROOM;
    memcpy(&buffer[1], &item_id, sizeof(item_id));

    // Gửi dữ liệu qua socket
    if (send(sockfd, buffer, sizeof(item_id) + 1, 0) < 0)
    {
        perror("Failed to send delete room request");
        return -1;
    }

    // Nhận phản hồi từ server
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Failed to receive response");
        return -1;
    }

    return buffer[0]; // Trả về ID vật phẩm được xoá
}

// Client gửi yêu cầu tạo vật phẩm đấu giá
int handle_create_item(int sockfd, const char *item_name, int startingPrice, int buyNowPrice, int room_id)
{
    char buffer[BUFFER_SIZE];
    Item item;
    item.room_id = room_id;
    strncpy(item.item_name, item_name, sizeof(item.item_name));
    item.startingPrice = startingPrice;
    item.buyNowPrice = buyNowPrice;

    // Đóng gói dữ liệu
    buffer[0] = CREATE_ITEM;
    memcpy(&buffer[1], &item, sizeof(item));

    // Gửi dữ liệu qua socket
    if (send(sockfd, buffer, sizeof(item) + 1, 0) < 0)
    {
        perror("Failed to send add item request");
        return -1;
    }

    // Nhận phản hồi từ server
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Failed to receive response");
        return -1;
    }

    return buffer[0];
}

// Client gửi yêu cầu xoá vật phẩm đấu giá
int handle_delete_item(int sockfd, int itemId)
{
    char buffer[BUFFER_SIZE];

    // Đóng gói dữ liệu
    buffer[0] = DELETE_ITEM;
    memcpy(&buffer[1], &itemId, sizeof(itemId));

    // debug
    printf("item ID to delete: %d\n", itemId);

    // Gửi dữ liệu qua socket
    if (send(sockfd, buffer, sizeof(itemId) + 1, 0) < 0)
    {
        perror("Failed to send delete item request");
        return -1;
    }

    // Nhận phản hồi từ server
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Failed to receive response");
        return -1;
    }

    return buffer[0]; // Trả về ID vật phẩm được xoá
}

// Client gửi yêu cầu lấy danh sách tất cả các phòng đấu giá
int handle_fetch_all_rooms(int sockfd, Room *rooms)
{
    char buffer[BUFFER_SIZE];
    buffer[0] = FETCH_ALL_ROOMS;

    // Gửi yêu cầu qua socket
    if (send(sockfd, buffer, 1, 0) < 0)
    {
        perror("Failed to send fetch rooms request");
        return -1;
    }

    // Nhận danh sách phòng từ server
    if (recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Failed to receive response");
        return -1;
    }

    int room_count = buffer[0]; // buffer[0] chứa số lượng phòng đấu giá

    if (room_count <= 0)
    {
        printf("No rooms found.\n");
        return 0; // Không có phòng nào
    }

    memcpy(rooms, &buffer[1], room_count * sizeof(Room));

    return room_count; // Trả về số lượng phòng nhận được
}

// Client gửi yêu cầu lấy danh sách các phòng đấu giá đang sở hữu
int handle_fetch_own_rooms(int sockfd, Room *rooms)
{
    char buffer[BUFFER_SIZE];
    buffer[0] = FETCH_OWN_ROOMS;

    // Gửi yêu cầu qua socket
    if (send(sockfd, buffer, 1, 0) < 0)
    {
        perror("Failed to send fetch rooms request");
        return -1;
    }

    // Nhận danh sách phòng từ server
    if (recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Failed to receive response");
        return -1;
    }

    int room_count = buffer[0]; // Số lượng phòng đấu giá được server trả về
    memcpy(rooms, &buffer[1], room_count * sizeof(Room));
    return room_count;
}

// Lấy danh sách vật phẩm trong phòng đấu giá
int handle_fetch_items(int sockfd, int room_id, Item *items)
{
    char buffer[BUFFER_SIZE];

    // Đóng gói yêu cầu
    buffer[0] = FETCH_ITEMS;
    memcpy(&buffer[1], &room_id, sizeof(room_id));

    // Gửi yêu cầu qua socket
    if (send(sockfd, buffer, 1 + sizeof(int), 0) < 0)
    {
        perror("Failed to send fetch items request");
        return -1;
    }

    // Nhận danh sách vật phẩm từ server
    if (recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Failed to receive response");
        return -1;
    }

    int item_count = buffer[0]; // Số lượng vật phẩm được server trả về
    memcpy(items, &buffer[1], item_count * sizeof(Item));
    return item_count;
}

int handle_join_room(int sockfd, int room_id, Room *room)
{
    char buffer[BUFFER_SIZE];

    // Định dạng thông điệp gửi đi
    buffer[0] = JOIN_ROOM;
    buffer[1] = room_id;

    // Gửi yêu cầu qua socket
    if (send(sockfd, buffer, 2, 0) < 0)
    {
        perror("Failed to send join room request");
        return -1;
    }

    // Nhận danh sách phòng từ server
    if (recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Failed to receive response");
        return -1;
    }

    int role = buffer[0]; // buffer[0] trả về role của client tham gia phòng
    printf("Role : %d\n", role);

    memcpy(room, &buffer[1], sizeof(Room));

    return role;
}

int handle_exit_room(int sockfd, int room_id) {
    char buffer[BUFFER_SIZE];

    // Định dạng thông điệp gửi đi
    buffer[0] = EXIT_ROOM;
    buffer[1] = room_id;

    // Gửi yêu cầu qua socket
    if (send(sockfd, buffer, 2, 0) < 0)
    {
        perror("Failed to send exit room request");
        return -1;
    }

    // Nhận danh sách phòng từ server
    if (recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Failed to receive response");
        return -1;
    }

    return buffer[0];
}

int handle_start_auction(int sockfd, int room_id)
{
    char buffer[BUFFER_SIZE];
    buffer[0] = START_AUCTION;

    // Gửi yêu cầu qua socket
    if (send(sockfd, buffer, 1, 0) < 0)
    {
        perror("Failed to send start auction request");
        return -1;
    }

    // Nhận danh sách phòng từ server
    memset(buffer, 0, BUFFER_SIZE);
    if (recv(sockfd, buffer, BUFFER_SIZE, 0) < 0)
    {
        perror("Failed to receive response");
        return -1;
    }

    return buffer[0];
}