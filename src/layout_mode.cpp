#include "layout_mode.hpp"

using namespace geode::prelude;

std::string LayoutMode::getModifiedString(std::string levelString) {
    if (levelString.empty()) return "";

    std::string decompString = ZipUtils::decompressString(levelString.c_str(), true, 0);

    std::vector<std::string> objectStrings = geode::utils::string::split(decompString, ";");
    if (objectStrings.empty()) return levelString;
    std::string firstPart = objectStrings[0];

    std::vector<std::string> levelSettings = geode::utils::string::split(firstPart, ",");
    if (levelSettings.size() < 2) return levelString;

    std::vector<std::string> levelColors = geode::utils::string::split(levelSettings[1], "|");

    if (levelColors.size() >= 6)
        levelColors.erase(levelColors.begin(), levelColors.begin() + 6);

    levelSettings[1] = newColors;
    firstPart = mergeVector(levelSettings);

    std::unordered_set<int> importantGroups;

    for (size_t i = 0; i < levelSettings.size(); i++) {
        if (levelSettings[i] == "kA36" && i + 1 < levelSettings.size()) {
            if (std::stoi(levelSettings[i + 1]) != 0)
                importantGroups.insert(std::stoi(levelSettings[i + 1]));
            break;
        }
    }

    std::vector<LevelObject> objects;

    for (size_t i = 1; i < objectStrings.size(); i++) {
        std::vector<std::string> result = geode::utils::string::split(objectStrings[i], ",");
        std::map<int, std::string> props;
        size_t hidden = 0;

        for (size_t j = 0; j + 1 < result.size(); j += 2) {
            auto propID_res = geode::utils::numFromString<int>(result[j]);
            if (propID_res.isErr()) continue;
            int propID = propID_res.unwrap();

            if (result[j] == "135") hidden = j;
            props[propID] = result[j + 1];
        }

        objects.push_back({ result, props, hidden, objectStrings[i] });

        if (!props.contains(1)) continue;
        int id = std::stoi(props.at(1));
        if (!importantTriggerIDs.contains(id)) continue;

        for (int propID : importantTriggerIDs.at(id)) {
            if (!props.contains(propID)) continue;
            if (std::stoi(props[propID]) != 0)
                importantGroups.insert(std::stoi(props[propID]));
        }
    }

    std::string newString = "";

    for (const LevelObject& obj : objects) {
        std::vector<std::string> vec = obj.vecString;
        std::map<int, std::string> props = obj.props;
        size_t hidden = obj.hiddenIndex;

        if (!props.contains(1)) continue;
        int id = std::stoi(props.at(1));

        if (decoObjectIDs.contains(id) || props.contains(121)) {
            if (!props.contains(57)) continue;

            bool kept = false;
            for (const auto& el : geode::utils::string::split(props.at(57), ".")) {
                if (el.empty()) continue;
                if (!importantGroups.contains(std::stoi(el))) continue;

                if (!props.contains(135)) {
                    vec.push_back("135");
                    vec.push_back("1");
                }
                newString += ";" + mergeVector(vec);
                kept = true;
                break;
            }
            if (kept) continue;
            continue;
        }

        if (!solidObjectIDs.contains(id)) {
            newString += ";" + obj.ogString;
            continue;
        }

        if (props.contains(129)) {
            if (std::stof(props.at(129)) <= 0.f && hidden == 0) {
                vec.push_back("135");
                vec.push_back("1");

                newString += ";" + mergeVector(vec);
                continue;
            }
        }

        if (hidden != 0 && hidden + 1 < vec.size()) {
            vec.erase(vec.begin() + hidden, vec.begin() + hidden + 2);
            newString += ";" + mergeVector(vec);
        } else {
            newString += ";" + obj.ogString;
        }
    }

    return firstPart + newString + ";";
}

std::string LayoutMode::mergeVector(std::vector<std::string> vec, std::string separator) {
    if (vec.empty()) return "";

    std::string result;
    size_t total_size = 0;
    for (const auto& s : vec) total_size += s.size() + separator.size();
    result.reserve(total_size);

    for (size_t i = 0; i < vec.size(); ++i) {
        result += vec[i];
        if (i != vec.size() - 1) result += separator;
    }
    return result;
}
