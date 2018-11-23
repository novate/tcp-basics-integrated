#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <ifaddrs.h>
#include <netdb.h>
#include <thread>
#include <chrono>
#include <fcntl.h>  // setting non-blocking socket option

using namespace std;

// get sockaddr. Supports IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " listen_port\n";
        exit(1);
    }

    int listen_port = stoi(argv[1]);

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0) {
        perror("socket");
        exit(3);
    }

    // lose the pesky "address already in use" error message
    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    // set nonblocking listener
    fcntl(listener, F_SETFL, O_NONBLOCK);
    
    // bind with select
    sockaddr_in listen_addr {};
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(listen_port);
    listen_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(listener, (sockaddr*) &listen_addr, sizeof(sockaddr)) == -1) {
        perror("bind");
        exit(4);
    }

    cout << "Listening on port " << listen_port << " on all interfaces...\n";

    // begin listening
    listen(listener,10);

    // accept with select
    // prepare variables used by select()
    fd_set master, readfds;      // master file descriptor list
    int fdmax = listener;          // maximum file descriptor number

    FD_ZERO(&master);
    FD_SET(listener, &master);
    fd_set select_fd = master; // copy
    if (select(fdmax+1, &select_fd, NULL, NULL, NULL) == -1) {
        perror("Select on read");
        exit(4);
    }

    socklen_t addrlen;
    struct sockaddr_storage remoteaddr; // client address
    int newfd = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);
    FD_SET(newfd, &master);
    if (newfd > fdmax) fdmax = newfd; // update fdmax
    cout << "A client is accepted on sfd " << newfd << endl;

    // set non-blocking connection
    fcntl(newfd, F_SETFL, O_NONBLOCK);

    // main loop
    // initialize buffer
    int buf_send_size = 10;
    char buf_send[buf_send_size+1];
    for (int i = 0; i < buf_send_size; i++)
        buf_send[i] = 'a' + i % 26;
    buf_send[buf_send_size] = '\0';

    int buf_read_size = 100;
    char buf_read[buf_read_size+1];

    // client info storage
    char remoteIP[INET6_ADDRSTRLEN]; 
    timeval tv = {1, 0}; // only send when the timer is up
    for(;;) {
        readfds = master; // copy at the last minutes
        int rv = select(fdmax+1, &readfds, NULL, NULL, &tv);
        int nbytes;
        switch (rv) {
            case -1:
                perror("select on write");
                exit(4);
                break;
            case 0:
                // time's up, send
                // run through the existing connections looking for data to send
                cout << "time's up\n";
                for (int i = 0; i <= fdmax; i++) {
                    if (FD_ISSET(i, &readfds)) { // we got one!!
                        // time's up, send
                        nbytes = send(i, buf_send, buf_send_size, 0);
                        if (nbytes <= 0) {
                            perror("send");
                            exit(5);
                        }
                        cout << nbytes << " bytes sent.\n";
                    }
                }
                // reset timer to 1 second
                tv = {1, 0};
                break;
            case 1:
                // run through the existing connections looking for data to read
                for (int i = 0; i <= fdmax; i++) {
                    if (FD_ISSET(i, &readfds)) { // we got one!!
                        if (i == listener) {
                            // handle new connections
                            addrlen = sizeof remoteaddr;
                            newfd = accept(listener, (struct sockaddr *) &remoteaddr, &addrlen);

                            // set non-blocking connection
                            fcntl(newfd, F_SETFL, O_NONBLOCK);

                            if (newfd == -1) {
                                perror("accept");
                            } else {
                                FD_SET(newfd, &master); // add to master set
                                if (newfd > fdmax)      // keep track of the max
                                    fdmax = newfd;

                                cout << "New connection from " << inet_ntop(remoteaddr.ss_family,
                                                                    get_in_addr((struct sockaddr*) &remoteaddr),                                                   
                                                                    remoteIP, INET6_ADDRSTRLEN)
                                     << " on socket " << newfd << endl;
                            }
                        } else { 
                            // handle data from a client
                            nbytes = recv(i, buf_read, buf_read_size, 0);
                            if (nbytes <= 0) {
                                perror("recv");
                                exit(6);
                            }
                            cout << nbytes << " bytes received.\n";
                        }
                    }
                }
            default: break;
        }
    }

    // finish up
    close(listener);
    close(newfd);
    return 0;
}