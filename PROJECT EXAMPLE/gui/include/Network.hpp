#pragma once

#include <string>
#include <queue>
#include <thread>
#include <mutex>

class Network {
public:
    Network();
    ~Network();

    bool Connect(const std::string& host, int port);
    void Disconnect();
    bool SendCommand(const std::string& command);
    bool ReceiveMessage(std::string& message);
    bool IsConnected() const { return m_connected; }

private:
    int m_socket = -1;
    bool m_connected = false;
    
    // Receive buffer
    std::string m_buffer;
    std::queue<std::string> m_messageQueue;
    std::mutex m_queueMutex;
    
    // Background receive thread
    std::thread m_receiveThread;
    bool m_running = false;
    void ReceiveLoop();
    
    // Helper
    bool ReadLine(std::string& line);
}; 