#include "Config.hpp"
#include "toml.hpp"
#include "lib.hpp"
#include "fs.hpp" // Required for nn::fs

bool Config::enableDebugConsole = false;
std::string Config::modsDirectoryPath = "ExlSD:/DMLSwitchPort/mods";
std::vector<std::string> Config::priorityPaths;

bool Config::init() {
    nn::fs::FileHandle h;
    
    // 1. Open the config file using Nintendo's safe FS API
    if (R_FAILED(nn::fs::OpenFile(&h, "ExlSD:/DMLSwitchPort/config.toml", nn::fs::OpenMode_Read))) {
        return true; // Fallback to defaults if file doesn't exist
    }

    int64_t size = 0;
    nn::fs::GetFileSize(&size, h);
    
    if (size <= 0) {
        nn::fs::CloseFile(h);
        return true;
    }

    // 2. Read the file content into a string buffer
    std::string content(size, '\0');
    nn::fs::ReadFile(h, 0, content.data(), size);
    nn::fs::CloseFile(h);

    // 3. Parse the TOML content from the memory string
    toml::parse_result result = toml::parse(content);
    if (!result) return true;

    toml::table config = std::move(result).table();

    if (!config["enabled"].value_or(true)) return false;

    enableDebugConsole = config["console"].value_or(false);
    
    std::string modsFolder = config["mods"].value_or("mods");
    if (modsFolder.find("ExlSD:/") == std::string::npos) {
        modsDirectoryPath = "ExlSD:/DMLSwitchPort/" + modsFolder;
    } else {
        modsDirectoryPath = modsFolder;
    }

    if (toml::array* priorityArr = config["priority"].as_array()) {
        for (auto& pathElem : *priorityArr) {
            const std::string path = pathElem.value_or("");
            if (!path.empty()) {
                priorityPaths.push_back(path);
            }
        }
    }

    return true;
}