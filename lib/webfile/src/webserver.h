/**
 * @file webserver.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Web file server functions declarations
 * @version 0.3.0
 * @date 2026-09
 */

#pragma once

#include <string>
#include <vector>
#include "storage.hpp"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "esp_system.h"

static const char* WEB_TAG = "WebServer";
static const char* hostname = "icenav";

// Global state
static std::string oldDir = "";
static std::string newDir = "";
static bool updateList = true;
static bool deleteDir = false;
static std::string deletePath = "";
static std::string statusMessage = "";
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
    std::string name;
    bool isDirectory;
    size_t size;
};
static std::vector<FileEntry> fileCache;

// Function declarations for webserver.cpp
static std::string humanReadableSize(uint64_t bytes);
static int extractNumber(const std::string& str, int& pos);
static bool naturalCompare(const std::string& a, const std::string& b);
static bool compareFileEntries(const FileEntry& a, const FileEntry& b);
static void sortFileCache();
static void cacheDirectoryContent(const std::string& dir);
static bool getQueryParam(httpd_req_t *req, const char* param, char* value, size_t maxLen);
static void urlDecode(char* str);
static std::string listFiles(bool ishtml, int page);
static bool deleteDirRecursive(const char *dirPath);
static bool createDirectories(const std::string& filepath);
static std::string processTemplate(const char* html);
static esp_err_t rootHandler(httpd_req_t *req);
static esp_err_t statusHandler(httpd_req_t *req);
static esp_err_t listFilesHandler(httpd_req_t *req);
static esp_err_t changeDirectoryHandler(httpd_req_t *req);
static esp_err_t fileHandler(httpd_req_t *req);
static esp_err_t rebootHandler(httpd_req_t *req);
static esp_err_t sendSpiffsImage(httpd_req_t *req, const char *imageFile);
static esp_err_t sendSpiffsJS(httpd_req_t *req, const char *jsFile);
static esp_err_t listFolderHandler(httpd_req_t *req);
static uint8_t* findBytes(uint8_t* haystack, size_t haystackLen, const uint8_t* needle, size_t needleLen);
static esp_err_t uploadHandler(httpd_req_t *req);
static esp_err_t notFoundHandler(httpd_req_t *req, httpd_err_code_t err);

// Image handlers
static esp_err_t logoHandler(httpd_req_t *req);
static esp_err_t filesHandler(httpd_req_t *req);
static esp_err_t folderHandler(httpd_req_t *req);
static esp_err_t downHandler(httpd_req_t *req);
static esp_err_t upHandler(httpd_req_t *req);
static esp_err_t delHandler(httpd_req_t *req);
static esp_err_t rebHandler(httpd_req_t *req);
static esp_err_t listHandler(httpd_req_t *req);

// Public interface functions
void setWebStatus(const char* message, bool refresh);
void processWebServerTasks();
void configureWebServer();
void stopWebServer();

// Forward declarations for webpage content
extern const char index_html[];
extern const char reboot_html[];
