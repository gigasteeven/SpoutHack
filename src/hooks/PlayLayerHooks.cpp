#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <cmath>

using namespace geode::prelude;

static Ref<PlayLayer> g_shadow = nullptr;
static bool g_creatingShadow = false;
static bool g_levelActive = false;

static void createShadow(GJGameLevel* level) {
    if (g_shadow) return;
    if (!level) { log::error("createShadow: level == null."); return; }

    log::info("createShadow: создаю второй PlayLayer...");
    auto gm = GameManager::get();
    PlayLayer* primary = gm->m_playLayer;

    g_creatingShadow = true;
    PlayLayer* shadow = PlayLayer::create(level, false, false);
    g_creatingShadow = false;

    if (!shadow) { log::error("createShadow: вернул null."); return; }

    g_shadow = shadow;
    gm->m_playLayer = primary;
    shadow->pauseSchedulerAndActions();
    log::info("createShadow: shadow готов -> {}", static_cast<void*>(shadow));
}

static void destroyShadow() {
    if (!g_shadow) return;
    log::info("destroyShadow.");
    if (g_shadow->getParent()) g_shadow->removeFromParent();
    g_shadow = nullptr;
}

class $modify(ShadowPLHook, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (g_creatingShadow) {
            return PlayLayer::init(level, useReplay, dontCreateObjects);
        }
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        g_levelActive = true;
        Loader::get()->queueInMainThread([level]() { createShadow(level); });
        return true;
    }

    // Зеркалируем нажатия на shadow.
    void handleButton(bool down, int button, bool isPlayerOne) {
        PlayLayer::handleButton(down, button, isPlayerOne);
        if (this != g_shadow && g_shadow) {
            g_shadow->handleButton(down, button, isPlayerOne);
        }
    }

    void update(float dt) {
        if (this == g_shadow) { PlayLayer::update(dt); return; }

        PlayLayer::update(dt);

        if (g_shadow) {
            g_shadow->update(dt);
            if (this->m_player1 && g_shadow->m_player1) {
                auto a = this->m_player1->getPosition();
                auto b = g_shadow->m_player1->getPosition();
                if (std::abs(a.x - b.x) > 1.f || std::abs(a.y - b.y) > 1.f) {
                    log::warn("DESYNC primary=({:.1f},{:.1f}) shadow=({:.1f},{:.1f})",
                        a.x, a.y, b.x, b.y);
                }
            }
        }
    }

    void onQuit() {
        destroyShadow();
        g_levelActive = false;
        PlayLayer::onQuit();
    }
};