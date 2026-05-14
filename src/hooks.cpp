#include <Geode/Geode.hpp>
#include <Geode/modify/LevelCell.hpp>
#include <Geode/modify/LevelBrowserLayer.hpp>

#include "ListManager.hpp"
#include "ListBadgeNode.hpp"

using namespace geode::prelude;

static constexpr int BADGE_TAG = 0x4C495354; // "LIST"

template <class F>
static void visitNodes(CCNode* node, F&& fn) {
    if (!node) return;
    fn(node);

    auto children = node->getChildren();
    if (!children) return;

    for (auto* child : CCArrayExt<CCNode*>(children)) {
        visitNodes(child, fn);
    }
}

static int getLevelId(LevelCell* cell) {
    if (!cell || !cell->m_level) return -1;
    return cell->m_level->m_levelID.value();
}

static CCPoint getBadgePosition(LevelCell* cell, CCNode* badge) {
    auto setting = Mod::get()->getSettingValue<std::string>("badge-position");
    auto cellSize = cell->getContentSize();

    float bw = badge->getContentWidth();
    float bh = badge->getContentHeight();
    constexpr float margin = 4.f;

    if (setting == "Top Left") {
        return { margin + bw / 2.f, cellSize.height - margin - bh / 2.f };
    }
    if (setting == "Bottom Left") {
        return { margin + bw / 2.f, margin + bh / 2.f };
    }
    if (setting == "Bottom Right") {
        return { cellSize.width - margin - bw / 2.f, margin + bh / 2.f };
    }

    return { cellSize.width - margin - bw / 2.f, cellSize.height - margin - bh / 2.f };
}

struct ListsIntegrationsLevelCellHook : Modify<ListsIntegrationsLevelCellHook, LevelCell> {
    void loadCustomLevelCell() {
        LevelCell::loadCustomLevelCell();
        addListBadges(this);
    }

    static void addListBadges(LevelCell* cell) {
        if (!cell) return;
        if (!Mod::get()->getSettingValue<bool>("show-badges")) return;

        int levelId = getLevelId(cell);
        if (levelId <= 0) return;

        if (auto* old = cell->getChildByTag(BADGE_TAG)) {
            old->removeFromParent();
        }

        auto entries = ListManager::get()->getEntriesForLevel(levelId);
        if (entries.empty()) return;

        bool showRank = Mod::get()->getSettingValue<bool>("show-rank");

        auto* container = ListBadgeContainer::create(entries, showRank);
        if (!container) return;

        container->setTag(BADGE_TAG);
        container->setAnchorPoint({ 0.f, 1.f });

        float x = cell->getContentSize().width - container->getContentWidth() - 6.f;
        float y = cell->getContentSize().height - 6.f;

        container->setPosition({ x, y });
        cell->addChild(container, 999);
    }
};

struct ListsIntegrationsLevelBrowserHook : Modify<ListsIntegrationsLevelBrowserHook, LevelBrowserLayer> {
    bool init(GJSearchObject* searchObject) {
        if (!LevelBrowserLayer::init(searchObject)) {
            return false;
        }

        ListManager::get()->fetchAllLists();

        ListManager::get()->onListUpdated([this](const std::string&) {
            refreshBadges();
        });

        return true;
    }

    void refreshBadges() {
        int refreshed = 0;

        visitNodes(this, [&](CCNode* node) {
            if (auto* cell = typeinfo_cast<LevelCell*>(node)) {
                ListsIntegrationsLevelCellHook::addListBadges(cell);
                refreshed++;
            }
        });

        log::info("[ListsIntegrations] Refreshed {} cells", refreshed);
    }
};
