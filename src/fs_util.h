#pragma once
#include "esp_vfs_fat.h"
#include <string>
#include "cJSON.h"
#include "esp_log.h"
#include <cstring>
#include <array>

class FsUtil
{
private:
    const char *TAG = "Fs";

public:
    FsUtil();
    
    const char *root_file_path = "/spiflash/config.json";
    std::string root_obj = "root";
    std::array<std::string, 5> initial_objects = {"wifi", "audio", "speech", "http", "radio"};

    void checkAndInitialObjectInFile(const char *file_path);
    bool writeInitialObjectsToFile(const char *file_path);
    bool writeToFile(const char *file_path, const char *data);
    cJSON *createcJsonObjectInsideAnother(cJSON *parent, const char *name);
    char *getBufferFromFile(const char *file_path);
    std::pair<cJSON *, cJSON *> getObjectFromRoot(const char *file_path, const char *object_name);
    std::string getStringConfigValue(const char *file_path, const char *object_name, const char *property);
    int getIntConfigValue(const char *file_path, const char *object_name, const char *property);
    bool setStringConfigValue(const char *file_path, const char *object_name, const char *property, std::string val);
    void ensureConfigFileExists(const char *file_path);
    bool setIntConfigValue(const char *file_path, const char *object_name, const char *property, int val);
};

extern FsUtil fsUtil;