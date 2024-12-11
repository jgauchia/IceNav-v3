 /**
 * @file webserver.h
 * @author Jordi Gauchía (jgauchia@gmx.es)
 * @brief Web file server functions
 * @version 0.1.9
 * @date 2024-12
 */

#include "SPIFFS.h"
#include <ESPmDNS.h>
#include <esp_task_wdt.h>
#include <algorithm> 

/**
 * @brief Current directory
 * 
 */
String oldDir;
String newDir;
String currentDir;


String createDir;
uint8_t nextSlash = 0;

bool updateList = true;

const int FILES_PER_PAGE = 10; 

/**
 * @brief File directory cache
 *
 */
struct FileEntry {
  String name;
  bool isDirectory;
  size_t size;
};
std::vector<FileEntry> fileCache;

/**
 * @brief Web Server Declaration
 *
 */
static AsyncWebServer server(80);
static const char* hostname = "icenav";

/**
 * @brief Convert bytes to Human Readable Size
 *
 * @param bytes
 * @return String
 */
String humanReadableSize(uint64_t bytes)
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
 * @brief Extract position of numeric part of string
 *
 * @param String
 * @return pos 
 */
int extractNumber(const String& str, int& pos)
{
  int num = 0;
  while (pos < str.length() && isdigit(str[pos])) 
  {
    num = num * 10 + (str[pos] - '0');
    pos++;
  }
  return num;
}

/**
 * @brief Compare alphanumerical two strings
 *
 * @param a -> First string
 * @param b -> Second string
 * @return true/false
 */
bool naturalCompare(const String& a, const String& b) 
{
  int i = 0, j = 0;
  while (i < a.length() && j < b.length()) 
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
 * @brief Compare File Cache entries
 *
 * @param a -> File Cache entries 
 * @param b -> File Cache entries
 * @return true/false
 */
bool compareFileEntries(const FileEntry& a, const FileEntry& b)
{
  if (a.isDirectory != b.isDirectory) 
    return a.isDirectory > b.isDirectory; 

  return naturalCompare(a.name, b.name);
}

/**
 * @brief Sort File Cache entries
 *
 */
void sortFileCache() 
{
  std::sort(fileCache.begin(), fileCache.end(), compareFileEntries);
}

/**
 * @brief Cache file entries in directory
 *
 * @param dir
 */
void cacheDirectoryContent(const String& dir) 
{
  fileCache.clear();  

  File root = SD.open(dir.c_str());
  File foundFile = root.openNextFile();

  while (foundFile)
  {
    FileEntry entry;
    entry.name = foundFile.name();
    entry.isDirectory = foundFile.isDirectory();
    entry.size = foundFile.size();

    fileCache.push_back(entry);
    
    esp_task_wdt_reset(); 

    foundFile = root.openNextFile();
  }

  root.close();
 
  currentDir = dir;

  sortFileCache();
}

/**
 * @brief Manage Web not found error
 *
 * @param request
 */
void webNotFound(AsyncWebServerRequest *request)
{
  String logMessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  log_i("%s", logMessage.c_str());
  request->send(404, "text/plain", "Not found");
}

/**
 * @brief Replace HTML Vars with values
 *
 * @param var
 * @return String
 */
String webParser(const String &var)
{
  if (var == "FIRMWARE")
    return String(VERSION) + " - Rev: " + String(REVISION);
  else if (var == "FREEFS")
    return humanReadableSize((SD.totalBytes() - SD.usedBytes()));
  else if (var == "USEDFS")
    return humanReadableSize(SD.usedBytes());
  else if (var == "TOTALFS")
    return humanReadableSize(SD.cardSize());
  else if (var == "TYPEFS")
  {
    uint8_t cardType = SD.cardType();
    if (cardType == CARD_MMC)
    {
      return "MMC";
    }
    else if (cardType == CARD_SD)
    {
      return "SDSC";
    }
    else if (cardType == CARD_SDHC)
    {
      return "SDHC";
    }
    else
    {
      return "UNKNOWN";
    }
  }
  else
    return "";
}

/**
 * @brief Reboot ESP
 * 
 */
void rebootESP()
{
  log_i("Rebooting ESP32: ");
  ESP.restart();
}

/**
 * @brief List Files in Web Page
 * 
 * @param ishtml 
 * @param page
 * @return String 
 */
String listFiles(bool ishtml, int page = 0)
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

  for (int i = startIdx; i < endIdx && i < fileCache.size(); ++i) 
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

    returnText += "<ti><span> Page " + String(page + 1) + "/" + String((fileCache.size() / 10)+1) + " </span></ti>";

    if (fileCache.size() > endIdx)
    {
      returnText += "<ti><button class=\"button\" onclick='loadPage(" + String(page + 1) + ")'>Next</button></ti>";
      returnText += "<ti><button class=\"button\" onclick='loadPage(" + String((fileCache.size() / 10)) + ")'>Last</button></ti>";
    }

    returnText += "</tr></p>"; 
  }
 
  return returnText;
}

/**
 * @brief Create directories if needed for upload
 * 
 * @param filepath -> Full file path 
 * @return true/false if successful
 */
bool createDirectories(String filepath)
{
  uint8_t lastSlash = 0;
  while (true) 
  {
    nextSlash = filepath.indexOf('/', lastSlash + 1);
    String dir = filepath.substring(0, nextSlash);
        
    if (!SD.exists(oldDir + "/" + dir))
    {
      if (!SD.mkdir(oldDir + "/" + dir))
      {
        log_e("Directory %s creation error", dir.c_str());
        return false;
      }
      log_v("Directory %s created",dir.c_str());
    }
    if (nextSlash == 255) break;
    lastSlash = nextSlash;

    esp_task_wdt_reset();
  }
  return true;
}

/**
 * @brief Upload file handle
 * 
 * @param request 
 * @param filename 
 * @param index 
 * @param data 
 * @param len 
 */
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
  waitScreenRefresh = true;
  uint8_t lastSlashIndex = filename.lastIndexOf("/");
  
  if (lastSlashIndex != 255) 
  {
    String path = filename.substring(0, lastSlashIndex);
    if (createDir != path)
    {
      log_v("%s",path.c_str());
      if (!createDirectories(path))
        log_e("Directory creation error");
      createDir = path;
    }
  } 
  
  if (!index)
  {
    request->client()->setRxTimeout(15000);
    request->_tempFile = SD.open(oldDir + "/" + filename, "w");
  }

  if (len)
    request->_tempFile.write(data, len);

  if (final)
  {
    request->_tempFile.close();
    updateList = true;
    waitScreenRefresh = false;
  }
}

/**
 * @brief Send PNG file from SPIFFS to webpage
 * 
 * @param imageFile
 * @param request 
 */
void sendSpiffsImage(const char *imageFile,AsyncWebServerRequest *request)
{
  FILE *f = fopen(imageFile,"r");
  fseek(f, 0, SEEK_END);
  size_t size = ftell(f);
  rewind(f);
  uint8_t *buffer = (uint8_t*)ps_malloc(sizeof(uint8_t)*size);

  fread(buffer, sizeof(uint8_t),size,f);
  fclose(f);
  request->send_P(200,"image/png",buffer,size);
            
  free(buffer);
}

/**
 * @brief Delete directory recursive
 * 
 * @param dirPath -> directory path
 * @return true/false if successful
 */
bool deleteDirRecursive(const char *dirPath)
{
  String basePath = dirPath;

  log_v("Processing directory: %s", basePath.c_str());

  File dir = SD.open(basePath.c_str());
  if (!dir || !dir.isDirectory())
  {
    log_e("Error: %s isn't a directory or can't open", basePath.c_str());
    return false;
  }

  while (true)
  {
    File entry = dir.openNextFile();
    if (!entry)
    {
      break; 
    }

    if (!basePath.endsWith("/"))
    {
      basePath += "/";
    }

    String entryPath = basePath + entry.name(); 

    if (entry.isDirectory())
    {
        
      log_v("Found subdirectory: %s", entryPath.c_str());
      if (!deleteDirRecursive(entryPath.c_str()))
      {
        entry.close();
        return false;
      }
    }
    else
    {
      log_v("Found file: %s", entryPath.c_str());
      if (!SD.remove(entryPath.c_str()))
      {
        log_e("Error deleting file: %s", entryPath.c_str());
        entry.close();
        return false;
      }
      log_v("Deleted file: %s", entryPath.c_str());
    }
    entry.close();
  }

  dir.close();

  if (basePath.endsWith("/"))
  {
    basePath = dirPath;
  }

  if (!SD.rmdir(basePath.c_str()))
  {
    log_e("Error deleting directory: %s", basePath.c_str());
    return false;
  }

  log_v("Deleted directory: %s", basePath.c_str());
  return true;
}

/**
 * @brief Configure Web Server
 * 
 */
void configureWebServer()
{

  server.onNotFound(webNotFound);
  server.onFileUpload(handleUpload);
  oldDir = "/";
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String logMessage = "Client:" + request->client()->remoteIP().toString() + +" " + request->url();
                log_i("%s", logMessage.c_str());
                request->send_P(200, "text/html", index_html, webParser); 
            });


  server.on("/logo", HTTP_GET, [](AsyncWebServerRequest *request)
            { sendSpiffsImage("/spiffs/LOGO_LARGE.png",request); });
            
  server.on("/files", HTTP_GET, [](AsyncWebServerRequest *request)
            { sendSpiffsImage("/spiffs/file.png",request); });

  server.on("/folder", HTTP_GET, [](AsyncWebServerRequest *request)
            { sendSpiffsImage("/spiffs/folder.png",request); });
  
  server.on("/down", HTTP_GET, [](AsyncWebServerRequest *request)
            { sendSpiffsImage("/spiffs/download.png", request); });
  
  server.on("/up", HTTP_GET, [](AsyncWebServerRequest *request)
            { sendSpiffsImage("/spiffs/upload.png", request); });          

  server.on("/del", HTTP_GET, [](AsyncWebServerRequest *request)
            { sendSpiffsImage("/spiffs/delete.png", request); });

  server.on("/reb", HTTP_GET, [](AsyncWebServerRequest *request)
            { sendSpiffsImage("/spiffs/reboot.png", request); });    

  server.on("/list", HTTP_GET, [](AsyncWebServerRequest *request)
            { sendSpiffsImage("/spiffs/list.png", request); });                    

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String logMessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
              request->send(200, "text/html", reboot_html);
              log_i("%s",logMessage.c_str());
              rebootESP(); });

  server.on("/listfiles", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String logMessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
              log_i("%s", logMessage.c_str());

              int page = 0;
              if (request->hasParam("page"))
              {
                page = request->getParam("page")->value().toInt();
              }
        
              if (updateList)
                cacheDirectoryContent(oldDir);

              request->send(200, "text/html", listFiles(true, page)); });

  server.on("/file", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String logMessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
              log_i("%s", logMessage.c_str());

              if (request->hasParam("name") && request->hasParam("action"))
              {
                const char *fileName = request->getParam("name")->value().c_str();
                const char *fileAction = request->getParam("action")->value().c_str();

                logMessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "?name=" + String(fileName) + "&action=" + String(fileAction);

                String path = oldDir + "/" + String(fileName);
                log_i("%s",path.c_str());

                if (!SD.exists(path))
                {
                  request->send(400, "text/plain", "ERROR: file does not exist");
                }
                else
                {
                  log_i("%s", logMessage + " file exists");
                  if (strcmp(fileAction, "download") == 0)
                  {
                    logMessage += " downloaded";
                    request->send(SD, path, "application/octet-stream");
                  }
                  else if (strcmp(fileAction, "deldir") == 0)
                  {
                    logMessage += " deleted";
                    deleteDirRecursive(path.c_str());
                    request->send(200, "text/plain", "Deleted Folder: " + String(fileName));
                    updateList = true;
                  }
                  else if (strcmp(fileAction, "delete") == 0)
                  {
                    logMessage += " deleted";
                    SD.remove(path);
                    request->send(200, "text/plain", "Deleted File: " + String(fileName));
                    updateList = true;
                  }
                  else
                  {
                    logMessage += " ERROR: invalid action param supplied";
                    request->send(400, "text/plain", "ERROR: invalid action param supplied");
                  }
                  log_i("%s", logMessage.c_str());
                }
              }
              else
              {
                request->send(400, "text/plain", "ERROR: name and action params required");
              } });

  server.on("/changedirectory", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              if (request->hasParam("dir"))
              {
                updateList = false;

                newDir = request->getParam("dir")->value();
                log_i("new dir %s", newDir.c_str());
                log_i("old dir %s", oldDir.c_str());
                // UP Directory
                if (newDir == "/..")
                {
                  if (oldDir != "/..")
                  {
                    oldDir = oldDir.substring(0, oldDir.lastIndexOf("/"));
                    if (oldDir == "")
                      oldDir = "/";
                    currentDir = "";
                    request->send(200, "text/plain", "Path:" + oldDir );
                  }
                  else
                  {
                    request->send(400, "text/plain", "Cannot go up from root directory");
                  }
                }
                else
                // DOWN Directory
                {
                  if (oldDir != "/")
                    oldDir = oldDir + newDir;
                  else
                    oldDir = newDir;
                  currentDir = "";
                  request->send(200, "text/plain", "Path:" + oldDir);
                }

                cacheDirectoryContent(oldDir);

              }
              else
              {
                request->send(400, "text/plain", "ERROR: dir parameter required");
              }
            });
}