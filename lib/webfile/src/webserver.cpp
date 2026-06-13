/**
 * @file webserver.cpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Web file server functions implementation
 * @version 0.2.9
 * @date 2026-06
 */

#include "webserver.h"
#include "webpage.h"
#include <algorithm>
#include <dirent.h>
#include <stdio.h>
#include <stack>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

/**
 * @brief Convert bytes to Human Readable Size
 */
static std::string humanReadableSize(uint64_t bytes)
{
    char buf[32];
    if (bytes < 1024)
        snprintf(buf, sizeof(buf), "%llu B", (unsigned long long)bytes);
    else if (bytes < (1024 * 1024))
        snprintf(buf, sizeof(buf), "%.2f KB", bytes / 1024.0);
    else if (bytes < (1024ULL * 1024 * 1024))
        snprintf(buf, sizeof(buf), "%.2f MB", bytes / (1024.0 * 1024.0));
    else
        snprintf(buf, sizeof(buf), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    return std::string(buf);
}

/**
 * @brief Extract numeric part from string
 */
static int extractNumber(const std::string& str, int& pos)
{
    int num = 0;
    while (pos < (int)str.size() && isdigit(str[pos]))
    {
        num = num * 10 + (str[pos] - '0');
        pos++;
    }
    return num;
}

/**
 * @brief Natural compare for sorting
 */
static bool naturalCompare(const std::string& a, const std::string& b)
{
    int i = 0;
    int j = 0;
    while (i < (int)a.size() && j < (int)b.size())
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
    return a.size() < b.size();
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
static void cacheDirectoryContent(const std::string& dir)
{
    fileCache.clear();
    std::string fullDir = "/sdcard" + dir;

    DIR* dp = opendir(fullDir.c_str());
    if (dp != nullptr)
    {
        struct dirent* ep;
        while ((ep = readdir(dp)))
        {
            FileEntry entry;
            entry.name = std::string(ep->d_name);
            entry.isDirectory = (ep->d_type == DT_DIR);

            if (!entry.isDirectory)
            {
                std::string filePath = fullDir + "/" + entry.name;
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
    char a;
    char b;
    while (*str)
    {
        if ((*str == '%') && ((a = str[1]) && (b = str[2])) && (isxdigit(a) && isxdigit(b)))
        {
            if (a >= 'a')
                a -= 'a' - 'A';
            if (a >= 'A')
                a -= ('A' - 10);
            else
                a -= '0';
            if (b >= 'a')
                b -= 'a' - 'A';
            if (b >= 'A')
                b -= ('A' - 10);
            else
                b -= '0';
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
static std::string listFiles(bool ishtml, int page)
{
    std::string returnText = "";
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
                returnText += "<td><button class=\"button\" onclick=\"downloadFolder('" + entry.name + "')\"><img src=\"down\"> Download (ZIP)</button></td>";
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
        char pageBuf[128];
        int totalPages = (int)(fileCache.size() / FILES_PER_PAGE) + 1;

        returnText += "</table></div><p></p><p>";
        returnText += "<tr align='left'>";
        if (page > 0)
        {
            snprintf(pageBuf, sizeof(pageBuf), "<ti><button class=\"button\" onclick='loadPage(%d)'>First</button></ti>", 0);
            returnText += pageBuf;
            snprintf(pageBuf, sizeof(pageBuf), "<ti><button class=\"button\" onclick='loadPage(%d)'>Prev</button></ti>", page - 1);
            returnText += pageBuf;
        }

        snprintf(pageBuf, sizeof(pageBuf), "<ti><span> Page %d/%d </span></ti>", page + 1, totalPages);
        returnText += pageBuf;

        if ((int)fileCache.size() > endIdx)
        {
            snprintf(pageBuf, sizeof(pageBuf), "<ti><button class=\"button\" onclick='loadPage(%d)'>Next</button></ti>", page + 1);
            returnText += pageBuf;
            snprintf(pageBuf, sizeof(pageBuf), "<ti><button class=\"button\" onclick='loadPage(%d)'>Last</button></ti>", totalPages - 1);
            returnText += pageBuf;
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
                dirStack.push(std::string(entryPath));
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
static bool createDirectories(const std::string& filepath)
{
    size_t lastSlash = 0;
    while (true)
    {
        size_t nextSlash = filepath.find('/', lastSlash + 1);
        std::string dir = filepath.substr(0, nextSlash);
        std::string newDirPath = "/sdcard" + oldDir + "/" + dir;

        if (!storage.exists(newDirPath.c_str()))
        {
            if (!storage.mkdir(newDirPath.c_str()))
            {
                ESP_LOGE(WEB_TAG, "Directory %s creation error", newDirPath.c_str());
                return false;
            }
            ESP_LOGI(WEB_TAG, "Directory %s created", newDirPath.c_str());
        }
        if (nextSlash == std::string::npos)
            break;
        lastSlash = nextSlash;

        esp_task_wdt_reset();
    }
    return true;
}

/**
 * @brief Replace template variables
 */
static std::string processTemplate(const char* html)
{
    std::string result = std::string(html);
    SDCardInfo info = storage.getSDCardInfo();

    std::string firmware = std::string(VERSION) + " - Rev: " + std::to_string(REVISION);

    auto replaceAll = [](std::string& s, const std::string& from, const std::string& to)
    {
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos)
        {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    };

    replaceAll(result, "%FIRMWARE%", firmware);
    replaceAll(result, "%FREEFS%",   info.free_space);
    replaceAll(result, "%USEDFS%",   info.used_space);
    replaceAll(result, "%TOTALFS%",  info.total_space);
    replaceAll(result, "%TYPEFS%",   info.card_type);

    return result;
}

// ============ HTTP Handlers ============

/**
 * @brief Root handler - serves main page
 */
static esp_err_t root_handler(httpd_req_t *req)
{
    std::string html = processTemplate(index_html);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.size());
    return ESP_OK;
}

/**
 * @brief Status polling handler (replaces SSE)
 */
static esp_err_t status_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    std::string escaped = "";
    for (char c : statusMessage)
    {
        if (c == '"' || c == '\\')
            escaped += '\\';
        escaped += c;
    }

    std::string response = "{\"refresh\":";
    response += statusPending ? "true" : "false";
    response += ",\"message\":\"";
    response += escaped;
    response += "\"}";

    if (statusPending)
    {
        statusPending = false;
        statusMessage = "";
    }

    httpd_resp_send(req, response.c_str(), response.size());
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

    std::string html = listFiles(true, page);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html.c_str(), html.size());
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
    newDir = std::string(dirParam);

    ESP_LOGI(WEB_TAG, "new dir %s", newDir.c_str());
    ESP_LOGI(WEB_TAG, "old dir %s", oldDir.c_str());

    if (newDir == "/..")
    {
        if (oldDir != "/..")
        {
            size_t lastSlash = oldDir.rfind('/');
            oldDir = (lastSlash != std::string::npos) ? oldDir.substr(0, lastSlash) : "";
            std::string response = "Path:" + oldDir;
            httpd_resp_send(req, response.c_str(), response.size());
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
        std::string response = "Path:" + oldDir;
        httpd_resp_send(req, response.c_str(), response.size());
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
    std::string path = "/sdcard" + oldDir + "/" + std::string(fileName);

    ESP_LOGI(WEB_TAG, "File operation: %s on %s", fileAction, path.c_str());

    if (strcmp(fileAction, "deldir") == 0)
    {
        deletePath = path;
        deleteDir = true;
        std::string response = std::string("Deleting Folder: ") + fileName + " please wait...";
        httpd_resp_send(req, response.c_str(), response.size());
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

        size_t lastSlash = path.rfind('/');
        std::string filename = (lastSlash != std::string::npos) ? path.substr(lastSlash + 1) : path;
        std::string disposition = "attachment; filename=\"" + filename + "\"";
        httpd_resp_set_hdr(req, "Content-Disposition", disposition.c_str());

        char* chunk = (char*)heap_caps_malloc(4096, MALLOC_CAP_8BIT);
        if (!chunk)
        {
            storage.close(file);
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
            return ESP_FAIL;
        }

        size_t read;
        while ((read = storage.read(file, chunk, 4096)) > 0)
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
        std::string response = std::string("Deleted File: ") + fileName;
        httpd_resp_send(req, response.c_str(), response.size());
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
 * @brief List all files in a folder recursively, one path per line
 */
static esp_err_t listfolder_handler(httpd_req_t *req)
{
    char folderParam[128] = "";
    if (!getQueryParam(req, "path", folderParam, sizeof(folderParam)))
    {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "ERROR: path parameter required");
        return ESP_FAIL;
    }

    urlDecode(folderParam);

    std::string basePath = "/sdcard" + oldDir + "/" + std::string(folderParam);
    std::string result = "";
    size_t prefixLen = std::string("/sdcard").size() + oldDir.size();

    std::stack<std::string> dirStack;
    dirStack.push(basePath);

    while (!dirStack.empty())
    {
        std::string currentPath = dirStack.top();
        dirStack.pop();

        DIR* dp = opendir(currentPath.c_str());
        if (!dp)
            continue;

        struct dirent* ep;
        while ((ep = readdir(dp)))
        {
            if (strcmp(ep->d_name, ".") == 0 || strcmp(ep->d_name, "..") == 0)
                continue;

            std::string entryPath = currentPath + "/" + std::string(ep->d_name);

            if (ep->d_type == DT_DIR)
            {
                dirStack.push(entryPath);
            }
            else
            {
                std::string relPath = entryPath.substr(prefixLen);
                struct stat st;
                size_t fileSize = (stat(entryPath.c_str(), &st) == 0) ? st.st_size : 0;
                char sizeBuf[32];
                snprintf(sizeBuf, sizeof(sizeBuf), "%zu", fileSize);
                result += relPath + "|" + sizeBuf + "\n";
            }
        }
        closedir(dp);
        esp_task_wdt_reset();
    }

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_send(req, result.c_str(), result.size());
    return ESP_OK;
}

/**
 * @brief Reboot handler
 */
static esp_err_t reboot_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, reboot_html, strlen(reboot_html));

    vTaskDelay(pdMS_TO_TICKS(100));
    esp_restart();
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

/**
 * @brief Send JS file from SPIFFS
 */
static esp_err_t sendSpiffsJS(httpd_req_t *req, const char *jsFile)
{
    FILE *file = storage.open(jsFile, "r");
    if (!file)
    {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "JS file not found");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/javascript");

    char* chunk = (char*)heap_caps_malloc(4096, MALLOC_CAP_8BIT);
    if (!chunk)
    {
        storage.close(file);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    size_t read;
    while ((read = storage.read(file, chunk, 4096)) > 0)
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

// Image handlers
static esp_err_t logo_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/gfx/LOGO_LARGE.png"); }
static esp_err_t files_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/gfx/file.png"); }
static esp_err_t folder_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/gfx/folder.png"); }
static esp_err_t down_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/gfx/download.png"); }
static esp_err_t up_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/gfx/upload.png"); }
static esp_err_t del_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/gfx/delete.png"); }
static esp_err_t reb_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/gfx/reboot.png"); }
static esp_err_t list_handler(httpd_req_t *req) { return sendSpiffsImage(req, "/spiffs/gfx/list.png"); }
static esp_err_t jszip_handler(httpd_req_t *req) { return sendSpiffsJS(req, "/spiffs/utils/jszip.min.js"); }

/**
 * @brief Find byte sequence in buffer (like memmem but portable)
 */
static uint8_t* findBytes(uint8_t* haystack, size_t haystackLen, const uint8_t* needle, size_t needleLen)
{
    if (needleLen > haystackLen)
        return NULL;
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

    char boundary[128];
    snprintf(boundary, sizeof(boundary), "\r\n--%s", boundaryPtr);
    size_t boundaryLen = strlen(boundary);

    char firstBoundary[128];
    snprintf(firstBoundary, sizeof(firstBoundary), "--%s", boundaryPtr);
    size_t firstBoundaryLen = strlen(firstBoundary);

    static constexpr size_t UPLOAD_BUF_SIZE = 32768;
    static constexpr size_t MAX_UPLOAD_SIZE = 512UL * 1024 * 1024;

    if ((size_t)req->content_len > MAX_UPLOAD_SIZE)
    {
        waitScreenRefresh = false;
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File too large");
        return ESP_FAIL;
    }

    size_t bufSize = UPLOAD_BUF_SIZE;
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
    bool writeError = false;

    while (remaining > 0 || bufUsed > 0)
    {
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

        const char* searchBoundary = firstPart ? firstBoundary : boundary;
        size_t searchLen = firstPart ? firstBoundaryLen : boundaryLen;
        uint8_t* boundaryPos = findBytes(buf, bufUsed, (const uint8_t*)searchBoundary, searchLen);

        if (!boundaryPos && file)
        {
            size_t safeWrite = (bufUsed > boundaryLen + 4) ? bufUsed - boundaryLen - 4 : 0;
            if (safeWrite > 0)
            {
                if (fwrite(buf, 1, safeWrite, file) != safeWrite)
                {
                    ESP_LOGE(WEB_TAG, "fwrite error (SD full?)");
                    writeError = true;
                }
                memmove(buf, buf + safeWrite, bufUsed - safeWrite);
                bufUsed -= safeWrite;
            }
            if (remaining == 0 && bufUsed > 0)
            {
                uint8_t* finalBoundary = findBytes(buf, bufUsed, (const uint8_t*)boundary, boundaryLen);
                if (finalBoundary)
                {
                    size_t dataLen = finalBoundary - buf;
                    if (dataLen > 0)
                        if (fwrite(buf, 1, dataLen, file) != dataLen)
                        {
                            ESP_LOGE(WEB_TAG, "fwrite error (SD full?)");
                            writeError = true;
                        }
                    bufUsed = 0;
                }
                else
                {
                    if (fwrite(buf, 1, bufUsed, file) != bufUsed)
                    {
                        ESP_LOGE(WEB_TAG, "fwrite error (SD full?)");
                        writeError = true;
                    }
                    bufUsed = 0;
                }
            }
            esp_task_wdt_reset();
            continue;
        }

        if (boundaryPos)
        {
            if (file)
            {
                size_t dataLen = boundaryPos - buf;
                if (dataLen > 0)
                    if (fwrite(buf, 1, dataLen, file) != dataLen)
                    {
                        ESP_LOGE(WEB_TAG, "fwrite error (SD full?)");
                        writeError = true;
                    }
                storage.close(file);
                file = NULL;
                ESP_LOGI(WEB_TAG, "File uploaded: %s", currentFilename);
                filesUploaded++;
            }

            size_t offset = (boundaryPos - buf) + searchLen;
            firstPart = false;

            if (offset + 2 <= bufUsed && buf[offset] == '-' && buf[offset + 1] == '-')
            {
                bufUsed = 0;
                break;
            }

            if (offset + 2 <= bufUsed && buf[offset] == '\r' && buf[offset + 1] == '\n')
                offset += 2;

            memmove(buf, buf + offset, bufUsed - offset);
            bufUsed -= offset;

            uint8_t* headerEnd = findBytes(buf, bufUsed, (const uint8_t*)"\r\n\r\n", 4);
            if (!headerEnd && remaining > 0)
            {
                esp_task_wdt_reset();
                continue;
            }

            if (headerEnd)
            {
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

                            char* lastSlash = strrchr(currentFilename, '/');
                            if (lastSlash)
                            {
                                std::string pathDir = std::string(currentFilename, lastSlash - currentFilename);
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

                size_t headerLen = (headerEnd - buf) + 4;
                memmove(buf, buf + headerLen, bufUsed - headerLen);
                bufUsed -= headerLen;
            }
        }

        esp_task_wdt_reset();

        if (remaining == 0 && !boundaryPos)
            break;
    }

    if (file)
    {
        if (bufUsed > 0)
        {
            uint8_t* finalBoundary = findBytes(buf, bufUsed, (const uint8_t*)boundary, boundaryLen);
            size_t writeLen = finalBoundary ? (finalBoundary - buf) : bufUsed;
            if (writeLen > 0)
                if (fwrite(buf, 1, writeLen, file) != writeLen)
                {
                    ESP_LOGE(WEB_TAG, "fwrite error (SD full?)");
                    writeError = true;
                }
        }
        storage.close(file);
        ESP_LOGI(WEB_TAG, "File uploaded: %s", currentFilename);
        filesUploaded++;
    }

    heap_caps_free(buf);
    waitScreenRefresh = false;

    if (writeError)
    {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write error (SD full?)");
        return ESP_FAIL;
    }

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
    statusMessage = std::string(message);
    statusPending = refresh;
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
    httpd_uri_t uri_listfolder = { .uri = "/listfolder", .method = HTTP_GET, .handler = listfolder_handler };
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
    httpd_uri_t uri_jszip = { .uri = "/jszip", .method = HTTP_GET, .handler = jszip_handler };

    httpd_register_uri_handler(webServer, &uri_root);
    httpd_register_uri_handler(webServer, &uri_status);
    httpd_register_uri_handler(webServer, &uri_listfiles);
    httpd_register_uri_handler(webServer, &uri_changedirectory);
    httpd_register_uri_handler(webServer, &uri_file);
    httpd_register_uri_handler(webServer, &uri_listfolder);
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
    httpd_register_uri_handler(webServer, &uri_jszip);

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
