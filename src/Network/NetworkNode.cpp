#include <algorithm>
#include <chrono>
#include "NetworkNode.h"
#include "ConexaoRawSocket.h"
#include "RawSocketIncludes.h"
#include "../FileHandler/fileHandler.h"

unsigned char NetworkNode::message_delimiter = BEGIN_DELIMITER;

NetworkNode::NetworkNode() {
    this->m_socket = ConexaoRawSocket(DEVICE);
    this->logger = Logger::getInstance();

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(this->m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
//    setsockopt(this->m_socket, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);
}

bool NetworkNode::sendSequence() {
    logger->info("Sending sequence of messages.");

    bool sequenceSent = false;
    unsigned long seqStart = 0;
    unsigned long queueIdx = 0;

    this->lastMessageReceived.reset();
    while (!this->m_sendQueue.empty() && !sequenceSent) {
        for (unsigned long i = queueIdx; i < queueIdx + WINDOW_SIZE; i++) {
            this->sendMessage(this->m_sendQueue[i]);
            if (this->m_sendQueue[i].getType() == MessageType::END) {
                break;
            }
        }

        auto startTime = chrono::steady_clock::now();
        while (true) {
            receiveMessage();
            if (!m_receivedBuffer.empty()) {
                Message received = m_receivedBuffer.front();
                m_receivedBuffer.erase(m_receivedBuffer.begin());

                if (received.getType() == MessageType::ACK) {
                    unsigned long acceptedId = received.getDataAsUl();
                    if (acceptedId >= seqStart) {
                        queueIdx = queueIdx + acceptedId - seqStart + 1;
                    }
                    else {
                        queueIdx = queueIdx + ((seqStart + acceptedId + 2) % MAX_SEQ_COUNT) + 1;
                    }
                    seqStart = (acceptedId + 1) % MAX_SEQ_COUNT;
                    break;
                } else if (received.getType() == MessageType::NACK) {
                    unsigned long acceptedId = received.getDataAsUl();
                    if (acceptedId >= seqStart) {
                        queueIdx = queueIdx + acceptedId - seqStart;
                    }
                    else {
                        queueIdx = queueIdx + ((seqStart + acceptedId) % MAX_SEQ_COUNT);
                    }
                    seqStart = acceptedId;
                    break;
                }
            }

            auto timeElapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - startTime).count();
            if (timeElapsed > TIMEOUT) {
                logger->warn("Timeout while waiting for ACK/NACK. Trying to send the message again.");
                break;
            }
        }

        if (queueIdx > 0 && m_sendQueue[queueIdx-1].getType() == MessageType::END) {
            sequenceSent = true;
            this->m_sendQueue.erase(this->m_sendQueue.begin(), this->m_sendQueue.begin() + queueIdx);
            logger->info("Full sequence sent successfully, returning. " + to_string(queueIdx));
        }
    }

    return true;
}

bool NetworkNode::sendMessage(Message message) {
    auto vectorMessage = message.toCharVector();
    long int status = send(this->m_socket, vectorMessage.data(), vectorMessage.size(), 0);

    logger->debug("Sending message: " + (string)message);
    return status == message.getSize();
}

bool NetworkNode::receiveSequence() {
    logger->info("Waiting message sequence.");
    unsigned long startSeq = 0;
    auto startTime = chrono::steady_clock::now();

    while (m_receivedQueue.empty() || !(m_receivedQueue.back().getType() == MessageType::END ||
                                        m_receivedQueue.back().getType() == MessageType::ACK ||
                                        m_receivedQueue.back().getType() == MessageType::NACK)) {
        this->receiveMessage();

        if (this->m_receivedBuffer.size() == WINDOW_SIZE ||
            (!this->m_receivedBuffer.empty() && this->m_receivedBuffer.back().getType() == MessageType::END))
        {
            logger->debug("Received a full sequence.");
            unsigned long nextSeq = this->handleReceivedBuffer(startSeq);
            if (nextSeq != startSeq) {
                unsigned long acceptedSequence = nextSeq == 0 ? MAX_SEQ : nextSeq - 1;
                this->sendMessage(Message(MessageType::ACK, 0, acceptedSequence));
                startSeq = nextSeq;
            }
            else {
                this->sendMessage(Message(MessageType::NACK, 0, startSeq));
            }

            startTime = chrono::steady_clock::now();
        }

        auto timeElapsed = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - startTime).count();
        if (timeElapsed > TIMEOUT) {
            if (!this->m_receivedBuffer.empty()) {
                logger->warn("Timeout while waiting for messages, sending ack/nack.");
                unsigned long nextSeq = this->handleReceivedBuffer(startSeq);
                if (nextSeq != startSeq) {
                    unsigned long acceptedSequence = nextSeq == 0 ? MAX_SEQ : nextSeq - 1;
                    this->sendMessage(Message(MessageType::ACK, 0, acceptedSequence));
                    startSeq = nextSeq;
                } else {
                    this->sendMessage(Message(MessageType::NACK, 0, startSeq));
                }
            }
            startTime = chrono::steady_clock::now();
        }
    }

    return true;
}

bool NetworkNode::receiveMessage() {
    C_BYTE data[MAX_SIZE];
    long int bytesReceived = recv(this->m_socket, data, 64, 0);
    if (bytesReceived < MIN_SIZE || data[0] != NetworkNode::message_delimiter) {
        return false;
    }

    Message received(data);
    if (received.constructionError) {
        return false;
    }

    if (this->lastMessageReceived == nullptr || received != *(this->lastMessageReceived)) {
        this->m_receivedBuffer.push_back(received);
        this->lastMessageReceived = make_unique<Message>(received);
        logger->debug("Received message: " + (string)received);
        return true;
    }
    else {
        logger->debug("Received a duplicate message, ignoring it.");
        return false;
    }
}

unsigned long NetworkNode::handleReceivedBuffer(unsigned long startSeq) {
    sort(this->m_receivedBuffer.begin(), this->m_receivedBuffer.end(), [](const Message& a, const Message& b) {
        if (a.getSequenceId() > 11 && b.getSequenceId() < 4) {
            return true;
        }
        else if (b.getSequenceId() > 11 && a.getSequenceId() < 4) {
            return false;
        }
        return a.getSequenceId() < b.getSequenceId();
    });

    unsigned long expectedSequenceId = startSeq;
    for (const auto & message : this->m_receivedBuffer) {
        if (message.getSequenceId() != expectedSequenceId) {
            logger->info("Unexpected message " + to_string(message.getSequenceId()) + " received. Expected: " + to_string(expectedSequenceId));
            break;
        }
        this->m_receivedQueue.push(message);
        expectedSequenceId = (expectedSequenceId + 1) % MAX_SEQ_COUNT;
    }
    this->m_receivedBuffer.clear();

    return expectedSequenceId;
}

bool NetworkNode::handleReceivedQueue() {
    if (this->m_receivedQueue.empty()) {
        return false;
    }

    Message firstMessage = this->m_receivedQueue.front();

    bool executionResult = false;
    switch (firstMessage.getType()) {
        case MessageType::OK:
            executionResult = this->handleOk();
            break;
        case MessageType::ACK:
            break;
        case MessageType::NACK:
            break;
        case MessageType::ERROR:
            executionResult = this->handleError();
            break;
        case MessageType::CD:
            executionResult = this->handleCD();
            break;
        case MessageType::LS:
            executionResult = this->handleLS();
            break;
        case MessageType::LS_SHOW:
            executionResult = this->handleLS_SHOW();
            break;
        case MessageType::MKDIR:
            executionResult = this->handleMkdir();
            break;
        case MessageType::GET:
            executionResult = this->handleGET();
            break;
        case MessageType::END:
            break;
        case MessageType::PUT:
            executionResult = this->handlePUT();
            break;
        case MessageType::INVALID:
            break;
        default:
            executionResult = false;
            break;
    }

    this->popEndMessage();
    return executionResult;
}

bool NetworkNode::waitSequence(bool handleReceived) {
    while (m_receivedQueue.empty()) {
        this->receiveSequence();
    }
    if (handleReceived) {
        return this->handleReceivedQueue();
    }
    else {
        return true;
    }
}

bool NetworkNode::handleFileData(const string& filePath) {
    logger->info("Writing file to " + filePath);

    string fullFileData = this->getLongStringMessageData();

    writeFile(filePath, fullFileData);

    this->popEndMessage();
    return true;
}

string NetworkNode::handleFileDescriptor(const string &fileWritePath) {
    logger->info("Handling file descriptor.");
    string fileName = this->m_receivedQueue.front().getDataAsString();
    this->m_receivedQueue.pop();
    unsigned long fileSize = this->m_receivedQueue.front().getDataAsUl();
    this->m_receivedQueue.pop();

    if (!hasEnoughSpace(fileWritePath, fileSize)) {
        throw runtime_error("Not enough disk space in ");
    }
    if (fileExists(fileWritePath + "/" + fileName)) {
        throw runtime_error("File: " + fileName + " already exists in " + fileWritePath + "/" + fileName);
    }

    this->popEndMessage();
    return fileName;
}

bool NetworkNode::handleMkdir() {
    this->m_receivedQueue.pop();
    return false;
}

bool NetworkNode::handleOk() {
    this->m_receivedQueue.pop();
    return true;
}

bool NetworkNode::handleError() {
    string errorMessage = this->getLongStringMessageData();
    logger->error("Error received: " + errorMessage);
    return false;
}

bool NetworkNode::handleLS() {
    return false;
}

bool NetworkNode::handleLS_SHOW() {
    return false;
}

bool NetworkNode::handleCD() {
    return false;
}

bool NetworkNode::handlePUT() {
    return false;
}

bool NetworkNode::handleGET() {
    return false;
}

string NetworkNode::getLongStringMessageData() {
    string result;
    while (this->m_receivedQueue.front().getType() != MessageType::END) {
        result += this->m_receivedQueue.front().getDataAsString();
        this->m_receivedQueue.pop();
    }
    return result;
}

void NetworkNode::enqueueLongStringMessageData(MessageType type, int sequence, const string &text) {
    vector<Message> messages = Message::fromLongString(type, sequence, text);
    this->m_sendQueue.insert(this->m_sendQueue.end(), messages.begin(), messages.end());
    this->m_sendQueue.emplace_back(MessageType::END, messages.back().getSequenceId() + 1);
}

void NetworkNode::popEndMessage() {
    if (!this->m_receivedQueue.empty() && this->m_receivedQueue.front().getType() == MessageType::END) {
        // Remove the end from the queue.
        this->m_receivedQueue.pop();
    }
}
