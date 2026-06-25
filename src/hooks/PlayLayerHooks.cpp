#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "ShadowManager.hpp"

using namespace geode::prelude;

class $modify(ShadowPLHook, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (ShadowManager::get().m_creatingShadow) {
            return PlayLayer::init(level, useReplay, dontCreateObjects);
        }

        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        ShadowManager::get().m_levelActive = true;

        Loader::get()->queueInMainThread([level]() {
            ShadowManager::get().createShadow(level);
        });

        return true;
    }

    void onQuit() {
        ShadowManager::get().destroyShadow();
        ShadowManager::get().m_levelActive = false;
        PlayLayer::onQuit();
    }
};