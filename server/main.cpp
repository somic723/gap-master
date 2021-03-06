/*
* This sample is based on source code from this tutorial:
*   http://www.cs.rpi.edu/~moorthy/Courses/os98/Pgms/socket.html
*/

#include <cstring>
#include <iostream>
#include <array>
#include <string>
#include <system_error>
#include <ctime>
#include <utility>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "argh.h"

void error() {
    throw std::system_error{ errno, std::system_category().default_error_condition(errno).category() };
}

struct client_socket {
    explicit client_socket(int newfd) : fd_ { newfd } {
        if (fd_ < 0)
            error();
    }

    template<typename Container>
    auto read(Container& buffer) {
        auto n = ::read(fd_, buffer.data(), buffer.size() - 1);
        if (n < 0)
            error();
        return n;
    }

    template<typename Container>
    auto write(const Container& buffer) {
        auto n = ::write(fd_, buffer.data(), buffer.size());
        if (n < 0)
            error();
        return n;
    }

private:
    int fd_;
};

struct server {
    server(int portno) {
        fd_ = socket(AF_INET, SOCK_STREAM, 0);
        if (fd_ <= 0)
            error();
        bind_to_any(portno);
    }

    void start() {
        ::listen(fd_, 5) ;
    }

    client_socket next_client() {
        struct sockaddr_in cli_addr;
        auto clilen = sizeof(cli_addr);
        return client_socket{ accept(fd_, (struct sockaddr *) &cli_addr, (socklen_t *) &clilen) };
    }

private:
    int fd_;

    void bind_to_any(int portno) const {
        struct sockaddr_in serv_addr;
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);
        if (bind(fd_, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
            error();
    }
};

auto get_command_reply(std::string command) {
    if (command == "shutdown") {
        return std::make_pair(false, std::string{ "Have a nice day..." });
    }
    if (command == "time") {
        auto now = std::time(nullptr);
        return std::make_pair(true, std::string{ std::ctime(&now) });
    }
    return std::make_pair(true, std::string{ "Unknow command, but no problem." });
}

bool gap_with_client(client_socket clnt) {
    auto buffer = std::array<char, 256>{};
    clnt.read(buffer);

    auto command = std::string{ buffer.data() };
    std::cout << "Client says: " << command << std::endl;

    auto reply = get_command_reply(command);
    clnt.write(reply.second);

    return reply.first;
}

int main(int argc, char *argv[]) {
    auto cmd_line = argh::parser{ argv };
    int portno{};
    cmd_line({ "-p", "--port" }, 9900) >> portno;

    try {
        auto srv = server{ portno };
        srv.start();

        std::cout << "gap server started listening on port: " << portno << std::endl;

        while (gap_with_client(srv.next_client())) {
        }
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
