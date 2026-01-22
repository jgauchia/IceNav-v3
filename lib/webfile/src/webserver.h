/**
 * @file webserver.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Web file server functions declarations
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include <Arduino.h>
#include "storage.hpp"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"
#include <esp_task_wdt.h>
#include <vector>

static const char* WEB_TAG = "WebServer";
static const char* hostname = "icenav";

// Global state
static String oldDir = "";
static String newDir = "";
static String currentDir = "";
static String createDir = "";
static uint8_t nextSlash = 0;
static bool updateList = true;
static bool deleteDir = false;
static String deletePath = "";
static String statusMessage = "";
static bool statusPending = false;

static const int FILES_PER_PAGE = 10;
static httpd_handle_t webServer = NULL;

extern Storage storage;
extern bool waitScreenRefresh;

/**
 * @brief File directory cache entry
 */
struct FileEntry
{
    String name;
    bool isDirectory;
    size_t size;
};
static std::vector<FileEntry> fileCache;

// Function declarations for webserver.cpp
static String humanReadableSize(uint64_t bytes);
static int extractNumber(const String& str, int& pos);
static bool naturalCompare(const String& a, const String& b);
static bool compareFileEntries(const FileEntry& a, const FileEntry& b);
static void sortFileCache();
static void cacheDirectoryContent(const String& dir);
static bool getQueryParam(httpd_req_t *req, const char* param, char* value, size_t maxLen);
static void urlDecode(char* str);
static String listFiles(bool ishtml, int page);
static bool deleteDirRecursive(const char *dirPath);
static bool createDirectories(String filepath);
static String processTemplate(const char* html);
static esp_err_t root_handler(httpd_req_t *req);
static esp_err_t status_handler(httpd_req_t *req);
static esp_err_t listfiles_handler(httpd_req_t *req);
static esp_err_t changedirectory_handler(httpd_req_t *req);
static esp_err_t file_handler(httpd_req_t *req);
static esp_err_t reboot_handler(httpd_req_t *req);
static esp_err_t sendSpiffsImage(httpd_req_t *req, const char *imageFile);
static uint8_t* findBytes(uint8_t* haystack, size_t haystackLen, const uint8_t* needle, size_t needleLen);
static esp_err_t upload_handler(httpd_req_t *req);
static esp_err_t notfound_handler(httpd_req_t *req, httpd_err_code_t err);

// Image handlers
static esp_err_t logo_handler(httpd_req_t *req);
static esp_err_t files_handler(httpd_req_t *req);
static esp_err_t folder_handler(httpd_req_t *req);
static esp_err_t down_handler(httpd_req_t *req);
static esp_err_t up_handler(httpd_req_t *req);
static esp_err_t del_handler(httpd_req_t *req);
static esp_err_t reb_handler(httpd_req_t *req);
static esp_err_t list_handler(httpd_req_t *req);

// Public interface functions
void setWebStatus(const char* message, bool refresh);
bool isDeleteDirPending();
String getDeletePath();
void processWebServerTasks();
void configureWebServer();
void stopWebServer();

// Forward declarations for webpage content
extern const char index_html[];
extern const char reboot_html[];