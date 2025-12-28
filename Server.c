#include "common.h"
#define MAX_CLIENTS 10

typedef struct
{
    int sockfd;
    char username[20];
} client_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
void *client_handler(void *arg);
void broadcast_group_message(chatroom_packet *packet, int sender_fd);
void add_client(int sockfd, char *username);
void remove_client(int sockfd);
int main()
{
    int server_fd, client_fd;
    struct sockaddr_in serv_addr;
    pthread_t tid;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(6001);

    bind(server_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    listen(server_fd, 10);

    printf("\n==================================================\n\t\tTCP CHAT SERVER\n==================================================\n Status : RUNNING\n Port   : %d\n--------------------------------------------------\n", ntohs(serv_addr.sin_port));

    while (1)
    {
        client_fd = accept(server_fd, NULL, NULL);
        printf("[INFO] New client connected (FD=%d)\n", client_fd);
        int *pclient = malloc(sizeof(int));
        *pclient = client_fd;
        pthread_create(&tid, NULL, client_handler, pclient);
        pthread_detach(tid);
    }
}
void *client_handler(void *arg)
{
    int client_fd = *(int *)arg;
    chatroom_packet packet;

    while (1)
    {
        if (recv(client_fd, &packet, sizeof(packet), 0) <= 0)
        {
            remove_client(client_fd);
            close(client_fd);
            pthread_exit(NULL);
        }

        switch (packet.type)
        {
        case LOGIN:
        case REGISTER:
            printf("[INFO] %s request from user %s\n", packet.type == LOGIN ? "Login" : "Register", packet.user.username);
            handle_login_register(client_fd, &packet);
            break;
        case ONLINE_USERS_LIST:
        {
            printf("[INFO] Online users list requested by %s\n", packet.user.username);
            pthread_mutex_lock(&clients_mutex);

            int count = 0;
            for (int i = 0; i < client_count; i++)
            {
                if (strcmp(clients[i].username, packet.user.username) != 0)
                {
                    strcpy(packet.online_users_packet.username[count++],
                           clients[i].username);
                }
            }
            packet.online_users_packet.user_count = count;

            pthread_mutex_unlock(&clients_mutex);
            send(client_fd, &packet, sizeof(packet), 0);
            break;
        }
        case PRIVATE_CHAT_EXIT:
        case GROUP_CHAT_EXIT:
            printf("[INFO] %s request from user %s\n", packet.type == PRIVATE_CHAT_EXIT ? "Private chat exit" : "Group chat exit", packet.user.username);
            break;
        case PRIVATE_CHAT:
            printf("[INFO]: Private message from %s to %s: %s\n", packet.user.from_username, packet.user.username, packet.chat_packet.message);
            handle_private_chat(&packet, client_fd);
            break;
        case GROUP_CHAT:
            printf("[INFO]: Group message from %s: %s", packet.user.username, packet.chat_packet.message);
            broadcast_group_message(&packet, client_fd);
            break;

        case LOGOUT:
            remove_client(client_fd);
            close(client_fd);
            pthread_exit(NULL);
        }
    }
    free(arg);
}
void broadcast_group_message(chatroom_packet *packet, int sender_fd)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].sockfd != sender_fd)
        {
            send(clients[i].sockfd, packet, sizeof(*packet), 0);
        }
    }

    pthread_mutex_unlock(&clients_mutex);
}
void add_client(int sockfd, char *username)
{
    pthread_mutex_lock(&clients_mutex);
    if (client_count < MAX_USERS)
    {
        clients[client_count].sockfd = sockfd;
        strcpy(clients[client_count].username, username);
        client_count++;
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int sockfd)
{
    pthread_mutex_lock(&clients_mutex);

    for (int i = 0; i < client_count; i++)
    {
        if (clients[i].sockfd == sockfd)
        {
            clients[i] = clients[client_count - 1];
            client_count--;
            break;
        }
    }
    printf("[INFO] Client disconnected (FD=%d)\n", sockfd);
    pthread_mutex_unlock(&clients_mutex);
}

void handle_login_register(int data_sock_fd, chatroom_packet *packet)
{
    int flag = 0;
    int fd = open("database.txt", O_RDWR | O_APPEND | O_CREAT);
    if (fd == -1)
    {
        printf("File not exist\n");
        exit(0);
    }
    if (lseek(fd, -1, SEEK_END) == 0)
    {
        printf("DEBUG: Database file is empty\n");
        if (packet->type == REGISTER) // Signup
        {
            lseek(fd, 0, SEEK_END);
            write(fd, packet->user.username, strlen(packet->user.username));
            write(fd, ",", 1);
            write(fd, packet->user.password, strlen(packet->user.password));
            write(fd, "\n", 1);
            strcpy(packet->error_packet.error_message, "register_success");
            printf("[INFO] User registered: %s\n", packet->user.username);
            send(data_sock_fd, (void *)packet, sizeof(*packet), 0);
        }
        else if (packet->type == LOGIN) // Login
        {
            strcpy(packet->error_packet.error_message, "login_failed");
            printf("[INFO] User %s login FAILED\n", packet->user.username);
            send(data_sock_fd, (void *)packet, sizeof(*packet), 0);
        }
    }
    else
    {
        lseek(fd, 0, SEEK_SET);
        int name_i = 0, pass_i = 0;
        int row = 0;
        int state = 0; // 0 = name, 1 = pass

        char ch;
        char buf_name[20], buf_pass[10];

        char (*name)[20] = malloc(sizeof(char[20]));
        char (*pass)[10] = malloc(sizeof(char[10]));

        lseek(fd, 0, SEEK_SET);

        while (read(fd, &ch, 1) > 0)
        {
            if (state == 0) // reading username
            {
                if (ch == ',')
                {
                    buf_name[name_i] = '\0';
                    name_i = 0;
                    state = 1;
                }
                else if (ch != '\n' && name_i < 19)
                {
                    buf_name[name_i++] = ch;
                }
            }
            else if (state == 1) // reading password
            {
                if (ch == '\n')
                {
                    buf_pass[pass_i] = '\0';
                    pass_i = 0;
                    state = 0;

                    /* store username & password */
                    name = realloc(name, sizeof(char[20]) * (row + 1));
                    pass = realloc(pass, sizeof(char[10]) * (row + 1));

                    strcpy(name[row], buf_name);
                    strcpy(pass[row], buf_pass);

                    row++;
                }
                else if (pass_i < 9)
                {
                    buf_pass[pass_i++] = ch;
                }
            }
        }
        for (int i = 0; i < row; i++)
        {
            if ((!(strcmp(name[i], packet->user.username))) && (!(strcmp(pass[i], packet->user.password))))
            {
                if (packet->type == LOGIN) // Login
                {
                    strcpy(packet->error_packet.error_message, "login_success");
                    printf("[INFO] User %s login SUCCESS\n", packet->user.username);
                    send(data_sock_fd, (void *)packet, sizeof(*packet), 0);
                    add_client(data_sock_fd, packet->user.username);
                    flag = 1;
                    break;
                }
                else if (packet->type == REGISTER) // Signup
                {
                    strcpy(packet->error_packet.error_message, "Username already exists");
                    printf("[INFO] User %s registration FAILED (already exists)\n", packet->user.username);
                    send(data_sock_fd, (void *)packet, sizeof(*packet), 0);
                    flag = 1;
                    break;
                }
            }
        }
        if (flag == 0)
        {
            if (packet->type == LOGIN) // Login
            {
                strcpy(packet->error_packet.error_message, "login_failed");
                printf("[INFO] User %s login FAILED\n", packet->user.username);
                send(data_sock_fd, (void *)packet, sizeof(*packet), 0);
            }
            else if (packet->type == REGISTER) // Signup
            {
                lseek(fd, 0, SEEK_END);
                write(fd, packet->user.username, strlen(packet->user.username));
                write(fd, ",", 1);
                write(fd, packet->user.password, strlen(packet->user.password));
                write(fd, "\n", 1);
                strcpy(packet->error_packet.error_message, "register_success");
                printf("[INFO] User registered: %s\n", packet->user.username);
                send(data_sock_fd, (void *)packet, sizeof(*packet), 0);
            }
        }
    }
    close(fd);
}

void handle_private_chat(chatroom_packet *packet, int sender_fd)
{
    pthread_mutex_lock(&clients_mutex);

    int receiver_fd = find_client_fd_by_name(packet->user.username);

    if (receiver_fd == -1)
    {
        chatroom_packet error_pkt = {0};
        error_pkt.type = ERROR;
        strcpy(error_pkt.error_packet.error_message, "User not online");
        send(sender_fd, &error_pkt, sizeof(error_pkt), 0);
    }
    else
    {
        printf("Forwarding private message from %s to %s: %s\n", packet->user.from_username, packet->user.username, packet->chat_packet.message);
        /* Forward message */
        send(receiver_fd, packet, sizeof(*packet), 0);
    }

    pthread_mutex_unlock(&clients_mutex);
}
int find_client_fd_by_name(char *username)
{
    for (int i = 0; i < client_count; i++)
    {
        if (strcmp(clients[i].username, username) == 0)
        {
            return clients[i].sockfd;
        }
    }
    return -1; // Not found
}
