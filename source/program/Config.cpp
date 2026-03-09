#include "Config.hpp"
#include "toml.hpp"
#include "lib.hpp"
#include "fs.hpp"
#include <cstring>
#include <algorithm>
#include <set>

bool Config::enableDebugConsole = false;
std::string Config::modsDirectoryPath = "";
std::vector<std::string> Config::priorityPaths;

/**
 * Generates a default configuration file and synchronizes it with 
 * existing mod folders found on the SD card.
 */
static void SaveConfig(const std::string& path) {
    std::string tomlContent = "# DML Switch Port - Global Configuration\n";
    tomlContent += "enabled = true\n";
    tomlContent += "console = " + std::string(Config::enableDebugConsole ? "true" : "false") + "\n\n";
    tomlContent += "# Priority list (Top is highest priority).\n";
    tomlContent += "# New mods found on SD are automatically appended to the bottom of this list.\n";
    tomlContent += "priority = [\n";
    
    for (const auto& name : Config::priorityPaths) {
        tomlContent += "    \"" + name + "\",\n";
    }
    tomlContent += "]\n";

    // Re-create the file to ensure the size is updated correctly
    nn::fs::DeleteFile(path.c_str());
    nn::fs::CreateFile(path.c_str(), tomlContent.length());
    
    nn::fs::FileHandle h;
    if (R_SUCCEEDED(nn::fs::OpenFile(&h, path.c_str(), nn::fs::OpenMode_Write))) {
        nn::fs::WriteFile(h, 0, tomlContent.c_str(), tomlContent.length(), nn::fs::WriteOption::CreateOption(nn::fs::WriteOptionFlag_Flush));
        nn::fs::CloseFile(h);
    }
}

bool Config::init() {
    // 1. Identify active Title ID to determine the Atmosphere contents path
    const char* possible_tids[] = { 
        "0100F3100DA46000", // JP (Mega39s)
        "01001CC00FA1A000", // EN (MegaMix)
        "0100BE300FF62000"  // KR (Mega39s)
    };
    
    std::string atmoPath = "";
    nn::fs::DirectoryHandle dh;
    for (const char* tid : possible_tids) {
        std::string testPath = "ExlSD:/atmosphere/contents/" + std::string(tid);
        if (R_SUCCEEDED(nn::fs::OpenDirectory(&dh, testPath.c_str(), nn::fs::OpenDirectoryMode_Directory))) {
            nn::fs::CloseDirectory(dh);
            atmoPath = testPath;
            break; 
        }
    }

    if (atmoPath.empty()) return false;

    // Define mod directory path (within RomFS to leverage LayeredFS caching)
    modsDirectoryPath = atmoPath + "/romfs/mods";
    nn::fs::CreateDirectory(modsDirectoryPath.c_str());
    nn::fs::CreateDirectory("ExlSD:/DMLSwitchPort");

    std::string configPath = "ExlSD:/DMLSwitchPort/config.toml";
    nn::fs::FileHandle h;
    bool configExists = R_SUCCEEDED(nn::fs::OpenFile(&h, configPath.c_str(), nn::fs::OpenMode_Read));

    // 2. Load existing settings if the configuration file exists
    if (configExists) {
        int64_t size = 0;
        nn::fs::GetFileSize(&size, h);
        std::string content(size, '\0');
        nn::fs::ReadFile(h, 0, content.data(), size);
        nn::fs::CloseFile(h);

        auto result = toml::parse(content);
        if (result) {
            auto config = std::move(result).table();
            enableDebugConsole = config["console"].value_or(false);
            if (auto pArr = config["priority"].as_array()) {
                for (auto& el : *pArr) {
                    std::string val = el.value_or("");
                    if (!val.empty()) priorityPaths.push_back(val);
                }
            }
            // Master kill-switch
            if (!config["enabled"].value_or(true)) return false;
        }
    }

    // 3. Scan physical mod directory for new entries
    std::vector<std::string> modsOnDisk;
    if (R_SUCCEEDED(nn::fs::OpenDirectory(&dh, modsDirectoryPath.c_str(), nn::fs::OpenDirectoryMode_Directory))) {
        int64_t count = 0;
        nn::fs::DirectoryEntry entry;
        while (R_SUCCEEDED(nn::fs::ReadDirectory(&count, &entry, dh, 1)) && count > 0) {
            // Cast to int to prevent enum-compare warnings/errors
            if ((int)entry.m_Type == (int)nn::fs::DirectoryEntryType_Directory) {
                modsOnDisk.push_back(entry.m_Name);
            }
        }
        nn::fs::CloseDirectory(dh);
    }
    std::sort(modsOnDisk.begin(), modsOnDisk.end()); // Sort new mods alphabetically

    // 4. Sync priority list with physical mod folders
    bool configChanged = !configExists;
    std::set<std::string> currentPrioritySet(priorityPaths.begin(), priorityPaths.end());

    // Automatically append any new mod folders found on SD card
    for (const auto& modName : modsOnDisk) {
        if (currentPrioritySet.find(modName) == currentPrioritySet.end()) {
            priorityPaths.push_back(modName);
            configChanged = true;
        }
    }

    // 5. Save the updated configuration if changes were detected
    if (configChanged) {
        SaveConfig(configPath);
    }

    return true;
}