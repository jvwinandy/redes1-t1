#ifndef REDES_1_T1_MESSAGE_H
#define REDES_1_T1_MESSAGE_H

#include <bitset>
#include <vector>
#include "../Logger/Logger.h"

#define BYTE 8
#define T_BYTE bitset<BYTE>
#define C_BYTE unsigned char

// TODO: The minimum size should probably by 64.
#define MIN_SIZE static_cast<size_t>(4)
#define MAX_SIZE static_cast<size_t>(63)
#define MAX_DATA_SIZE (MAX_SIZE - MIN_SIZE)

#define BEGIN_DELIMITER 0b01111110
#define MAX_SEQ 0b1111ul
#define MAX_SEQ_COUNT (MAX_SEQ + 1ul)

using namespace std;

/// \brief The bits represeting each message type. Those bits were chosen by the class...
enum class MessageType {
    OK = 0b000001,
    ACK = 0b000011,
    NACK = 0b000010,
    ERROR = 0b010001,
    CD = 0b000110,
    LS = 0b000111,
    LS_SHOW = 0b111111,
    MKDIR = 0b001000,
    GET = 0b001001,
    FILE_DESCRIPTOR = 0b011000,
    FILE_DATA = 0b100000,
    PUT = 0b001010,
    END = 0b101110,
    INVALID
};
/// \brief Enum to string.
string toString(MessageType messageType);

/**
 * @brief Represents a Message to be sent.
 */
class Message {
public:
    explicit Message(MessageType type, int sequenceId, vector<C_BYTE> &&data) :
            m_type(type), m_sequenceId(sequenceId), m_data(data)
    {
        if (m_data.size() > MAX_DATA_SIZE) {
            logger->error("Trying to create a message with size: " +
                            to_string(m_data.size()) + " content will be truncated.");
            m_data.erase(m_data.begin()+64, m_data.end());
        }
        this->m_size = MIN_SIZE + m_data.size();
        this->calculateParity();
    };

    explicit Message(MessageType type, int sequenceId, unsigned long data) :
            m_type(type), m_sequenceId(sequenceId) {
        this->m_data.push_back(static_cast<C_BYTE>(data));
        this->m_size = MIN_SIZE + m_data.size();
        this->calculateParity();
    }

    explicit Message(MessageType type, int sequenceId) : m_type(type), m_sequenceId(sequenceId) {
        this->m_size = MIN_SIZE;
        this->calculateParity();
    }

    explicit Message(const C_BYTE* bytesMessage);

    /**
     * @brief Construct messages as needed from a string containing the data to be sent.
     * @param type The type of the messages.
     * @param sequence The start sequence of the message.
     * @param stringData The string containing the data to be broken into multiple messages
     * @return A vector containing all the messages needed to send the data.
     */
    static std::vector<Message> fromLongString(MessageType type, int sequence, const string& stringData);

    /// \brief Transforms this message into a vector of bytes.
    vector<C_BYTE> toCharVector();

    size_t getSize() const { return this->m_size.to_ullong(); }
    size_t getSequenceId() const { return this->m_sequenceId.to_ullong(); }

    MessageType getType() const { return this->m_type; }

    vector<C_BYTE> getData() const { return this->m_data; }
    string getDataAsString() const { return string(this->m_data.begin(), this->m_data.end()); }
    unsigned long getDataAsUl() const { return (this->m_data.empty()) ? 0 : static_cast<unsigned long>(this->m_data[0]); }

    bitset<6> getTypeAsBitset() { return static_cast<int>(this->m_type); }

    operator std::string() const;

    bool constructionError = false;

    friend ostream& operator << (ostream& stream, const Message& message);

    friend bool operator== (const Message& msg1, const Message& msg2);
    friend bool operator!= (const Message& msg1, const Message& msg2);

private:
    T_BYTE m_delimiter = BEGIN_DELIMITER;
    bitset<6> m_size;
    bitset<4> m_sequenceId = 0b0;
    MessageType m_type;
    vector<C_BYTE> m_data;
    T_BYTE m_parity = 0b0;

    /// \brief calculates the parity of this message for error checking.
    T_BYTE calculateParity();
    vector<T_BYTE> toByteVector();

    Logger *logger = Logger::getInstance();
};


#endif //REDES_1_T1_MESSAGE_H
