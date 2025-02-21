#ifndef __SD_MANAGE_H__
#define __SD_MANAGE_H__

#define SD_MISO    2
#define SD_MOSI    15
#define SD_SCLK    14
#define SD_CS      13

#include "FS.h"
#include "config.h"
#include <message_queue.hpp>
#include <thread.hpp>

extern freertos::message_queue<uint8_t[CAN_MSG_SIZE]> queue;   // CAN Message Queue

class SD_Manage {
    private:
        freertos::thread SD_thread;
        void writeBinary(const char* path, uint8_t message[12]);
    public:
        SD_Manage();
        void exampleSD(void);
        void listDir(fs::FS &fs, const char * dirname, uint8_t levels);
        void createDir(fs::FS &fs, const char * path);
        void removeDir(fs::FS &fs, const char * path);
        void readFile(fs::FS &fs, const char * path);
        void writeFile(const char * path, const char * message);
        void appendFile(fs::FS &fs, const char * path, const char * message);
        void renameFile(fs::FS &fs, const char * path1, const char * path2);
        void testFileIO(fs::FS &fs, const char * path);
        void deleteFile(fs::FS &fs, const char * path);

};

#endif
