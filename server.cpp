#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <thread>

constexpr int PORT = 8080;
constexpr int MAX_CLIENTS = 10;

struct ClientInfo {
    int socket_fd;
    std::string username;
    std::string room;
};

std::mutex client_sockets_mutex;
std::vector<ClientInfo> clients;


void broadcastMessage(int sender_fd, const std::string& room, const std::string& message) {
    client_sockets_mutex.lock();
    for (const auto& client : clients) {
        if (client.room == room && client.socket_fd != sender_fd) {
            //https://man7.org/linux/man-pages/man2/send.2.html
            send(client.socket_fd, message.c_str(), message.length(), 0);
        }
    }
    client_sockets_mutex.unlock();
}


void handleClient(int client_fd) {
    char buffer[1024];
    int bytes_received;

    // https://man7.org/linux/man-pages/man2/recv.2.html
    bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    buffer[bytes_received] = '\0'; // null terminate up to bytes received
    std::string username = buffer;

    bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);
    buffer[bytes_received] = '\0';
    std::string room = buffer;

    ClientInfo new_client = {client_fd, username, room};

    client_sockets_mutex.lock();
    clients.push_back(new_client);
    client_sockets_mutex.unlock();

    std::string room_msg = username + " has joined room " + room + "\n";
    std::cout << room_msg << std::endl;
    broadcastMessage(client_fd, room, room_msg);

    while ((bytes_received = recv(client_fd, buffer, sizeof(buffer), 0)) > 0) {
        buffer[bytes_received] = '\0';
        std::string msg = username + ": " + std::string(buffer);
        std::cout << "Room " << room << " | " << msg << std::endl;
        broadcastMessage(client_fd, room, msg);
    }

    room_msg = username + " has left room " + room + "\n";
    broadcastMessage(client_fd, room, room_msg);

    close(client_fd);
    client_sockets_mutex.lock();
    
    clients.erase(std::remove_if(clients.begin(), clients.end(), 
        [client_fd](const ClientInfo& client) { return client.socket_fd == client_fd; }), 
        clients.end());

    client_sockets_mutex.unlock();

}

int main() {

    //https://man7.org/linux/man-pages/man2/socket.2.html
    //https://man7.org/linux/man-pages/man7/ip.7.html
    int server_fd = socket(AF_INET, SOCK_STREAM, 0); // get file descriptor for new TCP socket
    if (server_fd == -1) {
        std::cerr << "error creating socket... exiting";
        return -1;
    }

    //https://man7.org/linux/man-pages/man3/sockaddr.3type.html
    // sockaddr_in specifies socket type is for ip
    // sin = socket information
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET; // ipv4
    server_addr.sin_port = htons(PORT); // port, converted to big-endian order
    server_addr.sin_addr.s_addr = INADDR_ANY; // listen on any network interface

    socklen_t addr_len = sizeof(server_addr);

    //https://man7.org/linux/man-pages/man2/bind.2.html
    int ret_val = bind(server_fd, (struct sockaddr *) &server_addr, addr_len);
    if (ret_val == -1) {
        std::cerr << "error binding socket... exiting";
        return -1;
    }

    //https://man7.org/linux/man-pages/man2/listen.2.html
    listen(server_fd, MAX_CLIENTS);
    std::cout << "Listening on port " << PORT << "..." << std::endl;

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    while (true) {
        //https://man7.org/linux/man-pages/man2/accept.2.html
        int new_client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (new_client_fd == -1) {
            std::cout << "client accept failed" << std::endl;
            continue;
        }
        char client_ip[INET_ADDRSTRLEN];
        //https://man7.org/linux/man-pages/man3/inet_ntop.3.html
        // convert network bytes to string
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        uint16_t client_port = ntohs(client_addr.sin_port); // get port from client
        std::cout << "New client connected from " << client_ip << ":" << client_port << "\n";

        std::thread client_thread(handleClient, new_client_fd);
        client_thread.detach();
    }

    std::cout << "exiting" << std::endl;
    close(server_fd);
    return 0;
}