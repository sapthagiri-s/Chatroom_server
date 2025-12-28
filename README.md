# Chat Room Client-Server Application

A simple TCP-based chat room application written in C, supporting user registration, login, private messaging, and group chat.

## Features

- User registration and login
- Private (single user) chat
- Group chat with multiple users
- Online users list
- Secure password input (hidden during typing)

## Project Structure

```
chat room client part/
├── Client/
│   ├── Client_header.h
│   └── Client_main.c
└── server/
    ├── common.h
    ├── database.txt
    └── Server2.c
```

## Prerequisites

- GCC compiler
- POSIX threads library (pthread)
- Linux/Windows with socket support

## Compilation

### Client

```bash
gcc -o client Client/Client_main.c -lpthread
```

### Server

```bash
gcc -o server server/Server2.c -lpthread
```

## Running the Application

1. Start the server first:
   ```bash
   ./server
   ```

2. Run the client:
   ```bash
   ./client
   ```

The client will connect to the server at IP `172.30.244.176` on port `6001`.

## Usage

1. Choose to Login or Register
2. After successful authentication, select chat mode:
   - Single User Chat: Private messaging with one user
   - Multi User Chat: Group chat with all online users
3. Type messages and press Enter
4. Use `/exit` to quit chat
5. Use `/Users` to list online users

## Notes

- The server IP and port are hardcoded in the client code
- Passwords are hidden during input for security
- The application uses TCP sockets for communication
