#pragma once
#include <Geode/Geode.hpp>
#include "ListManager.hpp"

using namespace geode::prelude;

// A small badge node that displays list name + rank
// Attach this to a LevelCell or similar node
class ListBadgeNode : public CCNode {
public:
    static ListBadgeNode* create(const ListEntry& entry, bool showRank) {
        auto* node = new ListBadgeNode();
        if (node && node->init(entry, showRank)) {
            node->autorelease();
            return node;
        }
        CC_SAFE_DELETE(node);
        return nullptr;
    }

    bool init(const ListEntry& entry, bool showRank) {
        if (!CCNode::init()) return false;

        // Parse the hex color
        ccColor3B color = { 255, 255, 255 };
        if (entry.listColor.size() >= 7 && entry.listColor[0] == '#') {
            unsigned int r, g, b;
            sscanf(entry.listColor.c_str() + 1, "%02x%02x%02x", &r, &g, &b);
            color = { (GLubyte)r, (GLubyte)g, (GLubyte)b };
        }

        // Background pill
        auto* bg = CCScale9Sprite::create("square02_small.png", { 0,0,40,40 });
        bg->setColor(color);
        bg->setOpacity(210);

        std::string label = entry.listName;
        if (showRank && entry.position > 0) {
            label += " #" + std::to_string(entry.position);
        }

        auto* text = CCLabelBMFont::create(label.c_str(), "bigFont.fnt");
        text->setScale(0.28f);
        text->setColor({ 255, 255, 255 });
        text->setAnchorPoint({ 0.5f, 0.5f });

        float w = text->getContentWidth() * text->getScale() + 8.f;
        float h = 12.f;

        bg->setContentSize({ w / bg->getScale() + 4.f, h / bg->getScale() });
        bg->setScale(1.f);
        bg->setPosition({ w / 2.f, h / 2.f });

        text->setPosition({ w / 2.f, h / 2.f });

        this->addChild(bg);
        this->addChild(text);
        this->setContentSize({ w, h });

        return true;
    }
};

// Container that holds multiple ListBadgeNodes stacked vertically
class ListBadgeContainer : public CCNode {
public:
    static ListBadgeContainer* create(const std::vector<ListEntry>& entries, bool showRank) {
        auto* node = new ListBadgeContainer();
        if (node && node->init(entries, showRank)) {
            node->autorelease();
            return node;
        }
        CC_SAFE_DELETE(node);
        return nullptr;
    }

    bool init(const std::vector<ListEntry>& entries, bool showRank) {
        if (!CCNode::init()) return false;

        float yOffset = 0.f;
        float maxW = 0.f;
        constexpr float gap = 2.f;

        // Reverse so first list = top badge
        for (int i = (int)entries.size() - 1; i >= 0; i--) {
            auto* badge = ListBadgeNode::create(entries[i], showRank);
            if (!badge) continue;
            badge->setPosition({ 0.f, yOffset });
            this->addChild(badge);
            yOffset += badge->getContentHeight() + gap;
            if (badge->getContentWidth() > maxW) maxW = badge->getContentWidth();
        }

        this->setContentSize({ maxW, yOffset });
        return true;
    }
};
