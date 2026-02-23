#pragma once
#include <string>
#include <mutex>
#include <unordered_map>
#include <vector>

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

    std::mutex db_mutex;
    std::unordered_map<std::string, std::string> kv_store;
    std::unordered_map<std::string, std::vector<std::string>> list_store;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> hash_store;

};
