#include "Client.h"
#include "../FileHandler/fileHandler.h"
#include <algorithm>

std::string Client::execBashCmd(const string &command) {
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

void Client::requestLocalLS() {
    string lsText;
    getline(cin, lsText);

    try {
        cout << execBashCmd("ls " + lsText);
    } catch (runtime_error& e) {
        cerr << e.what();
    }
}

void Client::requestLocalMkdir() {
    string mkdirText;
    getline(cin, mkdirText);

    try {
        cout << execBashCmd("mkdir " + mkdirText);
    } catch (runtime_error& e) {
        cerr << e.what();
    }
}

bool Client::handleLS_SHOW() {
    logger->info("Handling a LS message.");

    cout << this->getLongStringMessageData();

    return true;
}

bool Client::handleOk() {
    logger->info("Command was executed successfully.");
    this->m_receivedQueue.pop();

    return true;
}

bool Client::handleError() {
    logger->info("Command execution failed: ");

    cerr << this->getLongStringMessageData() << endl;
    this->m_receivedQueue.pop();

    return false;
}

bool Client::waitCommand() {
    cout << "Entre um comando:" << endl;
    string command;
    while (cin >> command && command != "exit") {

        if (command == "ls") {
            this->requestLS();
        }
        else if (command == "cd") {
            this->requestCD();
        }
        else if (command == "mkdir") {
            this->requestMkdir();
        }
        else if (command == "put") {
            this->requestPUT();
        }
        else if (command == "get") {
            this->requestGET();
        }
        else if (command == "lls") {
            requestLocalLS();
        }
        else if (command == "lmkdir") {
            requestLocalMkdir();
        }
        else {
            cout << "Comando invalido." << endl;
        }
    }

    return true;
}

bool Client::requestLS() {
    std::string dirPath;
    getline(cin, dirPath);

    this->enqueueLongStringMessageData(MessageType::LS, 0, dirPath);
    this->sendSequence();

    bool executionResult = this->waitSequence();

    return executionResult;
}

bool Client::requestMkdir() {
    std::string dirPath;
    cin >> dirPath;

    this->enqueueLongStringMessageData(MessageType::MKDIR, 0, dirPath);
    this->sendSequence();

    bool executionResult = this->waitSequence();

    return executionResult;
}

void Client::requestCD() {
    string dirPath;
    cin >> dirPath;

    this->enqueueLongStringMessageData(MessageType::CD, 0, dirPath);
    this->sendSequence();

    this->waitSequence();
}

void Client::requestPUT() {
    std::string filePath, writePath;
    cin >> filePath >> writePath;

    if (!fileExists(filePath)) {
        cerr << "Put error: " << filePath << " file does not exist." << endl;
        return;
    }

    this->enqueueLongStringMessageData(MessageType::PUT, 0, writePath);
    this->sendSequence();
    logger->debug("Waiting answer.");
    bool result = this->waitSequence();
    if (!result) {
        return;
    }

    logger->debug("Preparing file descriptor.");
    string fileName = filePath.substr(filePath.rfind("/") + 1);
    this->m_sendQueue.emplace_back(MessageType::FILE_DESCRIPTOR, 0,
                                   vector<C_BYTE>(fileName.begin(), fileName.end()));
    size_t fileSize = getFileSize(filePath);
    this->m_sendQueue.emplace_back(MessageType::FILE_DESCRIPTOR, 1, fileSize);
    this->m_sendQueue.emplace_back(MessageType::END, 2);
    this->sendSequence();
    result = this->waitSequence();
    if (!result) {
        return;
    }

    logger->debug("Reading file: " + filePath);
    string fileData = readFile(filePath);
    this->enqueueLongStringMessageData(MessageType::FILE_DATA, 0, fileData);
    logger->info("Messages to send: " + to_string(this->m_sendQueue.size()));
    this->sendSequence();
    result = this->waitSequence();

    if (result) {
        cout << "File sent successfully." << endl;
    }
}

void Client::requestGET() {
    std::string filePath, writePath;
    cin >> filePath >> writePath;

    if (!hasWritePermission(writePath)) {
        cerr << "The current user doesn't have write permission in " << writePath << endl;
        return;
    }

    this->enqueueLongStringMessageData(MessageType::GET, 0, filePath);
    this->sendSequence();
    logger->debug("next");

    this->waitSequence(false);
    string fileName;
    try {
        fileName = this->handleFileDescriptor(writePath);
    }
    catch (runtime_error& e) {
        cerr << e.what() << endl;
        this->popEndMessage();
        this->enqueueLongStringMessageData(MessageType::ERROR, 0, e.what());
        this->sendSequence();
        logger->debug(to_string(this->m_receivedQueue.size()));
        return;
    }

    this->m_sendQueue.emplace_back(MessageType::OK, 0);
    this->m_sendQueue.emplace_back(MessageType::END, 1);
    this->sendSequence();

    this->waitSequence(false);
    this->handleFileData(writePath + "/" + fileName);
    this->popEndMessage();

    cout << "File received successfully." << endl;
}
