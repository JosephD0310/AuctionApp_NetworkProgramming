#ifndef AUCTION_VIEW_H
#define AUCTION_VIEW_H
#include <gtk/gtk.h>
#include "room.h"
#include "item.h"
#include "user.h"
#include "home_view.h"

// Khai báo hàm hoặc cấu trúc
void init_auction_view(int sockfd, GtkWidget *home_window, Room room, User user, int role);

#endif