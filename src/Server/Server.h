#ifndef REDES_1_T1_SERVER_H
#define REDES_1_T1_SERVER_H


#include "../Network/NetworkNode.h"

/**
 * @brief The server side of the connection, which handles messages send from the client.
 */
class Server: public NetworkNode {
    using NetworkNode::NetworkNode;

protected:
    /// \brief Handle a 'ls' command from the client, sending it the list of files in the current directoy.
    /// \return true if the execution was successfull, false otherwise.
    bool handleLS() override;
    /// \brief Handle a 'mkdir' command from the client, creating a new directory.
    /// \return true if the execution was successfull, false otherwise.
    bool handleMkdir() override;
    /// \brief Handle a 'cd' command from the client, changing the current directoy.
    /// \return true if the execution was successfull, false otherwise.
    bool handleCD() override;
    /// \brief Handle a 'put' command from the client, putting a file from the client into the server.
    /// \return true if the execution was successfull, false otherwise.
    bool handlePUT() override;
    /// \brief Handle a 'get' command from the client, sending it a file.
    /// \return true if the execution was successfull, false otherwise.
    bool handleGET() override;

private:
    /// \brief Send a execution error to the client.
    /// \param errorText The error description.
    void sendError(const string& errorText);

    /// \brief Send a execution success to the client.
    void sendOk();

    /// \brief executes a command in the server bash.
    std::string execBashCmd(const string& command);

    std::string getCompletePath(const string& pathExtension);

    string m_currentDirectory = "/";

    // TODO: used for debugging. Remove this.
    int m_debugCounter = 1;
};


#endif //REDES_1_T1_SERVER_H
