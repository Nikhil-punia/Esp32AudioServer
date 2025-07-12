#pragma once

/**
 * @file fs_util.h
 * @brief Utility class for SPIFFS/FAT file system operations on ESP32.
 *
 * This utility handles reading and writing JSON configuration files stored
 * in the SPIFFS/FAT partition.
 */

#include "esp_vfs_fat.h"
#include <string>
#include "cJSON.h"
#include "esp_log.h"
#include <cstring>
#include <array>

/**
 * @class FsUtil
 * @brief Singleton class that provides helper functions to manage a JSON configuration file
 *        stored in SPIFFS or FAT file system.
 */
class FsUtil
{
private:
    static constexpr const char *TAG = "Fs";  ///< Logging tag

    /**
     * @brief Private constructor to enforce singleton pattern.
     */
    FsUtil();

    // Disable copy constructor and assignment
    FsUtil(const FsUtil&) = delete;
    FsUtil& operator=(const FsUtil&) = delete;

public:
    /**
     * @brief Accessor to the singleton instance of FsUtil.
     * @return Pointer to the singleton instance.
     */
    static FsUtil* getInstance();

    const char *root_file_path = "/spiflash/config.json"; ///< Default config file path
    std::string root_obj = "root"; ///< Root JSON object name
    std::array<std::string, 5> initial_objects = {"wifi", "audio", "speech", "http", "radio"}; ///< Default top-level keys

    /**
     * @brief Check if file and required objects exist; initialize if not.
     * @param file_path Path to the configuration file.
     */
    void checkAndInitialObjectInFile(const char *file_path);

    /**
     * @brief Write initial root-level JSON objects to file.
     * @param file_path Path to the configuration file.
     * @return True on success, false on failure.
     */
    bool writeInitialObjectsToFile(const char *file_path);

    /**
     * @brief Write a raw string to a file.
     * @param file_path Path to the file.
     * @param data Data to write.
     * @return True on success, false on failure.
     */
    bool writeToFile(const char *file_path, const char *data);

    /**
     * @brief Create a new named cJSON object inside a parent object.
     * @param parent Parent cJSON object.
     * @param name Key name for the new object.
     * @return Pointer to the newly created object or nullptr.
     */
    cJSON *createcJsonObjectInsideAnother(cJSON *parent, const char *name);

    /**
     * @brief Read entire file contents into a heap-allocated buffer.
     * @param file_path Path to the file.
     * @return Pointer to allocated string buffer (needs to be freed), or nullptr on failure.
     */
    char *getBufferFromFile(const char *file_path);

    /**
     * @brief Parse the JSON and return both root and a named child object.
     * @param file_path Path to the file.
     * @param object_name Name of the target object.
     * @return Pair of pointers: {root, named_object}, or {nullptr, nullptr} on error.
     */
    std::pair<cJSON *, cJSON *> getObjectFromRoot(const char *file_path, const char *object_name);

    /**
     * @brief Retrieve a string config value from the JSON file.
     * @param file_path Path to the file.
     * @param object_name JSON object name.
     * @param property Key inside the object.
     * @return Value as a std::string.
     */
    std::string getStringConfigValue(const char *file_path, const char *object_name, const char *property);

    /**
     * @brief Retrieve an integer config value from the JSON file.
     * @param file_path Path to the file.
     * @param object_name JSON object name.
     * @param property Key inside the object.
     * @return Value as an int.
     */
    int getIntConfigValue(const char *file_path, const char *object_name, const char *property);

    /**
     * @brief Set a string value inside a JSON config file.
     * @param file_path Path to the file.
     * @param object_name JSON object name.
     * @param property Key to set.
     * @param val New string value.
     * @return True on success, false on failure.
     */
    bool setStringConfigValue(const char *file_path, const char *object_name, const char *property, std::string val);

    /**
     * @brief Ensure the file exists and has a valid JSON structure.
     * @param file_path Path to the config file.
     */
    void ensureConfigFileExists(const char *file_path);

    /**
     * @brief Set an integer value inside a JSON config file.
     * @param file_path Path to the file.
     * @param object_name JSON object name.
     * @param property Key to set.
     * @param val New integer value.
     * @return True on success, false on failure.
     */
    bool setIntConfigValue(const char *file_path, const char *object_name, const char *property, int val);
};

/**
 * @brief Global reference macro to singleton FsUtil instance.
 */
#define fsUtil (*FsUtil::getInstance())
