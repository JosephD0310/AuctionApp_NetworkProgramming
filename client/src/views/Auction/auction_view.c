#include <gtk/gtk.h>
#include <glib.h>
#include <sys/socket.h>
#include "style_manager.h"
#include "auction_view.h"
#include "auction_service.h"
#include "auction_manger.h"
#include "message_type.h"
#include "room.h"
#include "item.h"

typedef struct
{
    int sockfd;
    int role;
    int room_id;
    int item_id;
    GtkWidget *home_window;
    GtkWidget *label_room_joiner;
    GtkWidget *auction_window;
    GtkStack *auction_stack;
    GtkWidget *label_waiting;
    GtkWidget *label_countdown;

    int current_bid;
    int time_left;
    int reset_timer;
    guint timeout_id;
    GtkWidget *current_user;
    GtkWidget *highest_price;
    GtkWidget *auction_item_name;
    GtkWidget *auction_time;
    GtkWidget *start_price;
    GtkWidget *current_price;
    GtkWidget *btn_bid1;
    GtkWidget *btn_bid2;
    GtkWidget *btn_bid3;
    GtkWidget *btn_bid4;
    int bid1, bid2, bid3, bid4;

    GtkWidget *add_item_form;
    GtkWidget *item_name;
    GtkWidget *item_starting_price;
    GtkWidget *item_buynow_price;
    GtkWidget *item_auction_time;

    GtkWidget *add_item_msg;
    GtkWidget *item_card;
    GtkListBox *item_list;

} AuctionContext;

typedef struct
{
    int sockfd;
    int room_id;
    Item item;
    AuctionContext *auctionContext;
} ItemContext;

void on_auction_window_destroy(GtkWidget *widget, gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;
    if (handle_exit_room(context->sockfd, context->room_id))
    {
        gtk_widget_show(context->home_window);
        printf("Thoát\n");
    }
}

void clear_item_children(GtkListBox *listbox)
{
    GList *children = gtk_container_get_children(GTK_CONTAINER(listbox));
    for (GList *iter = children; iter != NULL; iter = iter->next)
    {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
}

GtkWidget *add_item_card(Item item, gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;

    GtkBuilder *builder;
    GtkWidget *card;
    GError *error = NULL;

    builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(builder, "client/src/views/Home/custom_card.glade", &error))
    {
        g_printerr("Error loading file: %s\n", error->message);
        g_clear_error(&error);
        return NULL;
    }
    ItemContext *itemContext = g_malloc(sizeof(ItemContext));
    itemContext->sockfd = context->sockfd;
    itemContext->room_id = context->room_id;
    itemContext->item = item;
    itemContext->auctionContext = context;

    // gtk_builder_connect_signals(builder, &item);
    gtk_builder_connect_signals(builder, itemContext);

    card = GTK_WIDGET(gtk_builder_get_object(builder, "item_card"));

    GtkWidget *parent = gtk_widget_get_parent(card);
    if (parent != NULL)
    {
        gtk_container_remove(GTK_CONTAINER(parent), card);
    }

    // Tìm và cập nhật nội dung trong card
    GtkWidget *item_name = GTK_WIDGET(gtk_builder_get_object(builder, "item_name"));
    GtkWidget *status = GTK_WIDGET(gtk_builder_get_object(builder, "status"));
    GtkWidget *buy_button = GTK_WIDGET(gtk_builder_get_object(builder, "buy_button"));
    GtkWidget *delete_button = GTK_WIDGET(gtk_builder_get_object(builder, "delete_button"));

    if (GTK_IS_LABEL(item_name))
    {
        gtk_label_set_text(GTK_LABEL(item_name), item.item_name);
    }

    if (GTK_IS_LABEL(status))
    {
        gtk_label_set_text(GTK_LABEL(status), item.status);
    }

    gtk_widget_show_all(card);

    if (context->role == 2)
    {
        gtk_widget_hide(delete_button);
    }
    if (context->role == 1)
    {
        gtk_widget_hide(buy_button);
    }

    return card;
}

void fetch_item(gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;

    // Tạo mảng Item để lưu danh sách item nhận được
    Item items[MAX_ITEM_IN_ROOM];
    int count = handle_fetch_items(context->sockfd, context->room_id, items);

    if (count < 0)
    {
        g_printerr("Failed to fetch item list from server\n");
        return;
    }

    // Duyệt qua danh sách item và thêm vào GTK_LIST_BOX
    for (int i = 0; i < count; i++)
    {
        GtkWidget *item_card = add_item_card(items[i], context);
        gtk_list_box_insert(context->item_list, item_card, -1);
    }

    // Hiển thị các widget vừa thêm
    // gtk_widget_show_all(GTK_WIDGET(context->item_list));
}

void on_delete_item(GtkWidget *button, gpointer user_data)
{
    // Item *item = (Item *)user_data;
    ItemContext *context = (ItemContext *)user_data;
    g_print("Delete button clicked\n");

    int result = handle_delete_item(context->sockfd, context->item.item_id);
    if (result > 0)
    {
        // Nếu xóa thành công, cập nhật lại danh sách item
        clear_item_children(GTK_LIST_BOX(context->auctionContext->item_list));
        fetch_item(context->auctionContext);
    }
}

////////////////// LIST ITEM /////////////////
void show_add_item_form(GtkWidget *button, gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;

    GtkWidget *form = GTK_WIDGET(context->add_item_form);
    gtk_widget_show(form);
}

/// BUTTON
void on_add_item_ok(GtkWidget *button, gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;

    GtkWidget *form = GTK_WIDGET(context->add_item_form);
    const gchar *item_name = gtk_entry_get_text(GTK_ENTRY(context->item_name));
    const gchar *item_starting_price_str = gtk_entry_get_text(GTK_ENTRY(context->item_starting_price));
    const gchar *item_buynow_price_str = gtk_entry_get_text(GTK_ENTRY(context->item_buynow_price));
    int item_buynow_price = atoi(item_buynow_price_str);
    int item_starting_price = atoi(item_starting_price_str);
    int item_id = handle_create_item(context->sockfd, item_name, item_starting_price, item_buynow_price, context->room_id);
    printf("%d\n", item_id);
    if (item_id > 0)
    {
        context->item_id = item_id;
        gtk_widget_hide(form);
        clear_item_children(GTK_LIST_BOX(context->item_list));
        fetch_item(context);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(context->add_item_msg), "Error");
    }
}

void on_add_item_cancel(GtkWidget *button, gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;

    GtkWidget *form = GTK_WIDGET(context->add_item_form);
    gtk_widget_hide(form);
}

////////////////// AUCTION //////////////////

void stop_countdown(AuctionContext *context)
{
    // Hủy callback hiện tại nếu có
    if (context->timeout_id != 0)
    {
        g_source_remove(context->timeout_id);
        context->timeout_id = 0;
    }
}

gboolean countdown_timer(gpointer data)
{
    AuctionContext *context = (AuctionContext *)data;

    if (context->reset_timer)
    {
        printf("Reset: %d\n", context->reset_timer);

        context->time_left = 30;  // Reset lại thời gian
        context->reset_timer = 0; // Tắt cờ reset
    }

    if (context->time_left > 0)
    {
        context->time_left--;
        printf("Time: %d\n", context->time_left);

        char label[32];
        snprintf(label, sizeof(label), "%d", context->time_left);
        gtk_label_set_text(GTK_LABEL(context->auction_time), label);
    }

    gboolean should_continue = context->time_left > 0;
    if (!should_continue)
    {
        stop_countdown(context);
    }

    return should_continue;
}

void start_countdown(AuctionContext *context, int time_left)
{
    // Hủy callback hiện tại nếu có
    if (context->timeout_id != 0)
    {
        g_source_remove(context->timeout_id);
        context->timeout_id = 0;
    }
    // Đặt thời gian đếm ngược mới
    context->time_left = time_left;

    // Đăng ký callback mới
    context->timeout_id = g_timeout_add(1000, countdown_timer, context);
}


// Hàm tính toán bước giá
void calculate_bid_steps(int item_price, int *step1, int *step2, int *step3, int *step4)
{
    int step = item_price * 0.02; // 2% của giá vật phẩm
    *step1 = step;
    *step2 = step * 2;
    *step3 = step * 3;
    *step4 = step * 4;
}

void update_bid_steps(int item_price, AuctionContext *context)
{
    calculate_bid_steps(item_price, &context->bid1, &context->bid2, &context->bid3, &context->bid4);

    char label1[50], label2[50], label3[50], label4[50];
    sprintf(label1, "%d", context->bid1);
    sprintf(label2, "%d", context->bid2);
    sprintf(label3, "%d", context->bid3);
    sprintf(label4, "%d", context->bid4);

    gtk_button_set_label(GTK_BUTTON(context->btn_bid1), label1);
    gtk_button_set_label(GTK_BUTTON(context->btn_bid2), label2);
    gtk_button_set_label(GTK_BUTTON(context->btn_bid3), label3);
    gtk_button_set_label(GTK_BUTTON(context->btn_bid4), label4);
}

void handle_start_msg(char buffer[BUFFER_SIZE], AuctionContext *context)
{
    AuctionSession auction_session;
    memcpy(&auction_session, &buffer[1], sizeof(AuctionSession));
    context->reset_timer = 0;
    gtk_stack_set_visible_child_name(context->auction_stack, "opening");

    printf("Giá khởi điểm %d\n", auction_session.current_bid);
    context->current_bid = auction_session.current_bid;
    update_bid_steps(auction_session.current_bid, context);
    gtk_label_set_text(GTK_LABEL(context->auction_item_name), auction_session.current_item_name);
    char label[32];
    snprintf(label, sizeof(label), "%d", auction_session.current_bid);
    gtk_label_set_text(GTK_LABEL(context->start_price), label);
    gtk_label_set_text(GTK_LABEL(context->current_price), label);
    gtk_label_set_text(GTK_LABEL(context->highest_price), label);

    char label2[32];
    snprintf(label2, sizeof(label2), "%d", auction_session.remaining_time);
    gtk_label_set_text(GTK_LABEL(context->auction_time), label2);

    start_countdown(context, auction_session.remaining_time);
}

void handle_update_msg(char buffer[BUFFER_SIZE], AuctionContext *context)
{
    AuctionSession auction_session;
    memcpy(&auction_session, &buffer[1], sizeof(AuctionSession));
    context->current_bid = auction_session.current_bid;
    context->reset_timer = 1;
    start_countdown(context, auction_session.remaining_time);

    char label1[32];
    snprintf(label1, sizeof(label1), "%d", auction_session.current_bid);
    gtk_label_set_text(GTK_LABEL(context->current_price), label1);
    gtk_label_set_text(GTK_LABEL(context->highest_price), label1);
    gtk_label_set_text(GTK_LABEL(context->current_user), auction_session.highest_bidder);

    char label2[32];
    snprintf(label2, sizeof(label2), "%d", auction_session.remaining_time);
    gtk_label_set_text(GTK_LABEL(context->auction_time), label2);
    return;
}

void handle_result_msg(char buffer[BUFFER_SIZE], AuctionContext *context)
{
    AuctionSession auction_session;
    memcpy(&auction_session, &buffer[1], sizeof(AuctionSession));
    gtk_stack_set_visible_child_name(context->auction_stack, "waiting");
    char label[50];
    snprintf(label, sizeof(label), "Đấu giá thành công - %s", auction_session.highest_bidder);
    gtk_label_set_text(GTK_LABEL(context->label_waiting), label);
    gtk_label_set_text(GTK_LABEL(context->label_countdown), "Chờ 5s - Đấu giá vật phẩm tiếp theo ");
    stop_countdown(context);
    return;
}

void handle_end_msg(char buffer[BUFFER_SIZE], AuctionContext *context)
{
    gtk_stack_set_visible_child_name(context->auction_stack, "waiting");
    gtk_label_set_text(GTK_LABEL(context->label_waiting), "Phiên đấu giá đã kế thúc");
    gtk_label_set_text(GTK_LABEL(context->label_countdown), "");
    return;
}

void on_btn_start_clicked(GtkWidget *button, gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;

    handle_start_auction(context->sockfd, context->room_id);
}

// Xử lý sự kiện click các bước giá
void on_bid_1(GtkWidget *button, gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;
    int bit_amount = context->bid1 + context->current_bid;
    handle_bid_request(context->sockfd, context->room_id, bit_amount);
    printf("You selected bid: %d\n", bit_amount);

}

void on_bid_2(GtkWidget *button, gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;
    int bit_amount = context->bid2 + context->current_bid;
    handle_bid_request(context->sockfd, context->room_id, bit_amount);
    printf("You selected bid: %d\n", bit_amount);
}

void on_bid_3(GtkWidget *button, gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;
    int bit_amount = context->bid3 + context->current_bid;
    handle_bid_request(context->sockfd, context->room_id, bit_amount);
    printf("You selected bid: %d\n", bit_amount);
}

void on_bid_4(GtkWidget *button, gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;
    int bit_amount = context->bid4 + context->current_bid;
    handle_bid_request(context->sockfd, context->room_id, bit_amount);
    printf("You selected bid: %d\n", bit_amount);
}

////////////////// ////////////////// //////////////////

void reload_auction_view(gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;

    // Cập nhập số người tham gia phòng
    if (GTK_IS_LABEL(context->label_room_joiner))
    {
        Room room;

        int role = handle_join_room(context->sockfd, context->room_id, &room);

        if (role <= 0)
        {
            g_printerr("Failed to update room's joiner");
            return;
        }
        char joiner_count_text[32];
        snprintf(joiner_count_text, sizeof(joiner_count_text), "%d", room.numUsers);
        gtk_label_set_text(GTK_LABEL(context->label_room_joiner), joiner_count_text);
    }
    // Xóa nội dung cũ
    clear_item_children(context->item_list);

    // Nạp lại dữ liệu
    fetch_item(context);
}

gboolean on_socket_event_auction(GIOChannel *source, GIOCondition condition, gpointer user_data)
{
    AuctionContext *context = (AuctionContext *)user_data;

    if (context->auction_window && gtk_widget_get_visible(context->auction_window))
    {
        if (condition & G_IO_IN) // Có dữ liệu từ server
        {
            int sockfd = g_io_channel_unix_get_fd(source);
            char buffer[BUFFER_SIZE];
            memset(buffer, 0, BUFFER_SIZE);

            int bytes_received = recv(sockfd, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0)
            {
                printf("Server disconnected.\n");
                return FALSE; // Dừng theo dõi socket
            }

            // Xử lý thông điệp từ server
            char message_type = buffer[0];
            switch (message_type)
            {
            case REFRESH:
                printf("Server yêu cầu refresh dữ liệu.\n");
                reload_auction_view(context);
                break;
            case START_AUCTION:
                printf("Bắt đầu phiên đấu giá.\n");
                handle_start_msg(buffer, context);
                break;
            case UPDATE_AUCTION:
                printf("Cập nhật thông tin phiên đấu giá.\n");
                handle_update_msg(buffer, context);
                break;
            case RESULT_AUCTION:
                printf("Kết quả đấu giá.\n");
                handle_result_msg(buffer, context);
                break;
            case END_AUCTION:
                printf("Kết thúc phiên đấu giá.\n");
                handle_end_msg(buffer, context);
                break;
            default:
                printf("Received unknown message type: %d\n", message_type);
                break;
            }
        }
    }
    return TRUE; // Tiếp tục theo dõi socket
}

////////////////// ////////////////// //////////////////

void init_auction_view(int sockfd, GtkWidget *home_window, Room room, int role)
{
    AuctionContext *auctionContext = g_malloc(sizeof(AuctionContext));
    auctionContext->sockfd = sockfd;
    auctionContext->home_window = home_window;
    auctionContext->role = role;
    auctionContext->room_id = room.room_id;

    GtkBuilder *builder;
    GtkWidget *window;
    GError *error = NULL;

    builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(builder, "client/src/views/Auction/auction_view.glade", &error))
    {
        g_printerr("Error loading file: %s\n", error->message);
        g_clear_error(&error);
        return;
    }

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window_auction"));
    g_signal_connect(window, "destroy", G_CALLBACK(on_auction_window_destroy), auctionContext);

    GtkWidget *label_room_name = GTK_WIDGET(gtk_builder_get_object(builder, "label_room_name"));
    GtkWidget *label_room_owner = GTK_WIDGET(gtk_builder_get_object(builder, "label_room_owner"));
    GtkWidget *add_button = GTK_WIDGET(gtk_builder_get_object(builder, "add_button"));
    GtkWidget *start_button = GTK_WIDGET(gtk_builder_get_object(builder, "start_button"));
    auctionContext->current_user = GTK_WIDGET(gtk_builder_get_object(builder, "current_user"));
    auctionContext->highest_price = GTK_WIDGET(gtk_builder_get_object(builder, "highest_price"));
    auctionContext->auction_item_name = GTK_WIDGET(gtk_builder_get_object(builder, "auction_item_name"));
    auctionContext->auction_time = GTK_WIDGET(gtk_builder_get_object(builder, "auction_time"));
    auctionContext->start_price = GTK_WIDGET(gtk_builder_get_object(builder, "start_price"));
    auctionContext->current_price = GTK_WIDGET(gtk_builder_get_object(builder, "current_price"));
    auctionContext->btn_bid1 = GTK_WIDGET(gtk_builder_get_object(builder, "btn_bid1"));
    auctionContext->btn_bid2 = GTK_WIDGET(gtk_builder_get_object(builder, "btn_bid2"));
    auctionContext->btn_bid3 = GTK_WIDGET(gtk_builder_get_object(builder, "btn_bid3"));
    auctionContext->btn_bid4 = GTK_WIDGET(gtk_builder_get_object(builder, "btn_bid4"));
    auctionContext->label_room_joiner = GTK_WIDGET(gtk_builder_get_object(builder, "label_room_joiner"));
    auctionContext->auction_stack = GTK_STACK(gtk_builder_get_object(builder, "auction_stack"));
    auctionContext->label_waiting = GTK_WIDGET(gtk_builder_get_object(builder, "label_waiting"));
    auctionContext->label_countdown = GTK_WIDGET(gtk_builder_get_object(builder, "label_countdown"));

    if (GTK_IS_LABEL(label_room_name))
    {
        gtk_label_set_text(GTK_LABEL(label_room_name), room.roomName);
    }
    if (GTK_IS_LABEL(label_room_owner))
    {
        gtk_label_set_text(GTK_LABEL(label_room_owner), room.username);
    }
    if (GTK_IS_LABEL(auctionContext->label_room_joiner))
    {
        char joiner_count_text[32];
        snprintf(joiner_count_text, sizeof(joiner_count_text), "%d", room.numUsers);
        gtk_label_set_text(GTK_LABEL(auctionContext->label_room_joiner), joiner_count_text);
    }

    auctionContext->auction_window = window;

    auctionContext->item_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "item_list"));
    auctionContext->item_name = GTK_WIDGET(gtk_builder_get_object(builder, "item_name"));
    auctionContext->item_starting_price = GTK_WIDGET(gtk_builder_get_object(builder, "item_starting_price"));
    auctionContext->item_buynow_price = GTK_WIDGET(gtk_builder_get_object(builder, "item_buynow_price"));
    auctionContext->item_auction_time = GTK_WIDGET(gtk_builder_get_object(builder, "item_auction_time"));
    auctionContext->item_card = GTK_WIDGET(gtk_builder_get_object(builder, "item_card"));
    auctionContext->add_item_form = GTK_WIDGET(gtk_builder_get_object(builder, "add_item_form"));
    auctionContext->add_item_msg = GTK_WIDGET(gtk_builder_get_object(builder, "add_item_msg"));

    gtk_builder_connect_signals(builder, auctionContext);

    apply_css();
    gtk_widget_show_all(window);

    if (role == 2)
    {
        gtk_widget_hide(add_button);
        gtk_widget_hide(start_button);
    }

    fetch_item(auctionContext);

    // Thêm watcher cho socket
    GIOChannel *io_channel = g_io_channel_unix_new(sockfd);
    g_io_add_watch(io_channel, G_IO_IN, on_socket_event_auction, auctionContext);

    g_object_unref(builder);
}
