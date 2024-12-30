# Compiler và cờ biên dịch
CC = gcc
CFLAGS = -Wall -rdynamic
GTK_FLAGS = `pkg-config --cflags --libs gtk+-3.0`

# Thư mục chính
CLIENT_DIR = client/src
AUTH_DIR = $(CLIENT_DIR)/views/Auth
HOME_DIR = $(CLIENT_DIR)/views/Home
AUCTION_DIR = $(CLIENT_DIR)/views/Auction
UTILS_DIR = $(CLIENT_DIR)/utils
SERVICES_DIR = $(CLIENT_DIR)/services

SERVER_DIR = server/src
MODELS_DIR = $(SERVER_DIR)/models
CONTROLLERS_DIR = $(SERVER_DIR)/controllers
SESSION_DIR = $(SERVER_DIR)/session

INCLUDE_DIR = include

# Tệp nguồn và đầu ra
CLIENT_SOURCES = 	$(CLIENT_DIR)/main.c \
					$(AUTH_DIR)/auth_view.c \
					$(HOME_DIR)/home_view.c \
					$(AUCTION_DIR)/auction_view.c \
					$(UTILS_DIR)/style_manager.c \
					$(SERVICES_DIR)/auth_service.c \
					$(SERVICES_DIR)/auction_service.c \
					$(MODELS_DIR)/user/user.c \
					$(MODELS_DIR)/room/room.c \
					$(MODELS_DIR)/item/item.c \
					$(MODELS_DIR)/auction/auction_manager.c \
					$(SESSION_DIR)/session_manager.c \
					$(INCLUDE_DIR)/log_system.c \

SERVER_SOURCES = 	$(SERVER_DIR)/main.c \
					$(CONTROLLERS_DIR)/server_controller.c \
					$(SESSION_DIR)/session_manager.c \
					$(MODELS_DIR)/user/user.c \
					$(MODELS_DIR)/room/room.c \
					$(MODELS_DIR)/item/item.c \
					$(MODELS_DIR)/auction/auction_manager.c \
					$(INCLUDE_DIR)/log_system.c \

CLIENT = client_exec
SERVER = server_exec

INCLUDES = 	-I$(AUTH_DIR) -I$(HOME_DIR) -I$(AUCTION_DIR) -I$(UTILS_DIR) -I$(SERVICES_DIR) \
			-I$(MODELS_DIR)/user -I$(MODELS_DIR)/room -I$(MODELS_DIR)/item -I$(MODELS_DIR)/auction \
			-I$(CONTROLLERS_DIR) \
			-I$(SESSION_DIR) \
			-I$(INCLUDE_DIR) \

# Quy tắc biên dịch
all: $(CLIENT) $(SERVER)

$(CLIENT): $(CLIENT_SOURCES) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(CLIENT) $(CLIENT_SOURCES) $(GTK_FLAGS)

$(SERVER): $(SERVER_SOURCES) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(SERVER) $(SERVER_SOURCES) $(GTK_FLAGS)

# Quy tắc dọn dẹp
clean:
	rm -f $(CLIENT) $(SERVER)