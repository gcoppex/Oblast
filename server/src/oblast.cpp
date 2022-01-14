#include <iostream>
#include <list>
#include <map>
#include <set>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_BUFFER_SIZE 1024
#define MULTICAST_ADDRESS "226.1.1.38"
#define MULTICAST_PORT 1291

#define UDP_CLIENT_PORT 1292
#define TCP_PORT 1291
#define TCP_KEEP_ALIVE_TIMEOUT 10
// After this amount of seconds + select waiting time, then a dummy tcp message
// is sent to make sure the connection is still alive...


void exitWithError(std::string const & errorMessage) {
    std::cout << errorMessage << std::endl;
    std::cout << "Terminating with errno " << errno << "." << std::endl;
    exit(errno);
}

// Todo: change the store to a map of string=>queue
std::map<std::string, std::string> keyValueStore;
std::map<std::string, std::list<int>> regularListeners;;
std::set<int> globalListeners;


void unregisterFd(int const & fd) {
    // Removing the fd from the list of regularListeners:
    for (auto& l : regularListeners) {
        for (std::list<int>::iterator it = l.second.begin(); it != l.second.end();) {
            if(*it == fd) {
                it = l.second.erase(it);
            } else {
                ++it;
            }
        }
    }
}

bool processClientQuery(int fd, char* query) {
    // if update, then notify, if register then add and return value
    char* action = strtok(query, ":");
    char* key = strtok(NULL, "=");
    char* value = strtok(NULL, "");

    if(strncmp(action, "subscribe", 1024)==0) {
        regularListeners[key].push_back(fd);
        return 1;
    } else if(strncmp(action, "subscribe_all", 1024)==0) {
        globalListeners.insert(fd);
        return 1;
    } else if(strncmp(action, "update", 1024)==0 && value!=NULL) {
        keyValueStore[key] = value;
        std::set<int> staleFds;

        std::set<int> selectedListeners;
        // 1. selecting the single-var subscribed listeners:
        for(std::list<int>::iterator it = regularListeners[key].begin();
            it!=regularListeners[key].end();
            ++it
        ) {
            if(*it == fd) {
                continue;
                // No need to update the client who updated the value
            }
            selectedListeners.insert(*it);
        }
        // 2. addinng the global listeners:
        for(int fd: globalListeners) {
            selectedListeners.insert(fd);
        }

        for(int fd: selectedListeners) {
            if(staleFds.count(fd)) {
                // If the fd already had an error, we ignore it here, it will
                // be cleaned up right after this loop (unregisterFd(fd)).
                continue;
            }
            char payload[1024];
            sprintf(payload, "update:%s=%s", key, value);
            uint16_t payloadLength = htons(strlen(payload));
            int n0 = write(fd, &payloadLength, sizeof(uint16_t));
            if(n0 >= 0) {
                int n1 = write(fd, payload, strlen(payload));
                if(n1 < 0) {
                    staleFds.insert(fd);
                }
            } else {
                staleFds.insert(fd);
            }

        }
        for(auto& fd: staleFds) {
            unregisterFd(fd);
        }

        return 1;
    } else {
        std::cout << "Not yet impl query: [" << query << "]. Ingoring..."
            << std::endl;
        return 0;
    }
}

void startServer() {
    // Setting the two sever sockets (UDP and TCP):
    // 1. setting the UDP listening socket:
    int udpSockFd = socket(AF_INET, SOCK_DGRAM, 0);
    if(udpSockFd==-1) {
        exitWithError("Unable to create the udp socket.");
    }
    struct sockaddr_in udpServerAddr;
    memset(&udpServerAddr, 0, sizeof(udpServerAddr));
    udpServerAddr.sin_family = AF_INET;
    udpServerAddr.sin_addr.s_addr = inet_addr(MULTICAST_ADDRESS);
    udpServerAddr.sin_port = htons(MULTICAST_PORT);

    if(bind(udpSockFd,
        (const struct sockaddr *)&udpServerAddr,
        sizeof(udpServerAddr)
    )==-1) {
        exitWithError("Unable to bind the UDP socket.");
    }

    // 1.b. requesting the kernel to join the multicast group:
    struct ip_mreq mreq;
    mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDRESS);
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    if(setsockopt(udpSockFd,
        IPPROTO_IP,
        IP_ADD_MEMBERSHIP,
        (char*)&mreq, sizeof(mreq)
    )<0) {
        exitWithError("Unable to register to the UDP multicast group.");
    }

    int val = 1;
    if(setsockopt(udpSockFd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int))<0) {
        exitWithError("Unable to set the udp socket as reuseable.");
    }

    // 2. Setting the TCP listening socket:
    int tcpSockFd = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSockFd == -1) {
        exitWithError("Unable to create the TCP socket.");
    }
    struct sockaddr_in tcpSockAddr;
    bzero(&tcpSockAddr, sizeof(tcpSockAddr));
    tcpSockAddr.sin_family = AF_INET;
    tcpSockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcpSockAddr.sin_port = htons(TCP_PORT);

    if(bind(tcpSockFd,
        (const struct sockaddr *)&tcpSockAddr,
        sizeof(tcpSockAddr)
    )==-1) {
        exitWithError("Unable to bind the TCP socket.");
    }

    if(listen(tcpSockFd, 5) == -1) {
        exitWithError("Unable to listen on the TCP socket.");
    }
    std::cout << "Server ready." << std::endl;

    fd_set fdSet;
    FD_ZERO(&fdSet);
    FD_SET(udpSockFd, &fdSet);
    FD_SET(tcpSockFd, &fdSet);

    int maxFd = std::max(udpSockFd, tcpSockFd);
    std::list<int> clientFds;
    std::map<int,double> clientFdsIdleTime;
    int selectTimeoutSeconds = 5;
    for(;;) {
        struct timeval tv;
        tv.tv_sec = selectTimeoutSeconds;
        tv.tv_usec = 0;
        int selectReturnValue = -1;
        if((selectReturnValue=select(maxFd + 1, &fdSet, NULL, NULL, &tv))<0) {
            exitWithError("Unable to start the select() syscall.");
        }
        double elapsedTime = (double)(selectTimeoutSeconds - tv.tv_sec)
            - (double)tv.tv_usec / 1000000.0;

        if(FD_ISSET(udpSockFd, &fdSet)) {
            char buffer[MAX_BUFFER_SIZE];
            struct sockaddr_in udpClientSockAddr;
            int udpClientSockAddrLength = sizeof(udpClientSockAddr);

            std::string expectedHelloMessage("oblast_discover");

            int n = recvfrom(udpSockFd, (char *)buffer, expectedHelloMessage.length(),
                MSG_WAITALL, (struct sockaddr *) &udpClientSockAddr,
                (socklen_t*) &udpClientSockAddrLength);
            buffer[n] = '\0';

            udpClientSockAddr.sin_port = htons(UDP_CLIENT_PORT);

            if(expectedHelloMessage.compare(buffer)==0) {
                usleep(10000); // 10msec sleep

                std::string response("oblast_server");
                udpClientSockAddrLength = sizeof(udpClientSockAddr);
                int responseLength = strlen(response.c_str())+1;
                int s = sendto(udpSockFd, response.c_str(), responseLength,
                    0, (struct sockaddr *) &udpClientSockAddr,
                    (socklen_t) udpClientSockAddrLength);
                std::cout << "Received (and responded " << s
                    << " bytes) to a client's discovery request "
                    << inet_ntoa(udpClientSockAddr.sin_addr) << ":"
                    << ntohs(udpClientSockAddr.sin_port) << std::endl;
                // No specific sending check here, if the client did not
                // receive our UDP response, he will keep polling using
                // multicast UDP.
            } else {
                std::cout << "Received an unexpected UDP message: " << buffer
                    << std::endl;
            }
        }
        if(FD_ISSET(tcpSockFd, &fdSet)) {
            struct sockaddr_in clientSockAddr;
            int clientSockAddrLength = sizeof(clientSockAddr);
            int tcpClientFd = accept(tcpSockFd,
                (struct sockaddr *)&clientSockAddr,
                (unsigned int*) &clientSockAddrLength
            );
            if(tcpClientFd) {
                std::cout << "Received a new TCP connection from "
                    << inet_ntoa(clientSockAddr.sin_addr) << ":"
                    << ntohs(clientSockAddr.sin_port) << std::endl;
                // Adding the new fd to the set of fd to be select'ed on:
                clientFds.push_back(tcpClientFd);
                clientFdsIdleTime[tcpClientFd] = 0;
            } else {
                std::cerr << "Warning : " << errno << " could not accept TCP "
                    << "client. Continuing..." << std::endl;
            }
        }
        // Registering again the file descriptors in the set of fd select'ed on:
        FD_SET(udpSockFd, &fdSet);
        FD_SET(tcpSockFd, &fdSet);
        maxFd = std::max(udpSockFd, tcpSockFd);

        // Then the TCP client file descriptors need to be handled:
        // 1. we check if they are ready to send data, 2. we add them back to
        // select'ed set of fd:
        for (std::list<int>::iterator it=clientFds.begin();
            it != clientFds.end();
        ) {
            int clientDisconnected = 0;
            if(FD_ISSET(*it, &fdSet)) {
                // Todo: read more than 1024 bytes in case...
                uint16_t messageLength = 0;
                int res = read(*it, &messageLength, sizeof(uint16_t));
                if(res <=0) {
                    clientDisconnected = 1;
                } else {
                    messageLength = ntohs(messageLength);

                    char buff[messageLength+1];
                    memset(buff, 0, sizeof(buff));
                    res = read(*it, &buff, messageLength);
                    if(res <=0) {
                        clientDisconnected = 1;
                    } else {
                        buff[res] = '\0';
                        if(processClientQuery(*it, buff)) {
                            clientFdsIdleTime[*it] = 0;
                        } else {

                        }
                    }
                }
            } else {
                clientFdsIdleTime[*it] += elapsedTime;
            }

            if(clientFdsIdleTime[*it] > TCP_KEEP_ALIVE_TIMEOUT) {
                char const* buff = "KEEP_ALIVE";
                int bufferSize = strlen(buff);

                uint16_t payloadLength = htons(bufferSize);
                int n0=-1, n1=-1;
                n0 = write(*it, &payloadLength, sizeof(uint16_t));
                if(n0>0) {
                    n1 = write(*it, buff, bufferSize);
                    if(n1>0) {
                        // The client is still on the line but was just idle,
                        // then we keep it in our fds.
                        clientFdsIdleTime[*it] = 0;
                    }
                }
                if(n0<0 || n1<0) {
                    // The client ill terminated. We remove it from our list of
                    // file descriptors.
                    clientDisconnected = 1;
                }
            }

            // If the client is still on the line, we put it back to the
            // list of fd that the select() need to react on.
            if(clientDisconnected) {
                std::cout << "The file descriptor " << *it
                    << " will be dropped." << std::endl;

                unregisterFd(*it);

                FD_CLR(*it, &fdSet);
                close(*it);
                clientFdsIdleTime.erase(*it);
                it = clientFds.erase(it);

            } else {
                FD_SET(*it, &fdSet);
                // updating the maxFd for the select() argument to be consistent:
                if(*it>maxFd) {
                    maxFd = *it;
                }
                ++it;
            }
        }
    }
}


int main() {
    startServer();
}
