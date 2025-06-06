# C++ POSIX TCP Chat Server

A multithreaded TCP chat server and client built using low-level POSIX sockets in C++. Supports multiple chat rooms and usernames.

## Features

- Handle multiple clients concurrently with `std::thread`
- Clients join chat rooms and only see messages from others in the same room
- Server broadcasts messages to all clients in the same room

## How to run
```bash
# Compile the server
g++ -std=c++17 server.cpp -o server
# Run server
./server

# Compile a client
g++ -std=c++17 client.cpp -o client
# Run client
./client
```

Can create as many clients as you want by opening new shell and running the client executable
