#include "../include/RedisServer.h"
#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"

#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <thread>
#include <cstring>
#include <csignal>

static RedisServer* globalServer = nullptr; // Global pointer to one instance of RedisServer Object. Initially points to nothing

void signalHandler(int signum) {
    if (globalServer) {
        std::cout << "Caught signal " << signum << " shutting down...\n";
        globalServer->shutdown();
    }
    exit(signum);
}

void RedisServer::setupSignalHandler() {
    signal(SIGINT, signalHandler);
}

RedisServer::RedisServer(int port) : port(port), server_socket(-1), running(true) {
    globalServer = this; // Now Points to the instance of RedisServer being created
    setupSignalHandler();
}

void RedisServer::shutdown() {
    running = false;
    if (server_socket != -1) {
        close(server_socket);
    }
    std::cout << "Server Shutdown Complete";
}

/*
* CREATING THE SOCKET
* Returns an integer file descriptor
* AF_INET = IPv4 address family -> Telling OS what type of addresses the socket will use ex: 192.168.1.1
* SOCK_STREAM = TCP
* 0 = Default protocol -> IPv4 Stream Based IPv4 + TCP
*/
void RedisServer::run() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error Creating Server Socket\n";
        return;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,&opt, sizeof(opt));      // Allows rebinding to a port in TIME_WAIT state (helps with fast restarts)


    sockaddr_in address{};                          // C Struct with 16 bytes of memory called address. This represents the entire IP + Port
    address.sin_family = AF_INET;                   // Telling us the struct will be IPv4
    address.sin_port = htons(port);                 // Convert port from host byte order to network byte order (big endian)
    address.sin_addr.s_addr = INADDR_ANY;           // Bind to all available network interfaces (0.0.0.0)

    // Bind tells the OS to attach the socket to this IP and port. Checks to see if port 8080 already in use then registers the socket as the owner of that port then associates with the given IP and finally add it to the networking table
    if (bind(server_socket, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0) {
        std::cerr << "Error binding Server Socket\n";
        return;
    }

    // Put the socket into listening mode; the backlog is max queued pending connections
    if (listen(server_socket, SOMAXCONN) < 0) {
        std::cerr << "Error Listening On Server Socket\n";
    }

    std::cout << "Redis Server Listening On Port " << port << "\n";
    RedisCommandHandler cmdHandler;

    while (running) {
        int client_socket = accept(server_socket, nullptr, nullptr);

        if (client_socket < 0) {
            if (running) {
                std::cerr << "Error Accepting Client Connection\n";
            }
            continue;
        }

        // Handle each client in its own thread (spawn only on successful accept)
        std::thread clientThread([client_socket, &cmdHandler]() {
            char buffer[1024];
            while (true) {
                memset(buffer, 0, sizeof(buffer));
                int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
                if (bytes <= 0) break;
                std::string request(buffer, bytes);
                std::string response = cmdHandler.processCommand(request);
                send(client_socket, response.c_str(), response.size(), 0);
            }
            close(client_socket);
        });
        clientThread.detach();
    }

    // Before shutdown persist database
    if (RedisDatabase::getInstance().dump("dump_my_rdb"))
        std::cout << "Database dumped to dump.my_rdb\n";
    else
        std::cerr << "Error dumping database\n";
}
