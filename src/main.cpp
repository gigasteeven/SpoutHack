#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;

class $modify(LSPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects))
            return false;

        if (Mod::get()->getSettingValue<bool>("enabled")) {
            log::info("Layout Stream: active on this level");
            // Phase 2 hook point: enable layout rendering here
        }
        return true;
    }

    void onQuit() {
        log::info("Layout Stream: leaving level, restoring normal render");
        // Phase 2: disable layout rendering / cleanup
        PlayLayer::onQuit();
    }
};

$on_mod(Loaded) {
    log::info("Layout Stream loaded");
}