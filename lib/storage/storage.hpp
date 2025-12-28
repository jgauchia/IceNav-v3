/**
 * @file storage.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Storage definition and functions
 * @version 0.2.4
 * @date 2025-12
 */

#pragma once

#include "esp_spiffs.h"
#include "esp_err.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "Stream.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>
#include <utime.h>
#include <string>

#ifdef SPI_SHARED
    #include "Arduino.h"
    #include "SD.h"
#endif

/**
 * @brief Structure for SD Card Information
 *
 * @details Contains descriptive and capacity-related information about an SD card,
 * 			including its name, capacity, sector size, read block length, type, and space details.
 */
struct SDCardInfo
{
    std::string name;         /**< Card name */
    std::string capacity;     /**< Card capacity as a string */
    int sector_size;          /**< Size of a sector in bytes */
    int read_block_len;       /**< Read block length in bytes */
    std::string card_type;    /**< Card type (e.g., SDHC, SDXC) */
    std::string total_space;  /**< Total space as a string */
    std::string free_space;   /**< Free space as a string */
    std::string used_space;   /**< Used space as a string */
};

/**
 * @class FileStream
 * @brief FileStream class to wrap FILE* operations as a Stream
 *
 * @details Provides a Stream-compatible interface for reading from and flushing a standard C FILE*.
 * 			Write operations are not implemented.
 */
class FileStream : public Stream
{
    public:
        FileStream(FILE *file) : file(file) {}

        /**
        * @brief Returns the number of bytes available to read from the file.
        * @return Number of available bytes, or 0 if file is nullptr.
        */
        virtual int available() override
        {
            if (!file)
                return 0;
            long current_pos = ftell(file);
            fseek(file, 0, SEEK_END);
            long end_pos = ftell(file);
            fseek(file, current_pos, SEEK_SET);
            return end_pos - current_pos;
        }

        /**
        * @brief Reads a single byte from the file.
        * @return The byte read, or -1 if file is nullptr or EOF.
        */
        virtual int read() override
        {
            if (!file)
                return -1;
            return fgetc(file);
        }

        /**
        * @brief Reads up to size bytes into the buffer.
        * @param buffer Buffer to store read bytes
        * @param size Maximum number of bytes to read
        * @return Number of bytes actually read
        */
        virtual size_t read(uint8_t *buffer, size_t size)
        {
            if (!file)
                return 0;
            return fread(buffer, 1, size, file);
        }

        /**
        * @brief Reads up to length bytes into the buffer (char version).
        * @param buffer Buffer to store read bytes
        * @param length Maximum number of bytes to read
        * @return Number of bytes actually read
        */
        virtual size_t readBytes(char *buffer, size_t length) override
        {
            if (!file)
                return 0;
            return fread(buffer, 1, length, file);
        }

        /**
        * @brief Peeks at the next byte in the file without advancing the file pointer.
        * @return The next byte, or -1 if file is nullptr or EOF.
        */
        virtual int peek() override
        {
            if (!file)
                return -1;
            int c = fgetc(file);
            if (c != EOF)
                ungetc(c, file);
            return c;
        }

        /**
        * @brief Flushes the file output buffer.
        */
        virtual void flush() override
        {
            if (file)
                fflush(file);
        }

        /**
        * @brief Not implemented: Write a single byte to the file.
        */
        size_t write(uint8_t) override
        {
            // Not implemented
            return 0;
        }

        /**
        * @brief Not implemented: Write multiple bytes to the file.
        */
        size_t write(const uint8_t *, size_t) override
        {
            // Not implemented
            return 0;
        }

    private:
        FILE *file; /**< Pointer to the wrapped C FILE object */
};

/**
 * @class Storage
 * @brief Storage class for SD and SPIFFS operations
 *
 * @details Provides an abstraction for file and directory operations on SD cards and SPIFFS,
 * 			including initialization, basic file I/O, and SD card information retrieval.
 */
class Storage
{
    private:
        bool isSdLoaded;           /**< Indicates if the SD card is loaded */
        sdmmc_card_t *card;        /**< Pointer to the SD card descriptor */

    public:
        Storage();

        esp_err_t initSD();
        esp_err_t initSPIFFS();
        SDCardInfo getSDCardInfo();
        bool getSdLoaded() const;
        FILE *open(const char *path, const char *mode);
        int close(FILE *file);
        bool exists(const char *path);
        bool mkdir(const char *path);
        bool remove(const char *path);
        bool rmdir(const char *path);
        size_t size(const char *path);
        size_t read(FILE* file, uint8_t* buffer, size_t size);
        size_t read(FILE* file, char* buffer, size_t size);
        size_t write(FILE* file, const uint8_t* buffer, size_t size);
        size_t write(FILE* file, const char* buffer, size_t size);
        int seek(FILE* file, long offset, int whence);
        int print(FILE* file, const char* str);
        int println(FILE* file, const char* str);
        size_t fileAvailable(FILE* file);
};