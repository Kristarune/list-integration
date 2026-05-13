#pragma once

#include <Geode/Geode.hpp>
#include <Geode/cocos/extensions/GUI/CCControlExtension/CCScale9Sprite.h>
#include <cstdio>

#include "ListManager.hpp"

using namespace geode::prelude;

// A small badge node that displays list name + rank
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
        if (!CCNode::init()) {
            return false;
        }

        // Parse hex color
        ccColor3B color = { 255, 255, 255 };

        if (entry.listColor.size() >= 7 && entry.listColor[0] == '#') {
            unsigned int r = 255;
            unsigned int g = 255;
            unsigned int b = 255;

            std::sscanf(
                entry.listColor.c_str() + 1,
                "%02x%02x%02x",
                &r,
                &g,
                &b
            );

            color = {
                static_cast<GLubyte>(r),
                static_cast<GLubyte>(g),
                static_cast<GLubyte>(b)
            };
        }

        // Background
        auto* bg = CCScale9Sprite::create(
            "square02_small.png",
            CCRectMake(0.f, 0.f, 40.f, 40.f)
        );

        if (!bg) {
            return false;
        }

        bg->setColor(color);
        bg->setOpacity(210);

        std::string label = entry.listName;

        if (showRank && entry.position > 0) {
            label += " #" + std::to_string(entry.position);
        }

        auto* text = CCLabelBMFont::create(
            label.c_str(),
            "bigFont.fnt"
        );

        if (!text) {
            return false;
        }

        text->setScale(0.28f);
        text->setColor({ 255, 255, 255 });
        text->setAnchorPoint(CCPointMake(0.5f, 0.5f));

        float w = text->getContentWidth() * text->getScale() + 8.f;
        float h = 12.f;

        bg->setContentSize(
            CCSizeMake(
                w / bg->getScale() + 4.f,
                h / bg->getScale()
            )
        );

        bg->setScale(1.f);
        bg->setPosition(CCPointMake(w / 2.f, h / 2.f));

        text->setPosition(CCPointMake(w / 2.f, h / 2.f));

        this->addChild(bg);
        this->addChild(text);

        this->setContentSize(CCSizeMake(w, h));

        return true;
    }
};

// Holds multiple stacked badges
class ListBadgeContainer : public CCNode {
public:
    static ListBadgeContainer* create(
        const std::vector<ListEntry>& entries,
        bool showRank
    ) {
        auto* node = new ListBadgeContainer();

        if (node && node->init(entries, showRank)) {
            node->autorelease();
            return node;
        }

        CC_SAFE_DELETE(node);
        return nullptr;
    }

    bool init(
        const std::vector<ListEntry>& entries,
        bool showRank
    ) {
        if (!CCNode::init()) {
            return false;
        }

        float yOffset = 0.f;
        float maxW = 0.f;

        constexpr float gap = 2.f;

        for (
            int i = static_cast<int>(entries.size()) - 1;
            i >= 0;
            --i
        ) {
            auto* badge = ListBadgeNode::create(
                entries[i],
                showRank
            );

            if (!badge) {
                continue;
            }

            badge->setPosition(CCPointMake(0.f, yOffset));

            this->addChild(badge);

            yOffset += badge->getContentHeight() + gap;

            if (badge->getContentWidth() > maxW) {
                maxW = badge->getContentWidth();
            }
        }

        this->setContentSize(
            CCSizeMake(maxW, yOffset)
        );

        return true;
    }
};
