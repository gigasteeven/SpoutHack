#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include "../ShadowManager.hpp"

using namespace geode::prelude;

class $modify(ShadowPLHook, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        // Если сейчас создаётся SHADOW — просто ванильный init, без нашей логики.
        if (ShadowManager::get().m_creatingShadow) {
            return PlayLayer::init(level, useReplay, dontCreateObjects);
        }

        // Это PRIMARY-слой.
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        ShadowManager::get().m_levelActive = true;

        // Создание shadow откладываем на следующий кадр,
        // чтобы primary успел полностью инициализироваться.
        Loader::get()->queueInMainThread([level]() {
            ShadowManager::get().createShadow(level);
        });

        return true;
    }

    void onQuit() {
        // При выходе из уровня убираем shadow.
        ShadowManager::get().destroyShadow();
        ShadowManager::get().m_levelActive = false;
        PlayLayer::onQuit();
    }
};