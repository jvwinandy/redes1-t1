#include <vector>
#include <iostream>
#include <sstream>
#include "Message.h"
#include "../Network/NetworkNode.h"

string toString(MessageType messageType) {
    switch (messageType) {
        case MessageType::OK:
            return "OK";
        case MessageType::ACK:
            return "ACK";
        case MessageType::NACK:
            return "NACK";
        case MessageType::ERROR:
            return "ERROR";
        case MessageType::CD:
            return "CD";
        case MessageType::LS:
            return "LS";
        case MessageType::LS_SHOW:
            return "LS_SHOW";
        case MessageType::MKDIR:
            return "MKDIR";
        case MessageType::GET:
            return "GET";
        case MessageType::FILE_DATA:
            return "FILE_DATA";
        case MessageType::FILE_DESCRIPTOR:
            return "FILE_DESCRIPTOR";
        case MessageType::PUT:
            return "PUT";
        case MessageType::END:
            return "END";
        case MessageType::INVALID:
            return "INVALID";
        default:
            return "Default";
    }
}

Message::Message(const C_BYTE *bytesMessage) {
    size_t messageIdx = 1;

    C_BYTE sizeSequence = bytesMessage[messageIdx++];
    this->m_size = sizeSequence >> 2;

    C_BYTE sequenceType = bytesMessage[messageIdx++];
    this->m_sequenceId = (sizeSequence & 0b00000011) << 2 | sequenceType >> 6;
    this->m_type = static_cast<MessageType>(sequenceType & 0b00111111);

    size_t dataSize = this->getSize() - MIN_SIZE;
    if (dataSize > MAX_DATA_SIZE) {
        logger->error("Received an invalid message with size: " + to_string(dataSize));
        this->constructionError = true;
    }
    else {
        this->m_data.reserve(dataSize);
        for (int _ = 0; _ < dataSize; _++) {
            this->m_data.push_back(bytesMessage[messageIdx++]);
        }
    }

    calculateParity();
    if (this->m_parity != bytesMessage[messageIdx++]) {
        logger->warn("Invalid parity received from: " + static_cast<string>(*this));
        this->constructionError = true;
    }
}

T_BYTE Message::calculateParity() {
    this->m_parity = 0b0;
//    T_BYTE parity = 0b00000000;
    T_BYTE parity = NetworkNode::message_delimiter;

    auto vectorMessage = this->toByteVector();
    for (const auto &byte: vectorMessage) {
        parity = parity ^ byte;
    }

    this->m_parity = parity;
    return parity;
}

vector<T_BYTE> Message::toByteVector() {
    vector<T_BYTE> vectorMessage;
    vectorMessage.reserve(this->getSize());

    // TODO: Change back to this->m_delimiter.
    if (DEVICE == "lo") {
        vectorMessage.emplace_back(~NetworkNode::message_delimiter);
    }
    else {
        vectorMessage.emplace_back(NetworkNode::message_delimiter);
    }

    string sizeSequenceType = this->m_size.to_string();
    sizeSequenceType += this->m_sequenceId.to_string();
    sizeSequenceType += this->getTypeAsBitset().to_string();

    T_BYTE byte1(sizeSequenceType.substr(0, 8));
    T_BYTE byte2(sizeSequenceType.substr(8, 8));
    vectorMessage.push_back(byte1);
    vectorMessage.push_back(byte2);

    for (const auto& dataByte : this->m_data) {
        vectorMessage.emplace_back(dataByte);
    }

    vectorMessage.push_back(this->m_parity);

    return vectorMessage;
}

vector<C_BYTE> Message::toCharVector() {
    vector<C_BYTE> vectorMessage;
    vectorMessage.reserve(64);

    auto byteMessage = this->toByteVector();
    for (const auto& byte : byteMessage) {
        vectorMessage.push_back(static_cast<C_BYTE>(byte.to_ulong()));
    }

    // fill the message with trash to get to the minimum of 64 bytes.
    if (vectorMessage.size() < 64) {
        vectorMessage.insert(vectorMessage.end(), 64 - vectorMessage.size(), 0);
    }

    return vectorMessage;
}

ostream& operator<<(ostream& stream, const Message& message) {
    stream << "Size: " << message.getSize() << ", Sequence: " << message.m_sequenceId.to_ulong();
    stream << ", Type: " << toString(message.m_type);
    if (!message.getData().empty()) {
        stream << ", Data_str: " << message.getDataAsString();
        stream << ", Data_ul: " << message.getDataAsUl();
    }
    stream << ", Parity: " << message.m_parity;
    return stream;
}

Message::operator std::string() const {
    stringstream ss;
    ss << *this;
    return ss.str();
}

bool operator==(const Message &msg1, const Message &msg2) {
    return msg1.m_sequenceId == msg2.m_sequenceId && msg1.m_type == msg2.m_type && msg1.m_parity == msg2.m_parity &&
            msg1.m_size == msg2.m_size && msg1.m_data == msg2.m_data;
}

bool operator!=(const Message &msg1, const Message &msg2) {
    return !(msg1 == msg2);
}

std::vector<Message> Message::fromLongString(MessageType type, int sequence, const string &stringData) {
    std::vector<Message> messages;

    if (stringData.empty()) {
        messages.emplace_back(type, sequence);
    }

    size_t index = 0;
    while (index < stringData.size()) {
        if (stringData.size() >= MAX_DATA_SIZE) {
            string substr = stringData.substr(index, min(stringData.size() - index, static_cast<size_t>(MAX_DATA_SIZE)));
            messages.emplace_back(type, sequence, vector<C_BYTE>(substr.begin(), substr.end()));
        }
        else {
            messages.emplace_back(type, sequence, vector<C_BYTE>(stringData.begin(), stringData.end()));
        }
        index += MAX_DATA_SIZE;
        sequence++;
    }

    return messages;
}
