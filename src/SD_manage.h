#ifndef __SD_MANAGE_H__
#define __SD_MANAGE_H__

#define SD_MISO    2
#define SD_MOSI    15
#define SD_SCLK    14
#define SD_CS      13

#include "FS.h"


void listDir(fs::FS &fs, char * dirname, uint8_t levels);
void createDir(fs::FS &fs, char * path);
void removeDir(fs::FS &fs, char * path);
void readFile(fs::FS &fs, char * path);
void writeFile(fs::FS &fs, char * path, char * message);
void appendFile(fs::FS &fs, char * path, char * message);
void renameFile(fs::FS &fs, char * path1, char * path2);
void deleteFile(fs::FS &fs, char * path);
void testFileIO(fs::FS &fs, char * path);
void setup_SD(void);

#endif
