#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/shm.h>
#include <pthread.h>

#define MAX_USERS 10 // Maximum number of users allowed in the chat room

typedef struct
{
    int user_count;
    int sock_fd[MAX_USERS];
    char username[MAX_USERS][20];
} online_users_packet_t;
enum
{
    LOGIN = 1,
    REGISTER,
    LOGOUT,
    ONLINE_USERS_LIST,
    PRIVATE_CHAT,
    GROUP_CHAT,
    OFFLINE_USERS_LIST,
    ERROR,
    PRIVATE_CHAT_EXIT,
    GROUP_CHAT_EXIT
} Type;

typedef struct
{
    int type;
    struct
    {
        char username[100];
        char password[100];
        char from_username[100];
    } user;

    struct
    {
        char message[1000];
    } chat_packet;

    struct
    {
        char error_message[100];
    } error_packet;
    struct
    {
        int user_count;
        int sock_fd[MAX_USERS];
        char username[MAX_USERS][20];
    } online_users_packet;
} chatroom_packet;

void handle_login_register(int data_sock_fd, chatroom_packet *packet);
void add_online_user(int fd, char *name);
void remove_online_user_by_name(char *name);
void handle_group_chat(int data_sock_fd, chatroom_packet *packet);
void handle_private_chat(chatroom_packet *packet, int sender_fd);
int find_client_fd_by_name(char *username);