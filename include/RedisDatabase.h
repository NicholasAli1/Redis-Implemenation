#pragma once
#include <string>

class RedisDatabase {
public:
    // Get Singleton instance
    static RedisDatabase& getInstance();

    // Persistence: Dump / Load the database from file.
    bool dump(const std::string& filename);
    bool load(const std::string& filename);


private:
    RedisDatabase() = default;
    ~RedisDatabase() = default;
    RedisDatabase(const RedisDatabase&) = delete;
    RedisDatabase& operator=(const RedisDatabase&) = delete;

};