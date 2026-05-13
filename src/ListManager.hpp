#pragma once
#include <Geode/Geode.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

using namespace geode::prelude;

// Represents a single demon list configuration
struct ListConfig {
    std::string id;         // Internal ID e.g. "aredl"
    std::string name;       // Display name e.g. "AREDL"
    std::string apiUrl;     // Full API URL
    std::string color;      // Hex color for the badge e.g. "#FF6B35"
    bool enabled;
};

// Represents a level's entry on a list
struct ListEntry {
    std::string listId;     // Which list this is from
    std::string listName;   // Display name of the list
    std::string listColor;  // Badge color
    int position;           // Rank/position on the list (-1 if unknown)
    std::string levelName;  // Level name from the list
};

// Cache entry for a list's data
struct ListCache {
    // Map of GD level ID (int) -> ListEntry
    std::unordered_map<int, ListEntry> entries;
    std::chrono::system_clock::time_point fetchedAt;
    bool isLoading = false;
};

class ListManager {
public:
    static ListManager* get();

    // Call once at startup to load configs from settings
    void loadConfigs();

    // Get all entries for a given GD level ID (from cache)
    std::vector<ListEntry> getEntriesForLevel(int levelID);

    // Trigger a (re)fetch for all enabled lists
    void fetchAllLists();

    // Fetch a single list by config ID
    void fetchList(const std::string& configId);

    // Check if any list is currently loading
    bool isAnyListLoading() const;

    // Returns the list of active configs
    const std::vector<ListConfig>& getConfigs() const { return m_configs; }

    // Register a callback for when list data updates
    // Callback receives the config ID of the updated list
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
