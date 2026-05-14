#include <Geode/Geode.hpp>
#include <Geode/modify/LevelCell.hpp>
#include "ListManager.hpp"
#include "ListBadgeNode.hpp"

using namespace geode::prelude;

static constexpr int BADGE_TAG = 0x4C495354;

// =====================================================
// Helper
// =====================================================

static int getLevelID(LevelCell* cell) {
    if (!cell) return -1;

    auto* level = cell->m_level;
    if (!level) return -1;

    return level->m_levelID.value();
}

static CCPoint getBadgePos(LevelCell* cell, CCNode* badge) {
    auto size = cell->getContentSize();

    float bw = badge->getContentWidth();
    float bh = badge->getContentHeight();

    constexpr float margin = 6.f;

    return {
        size.width - bw / 2.f - margin,
        size.height - bh / 2.f - margin
    };
}

// =====================================================
// Main Badge Adder
// =====================================================

static void addBadges(LevelCell* cell) {
    if (!cell) return;

    auto* mod = Mod::get();
    if (!mod) return;

    if (!mod->getSettingValue<bool>("show-badges"))
        return;

    int levelID = getLevelID(cell);

    if (levelID <= 0)
        return;

    // Remove old badges
    if (auto* old = cell->getChildByTag(BADGE_TAG)) {
        old->removeFromParent();
    }

    auto entries = ListManager::get()->getEntriesForLevel(levelID);

    if (entries.empty()) {
        log::debug("No badge for level {}", levelID);
        return;
    }

    bool showRank = mod->getSettingValue<bool>("show-rank");

    auto* container = ListBadgeContainer::create(entries, showRank);

    if (!container)
        return;

    container->setTag(BADGE_TAG);

    auto pos = getBadgePos(cell, container);

    container->setAnchorPoint({ 0.5f, 0.5f });
    container->setPosition(pos);

    cell->addChild(container, 999);

    log::info(
        "[ListsIntegrations] Added {} badges to level {}",
        entries.size(),
        levelID
    );
}

// =====================================================
// Hook
// =====================================================

class $modify(ListsLevelCell, LevelCell) {

    void loadFromObject(GJGameLevel* level) {
        LevelCell::loadFromObject(level);

        addBadges(this);
    }

    void updateBGColor(int idx) {
        LevelCell::updateBGColor(idx);

        addBadges(this);
    }

    void loadCustomLevelCell() {
        LevelCell::loadCustomLevelCell();

        addBadges(this);
    }
};
