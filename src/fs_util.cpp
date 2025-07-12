#include "fs_util.h"
#include <HardwareSerial.h>

FsUtil::FsUtil()
{
    // MOUNT the filesystem if needed
    esp_vfs_fat_mount_config_t mount_config = {
        .format_if_mount_failed = true,
        .max_files = 10,
        .allocation_unit_size = CONFIG_WL_SECTOR_SIZE,
        .use_one_fat = false};
        
    wl_handle_t s_wl_handle;
    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl("/spiflash", "ffat", &mount_config, &s_wl_handle);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to mount FAT filesystem: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "FAT filesystem mounted successfully at /spiflash");
    // Ensure the config file exists and is initialized
    ensureConfigFileExists("/spiflash/config.json");
    ESP_LOGI(TAG, "Config file checked and initialized if necessary.");
}

FsUtil *FsUtil::getInstance()
{
    static FsUtil instance;
    return &instance;
}

void FsUtil::checkAndInitialObjectInFile(const char *file_path)
{
    char *buffer = getBufferFromFile(file_path);
    if (!buffer)
        return;

    cJSON *json = cJSON_Parse(buffer);
    free(buffer);
    if (!json)
    {
        ESP_LOGE(TAG, "Failed to parse JSON from file: %s", file_path);
        return;
    }

    cJSON *root = cJSON_GetObjectItem(json, root_obj.c_str());
    if (!root)
    {
        ESP_LOGE(TAG, "Root object not found in JSON file: %s", file_path);
        cJSON_Delete(json);
        writeInitialObjectsToFile(file_path);
        return;
    }

    for (const auto &obj_name : initial_objects)
    {
        if (!cJSON_GetObjectItem(root, obj_name.c_str()))
        {
            ESP_LOGW(TAG, "Initial object '%s' not found. Adding...", obj_name.c_str());
            createcJsonObjectInsideAnother(root, obj_name.c_str());
        }
    }

    char *json_str = cJSON_Print(json);
    if (json_str)
    {
        writeToFile(file_path, json_str);
        free(json_str);
    }

    cJSON_Delete(json);
}

bool FsUtil::writeInitialObjectsToFile(const char *file_path)
{
    cJSON *base = cJSON_CreateObject();
    cJSON_AddItemToObject(base, root_obj.c_str(), cJSON_CreateObject());

    cJSON *root = cJSON_GetObjectItem(base, root_obj.c_str());
    if (!root)
    {
        ESP_LOGE(TAG, "Failed to create root object in JSON");
        cJSON_Delete(base);
        return false;
    }

    for (const auto &obj_name : initial_objects)
    {
        createcJsonObjectInsideAnother(root, obj_name.c_str());
    }

    char *json_str = cJSON_Print(base);
    if (!json_str)
    {
        ESP_LOGE(TAG, "Failed to serialize JSON");
        cJSON_Delete(base);
        return false;
    }

    bool success = writeToFile(file_path, json_str);
    free(json_str);
    cJSON_Delete(base);
    return success;
}

bool FsUtil::writeToFile(const char *file_path, const char *data)
{
    uint64_t total = 0, free = 0;
    esp_vfs_fat_info("/spiflash", &total, &free);
    Serial.printf("%s FS total: %llu, used: %llu, free: %llu\n", TAG, total, total - free, free);

    FILE *f = fopen(file_path, "w");
    if (!f)
    {
        ESP_LOGE(TAG, "Failed to open file for writing: %s", file_path);
        return false;
    }
    fwrite(data, 1, strlen(data), f);
    fclose(f);
    ESP_LOGI(TAG, "Data written to file: %s", file_path);
    return true;
}

cJSON *FsUtil::createcJsonObjectInsideAnother(cJSON *parent, const char *name)
{
    cJSON *new_obj = cJSON_CreateObject();

    if (!new_obj)
    {
        ESP_LOGE(TAG, "Failed to create JSON object for %s", name);
        return NULL;
    }

    if (!cJSON_AddItemToObject(parent, name, new_obj))
    {
        ESP_LOGE(TAG, "Failed to add object %s to parent", name);
        cJSON_Delete(new_obj);
        return NULL;
    }

    ESP_LOGI(TAG, "Created JSON object '%s' inside parent", name);
    return new_obj;
}

void FsUtil::ensureConfigFileExists(const char *file_path)
{
    FILE *f = fopen(file_path, "r");

    if (f)
    {
        fclose(f);
        ESP_LOGI(TAG, "Config file already exists: %s", file_path);
        checkAndInitialObjectInFile(file_path);
        return;
    }

    ESP_LOGW(TAG, "Config file not found, creating default one: %s", file_path);
    writeInitialObjectsToFile(file_path);
}

char *FsUtil::getBufferFromFile(const char *file_path)
{
    FILE *f = fopen(file_path, "r");
    if (!f)
    {
        ESP_LOGE(TAG, "Failed to open file: %s", file_path);
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    if (size <= 0)
    {
        ESP_LOGE(TAG, "File is empty or invalid size: %s", file_path);
        fclose(f);
        return NULL;
    }

    char *buffer = (char *)malloc(size + 1);
    if (!buffer)
    {
        fclose(f);
        return NULL;
    }

    size_t read_bytes = fread(buffer, 1, size, f);
    buffer[read_bytes] = '\0';
    fclose(f);
    return buffer;
}

std::pair<cJSON *, cJSON *> FsUtil::getObjectFromRoot(const char *file_path, const char *object_name)
{
    char *buffer = getBufferFromFile(file_path);
    if (!buffer)
        return {nullptr, nullptr};

    cJSON *json = cJSON_Parse(buffer);
    free(buffer);
    if (!json)
        return {nullptr, nullptr};

    cJSON *root = cJSON_GetObjectItem(json, root_obj.c_str());
    if (!root)
    {
        cJSON_Delete(json);
        return {nullptr, nullptr};
    }

    cJSON *object = cJSON_GetObjectItem(root, object_name);
    if (!object)
    {
        cJSON_Delete(json);
        return {nullptr, nullptr};
    }

    return {json, object};
}

std::string FsUtil::getStringConfigValue(const char *file_path, const char *object_name, const char *property)
{
    auto [json, object] = getObjectFromRoot(file_path, object_name);
    if (!json || !object)
        return "";

    cJSON *target = cJSON_GetObjectItem(object, property);
    if (!target || !cJSON_IsString(target))
    {
        cJSON_Delete(json);
        return "";
    }

    std::string result = target->valuestring;
    cJSON_Delete(json);
    return result;
}

bool FsUtil::setStringConfigValue(const char *file_path, const char *object_name, const char *property, std::string val)
{
    char *buffer = getBufferFromFile(file_path);
    if (!buffer)
        return false;

    cJSON *json = cJSON_Parse(buffer);
    free(buffer);
    if (!json)
        return false;

    cJSON *root = cJSON_GetObjectItem(json, root_obj.c_str());
    if (!root)
    {
        cJSON_Delete(json);
        return false;
    }

    cJSON *object = cJSON_GetObjectItem(root, object_name);
    if (!object)
    {
        cJSON_Delete(json);
        return false;
    }

    cJSON *target = cJSON_GetObjectItem(object, property);
    if (target)
    {
        cJSON_SetValuestring(target, val.c_str());
    }
    else
    {
        cJSON_AddStringToObject(object, property, val.c_str());
    }

    char *json_str = cJSON_Print(json);
    if (!json_str)
    {
        cJSON_Delete(json);
        return false;
    }

    bool success = writeToFile(file_path, json_str);
    free(json_str);
    cJSON_Delete(json);
    return success;
}

int FsUtil::getIntConfigValue(const char *file_path, const char *object_name, const char *property)
{
    auto [json, object] = getObjectFromRoot(file_path, object_name);
    if (!json || !object)
        return 0;

    cJSON *target = cJSON_GetObjectItem(object, property);
    if (!target || !cJSON_IsNumber(target))
    {
        cJSON_Delete(json);
        return 0;
    }

    int result = target->valueint;
    cJSON_Delete(json);
    return result;
}

bool FsUtil::setIntConfigValue(const char *file_path, const char *object_name, const char *property, int val)
{
    char *buffer = getBufferFromFile(file_path);
    if (!buffer)
        return false;

    cJSON *json = cJSON_Parse(buffer);
    free(buffer);
    if (!json)
        return false;

    cJSON *root = cJSON_GetObjectItem(json, root_obj.c_str());
    if (!root)
    {
        cJSON_Delete(json);
        return false;
    }

    cJSON *object = cJSON_GetObjectItem(root, object_name);
    if (!object)
    {
        cJSON_Delete(json);
        return false;
    }

    cJSON *target = cJSON_GetObjectItem(object, property);
    if (target)
    {
        target->valueint = val;
    }
    else
    {
        cJSON_AddNumberToObject(object, property, val);
    }

    char *json_str = cJSON_Print(json);
    if (!json_str)
    {
        cJSON_Delete(json);
        return false;
    }

    bool success = writeToFile(file_path, json_str);
    free(json_str);
    cJSON_Delete(json);
    return success;
}
