#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

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

    auto gm = GameManager::get();
    PlayLayer* primary = gm->m_playLayer;
    log::info("createShadow: primary = {}", static_cast<void*>(primary));

    g_creatingShadow = true;
    PlayLayer* shadow = PlayLayer::create(level, false, false);
    g_creatingShadow = false;

    if (!shadow) {
        log::error("createShadow: PlayLayer::create вернул null.");
        return;
    }

    g_shadow = shadow;
    gm->m_playLayer = primary;
    shadow->pauseSchedulerAndActions();

    log::info("createShadow: shadow создан, заморожен, синглтон восстановлен -> {}",
        static_cast<void*>(shadow));
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
        if (g_creatingShadow) {
            return PlayLayer::init(level, useReplay, dontCreateObjects);
        }
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            return false;
        }
        g_levelActive = true;
        Loader::get()->queueInMainThread([level]() {
            createShadow(level);
        });
        return true;
    }

    void update(float dt) {
        // Если это сам shadow — обычный update (на всякий случай).
        if (this == g_shadow) {
            PlayLayer::update(dt);
            return;
        }

        // PRIMARY: сначала обновляем его как обычно.
        PlayLayer::update(dt);

        // Затем тем же dt вручную шагаем shadow.
        if (g_shadow) {
            log::debug("shadow update: до вызова");
            g_shadow->update(dt);
            log::debug("shadow update: после вызова");

            if (this->m_player1 && g_shadow->m_player1) {
                auto a = this->m_player1->getPosition();
                auto b = g_shadow->m_player1->getPosition();
                log::debug("primary=({:.1f},{:.1f}) shadow=({:.1f},{:.1f})",
                    a.x, a.y, b.x, b.y);
            }
        }
    }

    void onQuit() {
        destroyShadow();
        g_levelActive = false;
        PlayLayer::onQuit();
    }
};