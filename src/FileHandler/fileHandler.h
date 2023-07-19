#ifndef REDES_1_T1_FILEHANDLER_H
#define REDES_1_T1_FILEHANDLER_H

#include <string>

using namespace std;


bool writeFile(const string& filePath, const string& data);

string readFile(const string& filePath);

size_t getFileSize(const string& filePath);

pair<bool, string> canWriteFile(const string& filePath, size_t size);

bool hasEnoughSpace(const string& dirPath, size_t size);

bool hasWritePermission(const string& dirPath);

bool fileExists(const string& filePath);

#endif //REDES_1_T1_FILEHANDLER_H
