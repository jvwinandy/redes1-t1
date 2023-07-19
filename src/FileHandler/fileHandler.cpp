#include <sys/statvfs.h>
#include <unistd.h>
#include <fstream>
#include <sys/stat.h>
#include <sstream>

#include "fileHandler.h"

using namespace std;

bool writeFile(const string& filePath, const string& data) {
    ofstream file;
    file.open(filePath);
    file << data;
    file.close();

    return true;
}

string readFile(const string& filePath) {
    std::ifstream  file(filePath);
    std::stringstream buffer;
    buffer << file.rdbuf();

    return buffer.str();
}

size_t getFileSize(const string& filePath) {
    struct stat stat_buf;
    stat(filePath.c_str(), &stat_buf);
    return stat_buf.st_size;
}

pair<bool, string> canWriteFile(const string& filePath, size_t size) {
    string dirPath = filePath.substr(0, filePath.rfind('/'));
    string fileName = filePath.substr(filePath.rfind('/') + 1);

    if (!hasEnoughSpace(dirPath, size)) {
        return {false, "Not enough disk space in " + dirPath};
    }
    else if (!hasWritePermission(dirPath)) {
        return {false, "The current user doesn't have write permission in " + dirPath};
    }
    else if (fileExists(dirPath + "/" + fileName)) {
        return {false, "File already exists in " + dirPath};
    }

    return {true, ""};
}

bool hasEnoughSpace(const string& dirPath, size_t size) {
    struct statvfs stats;
    statvfs(dirPath.c_str(), &stats);
    auto freeSpace = stats.f_bfree * stats.f_frsize;
    return freeSpace > size;
}

bool hasWritePermission(const string& dirPath) {
    return access(dirPath.c_str(), W_OK) == 0;
}

bool fileExists(const string& filePath) {
    return access(filePath.c_str(), F_OK) != -1;
}
