#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <thread>

constexpr int SERVER_PORT = 8080;

void receiveMessages(int client_fd) {
    char buffer[1024]; 
    int bytes_received;
    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytes_received] = '\0';
        std::cout << buffer << std::endl;
    }
    std::cout << "Server closed the connection.\n";
    close(client_fd);
    exit(0);
}

int main() {
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == -1) {
        std::cerr << "Error creating socket... exiting";
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET; // ipv4
    server_addr.sin_port = htons(SERVER_PORT); // port, converted to big-endian order

    // convert character string to network address https://man7.org/linux/man-pages/man3/inet_pton.3.html
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) == -1){
        std::cerr << "Error: invalid address\n";
        return -1;
    }

    // client does not need to bind, kernel will assign an ip address/port 
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1){
        std::cerr << "Failed to connect to chat server...";
        return -1;
    }

    std::thread receiver_thread(receiveMessages, client_fd);
    receiver_thread.detach();

    std::string input;
    std::cout << "Welcome to chat server!" << std::endl;
    std::cout << "Please enter a username: ";
    std::getline(std::cin, input);
    send(client_fd, input.c_str(), input.length(), 0);

    std::cout << "Please enter a chat room to join: ";
    std::getline(std::cin, input);
    send(client_fd, input.c_str(), input.length(), 0);
    
    while (true) {
        std::getline(std::cin, input);
        if (input == "/q") break;
        send(client_fd, input.c_str(), input.length(), 0);
    }

    std::cout << "disconnecting...\n";
    close(client_fd);
    return 0;
}