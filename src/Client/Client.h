#ifndef REDES_1_T1_CLIENT_H
#define REDES_1_T1_CLIENT_H


#include "../Network/NetworkNode.h"

/**
 * @brief Represent the client side of the network connection, sending messages to a bash server.
 */
class Client: public NetworkNode {
    using NetworkNode::NetworkNode;

public:
    /// \brief Wait for a new command to be inputted by the user.
    bool waitCommand();

protected:
    /// \brief Show the user the result of a LS command execution.
    /// \return true if the execution was successfull, false otherwise.
    bool handleLS_SHOW() override;

    /// \brief Handle an 'OK' message. (command was executed successfully).
    /// \return true, since the execution was successfull.
    bool handleOk() override;

    /// \brief Handle an 'ERROR' message.
    /// \return false since the execution failed.
    bool handleError() override;

    /// \brief Executes a command in the user's bash.
    std::string execBashCmd(const string &command);

private:
    /// \brief Requests a 'ls' command to the server.
    /// \return true if the execution was successfull, false otherwise.
    bool requestLS();
    /// \brief Requests a 'mkdir' command to the server.
    /// \return true if the execution was successfull, false otherwise.
    bool requestMkdir();
    /// \brief Requests a 'cd' command to the server.
    /// \return true if the execution was successfull, false otherwise.
    void requestCD();
    /// \brief Requests a 'put' command to the server, sending a file to the server.
    /// \return true if the execution was successfull, false otherwise.
    void requestPUT();
    /// \brief Requests a 'get' command to the server, getting a file from the server.
    /// \return true if the execution was successfull, false otherwise.
    void requestGET();

    /// \brief Executes a ls on the client.
    void requestLocalLS();
    /// \brief executes a mkdir on the client.
    void requestLocalMkdir();
};


#endif //REDES_1_T1_CLIENT_H
