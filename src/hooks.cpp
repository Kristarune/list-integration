#include <Geode/Geode.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>

#include "ListManager.hpp"
#include "ListBadgeNode.hpp"

using namespace geode::prelude;

// -------------------------------------------------------
// Badge tag
// -------------------------------------------------------

static constexpr int BADGE_TAG = 0x4C495354;

// -------------------------------------------------------
// Get level ID safely
// -------------------------------------------------------

static int getLevelId(LevelCell* cell) {
    if (!cell) {
        return -1;
    }

    if (!cell->m_level) {
        return -1;
    }

    return cell->m_level->m_levelID.value();
}

// -------------------------------------------------------
// LEVEL CELL HOOK
// -------------------------------------------------------

struct ListsIntegrationsLevelCellHook :
    Modify<ListsIntegrationsLevelCellHook, LevelCell> {

    void loadCustomLevelCell() {
        LevelCell::loadCustomLevelCell();

        addListBadges(this);
    }

    static void addListBadges(LevelCell* cell) {
        if (!cell) {
            return;
        }

        // ---------------------------------------------------
        // SETTINGS CHECK
        // ---------------------------------------------------

        if (!Mod::get()->getSettingValue<bool>("show-badges")) {
            return;
        }

        // ---------------------------------------------------
        // LEVEL ID
        // ---------------------------------------------------

        int levelId = getLevelId(cell);

        if (levelId <= 0) {
            return;
        }

        // ---------------------------------------------------
        // REMOVE OLD BADGES
        // ---------------------------------------------------

        if (auto* old = cell->getChildByTag(BADGE_TAG)) {
            old->removeFromParent();
        }

        // ---------------------------------------------------
        // GET LIST ENTRIES
        // ---------------------------------------------------

        auto entries =
            ListManager::get()->getEntriesForLevel(levelId);

        // ---------------------------------------------------
        // DEBUG TEST BADGE
        // Uncomment this if badges still don't show
        // ---------------------------------------------------

        /*
        entries.push_back({
            "test",
            "TEST",
            "#FF0000",
            1,
            "Test"
        });
        */

        if (entries.empty()) {
            return;
        }

        bool showRank =
            Mod::get()->getSettingValue<bool>("show-rank");

        // ---------------------------------------------------
        // CREATE CONTAINER
        // ---------------------------------------------------

        auto* container =
            ListBadgeContainer::create(entries, showRank);

        if (!container) {
            log::error(
                "[ListsIntegrations] Failed to create badge container"
            );
            return;
        }

        container->setTag(BADGE_TAG);

        // ---------------------------------------------------
        // FORCE TOP RIGHT POSITION
        // ---------------------------------------------------

        container->setAnchorPoint({ 0.f, 1.f });

        float x =
            cell->getContentSize().width -
            container->getContentWidth() -
            6.f;

        float y =
            cell->getContentSize().height -
            6.f;

        container->setPosition({ x, y });

        // ---------------------------------------------------
        // HIGH Z ORDER
        // ---------------------------------------------------

        cell->addChild(container, 999);

        // ---------------------------------------------------
        // DEBUG LOG
        // ---------------------------------------------------

        log::info(
            "[ListsIntegrations] Added badge to level {} at ({}, {})",
            levelId,
            x,
            y
        );
    }
};

// -------------------------------------------------------
// LEVEL BROWSER HOOK
// -------------------------------------------------------

struct ListsIntegrationsLevelBrowserHook :
    Modify<ListsIntegrationsLevelBrowserHook, LevelBrowserLayer> {

    bool init(GJSearchObject* searchObject) {
        if (!LevelBrowserLayer::init(searchObject)) {
            return false;
        }

        log::info(
            "[ListsIntegrations] LevelBrowserLayer initialized"
        );

        // ---------------------------------------------------
        // FETCH LISTS
        // ---------------------------------------------------

        ListManager::get()->fetchAllLists();

        // ---------------------------------------------------
        // UPDATE CALLBACK
        // ---------------------------------------------------

        ListManager::get()->onListUpdated(
            [this](const std::string& id) {
                log::info(
                    "[ListsIntegrations] List updated: {}",
                    id
                );

                refreshBadges();
            }
        );

        return true;
    }

    void refreshBadges() {
        auto* listLayer =
            this->getChildByType<GJListLayer>(0);

        if (!listLayer) {
            log::warn(
                "[ListsIntegrations] No GJListLayer found"
            );
            return;
        }

        auto* table =
            listLayer->getChildByType<TableView>(0);

        if (!table) {
            log::warn(
                "[ListsIntegrations] No TableView found"
            );
            return;
        }

        auto* children = table->getChildren();

        if (!children) {
            log::warn(
                "[ListsIntegrations] TableView has no children"
            );
            return;
        }

        int refreshed = 0;

        for (auto* node : CCArrayExt<CCNode*>(children)) {
            auto* cell =
                typeinfo_cast<LevelCell*>(node);

            if (!cell) {
                continue;
            }

            ListsIntegrationsLevelCellHook::addListBadges(cell);

            refreshed++;
        }

        log::info(
            "[ListsIntegrations] Refreshed {} cells",
            refreshed
        );
    }
};
