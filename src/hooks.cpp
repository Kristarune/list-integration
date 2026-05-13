#include <Geode/Geode.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>

#include "ListManager.hpp"
#include "ListBadgeNode.hpp"

using namespace geode::prelude;

// Tag used to find/remove badge containers we add
static constexpr int BADGE_TAG = 0x4C495354; // "LIST"

// -------------------------------------------------------
// Helper: get the GD level ID from a LevelCell
// -------------------------------------------------------
static int getLevelId(LevelCell* cell) {
    if (!cell) return -1;

    if (auto* lvl = cell->m_level) {
        return lvl->m_levelID.value();
    }

    return -1;
}

// -------------------------------------------------------
// Helper: compute badge anchor position
// -------------------------------------------------------
static CCPoint getBadgePosition(LevelCell* cell, CCNode* badge) {
    auto* mod = Mod::get();

    auto setting = mod->getSettingValue<std::string>("badge-position");

    auto cellSize = cell->getContentSize();

    float bw = badge->getContentWidth();
    float bh = badge->getContentHeight();

    constexpr float margin = 4.f;

    if (setting == "Top Left") {
        return {
            margin + bw / 2.f,
            cellSize.height - margin - bh / 2.f
        };
    }

    if (setting == "Bottom Left") {
        return {
            margin + bw / 2.f,
            margin + bh / 2.f
        };
    }

    if (setting == "Bottom Right") {
        return {
            cellSize.width - margin - bw / 2.f,
            margin + bh / 2.f
        };
    }

    // Default: Top Right
    return {
        cellSize.width - margin - bw / 2.f,
        cellSize.height - margin - bh / 2.f
    };
}

// -------------------------------------------------------
// LevelCell hook
// -------------------------------------------------------
struct ListsIntegrationsLevelCellHook :
    Modify<ListsIntegrationsLevelCellHook, LevelCell> {

    void loadCustomLevelCell() {
        LevelCell::loadCustomLevelCell();

        addListBadges(this);
    }

    static void addListBadges(LevelCell* cell) {
        if (!cell) return;

        if (!Mod::get()->getSettingValue<bool>("show-badges")) {
            return;
        }

        int levelId = getLevelId(cell);

        if (levelId <= 0) {
            return;
        }

        // Remove old badge container
        if (auto* old = cell->getChildByTag(BADGE_TAG)) {
            old->removeFromParent();
        }

        auto entries = ListManager::get()->getEntriesForLevel(levelId);

        if (entries.empty()) {
            return;
        }

        bool showRank =
            Mod::get()->getSettingValue<bool>("show-rank");

        auto* container =
            ListBadgeContainer::create(entries, showRank);

        if (!container) {
            return;
        }

        container->setTag(BADGE_TAG);

        CCPoint pos = getBadgePosition(cell, container);

        container->setPosition(
            pos - CCPoint{
                container->getContentWidth() / 2.f,
                container->getContentHeight() / 2.f
            }
        );

        // Add above cell contents
        cell->addChild(container, 10);
    }
};

// -------------------------------------------------------
// LevelBrowserLayer hook
// -------------------------------------------------------
struct ListsIntegrationsLevelBrowserHook :
    Modify<ListsIntegrationsLevelBrowserHook, LevelBrowserLayer> {

    bool init(GJSearchObject* searchObject) {
        if (!LevelBrowserLayer::init(searchObject)) {
            return false;
        }

        // Fetch all lists
        ListManager::get()->fetchAllLists();

        // Refresh badges when list updates
        ListManager::get()->onListUpdated(
            [this](const std::string&) {
                refreshBadges();
            }
        );

        return true;
    }

    void refreshBadges() {
        auto* listLayer =
            this->getChildByType<GJListLayer>(0);

        if (!listLayer) {
            return;
        }

        auto* table =
            listLayer->getChildByType<TableView>(0);

        if (!table) {
            return;
        }

        auto* children = table->getChildren();

        if (!children) {
            return;
        }

        for (auto* node : CCArrayExt<CCNode*>(children)) {
            auto* cell = typeinfo_cast<LevelCell*>(node);

            if (!cell) {
                continue;
            }

            ListsIntegrationsLevelCellHook::addListBadges(cell);
        }
    }
};
