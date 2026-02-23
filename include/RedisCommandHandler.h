#pragma once

#include <string>

class RedisCommandHandler {
public:
    RedisCommandHandler();
    //Process a command from a client and return RESP-formatted response
    std::string processCommand(const std::string& commandLine);

private:

};