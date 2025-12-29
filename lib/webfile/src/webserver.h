/**
 * @file webserver.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Web file server functions
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include "storage.hpp"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include <esp_task_wdt.h>
#include <algorithm>
#include <dirent.h>
#include <stdio.h>
#include <stack>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>

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

/**
 * @brief Convert bytes to Human Readable Size
 */
static String humanReadableSize(uint64_t bytes)
{
    if (bytes < 1024)
        return String(bytes) + " B";
    else if (bytes < (1024 * 1024))
        return String(bytes / 1024.0) + " KB";
    else if (bytes < (1024 * 1024 * 1024))
        return String(bytes / (1024.0 * 1024.0)) + " MB";
    else
        return String(bytes / (1024.0 * 1024.0 * 1024.0)) + " GB";
}

/**
 * @brief Extract numeric part from string
 */
static int extractNumber(const String& str, int& pos)
{
    int num = 0;
    while (pos < (int)str.length() && isdigit(str[pos]))
    {
        num = num * 10 + (str[pos] - '0');
        pos++;
    }
    return num;
}

/**
 * @brief Natural compare for sorting
 */
static bool naturalCompare(const String& a, const String& b)
{
    int i = 0, j = 0;
    while (i < (int)a.length() && j < (int)b.length())
    {
        if (isdigit(a[i]) && isdigit(b[j]))
        {
            int numA = extractNumber(a, i);
            int numB = extractNumber(b, j);
            if (numA != numB)
                return numA < numB;
        }
        else
        {
            if (tolower(a[i]) != tolower(b[j]))
                return tolower(a[i]) < tolower(b[j]);
            i++;
            j++;
        }
    }
    return a.length() < b.length();
}

/**
 * @brief Compare FileEntry for sorting
 */
static bool compareFileEntries(const FileEntry& a, const FileEntry& b)
{
    if (a.isDirectory != b.isDirectory)
        return a.isDirectory > b.isDirectory;
    return naturalCompare(a.name, b.name);
}

/**
 * @brief Sort file cache
 */
static void sortFileCache()
{
    std::sort(fileCache.begin(), fileCache.end(), compareFileEntries);
}

/**
 * @brief Cache directory content
 */
static void cacheDirectoryContent(const String& dir)
{
    fileCache.clear();
    String fullDir = "/sdcard" + dir;

    DIR* dp = opendir(fullDir.c_str());
    if (dp != nullptr)
    {
        struct dirent* ep;
        while ((ep = readdir(dp)))
        {
            FileEntry entry;
            entry.name = String(ep->d_name);
            entry.isDirectory = (ep->d_type == DT_DIR);

            if (!entry.isDirectory)
            {
                String filePath = fullDir + "/" + entry.name;
                FILE* file = fopen(filePath.c_str(), "r");
                if (file)
                {
                    fseek(file, 0, SEEK_END);
                    entry.size = ftell(file);
                    fclose(file);
                }
            }
            else
                entry.size = 0;

            fileCache.push_back(entry);
            esp_task_wdt_reset();
        }
        closedir(dp);
    }

    currentDir = dir;
    sortFileCache();
}

/**
 * @brief Get query parameter value
 */
static bool getQueryParam(httpd_req_t *req, const char* param, char* value, size_t maxLen)
{
    size_t bufLen = httpd_req_get_url_query_len(req) + 1;
    if (bufLen > 1)
    {
        char* buf = (char*)malloc(bufLen);
        if (httpd_req_get_url_query_str(req, buf, bufLen) == ESP_OK)
        {
            if (httpd_query_key_value(buf, param, value, maxLen) == ESP_OK)
            {
                free(buf);
                return true;
            }
        }
        free(buf);
    }
    return false;
}

/**
 * @brief URL decode string
 */
static void urlDecode(char* str)
{
    char* dst = str;
    char a, b;
    while (*str)
    {
        if ((*str == '%') && ((a = str[1]) && (b = str[2])) && (isxdigit(a) && isxdigit(b)))
        {
            if (a >= 'a') a -= 'a' - 'A';
            if (a >= 'A') a -= ('A' - 10);
            else a -= '0';
            if (b >= 'a') b -= 'a' - 'A';
            if (b >= 'A') b -= ('A' - 10);
            else b -= '0';
            *dst++ = 16 * a + b;
            str += 3;
        }
        else if (*str == '+')
        {
            *dst++ = ' ';
            str++;
        }
        else
        {
            *dst++ = *str++;
        }
    }
    *dst = '\0';
}

/**
 * @brief List files HTML generation
 */
static String listFiles(bool ishtml, int page = 0)
{
    String returnText = "";
    int startIdx = page * FILES_PER_PAGE;
    int endIdx = startIdx + FILES_PER_PAGE;

    if (ishtml)
    {
        returnText += "<div style=\"overflow-y:scroll;\"><table><tr><th>Name</th><th style=\"text-align:center\">Size</th><th></th><th></th></tr>";
        if (oldDir != "/")
        {
            returnText += "<tr align='left'><td style=\"width:300px\">";
            returnText += "<img src=\"folder\"> <a href='#' onclick='changeDirectory(\"..\")'>..</a>";
            returnText += "</td><td style=\"text-align:center\">dir</td><td></td><td></td>";
            returnText += "</tr>";
        }
    }

    for (int i = startIdx; i < endIdx && i < (int)fileCache.size(); ++i)
    {
        FileEntry& entry = fileCache[i];

        if (ishtml)
        {
            returnText += "<tr align='left'><td style=\"width:300px\">";
            if (entry.isDirectory)
            {
                returnText += "<img src=\"folder\"> <a href='#' onclick='changeDirectory(\"" + entry.name + "\")'>" + entry.name + "</a>";
                returnText += "</td><td style=\"text-align:center\">dir</td>";
                returnText += "<td></td>";
                returnText += "<td><button class=\"button\" onclick=\"downloadDeleteButton('" + entry.name + "', 'deldir')\"><img src=\"del\"> Delete</button></td>";
            }
            else
            {
                returnText += "<img src=\"files\"> " + entry.name;
                returnText += "</td><td style=\"text-align:right\">" + humanReadableSize(entry.size) + "</td>";
                returnText += "<td><button class=\"button\" onclick=\"downloadDeleteButton('" + entry.name + "', 'download')\"><img src=\"down\"> Download</button></td>";
                returnText += "<td><button class=\"button\" onclick=\"downloadDeleteButton('" + entry.name + "', 'delete')\"><img src=\"del\"> Delete</button></td>";
            }
            returnText += "</tr>";
        }
        else
        {
            returnText += "File: " + entry.name + " Size: " + humanReadableSize(entry.size) + "\n";
        }
    }

    if (ishtml)
    {
        returnText += "</table></div><p></p><p>";
        returnText += "<tr align='left'>";
        if (page > 0)
        {
            returnText += "<ti><button class=\"button\" onclick='loadPage(" + String(0) + ")'>First</button></ti>";
            returnText += "<ti><button class=\"button\" onclick='loadPage(" + String(page - 1) + ")'>Prev</button></ti>";
        }

        returnText += "<ti><span> Page " + String(page + 1) + "/" + String((fileCache.size() / 10) + 1) + " </span></ti>";

        if ((int)fileCache.size() > endIdx)
        {
            returnText += "<ti><button class=\"button\" onclick='loadPage(" + String(page + 1) + ")'>Next</button></ti>";
            returnText += "<ti><button class=\"button\" onclick='loadPage(" + String((fileCache.size() / 10)) + ")'>Last</button></ti>";
        }
        returnText += "</tr></p>";
    }

    return returnText;
}

/**
 * @brief Delete directory recursively
 */
static bool deleteDirRecursive(const char *dirPath)
{
    if (!dirPath || strlen(dirPath) == 0)
    {
        ESP_LOGE(WEB_TAG, "Error: Invalid directory path");
        return false;
    }

    std::string rootDir(dirPath);
    std::stack<std::string> dirStack;
    std::stack<std::string> deleteStack;
    dirStack.push(rootDir);

    while (!dirStack.empty())
    {
        std::string currentDirPath = dirStack.top();
        dirStack.pop();

        ESP_LOGI(WEB_TAG, "Processing directory: %s", currentDirPath.c_str());

        DIR *dir = opendir(currentDirPath.c_str());
        if (!dir)
        {
            ESP_LOGE(WEB_TAG, "Error opening directory: %s", currentDirPath.c_str());
            return false;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;

            char entryPath[PATH_MAX];
            snprintf(entryPath, sizeof(entryPath), "%s/%s", currentDirPath.c_str(), entry->d_name);

            struct stat entryStat;
            if (stat(entryPath, &entryStat) == -1)
            {
                ESP_LOGE(WEB_TAG, "Error getting entry stats for: %s", entryPath);
                closedir(dir);
                return false;
            }

            if (S_ISDIR(entryStat.st_mode))
            {
                dirStack.push(std::string(entryPath));
            }
            else
            {
                if (remove(entryPath) != 0)
                {
                    ESP_LOGE(WEB_TAG, "Error deleting file: %s", entryPath);
                    closedir(dir);
                    return false;
                }
                ESP_LOGI(WEB_TAG, "Deleted file: %s", entryPath);
            }
        }

        closedir(dir);
        deleteStack.push(currentDirPath);
    }

    while (!deleteStack.empty())
    {
        std::string dirToDelete = deleteStack.top();
        deleteStack.pop();

        if (rmdir(dirToDelete.c_str()) != 0)
        {
            ESP_LOGE(WEB_TAG, "Error deleting directory: %s", dirToDelete.c_str());
            return false;
        }
        ESP_LOGI(WEB_TAG, "Deleted directory: %s", dirToDelete.c_str());
    }

    return true;
}

/**
 * @brief Create directories for upload
 */
static bool createDirectories(String filepath)
{
    uint8_t lastSlash = 0;
    while (true)
    {
        nextSlash = filepath.indexOf('/', lastSlash + 1);
        String dir = filepath.substring(0, nextSlash);

        String newDirPath = "/sdcard" + oldDir + "/" + dir;
        if (!storage.exists(newDirPath.c_str()))
        {
            if (!storage.mkdir(newDirPath.c_str()))
            {
                ESP_LOGE(WEB_TAG, "Directory %s creation error", newDirPath.c_str());
                return false;
            }
            ESP_LOGI(WEB_TAG, "Directory %s created", newDirPath.c_str());
        }
        if (nextSlash == 255) break;
        lastSlash = nextSlash;

        esp_task_wdt_reset();
    }
    return true;
}

// Forward declarations for webpage content
extern const char index_html[];
extern const char reboot_html[];

/**
 * @brief Replace template variables
 */
static String processTemplate(const char* html)
{
    String result = String(html);
    SDCardInfo info = storage.getSDCardInfo();

    result.replace("%FIRMWARE%", String(VERSION) + " - Rev: " + String(REVISION));
    result.replace("%FREEFS%", info.free_space.c_str());
    result.replace("%USEDFS%", info.used_space.c_str());
    result.replace("%TOTALFS%", info.total_space.c_str());
    result.replace("%TYPEFS%", info.card_type.c_str());

    return result;
}

// ============ HTTP Handlers ============

/**
 * @brief Root handler - serves main page
 */
static esp_err_t root_handler(httpd_req_t *req)
{
    String html = processTemplate(index_html);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
    return ESP_OK;
}

/**
 * @brief Status polling handler (replaces SSE)
 */
static esp_err_t status_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    String response = "{\"refresh\":";
    response += statusPending ? "true" : "false";
    response += ",\"message\":\"";
    response += statusMessage;
    response += "\"}";

    if (statusPending)
    {
        statusPending = false;
        statusMessage = "";
    }

    httpd_resp_send(req, response.c_str(), response.length());
    return ESP_OK;
}

/**
 * @brief List files handler
 */
static esp_err_t listfiles_handler(httpd_req_t *req)
{
    char pageStr[8] = "0";
    getQueryParam(req, "page", pageStr, sizeof(pageStr));
    int page = atoi(pageStr);

    if (updateList)
    {
        esp_task_wdt_reset();
        cacheDirectoryContent(oldDir);
    }

    String html = listFiles(true, page);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.length());
    return ESP_OK;
}

/**
 * @brief Change directory handler
 */
static esp_err_t changedirectory_handler(httpd_req_t *req)
{
    char dirParam[128] = "";
    if (!getQueryParam(req, "dir", dirParam, sizeof(dirParam)))
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ERROR: dir parameter required");
        return ESP_FAIL;
    }

    urlDecode(dirParam);
    updateList = false;
    newDir = String(dirParam);

    ESP_LOGI(WEB_TAG, "new dir %s", newDir.c_str());
    ESP_LOGI(WEB_TAG, "old dir %s", oldDir.c_str());

    if (newDir == "/..")
    {
        if (oldDir != "/..")
        {
            oldDir = oldDir.substring(0, oldDir.lastIndexOf("/"));
            currentDir = "";
            String response = "Path:" + oldDir;
            httpd_resp_send(req, response.c_str(), response.length());
        }
        else
        {
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Cannot go up from root directory");
            return ESP_FAIL;
        }
    }
    else
    {
        if (oldDir != "/")
            oldDir = oldDir + newDir;
        else
            oldDir = newDir;
        currentDir = "";
        String response = "Path:" + oldDir;
        httpd_resp_send(req, response.c_str(), response.length());
    }

    cacheDirectoryContent(oldDir);
    return ESP_OK;
}

/**
 * @brief File operations handler (download, delete)
 */
static esp_err_t file_handler(httpd_req_t *req)
{
    char fileName[128] = "";
    char fileAction[16] = "";

    if (!getQueryParam(req, "name", fileName, sizeof(fileName)) ||
        !getQueryParam(req, "action", fileAction, sizeof(fileAction)))
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ERROR: name and action params required");
        return ESP_FAIL;
    }

    urlDecode(fileName);
    String path = "/sdcard" + oldDir + "/" + String(fileName);

    ESP_LOGI(WEB_TAG, "File operation: %s on %s", fileAction, path.c_str());

    if (strcmp(fileAction, "deldir") == 0)
    {
        deletePath = path;
        deleteDir = true;
        String response = "Deleting Folder: " + String(fileName) + " please wait...";
        httpd_resp_send(req, response.c_str(), response.length());
        updateList = true;
        return ESP_OK;
    }

    FILE* file = storage.open(path.c_str(), "r");

    if (!file)
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ERROR: file does not exist");
        return ESP_FAIL;
    }

    if (strcmp(fileAction, "download") == 0)
    {
        fseek(file, 0, SEEK_END);
        size_t fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);

        httpd_resp_set_type(req, "application/octet-stream");

        String disposition = "attachment; filename=\"" + path.substring(path.lastIndexOf('/') + 1) + "\"";
        httpd_resp_set_hdr(req, "Content-Disposition", disposition.c_str());

        char* chunk = (char*)heap_caps_malloc(4096, MALLOC_CAP_8BIT);
        if (!chunk)
        {
            storage.close(file);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_FAIL;
        }

        size_t read;
        while ((read = fread(chunk, 1, 4096, file)) > 0)
        {
            if (httpd_resp_send_chunk(req, chunk, read) != ESP_OK)
            {
                heap_caps_free(chunk);
                storage.close(file);
                return ESP_FAIL;
            }
        }

        heap_caps_free(chunk);
        storage.close(file);
        httpd_resp_send_chunk(req, NULL, 0);
        return ESP_OK;
    }
    else if (strcmp(fileAction, "delete") == 0)
    {
        storage.close(file);
        storage.remove(path.c_str());
        String response = "Deleted File: " + String(fileName);
        httpd_resp_send(req, response.c_str(), response.length());
        updateList = true;
        return ESP_OK;
    }
    else
    {
        storage.close(file);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ERROR: invalid action param supplied");
        return ESP_FAIL;
    }
}

/**
 * @brief Reboot handler
 */
static esp_err_t reboot_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, reboot_html, strlen(reboot_html));

    vTaskDelay(pdMS_TO_TICKS(100));
    ESP.restart();
    return ESP_OK;
}

/**
 * @brief Send PNG image from SPIFFS
 */
static esp_err_t sendSpiffsImage(httpd_req_t *req, const char *imageFile)
{
    FILE *file = storage.open(imageFile, "r");

    if (file)
    {
        size_t size = storage.size(imageFile);

        #ifdef BOARD_HAS_PSRAM
            uint8_t *buffer = (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
        #else
            uint8_t *buffer = (uint8_t*)heap_caps_malloc(size, MALLOC_CAP_8BIT);
        #endif

        if (!buffer)
        {
            storage.close(file);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_FAIL;
        }

        storage.read(file, buffer, size);
        storage.close(file);

        httpd_resp_set_type(req, "image/png");
        httpd_resp_send(req, (const char*)buffer, size);

        heap_caps_free(buffer);
        return ESP_OK;
    }

    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Image not found");
    return ESP_FAIL;
}

// Image handlers
static esp_err_t logo_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/LOGO_LARGE.png"); }
static esp_err_t files_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/file.png"); }
static esp_err_t folder_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/folder.png"); }
static esp_err_t down_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/download.png"); }
static esp_err_t up_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/upload.png"); }
static esp_err_t del_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/delete.png"); }
static esp_err_t reb_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/reboot.png"); }
static esp_err_t list_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/list.png"); }

/**
 * @brief Find byte sequence in buffer (like memmem but portable)
 */
static uint8_t* findBytes(uint8_t* haystack, size_t haystackLen, const uint8_t* needle, size_t needleLen)
{
    if (needleLen > haystackLen) return NULL;
    for (size_t i = 0; i <= haystackLen - needleLen; i++)
    {
        if (memcmp(haystack + i, needle, needleLen) == 0)
            return haystack + i;
    }
    return NULL;
}

/**
 * @brief File upload handler - supports multiple files in single request
 */
static esp_err_t upload_handler(httpd_req_t *req)
{
    waitScreenRefresh = true;

    // Get content type to parse boundary
    char contentType[256] = "";
    httpd_req_get_hdr_value_str(req, "Content-Type", contentType, sizeof(contentType));

    char* boundaryPtr = strstr(contentType, "boundary=");
    if (!boundaryPtr)
    {
        waitScreenRefresh = false;
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No boundary found");
        return ESP_FAIL;
    }
    boundaryPtr += 9;

    // Build boundary with \r\n-- prefix (between parts)
    char boundary[128];
    snprintf(boundary, sizeof(boundary), "\r\n--%s", boundaryPtr);
    size_t boundaryLen = strlen(boundary);

    // First boundary doesn't have \r\n prefix
    char firstBoundary[128];
    snprintf(firstBoundary, sizeof(firstBoundary), "--%s", boundaryPtr);
    size_t firstBoundaryLen = strlen(firstBoundary);

    // Allocate working buffer
    size_t bufSize = 32768;
    uint8_t* buf = (uint8_t*)heap_caps_malloc(bufSize, MALLOC_CAP_8BIT);
    if (!buf)
    {
        waitScreenRefresh = false;
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    FILE* file = NULL;
    char currentFilename[256] = "";
    int filesUploaded = 0;
    int remaining = req->content_len;
    size_t bufUsed = 0;
    bool firstPart = true;

    while (remaining > 0 || bufUsed > 0)
    {
        // Read more data if buffer has space and data remains
        if (remaining > 0 && bufUsed < bufSize - 8192)
        {
            int toRead = (remaining < 8192) ? remaining : 8192;
            int recv = httpd_req_recv(req, (char*)(buf + bufUsed), toRead);
            if (recv <= 0)
            {
                if (recv == HTTPD_SOCK_ERR_TIMEOUT)
                    continue;
                break;
            }
            remaining -= recv;
            bufUsed += recv;
        }

        // Search for boundary
        const char* searchBoundary = firstPart ? firstBoundary : boundary;
        size_t searchLen = firstPart ? firstBoundaryLen : boundaryLen;
        uint8_t* boundaryPos = findBytes(buf, bufUsed, (const uint8_t*)searchBoundary, searchLen);

        if (!boundaryPos && file)
        {
            // No boundary found - write data keeping buffer for potential split boundary
            size_t safeWrite = (bufUsed > boundaryLen + 4) ? bufUsed - boundaryLen - 4 : 0;
            if (safeWrite > 0)
            {
                fwrite(buf, 1, safeWrite, file);
                memmove(buf, buf + safeWrite, bufUsed - safeWrite);
                bufUsed -= safeWrite;
            }
            if (remaining == 0 && bufUsed > 0)
            {
                // End of data, check for final boundary
                uint8_t* finalBoundary = findBytes(buf, bufUsed, (const uint8_t*)boundary, boundaryLen);
                if (finalBoundary)
                {
                    size_t dataLen = finalBoundary - buf;
                    if (dataLen > 0)
                        fwrite(buf, 1, dataLen, file);
                    bufUsed = 0;
                }
                else
                {
                    fwrite(buf, 1, bufUsed, file);
                    bufUsed = 0;
                }
            }
            esp_task_wdt_reset();
            continue;
        }

        if (boundaryPos)
        {
            // Close previous file if open
            if (file)
            {
                size_t dataLen = boundaryPos - buf;
                if (dataLen > 0)
                    fwrite(buf, 1, dataLen, file);
                storage.close(file);
                file = NULL;
                ESP_LOGI(WEB_TAG, "File uploaded: %s", currentFilename);
                filesUploaded++;
            }

            // Move past boundary
            size_t offset = (boundaryPos - buf) + searchLen;
            firstPart = false;

            // Check for final boundary (--)
            if (offset + 2 <= bufUsed && buf[offset] == '-' && buf[offset + 1] == '-')
            {
                bufUsed = 0;
                break;
            }

            // Skip CRLF after boundary
            if (offset + 2 <= bufUsed && buf[offset] == '\r' && buf[offset + 1] == '\n')
                offset += 2;

            // Shift buffer
            memmove(buf, buf + offset, bufUsed - offset);
            bufUsed -= offset;

            // Find end of headers (\r\n\r\n)
            uint8_t* headerEnd = findBytes(buf, bufUsed, (const uint8_t*)"\r\n\r\n", 4);
            if (!headerEnd && remaining > 0)
            {
                esp_task_wdt_reset();
                continue; // Need more data for headers
            }

            if (headerEnd)
            {
                // Extract filename from headers
                *headerEnd = '\0';
                char* fnStart = strstr((char*)buf, "filename=\"");
                if (fnStart)
                {
                    fnStart += 10;
                    char* fnEnd = strchr(fnStart, '"');
                    if (fnEnd)
                    {
                        size_t fnLen = fnEnd - fnStart;
                        if (fnLen < sizeof(currentFilename))
                        {
                            memcpy(currentFilename, fnStart, fnLen);
                            currentFilename[fnLen] = '\0';

                            // Create directories if path contains /
                            char* lastSlash = strrchr(currentFilename, '/');
                            if (lastSlash)
                            {
                                String pathDir = String(currentFilename).substring(0, lastSlash - currentFilename);
                                createDirectories(pathDir);
                            }

                            char fullPath[512];
                            snprintf(fullPath, sizeof(fullPath), "/sdcard%s/%s", oldDir.c_str(), currentFilename);
                            file = storage.open(fullPath, "w");
                            if (!file)
                                ESP_LOGE(WEB_TAG, "Failed to open: %s", fullPath);
                        }
                    }
                }

                // Move past headers
                size_t headerLen = (headerEnd - buf) + 4;
                memmove(buf, buf + headerLen, bufUsed - headerLen);
                bufUsed -= headerLen;
            }
        }

        esp_task_wdt_reset();

        if (remaining == 0 && !boundaryPos)
            break;
    }

    // Close any remaining file
    if (file)
    {
        if (bufUsed > 0)
        {
            // Strip trailing boundary if present
            uint8_t* finalBoundary = findBytes(buf, bufUsed, (const uint8_t*)boundary, boundaryLen);
            size_t writeLen = finalBoundary ? (finalBoundary - buf) : bufUsed;
            if (writeLen > 0)
                fwrite(buf, 1, writeLen, file);
        }
        storage.close(file);
        ESP_LOGI(WEB_TAG, "File uploaded: %s", currentFilename);
        filesUploaded++;
    }

    heap_caps_free(buf);
    waitScreenRefresh = false;
    updateList = true;

    char response[64];
    snprintf(response, sizeof(response), "Upload complete: %d file(s)", filesUploaded);
    httpd_resp_send(req, response, strlen(response));
    return ESP_OK;
}

/**
 * @brief 404 handler
 */
static esp_err_t notfound_handler(httpd_req_t *req, httpd_err_code_t err)
{
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Not found");
    return ESP_FAIL;
}

/**
 * @brief Set status for polling
 */
void setWebStatus(const char* message, bool refresh)
{
    statusMessage = String(message);
    statusPending = refresh;
}

/**
 * @brief Check if directory deletion is pending
 */
bool isDeleteDirPending()
{
    return deleteDir;
}

/**
 * @brief Get delete path and clear flag
 */
String getDeletePath()
{
    deleteDir = false;
    return deletePath;
}

/**
 * @brief Process directory deletion (call from main loop)
 */
void processWebServerTasks()
{
    if (deleteDir)
    {
        deleteDir = false;
        if (deleteDirRecursive(deletePath.c_str()))
        {
            updateList = true;
            setWebStatus("Folder deleted", true);
        }
    }
}

/**
 * @brief Configure and start web server
 */
void configureWebServer()
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 20;
    config.stack_size = 8192;
    config.lru_purge_enable = true;

    ESP_LOGI(WEB_TAG, "Starting web server on port %d", config.server_port);

    if (httpd_start(&webServer, &config) != ESP_OK)
    {
        ESP_LOGE(WEB_TAG, "Failed to start web server");
        return;
    }

    oldDir = "";

    // Register URI handlers
    httpd_uri_t uri_root = { .uri = "/", .method = HTTP_GET, .handler = root_handler };
    httpd_uri_t uri_status = { .uri = "/status", .method = HTTP_GET, .handler = status_handler };
    httpd_uri_t uri_listfiles = { .uri = "/listfiles", .method = HTTP_GET, .handler = listfiles_handler };
    httpd_uri_t uri_changedirectory = { .uri = "/changedirectory", .method = HTTP_GET, .handler = changedirectory_handler };
    httpd_uri_t uri_file = { .uri = "/file", .method = HTTP_GET, .handler = file_handler };
    httpd_uri_t uri_reboot = { .uri = "/reboot", .method = HTTP_GET, .handler = reboot_handler };
    httpd_uri_t uri_upload = { .uri = "/", .method = HTTP_POST, .handler = upload_handler };

    httpd_uri_t uri_logo = { .uri = "/logo", .method = HTTP_GET, .handler = logo_handler };
    httpd_uri_t uri_files = { .uri = "/files", .method = HTTP_GET, .handler = files_handler };
    httpd_uri_t uri_folder = { .uri = "/folder", .method = HTTP_GET, .handler = folder_handler };
    httpd_uri_t uri_down = { .uri = "/down", .method = HTTP_GET, .handler = down_handler };
    httpd_uri_t uri_up = { .uri = "/up", .method = HTTP_GET, .handler = up_handler };
    httpd_uri_t uri_del = { .uri = "/del", .method = HTTP_GET, .handler = del_handler };
    httpd_uri_t uri_reb = { .uri = "/reb", .method = HTTP_GET, .handler = reb_handler };
    httpd_uri_t uri_list = { .uri = "/list", .method = HTTP_GET, .handler = list_handler };

    httpd_register_uri_handler(webServer, &uri_root);
    httpd_register_uri_handler(webServer, &uri_status);
    httpd_register_uri_handler(webServer, &uri_listfiles);
    httpd_register_uri_handler(webServer, &uri_changedirectory);
    httpd_register_uri_handler(webServer, &uri_file);
    httpd_register_uri_handler(webServer, &uri_reboot);
    httpd_register_uri_handler(webServer, &uri_upload);
    httpd_register_uri_handler(webServer, &uri_logo);
    httpd_register_uri_handler(webServer, &uri_files);
    httpd_register_uri_handler(webServer, &uri_folder);
    httpd_register_uri_handler(webServer, &uri_down);
    httpd_register_uri_handler(webServer, &uri_up);
    httpd_register_uri_handler(webServer, &uri_del);
    httpd_register_uri_handler(webServer, &uri_reb);
    httpd_register_uri_handler(webServer, &uri_list);

    httpd_register_err_handler(webServer, HTTPD_404_NOT_FOUND, notfound_handler);

    ESP_LOGI(WEB_TAG, "Web server started");
}

/**
 * @brief Stop web server
 */
void stopWebServer()
{
    if (webServer)
    {
        httpd_stop(webServer);
        webServer = NULL;
        ESP_LOGI(WEB_TAG, "Web server stopped");
    }
}
