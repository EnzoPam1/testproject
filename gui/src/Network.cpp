#include "Network.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

Network::Network() {
}

Network::~Network() {
    Disconnect();
}

bool Network::Connect(const std::string& host, int port) {
    m_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (m_socket < 0) return false;
    
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
    
    if (connect(m_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(m_socket);
        return false;
    }
    
    m_connected = true;
    m_running = true;
    m_receiveThread = std::thread(&Network::ReceiveLoop, this);
    
    return true;
}

void Network::Disconnect() {
    m_running = false;
    m_connected = false;
    
    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
    }
    
    if (m_receiveThread.joinable()) {
        m_receiveThread.join();
    }
}

bool Network::SendCommand(const std::string& command) {
    if (!m_connected) return false;
    
    std::string msg = command + "\n";
    return send(m_socket, msg.c_str(), msg.length(), 0) == msg.length();
}

bool Network::ReceiveMessage(std::string& message) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    if (m_messageQueue.empty()) return false;
    
    message = m_messageQueue.front();
    m_messageQueue.pop();
    return true;
}

void Network::ReceiveLoop() {
    char buffer[1024];
    
    while (m_running && m_connected) {
        int n = recv(m_socket, buffer, sizeof(buffer) - 1, 0);
        if (n <= 0) break;
        
        buffer[n] = '\0';
        m_buffer += buffer;
        
        std::string line;
        while (ReadLine(line)) {
            std::lock_guard<std::mutex> lock(m_queueMutex);
            m_messageQueue.push(line);
        }
    }
}

bool Network::ReadLine(std::string& line) {
    size_t pos = m_buffer.find('\n');
    if (pos == std::string::npos) return false;
    
    line = m_buffer.substr(0, pos);
    m_buffer.erase(0, pos + 1);
    return true;
} 