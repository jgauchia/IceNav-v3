/**
 * @file storage.hpp
 * @author Jordi Gauchía (jgauchia@jgauchia.com)
 * @brief  Storage definition and functions
 * @version 0.2.0_alpha
 * @date 2025-03
 */

#ifndef STORAGE_HPP
#define STORAGE_HPP

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

class FileStream : public Stream
{
public:
    FileStream(FILE *file) : file(file) {}

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

    virtual int read() override
    {
        if (!file)
            return -1;
        return fgetc(file);
    }

    virtual size_t read(uint8_t *buffer, size_t size)
    {
        if (!file)
            return 0;
        return fread(buffer, 1, size, file);
    }

    virtual size_t readBytes(char *buffer, size_t length) override
    {
        if (!file)
            return 0;
        return fread(buffer, 1, length, file);
    }

    virtual int peek() override
    {
        if (!file)
            return -1;
        int c = fgetc(file);
        if (c != EOF)
        {
            ungetc(c, file);
        }
        return c;
    }

    virtual void flush() override
    {
        if (file)
        {
            fflush(file);
        }
    }

    size_t write(uint8_t) override
    {
        // Not implemented
        return 0;
    }

    size_t write(const uint8_t *, size_t) override
    {
        // Not implemented
        return 0;
    }

private:
    FILE *file;
};

class Storage
{
private:
    bool isSdLoaded;
    static const uint32_t sdFreq = 40000000;
    sdmmc_card_t *card;

public:
    Storage();

    esp_err_t initSD();
    esp_err_t initSPIFFS();
    bool getSdLoaded() const;
    FILE *open(const char *path, const char *mode);
    int close(FILE *file);
    size_t size(const char *path);
    size_t read(FILE* file, uint8_t* buffer, size_t size);
    size_t read(FILE* file, char* buffer, size_t size);
    size_t write(FILE* file, const uint8_t* buffer, size_t size);
    bool exists(const char *path);
    bool mkdir(const char *path);
    bool remove(const char *path);
    bool rmdir(const char *path);
    size_t fileAvailable(FILE* file);
};

#endif