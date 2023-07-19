#ifndef REDES_1_T1_NETWORKNODE_H
#define REDES_1_T1_NETWORKNODE_H

#include <queue>
#include <memory>
#include "../Message/Message.h"
#include "../Logger/Logger.h"

// #define DEVICE "lo"
#define DEVICE "enp5s0"

#define WINDOW_SIZE 4
#define TIMEOUT 5000

/**
 * @brief Abstract class representing a node in the network.
 */
class NetworkNode {
public:
    explicit NetworkNode();
    virtual ~NetworkNode() = default;

    /**
     * @brief Wait until a message sequence is received.
     * @param handleReceived Handle the received message/command in the queue imediately using the handle* methods.
     * @return true if the command execution was a success, false otherwise.
     */
    bool waitSequence(bool handleReceived = true);

    /// \brief Sequence of bits that represents the start of a new message.
    static unsigned char message_delimiter;

protected:
    /// \brief Queue storing the received messages in the correct order.
    queue<Message> m_receivedQueue;
    /// \brief Queue of messages to be sent to another node.
    vector<Message> m_sendQueue;

    /**
     * @brief Get the data from multiple messages as a single string (of the same command, until MessageType::END).
     * @return Returns the string containing the data.
     */
    string getLongStringMessageData();
    /**
     * @brief Create a message sequence from a data field string, breaking it as needed to fit in the maximum size of a message.
     * It also appends the END of the message.
     * @param type The type of the message to be enqueued.
     * @param sequence The start sequence of the message.
     * @param text The data field of the message.
     */
    void enqueueLongStringMessageData(MessageType type, int sequence, const string& text);

    /**
     * @brief Receives a sequence of messages, enqueueing it in the approiate order, while also sendiing ACKs and NACKs
     * as needed, making sure we receive all the correct messages.
     */
    bool receiveSequence();
    /**
     * @brief Sends a message sequence, using the Sliding windo protocol and retransmiting as needed if we receive a NACK.
     */
    bool sendSequence();

    /// \brief Remove the END message from the queue to prepare for a new sequence.
    void popEndMessage();

    /// \brief Handle a 'ls' command message.
    /// \return true if the execution was successfull, false otherwise.
    virtual bool handleLS();
    /// \brief Handle a message showing the result of a 'ls' command execution.
    /// \return true if the execution was successfull, false otherwise.
    virtual bool handleLS_SHOW();
    /// \brief Handle a 'mkdir' command message.
    /// \return true if the execution was successfull, false otherwise.
    virtual bool handleMkdir();
    /// \brief Handle a 'ok' message.
    /// \return true if the execution was successfull, false otherwise.
    virtual bool handleOk();
    /// \brief Handle a 'error' message.
    /// \return true if the execution was successfull, false otherwise.
    virtual bool handleError();
    /// \brief Handle a 'cd' command message.
    /// \return true if the execution was successfull, false otherwise.
    virtual bool handleCD();
    /// \brief Handle a 'put' command message.
    /// \return true if the execution was successfull, false otherwise.
    virtual bool handlePUT();
    /// \brief Handle a 'get' command message.
    /// \return true if the execution was successfull, false otherwise.
    virtual bool handleGET();

    /// \brief Handle a message containing a file, writing the file in the disk as specified by the message containing
    /// its descriptor.
    /// \return true if the execution was successfull, false otherwise.
    virtual bool handleFileData(const string& filePath);
    /// \brief Handle a message containing the description of a file to be written, verifying if the path is valid and
    /// if we have enough disk space to write it.
    /// \return true if the execution was successfull, false otherwise.
    virtual string handleFileDescriptor(const string& fileWritePath);

    /// \brief Stores a pointer to a logger object.
    Logger *logger;

private:
    int m_socket;

    /// \brief Stores the received message unordered, exactly in the way it was received.
    vector<Message> m_receivedBuffer;

    /// \brief Points to the last message we received to avoid duplicates.
    unique_ptr<Message> lastMessageReceived;

    /**
     * @brief handle a full message (from start to END), using the approite handle* method.
     * @return true if the execution was successfull, false otherwise.
     */
    bool handleReceivedQueue();
    /// \brief Receive a single message, storing on the message buffer and ignoring duplicates, noise and corrupted messages.
    /// \return true if we received a valid, non duplicate, non corrupted message.
    bool receiveMessage();
    /// \brief Walk through the buffer ordering the messages and verifying if we got all the messages needed in a sequence.
    /// \param startSeq the start of the sequence we are handling.
    /// \return The next expected message id.
    unsigned long handleReceivedBuffer(unsigned long startSeq);
    /// \brief Send a single message to the connected socket.
    /// \param message the message to be sent.
    /// \return true if the message was sent correctly.
    bool sendMessage(Message message);
};


#endif //REDES_1_T1_NETWORKNODE_H
