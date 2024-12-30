#include <gtk/gtk.h>
#include <sys/socket.h>
#include "style_manager.h"
#include "home_view.h"
#include "auction_view.h"
#include "auction_service.h"
#include "message_type.h"


typedef struct
{
    int sockfd;
    User user;
    GtkWidget *home_window;
    GtkWidget *dashboard;
    GtkWidget *room_item;
    GtkFlowBox *room_list_all;

    GtkWidget *my_auction;
    GtkWidget *create_room_form;
    GtkWidget *room_name;
    GtkWidget *create_room_msg;
    GtkFlowBox *room_list;
    GtkWidget *room_card;
} AppContext;

typedef struct
{
    int sockfd;
    int room_id;
    User user;
    GtkWidget *home_window;
} RoomContext;

void on_home_window_destroy(GtkWidget *widget, gpointer user_data)
{
    GtkWidget *auth_window = (GtkWidget *)user_data;

    // Hiển thị lại cửa sổ đăng nhập khi cửa sổ Home bị đóng
    gtk_widget_show(auth_window);
}

void clear_all_children(GtkFlowBox *flowbox)
{
    GList *children = gtk_container_get_children(GTK_CONTAINER(flowbox));
    for (GList *iter = children; iter != NULL; iter = iter->next)
    {
        gtk_widget_destroy(GTK_WIDGET(iter->data));
    }
    g_list_free(children);
}

////////////////// DASHBOARD PAGE //////////////////

GtkWidget *create_room_card(Room room, gpointer user_data)
{
    AppContext *context = (AppContext *)user_data;

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
    RoomContext *roomContext = g_malloc(sizeof(RoomContext));
    roomContext->sockfd = context->sockfd;
    roomContext->room_id = room.room_id;
    roomContext->home_window = context->home_window;
    roomContext->user = context->user;

    gtk_builder_connect_signals(builder, roomContext);

    card = GTK_WIDGET(gtk_builder_get_object(builder, "room_card"));

    GtkWidget *parent = gtk_widget_get_parent(card);
    if (parent != NULL)
    {
        gtk_container_remove(GTK_CONTAINER(parent), card);
    }

    // Tìm và cập nhật nội dung trong card
    GtkWidget *label_room_name = GTK_WIDGET(gtk_builder_get_object(builder, "label_room_name"));
    GtkWidget *label_room_owner = GTK_WIDGET(gtk_builder_get_object(builder, "label_room_owner"));
    GtkWidget *label_item_count = GTK_WIDGET(gtk_builder_get_object(builder, "label_item_count"));
    GtkWidget *label_room_status = GTK_WIDGET(gtk_builder_get_object(builder, "label_room_status"));

    if (GTK_IS_LABEL(label_room_name))
    {
        gtk_label_set_text(GTK_LABEL(label_room_name), room.roomName);
    }
    if (GTK_IS_LABEL(label_room_owner))
    {
        gtk_label_set_text(GTK_LABEL(label_room_owner), room.username);
    }
    if (GTK_IS_LABEL(label_room_status))
    {
        gtk_label_set_text(GTK_LABEL(label_room_status), room.status);
    }
    if (GTK_IS_LABEL(label_item_count))
    {
        char item_count_text[32];
        snprintf(item_count_text, sizeof(item_count_text), "%d", room.numItems);
        gtk_label_set_text(GTK_LABEL(label_item_count), item_count_text);
    }

    return card;
}

void on_join_btn_clicked(GtkWidget *button, gpointer user_data)
{
    RoomContext *context = (RoomContext *)user_data;
    Room room;

    int role = handle_join_room(context->sockfd, context->room_id, &room);

    if (role <= 0)
    {
        g_printerr("Failed to join room %d\n", context->room_id);
        return;
    }

    gtk_widget_hide(context->home_window);
    init_auction_view(context->sockfd, context->home_window, room, context->user, role);
}

void fetch_all_room(gpointer user_data)
{
    AppContext *context = (AppContext *)user_data;

    // Tạo mảng Room để lưu danh sách phòng nhận được
    Room rooms[MAX_ROOMS];
    int room_count = handle_fetch_all_rooms(context->sockfd, rooms);

    if (room_count < 0)
    {
        g_printerr("Failed to fetch room list from server\n");
        return;
    }

    // Duyệt qua danh sách phòng và thêm vào GTK_FLOW_BOX
    for (int i = 0; i < room_count; i++)
    {

        GtkWidget *room_card = create_room_card(rooms[i], context);
        gtk_flow_box_insert(context->room_list_all, room_card, -1);
    }

    // Hiển thị các widget vừa thêm
    gtk_widget_show_all(GTK_WIDGET(context->room_list_all));
}

////////////////// MY AUCTION PAGE //////////////////

void fetch_own_room(gpointer user_data)
{
    AppContext *context = (AppContext *)user_data;

    // Tạo mảng Room để lưu danh sách phòng nhận được
    Room rooms[MAX_ROOMS];
    int room_count = handle_fetch_own_rooms(context->sockfd, rooms);

    if (room_count < 0)
    {
        g_printerr("Failed to fetch room list from server\n");
        return;
    }

    // Duyệt qua danh sách phòng và thêm vào GTK_FLOW_BOX
    for (int i = 0; i < room_count; i++)
    {

        GtkWidget *room_card = create_room_card(rooms[i], context);
        gtk_flow_box_insert(context->room_list, room_card, -1);
    }

    // Hiển thị các widget vừa thêm
    gtk_widget_show_all(GTK_WIDGET(context->room_list));
}

void show_create_room_form(GtkWidget *button, gpointer user_data)
{
    AppContext *context = (AppContext *)user_data;

    GtkWidget *form = GTK_WIDGET(context->create_room_form);
    gtk_widget_show(form);
}

void on_create_room_ok(GtkWidget *button, gpointer user_data)
{
    AppContext *context = (AppContext *)user_data;

    GtkWidget *form = GTK_WIDGET(context->create_room_form);
    const gchar *room_name = gtk_entry_get_text(GTK_ENTRY(context->room_name));

    int roomId = handle_create_room(context->sockfd, room_name);
    if (roomId > 0)
    {
        gtk_widget_hide(form);
        clear_all_children(context->room_list);
        fetch_own_room(context);
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(context->create_room_msg), "This room's name already exists.");
    }
}

void on_create_room_cancel(GtkWidget *button, gpointer user_data)
{
    AppContext *context = (AppContext *)user_data;

    GtkWidget *form = GTK_WIDGET(context->create_room_form);
    gtk_widget_hide(form);
}

////////////////// ////////////////// //////////////////

void reload_home_view(gpointer user_data)
{
    AppContext *context = (AppContext *)user_data;
    // Xóa nội dung cũ
    clear_all_children(context->room_list_all);

    // Nạp lại dữ liệu
    fetch_all_room(context);
}

void on_tab_switch(GtkStack *stack, GParamSpec *pspec, gpointer user_data)
{
    AppContext *context = (AppContext *)user_data;

    // Lấy widget hiện tại đang hiển thị
    GtkWidget *current_child = gtk_stack_get_visible_child(stack);

    // Kiểm tra nếu là tab All Rooms
    if (current_child == GTK_WIDGET(context->dashboard))
    {
        printf("Switched to Dashboard tab.\n");

        reload_home_view(context);
    }
    // Kiểm tra nếu là tab My Auctions
    else if (current_child == GTK_WIDGET(context->my_auction))
    {
        printf("Switched to My Auctions tab.\n");

        // // Xóa các widget hiện có trong flowbox trước khi fetch lại
        clear_all_children(context->room_list);

        // // Fetch lại danh sách các phòng của người dùng
        fetch_own_room(context);
    }
}

gboolean on_socket_event(GIOChannel *source, GIOCondition condition, gpointer user_data)
{
    AppContext *context = (AppContext *)user_data;

    if (gtk_widget_get_visible(context->home_window))
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
            char message_type = buffer[0]; // Loại thông điệp
            switch (message_type)
            {
            case -1:
                printf("Server yêu cầu refresh dữ liệu.\n");
                reload_home_view(context);
                break;
            default:
                printf("Received unknown message type: %d\n", message_type);
                break;
            }
        }
    }

    return TRUE; // Tiếp tục theo dõi socket
}

void init_home_view(int sockfd, GtkWidget *auth_window, User user)
{
    GtkBuilder *builder;
    GtkWidget *window;
    GError *error = NULL;

    builder = gtk_builder_new();
    if (!gtk_builder_add_from_file(builder, "client/src/views/Home/home_view.glade", &error))
    {
        g_printerr("Error loading file: %s\n", error->message);
        g_clear_error(&error);
        return;
    }

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window_home"));
    g_signal_connect(window, "destroy", G_CALLBACK(on_home_window_destroy), auth_window);
    GtkWidget *label_username = GTK_WIDGET(gtk_builder_get_object(builder, "label_username"));
    GtkWidget *label_money = GTK_WIDGET(gtk_builder_get_object(builder, "label_money"));
    gtk_label_set_text(GTK_LABEL(label_username), user.username);

    char money_text[32];
    snprintf(money_text, sizeof(money_text), "%d", user.money);
    gtk_label_set_text(GTK_LABEL(label_money), money_text);

    AppContext *appContext = g_malloc(sizeof(AppContext));
    appContext->sockfd = sockfd;
    appContext->home_window = window;
    appContext->user = user;

    appContext->dashboard = GTK_WIDGET(gtk_builder_get_object(builder, "dashboard"));
    appContext->room_list_all = GTK_FLOW_BOX(gtk_builder_get_object(builder, "room_list_all"));
    appContext->room_card = GTK_WIDGET(gtk_builder_get_object(builder, "room_card"));

    appContext->my_auction = GTK_WIDGET(gtk_builder_get_object(builder, "my_auction"));
    appContext->room_list = GTK_FLOW_BOX(gtk_builder_get_object(builder, "room_list"));
    appContext->create_room_form = GTK_WIDGET(gtk_builder_get_object(builder, "create_room_form"));
    appContext->room_name = GTK_WIDGET(gtk_builder_get_object(builder, "room_name"));
    appContext->create_room_msg = GTK_WIDGET(gtk_builder_get_object(builder, "create_room_msg"));

    GtkStack *stack = GTK_STACK(gtk_builder_get_object(builder, "stack_app"));
    g_signal_connect(stack, "notify::visible-child", G_CALLBACK(on_tab_switch), appContext);

    gtk_builder_connect_signals(builder, appContext);

    apply_css();
    gtk_widget_show_all(window);
    fetch_all_room(appContext);

    // Thêm watcher cho socket
    GIOChannel *io_channel = g_io_channel_unix_new(sockfd);
    g_io_add_watch(io_channel, G_IO_IN, on_socket_event, appContext);

    g_object_unref(builder);
}
