#include "ShadowManager.hpp"

using namespace geode::prelude;

ShadowManager& ShadowManager::get() {
    static ShadowManager instance;
    return instance;
}

void ShadowManager::createShadow(GJGameLevel* level) {
    if (m_shadow) {
        log::warn("Shadow уже существует — пропускаю создание.");
        return;
    }
    if (!level) {
        log::error("createShadow: level == null.");
        return;
    }

    log::info("createShadow: пробую создать второй PlayLayer...");

    m_creatingShadow = true;

    // Создаём второй PlayLayer того же уровня.
    // НЕ добавляем его в сцену — он живёт offscreen.
    PlayLayer* shadow = PlayLayer::create(level, false, false);

    m_creatingShadow = false;

    if (!shadow) {
        log::error("createShadow: PlayLayer::create вернул null.");
        return;
    }

    // Ref ретейнит, чтобы объект не освободился.
    m_shadow = shadow;

    log::info("createShadow: shadow создан успешно -> {}", static_cast<void*>(shadow));
}

void ShadowManager::destroyShadow() {
    if (!m_shadow) return;

    log::info("destroyShadow: уничтожаю shadow PlayLayer.");

    // На всякий случай вынимаем из родителя, если вдруг где-то добавился.
    if (m_shadow->getParent()) {
        m_shadow->removeFromParent();
    }

    // Сброс Ref -> release.
    m_shadow = nullptr;
}