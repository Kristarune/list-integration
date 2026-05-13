#include "ListManager.hpp"
#include <Geode/utils/web.hpp>
#include <matjson.hpp>

using namespace geode::prelude;

ListManager* ListManager::get() {
    static ListManager instance;
    return &instance;
}

void ListManager::loadConfigs() {
    m_configs.clear();

    auto* mod = Mod::get();

    // --- AREDL ---
    if (mod->getSettingValue<bool>("aredl-enabled")) {
        m_configs.push_back({
            "aredl",
            "AREDL",
            mod->getSettingValue<std::string>("aredl-url"),
            "#FF6B35",  // orange
            true
        });
    }

    // --- NARLL ---
    if (mod->getSettingValue<bool>("narll-enabled")) {
        m_configs.push_back({
            "narll",
            "NARLL",
            mod->getSettingValue<std::string>("narll-url"),
            "#4ECDC4",  // teal
            true
        });
    }

    // --- Pointercrate Demonlist ---
    if (mod->getSettingValue<bool>("demonlist-enabled")) {
        m_configs.push_back({
            "demonlist",
            "Demonlist",
            mod->getSettingValue<std::string>("demonlist-url"),
            "#E63946",  // red
            true
        });
    }

    // --- Custom List ---
    if (mod->getSettingValue<bool>("custom-list-enabled")) {
        auto url = mod->getSettingValue<std::string>("custom-list-url");
        auto name = mod->getSettingValue<std::string>("custom-list-name");
        if (!url.empty()) {
            m_configs.push_back({
                "custom",
                name,
                url,
                "#A8DADC",  // light blue
                true
            });
        }
    }
}

bool ListManager::isCacheValid(const std::string& configId) const {
    auto it = m_cache.find(configId);
    if (it == m_cache.end()) return false;
    if (it->second.isLoading) return true; // treat "loading" as valid to avoid duplicate requests

    auto now = std::chrono::system_clock::now();
    auto cacheMins = Mod::get()->getSettingValue<int64_t>("cache-duration");
    auto elapsed = std::chrono::duration_cast<std::chrono::minutes>(now - it->second.fetchedAt).count();
    return elapsed < cacheMins;
}

void ListManager::fetchAllLists() {
    for (auto& cfg : m_configs) {
        if (!isCacheValid(cfg.id)) {
            fetchList(cfg.id);
        }
    }
}

void ListManager::fetchList(const std::string& configId) {
    // Find config
    ListConfig* cfg = nullptr;
    for (auto& c : m_configs) {
        if (c.id == configId) { cfg = &c; break; }
    }
    if (!cfg) return;

    m_cache[configId].isLoading = true;

    std::string id = configId;
    std::string url = cfg->apiUrl;

    web::WebRequest req;
    req.header("User-Agent", "GeodeMod/ListsIntegrations");

    auto task = req.get(url);
    task.listen(
        // success
        [this, id](web::WebResponse* res) {
            if (!res || !res->ok()) {
                log::warn("[ListsIntegrations] Failed to fetch list '{}': HTTP {}", id, res ? res->code() : -1);
                m_cache[id].isLoading = false;
                return;
            }

            auto body = res->string().unwrapOrDefault();

            if (id == "aredl")          parseAREdl(id, body);
            else if (id == "narll")     parseNarll(id, body);
            else if (id == "demonlist") parseDemonlist(id, body);
            else                        parseGeneric(id, body);

            m_cache[id].fetchedAt = std::chrono::system_clock::now();
            m_cache[id].isLoading = false;
            notifyUpdated(id);
        },
        // progress (not needed)
        [](auto) {}
    );
}

// -------------------------------------------------------
// AREDL parser
// Expected response: array of objects with "level_id", "position", "name"
// -------------------------------------------------------
void ListManager::parseAREdl(const std::string& configId, const std::string& json) {
    auto parsed = matjson::parse(json);
    if (!parsed.has_value()) {
        log::error("[ListsIntegrations] AREDL JSON parse failed");
        return;
    }

    auto& arr = parsed.value();
    if (!arr.isArray()) return;

    ListConfig* cfg = nullptr;
    for (auto& c : m_configs) if (c.id == configId) { cfg = &c; break; }
    if (!cfg) return;

    auto& cache = m_cache[configId];
    cache.entries.clear();

    int pos = 1;
    for (auto& item : arr.asArray().unwrap()) {
        if (!item.isObject()) { pos++; continue; }

        // AREDL uses level_id (int) and name
        int levelId = -1;
        if (item.contains("level_id") && item["level_id"].isNumber())
            levelId = item["level_id"].asInt().unwrapOrDefault(-1);

        if (levelId <= 0) { pos++; continue; }

        std::string name = "";
        if (item.contains("name") && item["name"].isString())
            name = item["name"].asString().unwrapOrDefault("");

        int rank = pos;
        if (item.contains("position") && item["position"].isNumber())
            rank = item["position"].asInt().unwrapOrDefault(pos);

        cache.entries[levelId] = {
            configId,
            cfg->name,
            cfg->color,
            rank,
            name
        };
        pos++;
    }

    log::info("[ListsIntegrations] AREDL loaded {} entries", cache.entries.size());
}

// -------------------------------------------------------
// NARLL parser
// Response: plain array of GD level IDs (ints).
// Array index 0 = rank #1, index 1 = rank #2, etc.
// e.g. [134320925, 138145845, 138416537, ...]
// -------------------------------------------------------
void ListManager::parseNarll(const std::string& configId, const std::string& json) {
    auto parsed = matjson::parse(json);
    if (!parsed.has_value()) {
        log::error("[ListsIntegrations] NARLL JSON parse failed");
        return;
    }

    auto& arr = parsed.value();
    if (!arr.isArray()) {
        log::error("[ListsIntegrations] NARLL response is not an array");
        return;
    }

    ListConfig* cfg = nullptr;
    for (auto& c : m_configs) if (c.id == configId) { cfg = &c; break; }
    if (!cfg) return;

    auto& cache = m_cache[configId];
    cache.entries.clear();

    int rank = 1;
    for (auto& item : arr.asArray().unwrap()) {
        if (!item.isNumber()) { rank++; continue; }

        int levelId = item.asInt().unwrapOrDefault(-1);
        if (levelId <= 0) { rank++; continue; }

        // NARLL only provides IDs, no name data
        cache.entries[levelId] = { configId, cfg->name, cfg->color, rank, "" };
        rank++;
    }

    log::info("[ListsIntegrations] NARLL loaded {} entries", cache.entries.size());
}

// -------------------------------------------------------
// Pointercrate Demonlist parser
// Expected: array of { "demon": { "level_id": int, "name": str, "position": int } }
// -------------------------------------------------------
void ListManager::parseDemonlist(const std::string& configId, const std::string& json) {
    auto parsed = matjson::parse(json);
    if (!parsed.has_value()) {
        log::error("[ListsIntegrations] Demonlist JSON parse failed");
        return;
    }

    auto& arr = parsed.value();
    if (!arr.isArray()) return;

    ListConfig* cfg = nullptr;
    for (auto& c : m_configs) if (c.id == configId) { cfg = &c; break; }
    if (!cfg) return;

    auto& cache = m_cache[configId];
    cache.entries.clear();

    for (auto& item : arr.asArray().unwrap()) {
        if (!item.isObject()) continue;

        // Pointercrate wraps in "demon"
        auto* demonPtr = item.contains("demon") ? &item["demon"] : &item;

        int levelId = -1;
        if (demonPtr->contains("level_id") && (*demonPtr)["level_id"].isNumber())
            levelId = (*demonPtr)["level_id"].asInt().unwrapOrDefault(-1);

        if (levelId <= 0) continue;

        int rank = -1;
        if (demonPtr->contains("position") && (*demonPtr)["position"].isNumber())
            rank = (*demonPtr)["position"].asInt().unwrapOrDefault(-1);

        std::string name = "";
        if (demonPtr->contains("name") && (*demonPtr)["name"].isString())
            name = (*demonPtr)["name"].asString().unwrapOrDefault("");

        cache.entries[levelId] = { configId, cfg->name, cfg->color, rank, name };
    }

    log::info("[ListsIntegrations] Demonlist loaded {} entries", cache.entries.size());
}

// -------------------------------------------------------
// Generic parser — tries common field names
// Supports: { "id"/"level_id", "position"/"rank", "name" }
// -------------------------------------------------------
void ListManager::parseGeneric(const std::string& configId, const std::string& json) {
    auto parsed = matjson::parse(json);
    if (!parsed.has_value()) {
        log::error("[ListsIntegrations] Generic list '{}' JSON parse failed", configId);
        return;
    }

    auto& root = parsed.value();

    // Support both root array and { "data": [...] }
    matjson::Value* arr = nullptr;
    if (root.isArray()) {
        arr = &root;
    } else if (root.isObject() && root.contains("data") && root["data"].isArray()) {
        arr = &root["data"];
    } else {
        log::warn("[ListsIntegrations] Generic list '{}' - unrecognized JSON structure", configId);
        return;
    }

    ListConfig* cfg = nullptr;
    for (auto& c : m_configs) if (c.id == configId) { cfg = &c; break; }
    if (!cfg) return;

    auto& cache = m_cache[configId];
    cache.entries.clear();

    int pos = 1;
    for (auto& item : arr->asArray().unwrap()) {
        if (!item.isObject()) { pos++; continue; }

        int levelId = -1;
        for (auto& key : { "level_id", "id", "levelId", "gd_id" }) {
            if (item.contains(key) && item[key].isNumber()) {
                levelId = item[key].asInt().unwrapOrDefault(-1);
                if (levelId > 0) break;
            }
        }

        if (levelId <= 0) { pos++; continue; }

        int rank = pos;
        for (auto& key : { "position", "rank", "placement" }) {
            if (item.contains(key) && item[key].isNumber()) {
                rank = item[key].asInt().unwrapOrDefault(pos);
                break;
            }
        }

        std::string name = "";
        for (auto& key : { "name", "title", "level_name" }) {
            if (item.contains(key) && item[key].isString()) {
                name = item[key].asString().unwrapOrDefault("");
                break;
            }
        }

        cache.entries[levelId] = { configId, cfg->name, cfg->color, rank, name };
        pos++;
    }

    log::info("[ListsIntegrations] Custom list '{}' loaded {} entries", configId, cache.entries.size());
}

std::vector<ListEntry> ListManager::getEntriesForLevel(int levelID) {
    std::vector<ListEntry> result;
    for (auto& [id, cache] : m_cache) {
        auto it = cache.entries.find(levelID);
        if (it != cache.entries.end()) {
            result.push_back(it->second);
        }
    }
    return result;
}

bool ListManager::isAnyListLoading() const {
    for (auto& [id, cache] : m_cache) {
        if (cache.isLoading) return true;
    }
    return false;
}

void ListManager::notifyUpdated(const std::string& configId) {
    for (auto& cb : m_updateCallbacks) {
        cb(configId);
    }
}
