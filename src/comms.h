#include <iostream>
#include <array>
#include <vector>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#ifndef __COMMS_H__
#define __COMMS_H__

class Communication {
public:
    Communication(const std::string& to, int port) {
        to_sock_addr = {
                .sin_family = AF_INET,
                .sin_port = htons(port),
                .sin_addr = {inet_addr(to.c_str())}
        };
        to_sock_addr_len = sizeof(to_sock_addr);

        sockaddr_in address = {
                .sin_family = AF_INET,
                .sin_port = htons(port),
        };

        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (socket_fd < 0) {
            std::cerr << "Error: Unable to create socket" << std::endl;
            std::exit(1);
        }

        int bind_status = bind(socket_fd, (struct sockaddr*) &address, sizeof(address));
        if (bind_status < 0) {
            std::cerr << "Error: Unable to bind socket" << std::endl;
            std::exit(1);
        }
    }

    ~Communication() {
        close(this->socket_fd);
    }

    void send(std::vector<uint8_t> send_buf) {
        sendto(socket_fd, send_buf.data(), send_buf.size() * sizeof(uint8_t), 0, (struct sockaddr *) &to_sock_addr, to_sock_addr_len);
    }

    bool recv_nonblocking(std::vector<uint8_t> &data) {
        ssize_t n = recvfrom(socket_fd, buffer.data(), sizeof(buffer), MSG_DONTWAIT, (struct sockaddr*) &to_sock_addr, &to_sock_addr_len);
        if (n < 1) return false;

        data = std::vector<uint8_t>(buffer.begin(), buffer.begin() + n);
        return true;
    }

    bool recv(std::vector<uint8_t> &data) {
        size_t n = recvfrom(socket_fd, buffer.data(), sizeof(buffer), 0, (struct sockaddr*) &to_sock_addr, &to_sock_addr_len);
        if (n < 1) return false;

        data = std::vector<uint8_t>(buffer.begin(), buffer.begin() + n);
        return true;
    }

private:
    int socket_fd;
    sockaddr_in to_sock_addr{};
    socklen_t to_sock_addr_len;
    std::array<uint8_t, 256> buffer{};
};

#endif // __COMMS_H__