#include <gtk/gtk.h>
#include <glib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "user.h"
#include "message_type.h"
#include "session_manager.h"
#include "auction_manger.h"
#include "server_controller.h"
#include "room.h"
#include "item.h"

void handle_login(int client_socket, char buffer[BUFFER_SIZE])
{
    User user;
    memcpy(&user, &buffer[1], sizeof(user)); // Deserialize dữ liệu từ buffer

    printf("Nhận thông tin đăng nhập từ client: %s - %s\n", user.username, user.password);

    // Lưu thông tin người dùng
    int user_id = authenticateUser(user);
    ClientSession *session = find_session_by_socket(client_socket);

    if (user_id > 0 && session != NULL)
    {
        session->user_id = user_id;
        strncpy(session->username, user.username, sizeof(session->username));
        printf("User %s logged in with user_id %d\n", user.username, user_id);

        User user_data;
        int result = getUserById(user_id, &user_data);
        printf("Username: %s, Password: %d\n", user_data.username, user_data.money);
        if (result == 0)
        {
            int response = 0; // Error
            send(client_socket, &response, 1, 0);
            return;
        }
        // Đóng gói dữ liệu
        buffer[0] = 1;
        memcpy(&buffer[1], &user_data, sizeof(User));

        // Gửi dữ liệu cho client
        if (send(client_socket, buffer, sizeof(User) + 1, 0) < 0)
        {
            perror("Error sending room data");
            return;
        }
    }
    else
    {
        int response = 0; // Đăng nhập thất bại
        send(client_socket, &response, 1, 0);
    }
}

void handle_register(int client_socket, char buffer[BUFFER_SIZE])
{
    User user;
    memcpy(&user, &buffer[1], sizeof(user));

    printf("Nhận thông tin đăng ký từ client: %s - %s\n", user.username, user.password);

    // Lưu thông tin người dùng
    int user_id = saveUser(user);
    ClientSession *session = find_session_by_socket(client_socket);

    if (user_id > 0 && session != NULL)
    {
        session->user_id = user_id;
        strncpy(session->username, user.username, sizeof(session->username));
        printf("User %s register with user_id %d\n", user.username, user_id);

        User user_data;
        int result = getUserById(user_id, &user_data);
        printf("Username: %s, Password: %d\n", user_data.username, user_data.money);
        if (result == 0)
        {
            int response = 0; // Error
            send(client_socket, &response, 1, 0);
            return;
        }
        // Đóng gói dữ liệu
        buffer[0] = 1;
        memcpy(&buffer[1], &user_data, sizeof(User));

        // Gửi dữ liệu cho client
        if (send(client_socket, buffer, sizeof(User) + 1, 0) < 0)
        {
            perror("Error sending room data");
            return;
        }
    }
    else
    {
        int response = 0; // Đăng ký thất bại
        send(client_socket, &response, 1, 0);
    }
}

void handleCreateRoom(int client_socket, char buffer[BUFFER_SIZE])
{
    char roomName[MAX_LENGTH];
    memcpy(roomName, &buffer[1], MAX_LENGTH);
    ClientSession *session = find_session_by_socket(client_socket);

    if (session != NULL)
    {
        int room_id = createRoom(roomName, session->username);
        if (room_id)
        {
            add_auction(client_socket, room_id);
        }

        send(client_socket, &room_id, 1, 0);
    }
    else
    {
        int response = 0;
        send(client_socket, &response, 1, 0);
    }
}

void handleFetchAllRoom(int client_socket)
{
    char buffer[BUFFER_SIZE];
    Room rooms[MAX_ROOMS];
    int room_count = loadRooms(rooms, NULL);

    if (room_count < 0)
    {
        int response = 0;
        send(client_socket, &response, 1, 0);
        return;
    }

    memcpy(&buffer[0], &room_count, 1);
    memcpy(&buffer[1], &rooms, room_count * sizeof(Room));

    if (send(client_socket, buffer, (room_count * sizeof(Room)) + 1, 0) < 0)
    {
        perror("Error sending room data");
        return;
    }
}

void handleFetchOwnRoom(int client_socket)
{
    ClientSession *session = find_session_by_socket(client_socket);

    if (session != NULL)
    {
        char buffer[BUFFER_SIZE];
        Room rooms[MAX_ROOMS];
        int room_count = loadRooms(rooms, session->username);

        if (room_count < 0)
        {
            int response = 0;
            send(client_socket, &response, 1, 0);
            return;
        }

        memcpy(&buffer[0], &room_count, 1);
        memcpy(&buffer[1], &rooms, room_count * sizeof(Room));

        if (send(client_socket, buffer, (room_count * sizeof(Room)) + 1, 0) < 0)
        {
            perror("Error sending room data");
            return;
        }
    }
    else
    {
        int response = 0;
        send(client_socket, &response, 1, 0);
    }
}

// Hàm xóa phòng đấu giá
void handleDeleteRoom(int sockfd, char buffer[BUFFER_SIZE])
{
    int room_id = buffer[1];
    // Xử lý việc xóa phòng
    if (room_id >= 0)
    {
        deleteRoom(room_id); // Gọi hàm deleteRoom để xóa phòng
        send(sockfd, "The auction room has been deleted.", 28, 0);
    }
    else
    {
        send(sockfd, "Invalid room ID.", 24, 0);
    }
}

void handleJoinRoom(int client_socket, int room_id)
{
    char buffer[BUFFER_SIZE];
    ClientSession *session = find_session_by_socket(client_socket);
    Room room;
    int result = getRoomById(room_id, &room);
    printf("%d %s \n", room.room_id, session->username);

    if (result == 0 || session == NULL)
    {
        int response = 0; // Error
        send(client_socket, &response, 1, 0);
        return;
    }

    // Kiểm tra user
    int role = 2;
    if (strcmp(room.username, session->username) == 0)
    {
        role = 1; // Owner
    }
    else
    {
        role = 2; // Joiner
        int updateJoiner = room.numUsers + 1;
        int res = updateRoomById(room_id, updateJoiner, "joiner");
        printf("Update Joiner Room %d - %d\n", res, updateJoiner);
        if (res <= 0)
        {
            int response = 0; // Thất bại
            send(client_socket, &response, 1, 0);
            return;
        }

        AuctionSession *auction_session = find_auction_by_room_id(room_id);
        int i = auction_session->participant_count;
        auction_session->participants[i] = client_socket;
        auction_session->participant_count = i + 1;
    }

    // Đóng gói dữ liệu
    Room updatedRoom;

    if (getRoomById(room_id, &updatedRoom))
    {
        memcpy(&buffer[0], &role, 1);
        memcpy(&buffer[1], &updatedRoom, sizeof(Room));

        // Gửi dữ liệu cho client
        if (send(client_socket, buffer, sizeof(Room) + 1, 0) < 0)
        {
            perror("Error sending room data");
            return;
        }
    }
}

void handleExitRoom(int client_socket, int room_id)
{
    ClientSession *session = find_session_by_socket(client_socket);
    Room room;
    int result = getRoomById(room_id, &room);

    if (result == 0 || session == NULL)
    {
        int response = 0; // Error
        send(client_socket, &response, 1, 0);
        return;
    }

    // Kiểm tra user
    int response = 1;
    if (strcmp(room.username, session->username) != 0)
    {
        int updateJoiner = room.numUsers - 1;
        int res = updateRoomById(room_id, updateJoiner, "joiner");
        printf("Exit Room %d - %d joiner\n", res, updateJoiner);
        if (res <= 0)
        {
            int response = 0; // Thất bại
            send(client_socket, &response, 1, 0);
            return;
        }

        AuctionSession *auction_session = find_auction_by_room_id(room_id);
        int i = auction_session->participant_count;
        auction_session->participants[i] = 0;
        auction_session->participant_count = i - 1;
    }

    // Gửi dữ liệu cho client
    if (send(client_socket, &response, 1, 0) < 0)
    {
        perror("Error sending data");
        return;
    }
}

void handleCreateItem(int client_socket, char buffer[BUFFER_SIZE])
{
    Item item;

    // Kiểm tra nếu buffer đủ dữ liệu
    if (BUFFER_SIZE < sizeof(Item) + 1)
    {
        int response = 0; // Thất bại
        send(client_socket, &response, 1, 0);
        return;
    }
    memcpy(&item, &buffer[1], sizeof(Item));

    int item_id = createItem(item.item_name, item.startingPrice, item.buyNowPrice, item.room_id);
    if (item_id > 0)
    {
        Room room;
        if (getRoomById(item.room_id, &room))
        {
            int updateItem = room.numItems + 1;
            int res = updateRoomById(item.room_id, updateItem, "item");
            printf("Update Item Room %d - %d\n", res, updateItem);
            if (res <= 0)
            {
                int response = 0; // Thất bại
                send(client_socket, &response, 1, 0);
                return;
            }
        }
    }
    send(client_socket, &item_id, 1, 0);
}

void handleFetchItems(int client_socket, char buffer[BUFFER_SIZE])
{
    int room_id;
    memcpy(&room_id, &buffer[1], sizeof(int)); // Deserialize room_id từ buffer

    Item items[MAX_ITEM_IN_ROOM];
    int item_count = loadItems(room_id, items);

    char send_buffer[BUFFER_SIZE];
    memcpy(&send_buffer[0], &item_count, 1);
    memcpy(&send_buffer[1], items, item_count * sizeof(Item));

    if (send(client_socket, send_buffer, 1 + item_count * sizeof(Item), 0) < 0)
    {
        perror("Error sending item data");
    }
}

void handleDeleteItem(int sockfd, char buffer[BUFFER_SIZE])
{
    int item_id = buffer[1];
    int room_id = deleteItem(item_id);
    int response = 1;
    printf("%d\n", room_id);
    if (room_id > 0)
    {
        Room room;
        if (getRoomById(room_id, &room))
        {
            response = 1;
            int updateItem = room.numItems - 1;
            int res = updateRoomById(room_id, updateItem, "item");
            printf("Update Item Room %d - %d\n", res, updateItem);
            if (res <= 0)
            {
                int response = 0; // Thất bại
                send(sockfd, &response, 1, 0);
                return;
            }
        }
    }
    else
    {
        response = 0;
    }

    // Gửi phản hồi đến client
    if (send(sockfd, &response, 1, 0) < 0)
    {
        perror("Failed to send response to client");
    }
}

/////////////////////// AUCTION /////////////////////////

void broadcast_update_auction(AuctionSession *auction_session, int MSG_TYPE)
{
    char buffer[BUFFER_SIZE];
    buffer[0] = MSG_TYPE;
    // Sao chép toàn bộ dữ liệu của auction_session vào buffer[1]
    memcpy(&buffer[1], auction_session, sizeof(AuctionSession));

    // Gửi buffer cho tất cả client
    for (int i = 0; i < auction_session->participant_count; i++)
    {
        int client_socket = auction_session->participants[i];
        if (send(client_socket, buffer, 1 + sizeof(AuctionSession), 0) < 0)
        {
            perror("Error sending auction session data to client");
        }
    }
}

// Hàm đếm ngược thời gian đấu giá
void *countdown_timer(void *arg)
{
    AuctionSession *auction_session = (AuctionSession *)arg;

    while (auction_session->remaining_time > 0 && auction_session->auction_state == 1)
    {
        if (auction_session->reset_timer)
        {
            printf("Reset: %d\n", auction_session->reset_timer);
            auction_session->remaining_time = 30;
            auction_session->reset_timer = 0;
        }
        sleep(1); // Chờ 1 giây
        auction_session->remaining_time--;

        printf("Time: %d\n", auction_session->remaining_time);
    }

    if (auction_session->remaining_time == 0)
    {
        printf("Kết thúc đấu giá\n");
        auction_session->auction_state = 0; // Trạng thái chờ
        broadcast_update_auction(auction_session, RESULT_AUCTION);
        sleep(5);
        handleStartAuction(auction_session->participants[0], auction_session->room_id);
    }

    return NULL;
}

void handleStartAuction(int client_socket, int room_id)
{
    // Lấy phiên đấu giá
    AuctionSession *auction_session = find_auction_by_room_id(room_id);
    Item current_item;
    int result = getFirstAvailableItem(room_id, &current_item);
    if (result == 0)
    {
        int res = updateRoomById(room_id, 0, "Closed");
        printf("Update Room %d - Closed\n", res);
        auction_session->auction_state = 2; // Cập nhật trạng thái của phiên đấu giá: kết thúc
        broadcast_update_auction(auction_session, END_AUCTION);
        return;
    }

    int res = updateRoomById(room_id, 0, "Ongoing");
    printf("Update Room %d - Ongoing\n", res);
    if (res <= 0)
    {
        int response = 0; // Thất bại
        send(client_socket, &response, 1, 0);
        return;
    }

    auction_session->current_item_id = current_item.item_id;
    strncpy(auction_session->current_item_name, current_item.item_name, sizeof(auction_session->current_item_name));
    auction_session->current_bid = current_item.startingPrice;
    auction_session->highest_bidder_id = -1; // Chưa có ai đấu giá
    auction_session->remaining_time = 30;    // Thời gian đấu giá mặc định
    auction_session->auction_state = 1;      // Đang đấu giá

    // Gửi thông tin bắt đầu đấu giá cho tất cả client
    broadcast_update_auction(auction_session, START_AUCTION);

    // Tạo một luồng để quản lý đếm ngược thời gian
    pthread_t timer_thread;
    if (pthread_create(&timer_thread, NULL, countdown_timer, (void *)auction_session) != 0)
    {
        perror("Error creating timer thread");
    }
    pthread_detach(timer_thread); 
    
}

void handleBidRequest(int client_socket, char buffer[BUFFER_SIZE])
{
    printf("Nhận thông điệp đấu giá từ người dùng\n");
    int room_id, bid_amount;
    memcpy(&room_id, buffer + 1, sizeof(int));
    memcpy(&bid_amount, buffer + 1 + sizeof(int), sizeof(int));
    printf("Đấu giá phòng %d - %d\n", room_id, bid_amount);

    AuctionSession *auction_session = find_auction_by_room_id(room_id);
    ClientSession *session = find_session_by_socket(client_socket);
    // Kết thúc đấu giá cho vật phẩm hiện tại
    if (bid_amount >= 5000)
    {
        broadcast_update_auction(auction_session, RESULT_AUCTION);
        int res = updateItemById(auction_session->current_item_id, room_id, 0, "Sold");
        printf("Update item %d - Sold\n", res);
        if (res <= 0)
        {
            printf("Cập nhật vật phẩm đã bán bị lỗi");
            return;
        }
        sleep(5);
        handleStartAuction(client_socket, room_id);
    }

    if (bid_amount > auction_session->current_bid)
    {
        auction_session->current_bid = bid_amount;
        auction_session->highest_bidder_id = session->user_id;
        strncpy(auction_session->highest_bidder, session->username, sizeof(auction_session->highest_bidder));

        // Đặt cờ reset để khởi động lại đếm ngược
        auction_session->reset_timer = 1;
        auction_session->remaining_time = 30;
        // Gửi thông tin cập nhật đến tất cả client
        broadcast_update_auction(auction_session, UPDATE_AUCTION);
    }
}