#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/LevelTools.hpp>

#include "layout_mode.hpp"
#include <SpoutLibrary.h>

using namespace geode::prelude;

// Флаг для предотвращения бесконечной рекурсии
static bool g_isCreatingShadow = false;

class $modify(LevelTools) {
    static bool verifyLevelIntegrity(gd::string v1, int v2) {
        // Пропускаем проверку целостности только для PRIMARY слоя (когда не создаем shadow)
        if (!g_isCreatingShadow) {
            return true;
        }
        return LevelTools::verifyLevelIntegrity(v1, v2);
    }
};

class $modify(DualPlayLayer, PlayLayer) {
    struct Fields {
        Ref<PlayLayer> m_shadowLayer = nullptr; 
        bool m_isShadow = false;
        SPOUTLIBRARY* m_spout = nullptr;
        Ref<CCRenderTexture> m_renderTexture = nullptr;
        
        // Разрешение для OBS
        int m_spoutWidth = 1920;
        int m_spoutHeight = 1080;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (g_isCreatingShadow) {
            m_fields->m_isShadow = true;
            return PlayLayer::init(level, useReplay, dontCreateObjects);
        }

        // --- ИНИЦИАЛИЗАЦИЯ PRIMARY СЛОЯ ---
        // Модифицируем строку уровня перед загрузкой (Layout Mode)
        std::string oldString = level->m_levelString;
        level->m_levelString = LayoutMode::getModifiedString(level->m_levelString);

        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) {
            level->m_levelString = oldString;
            return false;
        }

        level->m_levelString = oldString;
        log::info("Primary PlayLayer created. Generating Shadow layer...");

        // --- СОЗДАНИЕ SHADOW СЛОЯ ---
        g_isCreatingShadow = true;
        auto shadowLayer = PlayLayer::create(level, useReplay, dontCreateObjects);
        g_isCreatingShadow = false;

        if (shadowLayer) {
            m_fields->m_shadowLayer = shadowLayer;
            
            // Возвращаем синглтоны
            auto gm = GameManager::sharedState();
            gm->m_playLayer = this;
            gm->m_gameLayer = this;
            
            // Отключаем автоматический апдейт теневого слоя
            shadowLayer->unscheduleUpdate();

            // Синхронизируем начальный random seed
            m_fields->m_shadowLayer->m_randomSeed = this->m_randomSeed;
            
            log::info("Shadow layer generated.");

            // Инициализация Spout2
            m_fields->m_spout = GetSpout();
            if (m_fields->m_spout) {
                m_fields->m_spout->SetSenderName("GD Dual Layout Mode");
                log::info("Spout2 initialized.");
            } else {
                log::error("Failed to initialize Spout2!");
            }

            // Создаем offscreen текстуру
            m_fields->m_renderTexture = CCRenderTexture::create(m_fields->m_spoutWidth, m_fields->m_spoutHeight);
            
            // Чтобы слой отрендерился правильно, масштабируем его под текстуру, если нужно
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            float scaleX = m_fields->m_spoutWidth / winSize.width;
            float scaleY = m_fields->m_spoutHeight / winSize.height;
            shadowLayer->setScaleX(scaleX);
            shadowLayer->setScaleY(scaleY);
        }

        return true;
    }

    void addObject(GameObject* obj) {
        if (!m_fields->m_isShadow) {
            // Применяем логику XDBot Layout Mode для PRIMARY
            if (excludedTriggerIDs.contains(obj->m_objectID)) return; 
            
            PlayLayer::addObject(obj);
            
            obj->m_activeMainColorID = -1;
            obj->m_activeDetailColorID = -1;
            obj->m_detailUsesHSV = false;
            obj->m_baseUsesHSV = false;
            obj->m_hasNoGlow = true;
            obj->m_isHide = obj->m_objectID == 2065;
            obj->setOpacity(obj->m_objectID == 2065 ? 0 : 255);
            obj->setVisible(obj->m_objectID != 2065);
        } else {
            // Нормальная логика для SHADOW (всё остается с декорациями)
            PlayLayer::addObject(obj);
        }
    }

    // --- СИНХРОНИЗАЦИЯ ТАЙМИНГОВ И ФИЗИКИ ---
    void update(float dt) {
        if (m_fields->m_isShadow) {
            return PlayLayer::update(dt);
        }

        PlayLayer::update(dt);

        if (m_fields->m_shadowLayer) {
            // Жесткий лок-степ: вызываем update с тем же самым dt
            m_fields->m_shadowLayer->update(dt);

            // Отладка рассинхрона (только если нужно)
            /*
            auto p1 = this->m_player1;
            auto s1 = m_fields->m_shadowLayer->m_player1;
            if (p1 && s1 && (p1->getPositionX() != s1->getPositionX() || p1->getPositionY() != s1->getPositionY())) {
                log::warn("Desync detected! P1: ({}, {}), S1: ({}, {})", p1->getPositionX(), p1->getPositionY(), s1->getPositionX(), s1->getPositionY());
            }
            */
        }
    }

    // --- СИНХРОНИЗАЦИЯ ИНПУТОВ ---
    void pushButton(int btn, bool p2) {
        if (m_fields->m_isShadow) {
            return PlayLayer::pushButton(btn, p2);
        }

        PlayLayer::pushButton(btn, p2);

        if (m_fields->m_shadowLayer) {
            m_fields->m_shadowLayer->pushButton(btn, p2);
        }
    }

    void releaseButton(int btn, bool p2) {
        if (m_fields->m_isShadow) {
            return PlayLayer::releaseButton(btn, p2);
        }

        PlayLayer::releaseButton(btn, p2);

        if (m_fields->m_shadowLayer) {
            m_fields->m_shadowLayer->releaseButton(btn, p2);
        }
    }

    // --- РЕНДЕР И ОТПРАВКА В SPOUT ---
    void visit() {
        if (m_fields->m_isShadow) {
            PlayLayer::visit();
            return;
        }

        // Рисуем основной слой на экран
        PlayLayer::visit();

        // Рендерим теневой слой в текстуру
        if (m_fields->m_shadowLayer && m_fields->m_renderTexture) {
            m_fields->m_renderTexture->beginWithClear(0, 0, 0, 1);
            
            m_fields->m_shadowLayer->visit();
            
            // Сюда можно добавить рисование индикаторов (FPS, текст),
            // вручную вызывая visit() нужных CCNode.

            m_fields->m_renderTexture->end();

            // Отправляем в Spout
            if (m_fields->m_spout) {
                auto tex = m_fields->m_renderTexture->getSprite()->getTexture();
                m_fields->m_spout->SendTexture(
                    tex->getName(),
                    GL_TEXTURE_2D,
                    m_fields->m_spoutWidth,
                    m_fields->m_spoutHeight,
                    true, 0 // true = invert, чтобы перевернуть OpenGL текстуру
                );
            }
        }
    }

    void onQuit() {
        if (m_fields->m_isShadow) {
            PlayLayer::onQuit();
            return;
        }

        if (m_fields->m_spout) {
            m_fields->m_spout->Release();
            m_fields->m_spout = nullptr;
        }

        if (m_fields->m_shadowLayer) {
            m_fields->m_shadowLayer = nullptr;
        }

        PlayLayer::onQuit();
    }
};
