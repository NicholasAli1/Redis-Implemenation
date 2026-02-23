#include "../include/RedisServer.h"
#include "../include/RedisDatabase.h"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    int port = 6379; // Default
    if (argc >= 2) port = std::stoi(argv[1]);

    RedisServer server(port);

    // Background Persistence: dump database every 300 seconds.
    std::thread persistenceThread([]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(300));
            if (!RedisDatabase::getInstance().dump("dump.my_rdb"))
                std::cerr << "Error Dumping Database\n";
            else
                std::cout << "Database Dumped to dump.my_rdb\n";
        }
    });
    persistenceThread.detach();

    server.run();

    return 0;
}