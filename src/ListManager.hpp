#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <chrono>

// Represents a single demon list configuration
struct ListConfig {
    std::string id;
    std::string name;
    std::string apiUrl;
    std::string color;
    bool enabled;
};

// Represents a level's entry on a list
struct ListEntry {
    std::string listId;
    std::string listName;
    std::string listColor;
    int position;
    std::string levelName;
};

// Cache entry for a list's data
struct ListCache {
    // One GD level ID can have multiple entries if needed
    std::unordered_map<int, std::vector<ListEntry>> entries;
    std::chrono::system_clock::time_point fetchedAt;
    bool isLoading = false;
};

class ListManager {
public:
    static ListManager* get();

    void loadConfigs();
    std::vector<ListEntry> getEntriesForLevel(int levelID);
    void fetchAllLists();
    void fetchList(const std::string& configId);
    bool isAnyListLoading() const;

    const std::vector<ListConfig>& getConfigs() const { return m_configs; }

    void onListUpdated(std::function<void(const std::string&)> cb) {
        m_updateCallbacks.push_back(cb);
    }

private:
    ListManager() {}

    void parseAREdl(const std::string& configId, const std::string& json);
    void parseNarll(const std::string& configId, const std::string& json);
    void parseDemonlist(const std::string& configId, const std::string& json);
    void parseGeneric(const std::string& configId, const std::string& json);
    void notifyUpdated(const std::string& configId);

    bool isCacheValid(const std::string& configId) const;

    std::vector<ListConfig> m_configs;
    std::unordered_map<std::string, ListCache> m_cache;
    std::vector<std::function<void(const std::string&)>> m_updateCallbacks;
};
