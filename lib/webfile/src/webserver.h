 /**
 * @file webserver.h
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief Web file server functions
 * @version 0.2.3
 * @date 2025-06
 */

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

String oldDir;                  /**< Previous directory */
String newDir;                  /**< Target/new directory */
String currentDir;              /**< Current directory */

String createDir;               /**< Directory to create */
uint8_t nextSlash = 0;          /**< Index of next slash for parsing paths */

bool updateList = true;         /**< Flag to update file/directory list */
bool deleteDir = false;         /**< Flag to delete a directory */
String deletePath = "";         /**< Path to directory/file to delete */

const int FILES_PER_PAGE = 10;  /**< Number of files per page (for pagination) */

extern Storage storage;

static const char* TAG = "Webserver";

/**
 * @brief File directory cache entry
 *
 * This structure represents an entry in a file directory cache,
 * storing the file or directory name, a flag indicating if it is a directory,
 * and its size in bytes.
 */
struct FileEntry
{
    String name;        /**< Name of the file or directory */
    bool isDirectory;   /**< True if entry is a directory, false if file */
    size_t size;        /**< Size of the file in bytes (0 for directories) */
};
std::vector<FileEntry> fileCache;  /**< Vector containing file/directory cache entries */

/**
 * @brief Web Server Declaration
 *
 * Declares the main AsyncWebServer instance, a server-sent events (SSE) event source for "/eventRefresh",
 * and sets the device hostname.
 */
static AsyncWebServer server(80);                 /**< HTTP server running on port 80 */
static AsyncEventSource eventRefresh("/eventRefresh"); /**< Server-Sent Events endpoint for refresh events */
static const char* hostname = "icenav";           /**< Device hostname */


/**
 * @brief Convert bytes to Human Readable Size
 *
 * Converts a byte count to a human-readable string representation (B, KB, MB, GB).
 *
 * @param bytes Number of bytes
 * @return String Human-readable representation of the size
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
 * @brief Extract numeric part from string starting at position
 *
 * Extracts an integer value from the given string starting at the
 * specified position. Updates the position to the character after
 * the numeric part.
 *
 * @param str Input string to parse
 * @param pos Reference to the starting position; updated after extraction
 * @return int Extracted integer value
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
 * @brief Compare two strings using natural (alphanumeric) order
 *
 * Compares two strings by segments, so that numeric parts are compared as numbers,
 * and character parts are compared lexicographically (case-insensitive).
 *
 * @param a First string to compare
 * @param b Second string to compare
 * @return true if a < b in natural order, false otherwise
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
 * Compares two FileEntry objects for sorting. Directories are ordered before files.
 * If both are the same type, performs a natural alphanumeric comparison on the names.
 *
 * @param a First FileEntry
 * @param b Second FileEntry
 * @return true if a should appear before b, false otherwise
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
 * Sorts the global fileCache vector, places directories before files and sorts names in natural alphanumeric order.
 */
void sortFileCache() 
{
	std::sort(fileCache.begin(), fileCache.end(), compareFileEntries);
}

/**
 * @brief Cache file entries in directory
 *
 * Reads the contents of the specified directory, populates the global fileCache vector
 * with FileEntry objects for each file and subdirectory. For files, the size is determined.
 * For directories, size is set to 0. The fileCache is sorted after reading.
 *
 * @param dir Directory path (relative to /sdcard)
 */
void cacheDirectoryContent(const String& dir) 
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
 * @brief Manage Web not found error
 *
 * Handles HTTP 404 (Not Found) errors for the web server. 
 *
 * @param request Pointer to the AsyncWebServerRequest containing the HTTP request data
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
 * Substitutes specific variable names with their corresponding runtime values for HTML templates.
 *
 * @param var Variable name to substitute (e.g., "FIRMWARE", "FREEFS")
 * @return String Value to replace the variable with
 */
String webParser(const String &var)
{
	SDCardInfo info = storage.getSDCardInfo();

	if (var == "FIRMWARE")
		return String(VERSION) + " - Rev: " + String(REVISION);
	else if (var == "FREEFS")
		return info.free_space.c_str();
	else if (var == "USEDFS")
		return info.used_space.c_str();
	else if (var == "TOTALFS")
		return info.total_space.c_str();
	else if (var == "TYPEFS")
		return info.card_type.c_str();
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
 * Generates a paginated list of files and directories from the file cache, either as HTML or plain text.
 * In HTML mode, displays a table with navigation buttons and options to download or delete files/directories.
 * In plain text mode, lists file names and their sizes.
 *
 * @param ishtml If true, output is formatted as HTML; otherwise, as plain text.
 * @param page Page number for pagination (default: 0).
 * @return String The formatted file list.
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
 * Ensures that all directories in the provided file path exist by creating each
 * missing directory in the path. Returns true if all directories are created/exist,
 * false if any creation fails.
 * 
 * @param filepath Full file path (relative to storage root)
 * @return true if successful, false otherwise
 */
bool createDirectories(String filepath)
{
	uint8_t lastSlash = 0;
	while (true) 
	{
		nextSlash = filepath.indexOf('/', lastSlash + 1);
		String dir = filepath.substring(0, nextSlash);
		
		String newDir = "/sdcard" + oldDir + "/" + dir;
		if (!storage.exists(newDir.c_str()))
		{
			if (!storage.mkdir(newDir.c_str()))
			{
				ESP_LOGE(TAG, "Directory %s creation error", newDir.c_str());
				return false;
			}
			ESP_LOGI(TAG, "Directory %s created",newDir.c_str());
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
 * Handles file upload requests by writing the uploaded data in chunks to storage.
 * Creates necessary directories if required, opens and writes to the file, and handles errors.
 * 
 * @param request Pointer to the AsyncWebServerRequest representing the upload
 * @param filename Name of the file being uploaded (may include path)
 * @param index Current index of the upload data chunk
 * @param data Pointer to the chunk data buffer
 * @param len Length of the current chunk
 * @param final True if this is the last chunk of the file
 */
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
	waitScreenRefresh = true;

	static FILE *file = nullptr;

	uint8_t lastSlashIndex = filename.lastIndexOf("/");
	
	if (lastSlashIndex != 255) 
	{
		String path = filename.substring(0, lastSlashIndex);
		if (createDir != path)
		{
			if (!createDirectories(path))
				ESP_LOGE(TAG, "Directory creation error");
			createDir = path;
		}
	} 

	if (!index)
	{
		String fullPath = "/sdcard" + oldDir + "/" + filename;
		file = storage.open(fullPath.c_str(), "w");
		if (!file) 
		{
			ESP_LOGE(TAG, "Failed to open file for writing: %s", fullPath.c_str());
			request->send(500, "text/plain", "Failed to open file for writing");
			return;
		}
		ESP_LOGI(TAG, "Started writing file: %s", fullPath.c_str());  
	}

	if (file)
	{
		if (fwrite(data, 1, len, file) != len) 
		{
			ESP_LOGE(TAG, "Failed to write data to file");
			request->send(500, "text/plain", "Failed to write data to file");
			storage.close(file);
			file = nullptr;
			return;
		}
	}

	if (final) 
	{
		ESP_LOGI(TAG, "Finished writing file");
		storage.close(file);
		file = nullptr;
		waitScreenRefresh = false;
	}
}

/**
 * @brief Send PNG file from SPIFFS to webpage
 * 
 * Reads a PNG image file from SPIFFS, sends it to the client as an HTTP response,
 * 
 * @param imageFile Path to the image file in storage
 * @param request Pointer to the AsyncWebServerRequest
 */
void sendSpiffsImage(const char *imageFile,AsyncWebServerRequest *request)
{
	FILE *file = storage.open(imageFile,"r");

	if (file)
	{
		size_t size = storage.size(imageFile);

		#ifdef BOARD_HAS_PSRAM
			uint8_t *buffer = (uint8_t*)ps_malloc(sizeof(uint8_t)*size);
		#else
			uint8_t *buffer = (uint8_t*)malloc(sizeof(uint8_t)*size);
		#endif

		storage.read(file,buffer,size);
		storage.close(file);
		request->send_P(200,"image/png",buffer,size);
				
		free(buffer);
	}
}

/**
 * @brief Delete directory recursively
 * 
 * Deletes a directory and all its contents (files and subdirectories) recursively.
 * Uses a non-recursive approach with stacks to avoid stack overflow on deeply nested structures.
 * 
 * @param dirPath Directory path to delete
 * @return true if successful, false otherwise
 */
bool deleteDirRecursive(const char *dirPath)
{
	if (!dirPath || strlen(dirPath) == 0)
	{
		ESP_LOGE(TAG, "Error: Invalid directory path");
		return false;
	}

	// Normalize the input path
	std::string rootDir(dirPath);

	// Use two stacks: one for processing directories and another for tracking deletable directories
	std::stack<std::string> dirStack;       // Stack for directories to process
	std::stack<std::string> deleteStack;    // Stack for directories to delete

	dirStack.push(rootDir);

	while (!dirStack.empty()) 
	{
		std::string currentDir = dirStack.top();
		dirStack.pop();

		ESP_LOGI(TAG, "Processing directory: %s", currentDir.c_str());

		DIR *dir = opendir(currentDir.c_str());
		if (!dir)
		{
			ESP_LOGE(TAG, "Error opening directory: %s", currentDir.c_str());
			return false;
		}

		bool hasEntries = false; // Track if the directory has any entries
		struct dirent *entry;
		while ((entry = readdir(dir)) != NULL) 
		{
			// Skip the current and parent directory entries
			if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
				continue;

			// Construct the full path for the current entry
			char entryPath[PATH_MAX];
			snprintf(entryPath, sizeof(entryPath), "%s/%s", currentDir.c_str(), entry->d_name);

			struct stat entryStat;
			if (stat(entryPath, &entryStat) == -1)
			{
				ESP_LOGE(TAG, "Error getting entry stats for: %s", entryPath);
				closedir(dir);
				return false;
			}

			if (S_ISDIR(entryStat.st_mode)) 
			{
				// Push the subdirectory onto the stack for later processing
				ESP_LOGI(TAG, "Found subdirectory: %s", entryPath);
				dirStack.push(std::string(entryPath));
			}
			else
			{
				// Delete the file
				ESP_LOGI(TAG, "Found file: %s", entryPath);
				if (remove(entryPath) != 0)
				{
					ESP_LOGE(TAG, "Error deleting file: %s", entryPath);
					closedir(dir);
					return false;
				}
				ESP_LOGI(TAG, "Deleted file: %s", entryPath);
			}

			hasEntries = true; // The directory is not empty
		}

		closedir(dir);

		// If the directory is empty, add it to the delete stack
		deleteStack.push(currentDir);
	}

	// Now delete all directories in reverse order (from deepest to root)
	while (!deleteStack.empty()) 
	{
		std::string dirToDelete = deleteStack.top();
		deleteStack.pop();

		ESP_LOGI(TAG, "Deleting directory: %s", dirToDelete.c_str());
		if (rmdir(dirToDelete.c_str()) != 0)
		{
			ESP_LOGE(TAG, "Error deleting directory: %s", dirToDelete.c_str());
			return false;
		}
		ESP_LOGI(TAG, "Deleted directory: %s", dirToDelete.c_str());
	}

	return true;
}

/**
 * @brief Configure Web Server
 * 
 * Sets up all routes, event sources, handlers, and static file/image serving for the web server.
 * Handles file uploads, downloads, directory navigation, image serving, and server reboot via HTTP endpoints.
 */
void configureWebServer()
{
	server.onNotFound(webNotFound);
	server.onFileUpload(handleUpload);
	server.addHandler(&eventRefresh);
	oldDir = "";
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
					{
						esp_task_wdt_reset();
						cacheDirectoryContent(oldDir);
					}
				request->send(200, "text/html", listFiles(true, page)); });

	server.on("/file", HTTP_GET, [](AsyncWebServerRequest *request)
				{
					if (request->hasParam("name") && request->hasParam("action"))
					{
						const char *fileName = request->getParam("name")->value().c_str();
						const char *fileAction = request->getParam("action")->value().c_str();

						String logMessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url() + "?name=" + String(fileName) + "&action=" + String(fileAction);
						String path = "/sdcard" + oldDir + "/" + String(fileName);

						FILE* file = storage.open(path.c_str(), "r");
				
						log_i("folder %s",path.c_str());
						if (!file)
						{
							if (strcmp(fileAction, "deldir") == 0)
							{
								logMessage += " deleted";
								deletePath = path;
								deleteDir = true;
								request->send(200, "text/plain", "Deleting Folder: " + String(fileName) + " please wait...");
								updateList = true;
							}
						else
							request->send(400, "text/plain", "ERROR: file/ does not exist");
						}
						else
						{
						if (strcmp(fileAction, "download") == 0)
							{
								logMessage += " downloaded";             
								AsyncWebServerResponse *response = request->beginChunkedResponse("application/octet-stream", [file](uint8_t *buffer, size_t maxLen, size_t index) -> size_t 
								{
									size_t bytesRead = storage.read(file, buffer, maxLen);
									if (bytesRead == 0)
										storage.close(file);
									return bytesRead;
								});
								response->addHeader("Content-Disposition", "attachment; filename=\"" + path.substring(path.lastIndexOf('/') + 1) + "\"");
								request->send(response);
						}
						// else if (strcmp(fileAction, "deldir") == 0)
						// {
						//   logMessage += " deleted";
						//   deletePath = path;
						//   deleteDir = true;
						//   request->send(200, "text/plain", "Deleting Folder: " + String(fileName) + " please wait...");
						//   updateList = true;
						// }
						else if (strcmp(fileAction, "delete") == 0)
						{
							logMessage += " deleted";
							storage.close(file);
							storage.remove(path.c_str());
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