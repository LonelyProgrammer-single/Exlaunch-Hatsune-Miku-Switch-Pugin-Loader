#pragma once
#include <string>
#include <vector>
#include <filesystem>

class ModLoader {
public:
    static std::vector<std::string> modDirectoryPaths;

    static void initMod(const std::string& path);
    static void init();
};
