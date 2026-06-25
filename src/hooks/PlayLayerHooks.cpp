#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

// Состояние прямо здесь, без отдельного header.
static Ref<PlayLayer> g_shadow = nullptr;
static bool g_creatingShadow = false;
static bool g_levelActive = false;

static void createShadow(GJGameLevel* level) {
    if (g_shadow) {
        log::warn("Shadow уже существует — пропускаю.");
        return;
    }
    if (!level) {
        log::error("createShadow: level == null.");
        return;
    }

    log::info("createShadow: пробую создать второй PlayLayer...");

    g_creatingShadow = true;
    PlayLayer* shadow = PlayLayer::create(level, false, false);
    g_creatingShadow = false;

    if (!shadow) {
        log::error("createShadow: PlayLayer::create вернул null.");
        return;
    }

    g_shadow = shadow;
    log::info("createShadow: shadow создан -> {}", static_cast<void*>(shadow));
}

static void destroyShadow() {
    if (!g_shadow) return;
    log::info("destroyShadow: уничтожаю shadow PlayLayer.");
    if (g_shadow->getParent()) {
        g_shadow->removeFromParent();
    }
    g_shadow = nullptr;
}

class $modify(ShadowPLHook, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        // Если сейчас создаётся SHADOW — просто ванильный init.
        if (g_creatingShadow) {
            return PlayLayer::init(level, useReplay, dontCreateObjects);
        }

        // PRIMARY-слой.
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }

        g_levelActive = true;

        // Создание shadow на следующий кадр.
        Loader::get()->queueInMainThread([level]() {
            createShadow(level);
        });

        return true;
    }

    void onQuit() {
        destroyShadow();
        g_levelActive = false;
        PlayLayer::onQuit();
    }
};