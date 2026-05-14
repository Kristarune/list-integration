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

    // AREDL
    if (mod->getSettingValue<bool>("aredl-enabled")) {
        m_configs.push_back({
            "aredl",
            "AREDL",
            mod->getSettingValue<std::string>("aredl-url"),
            "#FF6B35",
            true
        });
    }

    // NARLL
    if (mod->getSettingValue<bool>("narll-enabled")) {
        m_configs.push_back({
            "narll",
            "NARLL",
            mod->getSettingValue<std::string>("narll-url"),
            "#4ECDC4",
            true
        });
    }

    // Demonlist
    if (mod->getSettingValue<bool>("demonlist-enabled")) {
        m_configs.push_back({
            "demonlist",
            "Demonlist",
            mod->getSettingValue<std::string>("demonlist-url"),
            "#E63946",
            true
        });
    }

    // Custom
    if (mod->getSettingValue<bool>("custom-list-enabled")) {
        auto url = mod->getSettingValue<std::string>("custom-list-url");
        auto name = mod->getSettingValue<std::string>("custom-list-name");

        if (!url.empty()) {
            m_configs.push_back({
                "custom",
                name,
                url,
                "#A8DADC",
                true
            });
        }
    }
}

bool ListManager::isCacheValid(const std::string& configId) const {
    auto it = m_cache.find(configId);

    if (it == m_cache.end())
        return false;

    if (it->second.isLoading)
        return true;

    auto now = std::chrono::system_clock::now();

    auto cacheMins =
        Mod::get()->getSettingValue<int64_t>("cache-duration");

    auto elapsed =
        std::chrono::duration_cast<std::chrono::minutes>(
            now - it->second.fetchedAt
        ).count();

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
    ListConfig* cfg = nullptr;

    for (auto& c : m_configs) {
        if (c.id == configId) {
            cfg = &c;
            break;
        }
    }

    if (!cfg)
        return;

    m_cache[configId].isLoading = true;

    std::string id = configId;
    std::string url = cfg->apiUrl;

    web::WebRequest req;

    req.header(
        "User-Agent",
        "ListsIntegrations"
    );

    req.get(url).listen([this, id](web::WebResponse* res) {

        if (!res || !res->ok()) {
            log::error(
                "[ListsIntegrations] Failed request for {}",
                id
            );

            m_cache[id].isLoading = false;
            return;
        }

        auto body = res->string().unwrapOr("");

        if (id == "aredl") {
            parseAREdl(id, body);
        }
        else if (id == "narll") {
            parseNarll(id, body);
        }
        else if (id == "demonlist") {
            parseDemonlist(id, body);
        }
        else {
            parseGeneric(id, body);
        }

        m_cache[id].fetchedAt =
            std::chrono::system_clock::now();

        m_cache[id].isLoading = false;

        notifyUpdated(id);

    });
}

// =====================================================
// AREDL
// =====================================================

void ListManager::parseAREdl(
    const std::string& configId,
    const std::string& json
) {
    auto parsed = matjson::parse(json);

    if (!parsed) {
        log::error("[ListsIntegrations] AREDL parse failed");
        return;
    }

    auto root = parsed.unwrap();

    if (!root.isArray())
        return;

    ListConfig* cfg = nullptr;

    for (auto& c : m_configs) {
        if (c.id == configId) {
            cfg = &c;
            break;
        }
    }

    if (!cfg)
        return;

    auto& cache = m_cache[configId];
    cache.entries.clear();

    int rank = 1;

    for (auto& item : root.asArray().unwrap()) {

        if (!item.isObject()) {
            rank++;
            continue;
        }

        int levelID = -1;

        if (item.contains("level_id")) {
            auto idRes = item["level_id"].asInt();

            if (idRes)
                levelID = idRes.unwrap();
        }

        if (levelID <= 0) {
            rank++;
            continue;
        }

        std::string levelName = "";

        if (item.contains("name")) {
            auto nameRes = item["name"].asString();

            if (nameRes)
                levelName = nameRes.unwrap();
        }

        cache.entries[levelID] = {
            configId,
            cfg->name,
            cfg->color,
            rank,
            levelName
        };

        rank++;
    }

    log::info(
        "[ListsIntegrations] AREDL loaded {} entries",
        cache.entries.size()
    );
}

// =====================================================
// NARLL
// =====================================================

void ListManager::parseNarll(
    const std::string& configId,
    const std::string& json
) {
    auto parsed = matjson::parse(json);

    if (!parsed) {
        log::error("[ListsIntegrations] Failed to parse NARLL JSON");
        return;
    }

    auto root = parsed.unwrap();

    if (!root.isArray()) {
        log::error("[ListsIntegrations] NARLL root is not array");
        return;
    }

    ListConfig* cfg = nullptr;

    for (auto& c : m_configs) {
        if (c.id == configId) {
            cfg = &c;
            break;
        }
    }

    if (!cfg)
        return;

    auto& cache = m_cache[configId];
    cache.entries.clear();

    int rank = 1;

    for (auto& item : root.asArray().unwrap()) {

        if (!item.isObject()) {
            rank++;
            continue;
        }

        int levelID = -1;

        if (item.contains("id")) {
            auto idRes = item["id"].asInt();

            if (idRes)
                levelID = idRes.unwrap();
        }

        if (levelID <= 0) {
            rank++;
            continue;
        }

        std::string levelName = "";

        if (item.contains("name")) {
            auto nameRes = item["name"].asString();

            if (nameRes)
                levelName = nameRes.unwrap();
        }

        cache.entries[levelID] = {
            configId,
            cfg->name,
            cfg->color,
            rank,
            levelName
        };

        rank++;
    }

    log::info(
        "[ListsIntegrations] NARLL loaded {} entries",
        cache.entries.size()
    );
}

// =====================================================
// DEMONLIST
// =====================================================

void ListManager::parseDemonlist(
    const std::string& configId,
    const std::string& json
) {
    parseGeneric(configId, json);
}

// =====================================================
// GENERIC
// =====================================================

void ListManager::parseGeneric(
    const std::string& configId,
    const std::string& json
) {
    auto parsed = matjson::parse(json);

    if (!parsed) {
        log::error("[ListsIntegrations] Generic parse failed");
        return;
    }

    auto root = parsed.unwrap();

    if (!root.isArray())
        return;

    ListConfig* cfg = nullptr;

    for (auto& c : m_configs) {
        if (c.id == configId) {
            cfg = &c;
            break;
        }
    }

    if (!cfg)
        return;

    auto& cache = m_cache[configId];
    cache.entries.clear();

    int rank = 1;

    for (auto& item : root.asArray().unwrap()) {

        if (!item.isObject()) {
            rank++;
            continue;
        }

        int levelID = -1;

        if (item.contains("id")) {
            auto idRes = item["id"].asInt();

            if (idRes)
                levelID = idRes.unwrap();
        }

        if (levelID <= 0) {
            rank++;
            continue;
        }

        std::string levelName = "";

        if (item.contains("name")) {
            auto nameRes = item["name"].asString();

            if (nameRes)
                levelName = nameRes.unwrap();
        }

        cache.entries[levelID] = {
            configId,
            cfg->name,
            cfg->color,
            rank,
            levelName
        };

        rank++;
    }

    log::info(
        "[ListsIntegrations] Generic list loaded {} entries",
        cache.entries.size()
    );
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
        if (cache.isLoading)
            return true;
    }

    return false;
}

void ListManager::notifyUpdated(
    const std::string& configId
) {
    for (auto& cb : m_updateCallbacks) {
        cb(configId);
    }
}