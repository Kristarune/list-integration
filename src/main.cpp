#include <Geode/Geode.hpp>
#include "ListManager.hpp"

using namespace geode::prelude;

$on_mod(Loaded) {
    log::info("[ListsIntegrations] Mod loaded! Initializing list manager...");

    // Load list configurations from mod settings
    ListManager::get()->loadConfigs();

    log::info("[ListsIntegrations] Loaded {} list configurations.", ListManager::get()->getConfigs().size());
    for (auto& cfg : ListManager::get()->getConfigs()) {
        log::info("  - {} ({}): {}", cfg.name, cfg.id, cfg.apiUrl);
    }
}
