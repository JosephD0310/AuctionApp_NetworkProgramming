#include <gtk/gtk.h>
#include <sys/socket.h>
#include <string.h>
#include "user.h"
#include "message_type.h"
#include "session_manager.h"
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
        int response = createRoom(roomName, session->username);
        send(client_socket, &response, 1, 0);
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
    }

    // Gửi dữ liệu cho client
    if (send(client_socket, &response, 1, 0) < 0)
    {
        perror("Error sending data");
        return;
    }
}

void handleStartAuction(int client_socket, int room_id)
{
    Room room;
    int result = getRoomById(room_id, &room);

    if (result == 0)
    {
        int response = 0; // Error
        send(client_socket, &response, 1, 0);
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
    int response = 1;

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

void start_auction(AuctionSession *session, int room_id, int item_id, int start_price) {
    pthread_mutex_lock(&session->lock);
    session->room_id = room_id;
    session->current_item_id = item_id;
    session->current_bid = start_price;
    session->highest_bidder_id = -1; // Chưa có ai đấu giá
    session->remaining_time = 30;   // Thời gian đấu giá mặc định
    session->auction_state = 1;     // Đang đấu giá
    pthread_mutex_unlock(&session->lock);

    broadcast_to_clients(session, "START_AUCTION", room_id, item_id, start_price);
}

void update_auction_time(AuctionSession *session) {
    pthread_mutex_lock(&session->lock);
    if (session->remaining_time > 0) {
        session->remaining_time--;
        broadcast_to_clients(session, "TIME_UPDATE", session->remaining_time);
    } else if (session->auction_state == 1) {
        session->auction_state = 0; // Kết thúc đấu giá
        broadcast_to_clients(session, "AUCTION_END", session->highest_bidder_id, session->current_bid);
    }
    pthread_mutex_unlock(&session->lock);
}

void handle_bid_request(int client_id, AuctionSession *session, int bid_amount) {
    pthread_mutex_lock(&session->lock);

    if (session->auction_state == 1 && bid_amount > session->current_bid) {
        session->current_bid = bid_amount;
        session->highest_bidder_id = client_id;

        // Thông báo giá mới tới các client
        broadcast_to_clients(session, "NEW_BID", session->current_item_id, bid_amount, client_id);
    } else {
        send_to_client(client_id, "BID_REJECTED", session->current_bid);
    }

    pthread_mutex_unlock(&session->lock);
}