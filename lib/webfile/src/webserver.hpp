/**
 * @file webserver.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Web file server declarations
 * @version 0.2.1_alpha
 * @date 2025-04
 */

#pragma once

#include "storage.hpp"
#include <ESPmDNS.h>
#include <esp_task_wdt.h>
#include <algorithm>
#include <dirent.h>
#include <stdio.h>
#include <stack>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "webpage.hpp"

extern String oldDir;
extern String newDir;
extern String currentDir;
extern String createDir;
extern uint8_t nextSlash;

extern bool updateList;
extern bool deleteDir;
extern String deletePath;

extern const int FILES_PER_PAGE;

extern Storage storage;
extern bool waitScreenRefresh;

struct FileEntry
{
  String name;
  bool isDirectory;
  size_t size;
};
extern std::vector<FileEntry> fileCache;

extern AsyncWebServer server;
extern AsyncEventSource eventRefresh;
extern const char* hostname;

// Function declarations
String humanReadableSize(uint64_t bytes);
int extractNumber(const String& str, int& pos);
bool naturalCompare(const String& a, const String& b);
bool compareFileEntries(const FileEntry& a, const FileEntry& b);
void sortFileCache();
void cacheDirectoryContent(const String& dir);

void webNotFound(AsyncWebServerRequest *request);
String webParser(const String &var);
void rebootESP();
String listFiles(bool ishtml, int page = 0);
bool createDirectories(String filepath);
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void sendSpiffsImage(const char *imageFile, AsyncWebServerRequest *request);
bool deleteDirRecursive(const char *dirPath);
void configureWebServer();