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

    logSystem("SERVER", "LOGIN", "Nhận và xử lý yêu cầu đăng nhập - username: %s", user.username);

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

    logSystem("SERVER", "REGISTER", "Nhận và xử lý yêu cầu đăng ký - username: %s", user.username);

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
    logSystem("SERVER", "CREATE_ROOM", "Nhận và xử lý yêu cầu tạo phòng - username: %s", session->username);

    if (session != NULL)
    {
        int room_id = createRoom(roomName, session->user_id, session->username);
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
    logSystem("SERVER", "FETCH_ALL_ROOM", "Nhận và xử lý yêu cầu lấy danh sách tất cả các phòng hiện có");
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
    logSystem("SERVER", "FETCH_OWN_ROOM", "Nhận và xử lý yêu cầu lấy danh sách các phòng đang sử hữu - username: %s", session->username);
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

void handleJoinRoom(int client_socket, int room_id)
{
    char buffer[BUFFER_SIZE];
    ClientSession *session = find_session_by_socket(client_socket);
    logSystem("SERVER", "JOIN_ROOM", "Nhận và xử lý yêu cầu tham gia phòng %d - username: %s",room_id, session->username);
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
    logSystem("SERVER", "JOIN_ROOM", "Nhận và xử lý yêu cầu rời phòng %d - username: %s",room_id, session->username);
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
    // logSystem("SERVER", "CREAT_ITEM", "Nhận và xử lý yêu cầu tạo vật phẩm - client: %s", client_socket);
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
    logSystem("SERVER", "DELETE_ITEM", "Nhận và xử lý yêu cầu xoá vật phẩm - client: %s", sockfd);
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
        int res1 = updateItemById(auction_session->current_item_id, 0, "Sold");
        if (res1 <= 0)
        {
            printf("Lỗi thủ tục 1\n");
            return NULL;
        }
        int res2 = purchaseItem(auction_session->current_item_id, auction_session->highest_bidder, auction_session->current_bid);
        if (res2 <= 0)
        {
            printf("Lỗi thủ tục 2\n");
            return NULL;
        }
        int res3 = updateMoney(auction_session->highest_bidder_id, auction_session->current_bid, "spend");
        if (res3 <= 0)
        {
            printf("Lỗi thủ tục 3\n");
            return NULL;
        }
        ClientSession *owner_session = find_session_by_socket(auction_session->participants[0]);
        int res4 = updateMoney(owner_session->user_id, auction_session->current_bid, "collect");
        if (res4 <= 0)
        {
            printf("Lỗi thủ tục 4\n");
            return NULL;
        }
        printf("Update item %d - Sold\n", res1);
        sleep(5);
        printf("Check\n");
        handleStartAuction(auction_session->participants[0], auction_session->room_id);
    }

    return NULL;
}

void handleStartAuction(int client_socket, int room_id)
{
    printf("CheckStart\n");
    logSystem("SERVER", "START_AUCTION", "Nhận và xử lý yêu cầu bắt đầu phòng đấu giá %d", room_id);
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
    auction_session->participants[0] = client_socket; // Lưu lại chủ phòng
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
    logSystem("SERVER", "BID", "Nhận và xử lý yêu cầu đấu giá từ client -  %s", session->username);

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

void handleBuyNow(int client_socket, int item_id)
{
    ClientSession *session = find_session_by_socket(client_socket);
    logSystem("SERVER", "BUY_NOW", "Nhận và xử lý yêu cầu mua ngay vật phẩm từ client -  %s", session->username);
    printf("user buy %s - %d\n", session->username, session->user_id);
    Item item;
    Room room;
    int res1 = getItemById(item_id, &item);
    int res2 = getRoomById(item.room_id, &room);
    int res3 = updateItemById(item_id, 0, "Sold");
    int res4 = purchaseItem(item_id, session->username, item.buyNowPrice);
    int res5 = updateMoney(session->user_id, item.buyNowPrice, "spend");
    int res6 = updateMoney(room.user_id, item.buyNowPrice, "collect");
    printf("Update item %d - Sold\n", res1);
    if (res1 <= 0 || res2 <= 0 || res3 <= 0 || res4 <= 0 || res5 <= 0 || res6 <= 0)
    {
        printf("Lỗi thủ tục mua vật phẩm");
        int response = 0; // Error
        send(client_socket, &response, 1, 0);
        return ;
    }

    int response = 1;
    send(client_socket, &response, 1, 0);
    return ;
}