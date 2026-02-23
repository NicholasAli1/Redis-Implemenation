#include "../include/RedisCommandHandler.h"
#include "../include/RedisDatabase.h"

#include <vector>
#include <sstream>
#include <algorithm>

// RESP parser:
// *2\r\n$4\r\n\PING\r\n$4\r\nTEST\r\n
// *2 -> Array has two elements
// $4 -> next string has 4 characters
// PING ->
// TEST ->

std::vector<std::string> parseRespCommand(const std::string &input) {
    std::vector<std::string> tokens;            // Create an empty vector to store parsed command parts ['PING', 'TEST']
    if (input.empty()) return tokens;

    // if it doesnt start with '*', fallback to splitting by whitespaces
    if (input[0] != '*') {
        std::istringstream iss(input);
        std::string token;
        while (iss >> token)
            tokens.push_back(token);
        return tokens;
    }

    size_t pos = 0;
    // Expect '*' followed by number of elements
    if (input[pos] != '*') return tokens;           // Make sure input starts with *
    pos++; // Skip '*'

    // crlf = Carriage Return (\r), Line Feed (\n)
    size_t crlf = input.find("\r\n", pos);
    if (crlf == std::string::npos) return {};

    int numElements;
    try {
        numElements = std::stoi(input.substr(pos, crlf - pos));
    } catch (...) {
        return {};
    }

    if (numElements <= 0) return {};

    tokens.reserve(numElements);
    pos = crlf + 2;

    for (int i = 0; i < numElements; ++i) {
        if (pos >= input.size() || input[pos] != '$') break; // Formating Error
        pos++; // Slip '$'

        crlf = input.find("\r\n", pos);
        if (crlf == std::string::npos) return {};

        int len;
        try {
            len = std::stoi(input.substr(pos, crlf - pos));
        } catch (...) {
            return {};
        }

        if (len < 0) return {};

        pos = crlf + 2;

        if (pos + len + 2 > input.size()) return {};

        std::string token = input.substr(pos, len);
        tokens.push_back(token);
        pos += len;  // Skip Token

        // Validate trailing CRLF
        if (input.substr(pos, 2) != "\r\n") return {};
        pos += 2;  // Skip CRLF
    }

    return tokens;
}




RedisCommandHandler::RedisCommandHandler() {

}

std::string RedisCommandHandler::processCommand(const std::string &commandLine) {
    // Use RESP parser
    auto tokens = parseRespCommand(commandLine);
    if (tokens.empty()) return "-ERR Protocol error\r\n";

    std::string cmd = tokens[0];
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::toupper);
    std::ostringstream response;
    RedisDatabase& db = RedisDatabase::getInstance();

    // Check commands
    // Common Commands
    if (cmd == "PING") {
        response << "+PONG\r\n";
    }
    else if (cmd == "ECHO") {
        if (tokens.size() < 2)
            response << "-ERR ECHO requires input\r\n";
        else
            response << "$" << tokens[1].size() << "\r\n" << tokens[1] << "\r\n";
    }
    else if (cmd == "FLUSHALL") {   // Fixed typo
        db.flushAll();
        response << "+OK\r\n";
    }
    // TODO: Key/Value Ops
    else if (cmd == "SET") {
        if (tokens.size() < 3) {
            response << "-ERR SET requires key AND value\r\n";
        }
        else {
            db.set(tokens[1], tokens[2]);
            response << "+OK\r\n";
        }
    }
    else if (cmd == "GET") {
        if (tokens.size() < 2) {
            response << "-ERR GET requires key\r\n";
        }
        else {
            std::string value;
            if (db.get(tokens[1], value))
                response << "$" << value.size() << "\r\n" << value << "\r\n";
            else
                response << "$-1\r\n";
        }
    }
    else if (cmd == "KEYS") {
        std::vector<std::string> allKeys = db.keys();
        response << "*" << allKeys.size() << "\r\n";
        for (const auto& key : allKeys) {
            response << "$" << key.size() << "\r\n" << key << "\r\n";
        }
    }
    else if (cmd == "TYPE") {
        if (tokens.size() < 2)
            response << "-ERR TYPE requires key\r\n";
        else
            response << "+" << db.type(tokens[1]) << "\r\n";
    }
    else if (cmd == "DEL" || cmd == "UNLINK") {
        if (tokens.size() < 2)
            response << "-ERR DEL requires key\r\n";
        else {
            int count = 0;
            for (size_t i = 1; i < tokens.size(); ++i)
                if (db.del(tokens[i]))
                    count++;
            response << ":" << count << "\r\n";
        }
    }
    else if (cmd == "EXPIRE") {
        if (tokens.size() < 3)
            response << "-ERR EXPIRE requires key and time in seconds\r\n";
        else {
            bool result = db.expire(tokens[1], tokens[2]);
            response << ":" << (result ? 1 : 0) << "\r\n";
        }
    }
    else if (cmd == "RENAME") {
        if (tokens.size() < 3)
            response << "-ERR RENAME requires old key and new key names\r\n";
        else {
            bool result = db.rename(tokens[1], tokens[2]);
            if (!result)
                response << "-ERR no such key\r\n";
            else
                response << "+OK\r\n";
        }
    }
    // TODO: List Ops
    // TODO: Hash Ops
    else {
        response << "-ERR Unknown command\r\n";
    }
    return response.str();
}