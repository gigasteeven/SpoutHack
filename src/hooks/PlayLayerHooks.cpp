#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <cmath>

using namespace geode::prelude;

static Ref<PlayLayer> g_shadow = nullptr;
static bool g_creatingShadow = false;
static bool g_levelActive = false;
static CCRenderTexture* g_rt = nullptr;
static CCSprite* g_preview = nullptr;

static void createShadow(GJGameLevel* level) {
    if (g_shadow) return;
    if (!level) { log::error("createShadow: level == null."); return; }

    log::info("createShadow: создаю shadow...");
    auto gm = GameManager::get();
    PlayLayer* primary = gm->m_playLayer;

    g_creatingShadow = true;
    PlayLayer* shadow = PlayLayer::create(level, false, false);
    g_creatingShadow = false;

    if (!shadow) { log::error("createShadow: null."); return; }

    g_shadow = shadow;
    gm->m_playLayer = primary;
    shadow->pauseSchedulerAndActions();
    log::info("createShadow: shadow готов.");
}

static void destroyShadow() {
    if (g_preview) { g_preview->removeFromParent(); g_preview = nullptr; }
    if (g_rt) { g_rt->release(); g_rt = nullptr; }
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

    void handleButton(bool down, int button, bool isPlayerOne) {
        PlayLayer::handleButton(down, button, isPlayerOne);
        if (this != g_shadow && g_shadow) {
            g_shadow->handleButton(down, button, isPlayerOne);
        }
    }

    void update(float dt) {
        if (this == g_shadow) { PlayLayer::update(dt); return; }

        PlayLayer::update(dt);
        if (!g_shadow) return;

        g_shadow->update(dt);

        // Рендер shadow в текстуру.
        auto win = CCDirector::sharedDirector()->getWinSize();
        if (!g_rt) {
            g_rt = CCRenderTexture::create((int)win.width, (int)win.height);
            if (g_rt) g_rt->retain();
        }
        if (g_rt) {
            g_rt->beginWithClear(0.f, 0.f, 0.f, 1.f);
            g_shadow->visit();
            g_rt->end();

            // Предпросмотр в правом верхнем углу.
            if (!g_preview) {
                g_preview = CCSprite::createWithTexture(g_rt->getSprite()->getTexture());
                g_preview->setFlipY(true);
                g_preview->setScale(0.3f);
                g_preview->setAnchorPoint({1.f, 1.f});
                g_preview->setPosition({win.width, win.height});
                this->addChild(g_preview, 999999);
            }
        }
    }

    void onQuit() {
        destroyShadow();
        g_levelActive = false;
        PlayLayer::onQuit();
    }
};