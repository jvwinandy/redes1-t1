#include <memory>
#include "Server.h"
#include "../FileHandler/fileHandler.h"

void Server::sendError(const string &errorText) {
    this->enqueueLongStringMessageData(MessageType::ERROR, 0, errorText);
    this->sendSequence();
}

void Server::sendOk() {
    this->m_sendQueue.emplace_back(MessageType::OK, 0);
    this->m_sendQueue.emplace_back(MessageType::END, 1);
    this->sendSequence();
}

std::string Server::getCompletePath(const string &pathExtension) {
    if (pathExtension[0] == '/') {
        return pathExtension;
    }
    else {
        return this->m_currentDirectory + "/" + pathExtension;
    }
}


std::string Server::execBashCmd(const string &command) {
    logger->info("Executing bash command: " + command);

    string redirected_command = command + " 2>&1";

    string result;
    unique_ptr<FILE, decltype(&pclose)> pipe(popen(redirected_command.c_str(), "r"), pclose);
    if (!pipe) {
        logger->error("Command execution failed: " + command);
        throw runtime_error("Command execution failed: " + command);
    }

    char buffer[1024];
    while (fgets(buffer, 1024, pipe.get()) != nullptr) {
        result += buffer;
    }
    logger->debug("Command result: " + result);
    return result;
}

bool Server::handleLS() {
    logger->info("Handling a LS message");

    string dirPath = this->getLongStringMessageData();
    string arg = "";
    auto itStart = dirPath.find(" -");
    auto it = itStart;
    while (it < dirPath.size() && dirPath[it] != ' ') {
        arg += dirPath[it];
        it++;
    }

    dirPath = dirPath.substr(it);

    try {
        string result = execBashCmd("ls "+ arg + this->getCompletePath(dirPath));
        if (!result.empty()) {
            this->enqueueLongStringMessageData(MessageType::LS_SHOW, 0, result);
            this->sendSequence();
            return true;
        }
    }
    catch (runtime_error& error) {
        this->sendError(error.what());
        return false;
    }
}

bool Server::handleMkdir() {
    logger->info("Handling a MKDIR message");

    string dirPath = this->getLongStringMessageData();

    try {
        string result = execBashCmd("mkdir " + this->getCompletePath(dirPath));
        if (!result.empty()) {
            this->sendError(result);
            return false;
        }
    }
    catch (runtime_error& error) {
        this->sendError(error.what());
        return false;
    }

    this->sendOk();
    return true;
}

bool Server::handleCD() {
    logger->info("Handling a CD message: " + (string)this->m_receivedQueue.front());

    string dirPath = this->getCompletePath(this->getLongStringMessageData());
    try {
        string result = execBashCmd("cd " + dirPath);
        if (!result.empty()) {
            this->sendError(result);
            return false;
        }
    }
    catch (runtime_error& error) {
        this->sendError(error.what());
        return false;
    }

    this->m_currentDirectory = dirPath;
    this->sendOk();
    return true;
}

bool Server::handlePUT() {
    logger->info("Handling a PUT message");

    string fileWritePath = this->getCompletePath(this->getLongStringMessageData());
    this->popEndMessage();

    if (!hasWritePermission(fileWritePath)) {
        this->sendError( "The current user doesn't have write permission in " + fileWritePath);
        return false;
    }
    this->sendOk();

    this->waitSequence(false);
    string fileName;
    try {
        // TODO: Should we handle incorrect MessageTypes here?
        fileName = this->handleFileDescriptor(fileWritePath);
    } catch (runtime_error& e) {
        this->sendError(e.what());
        return false;
    }
    this->sendOk();

    this->waitSequence(false);

    this->handleFileData(fileWritePath + "/" + fileName);
    this->sendOk();
    return true;
}

bool Server::handleGET() {
    logger->info("Handling a GET message");

    string filePath = this->getCompletePath(this->getLongStringMessageData());
    this->popEndMessage();

    if (!fileExists(filePath)) {
        cerr << "Get error: " << filePath << " file does not exist." << endl;
        return false;
    }

    logger->debug("Preparing file descriptor.");
    string fileName = filePath.substr(filePath.rfind("/") + 1);
    this->m_sendQueue.emplace_back(MessageType::FILE_DESCRIPTOR, 0,
                                   vector<C_BYTE>(fileName.begin(), fileName.end()));
    size_t fileSize = getFileSize(filePath);
    this->m_sendQueue.emplace_back(MessageType::FILE_DESCRIPTOR, 1, fileSize);
    this->m_sendQueue.emplace_back(MessageType::END, 2);
    this->sendSequence();
    bool result = this->waitSequence();
    if (!result) {
        logger->info("Client recused file, cancelling send operation.");
        return false;
    }

    logger->debug("Reading file: " + filePath);
    string fileData = readFile(filePath);
    this->enqueueLongStringMessageData(MessageType::FILE_DATA, 0, fileData);
    logger->info("Messages to send: " + to_string(this->m_sendQueue.size()));
    this->sendSequence();

    logger->info("Full file sent.");

    return true;
}
