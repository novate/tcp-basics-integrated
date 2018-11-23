#include "shared_library.hpp"

// get sockaddr. Supports IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int server_bind_port(int listener, int listen_port) {
    // this function binds socket listener to listen_port on all interfaces
    // Precondition: listener is a valid ipv4 socket and listen_port is not used
    // Postconfition: socket listener is bound to listen_port on all interfaces
    sockaddr_in listen_addr {};
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_port = htons(listen_port);
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    return bind(listener, (sockaddr*) &listen_addr, sizeof(sockaddr));
}


int get_listener(Options opt) {
    // socket(), set blocking/nonblocking, bind(), listen()
    // Precondition: the port field and block field of opt is properly set
    // Postcondition: an ipv4 socket that is initialized, bound, and set to listening to all interfaces is returned

    int listen_port = stoi(opt.port);

    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener < 0)
        graceful("socket", 1);

    // lose the pesky "address already in use" error message
    int yes = 1;
    setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    // set nonblocking listener
    if (opt.block)
        fcntl(listener, F_SETFL, O_NONBLOCK);

    // bind
    if (server_bind_port(listener, listen_port) == -1)
        graceful("bind", 2);

    cout << "Listening on port " << listen_port << " on all interfaces...\n";

    // set listening
    listen(listener,50);

    return listener;
}


int loop_server_fork(int listener, Options opt) {
    // can be either blocking or non-blocking

}

int loop_server_nofork(int listener, Options opt) {
    // must be non-blocking otherwise simultaneous connections can't be handled

    // accept with select
    // prepare variables used by select()
    fd_set master, readfds;      // master file descriptor list
    int fdmax = listener;          // maximum file descriptor number

    // main loop
    // TODO: setup queueing machanism
    for(;;) {
        readfds = master; // copy at the last minutes
        // int rv = select(fdmax+1, &readfds, NULL, NULL, &tv);
        int rv = select(fdmax+1, &readfds, NULL, NULL, NULL);
        switch (rv) {
            case -1:
                graceful("select in main loop", 5);
                break;
            case 0;
                graceful("select returned 0\n", 6);
                break;
            default:
                for (int i = 0; i <= fdmax; i++) {
                    if (FD_ISSET(i, &readfds)) { // we got one
                        if (i == listener) {
                            // handle new connections;
                            server_accept_new_client(listener, opt.block, master, fdmax);
                        } else {
                            // handle data
                            server_communicate(i, opt.block);
                        }
                    }

                }
                break;
        }
    }

}

int server_communicate(int socketfd, bool block) {
    // exchange messages with client according to the protocol
    // Precondition: a connection is already established on socketfd
    // Postcondition: a sequence of messages are exchanged with the client,
    // abording connection if network error is encountered.

    // not implemented
    // remember to handle partial sends here

}

int server_accept_new_client(int listener, bool block, fd_set &master, int &fdmax) {
    // accepta a new connections from listener
    // Preconfition: if listener is non-blocking, the listener must have been select()ed
    // Postcondition: the new socket is added to master set and fdmax is
    // updated accordingly to reflex the maximum socket descriptor
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen = sizeof(remoteaddr);
    int newfd = accept(listener, (sockaddr *) &remoteaddr, &addrlen);


    if (newfd == -1) {
        graceful("server_accept_new_client", 7);
    } else {
        // set non-blocking connection
        if (block)
            fcntl(newfd, F_SETFL, O_NONBLOCK);
        FD_SET(newfd, &master); // add to master set
        if (newfd > fdmax)      // keep track of the max
            fdmax = newfd;

        char remoteIP[INET6_ADDRSTRLEN];
        cout << "New connection from " << inet_ntop(remoteaddr.ss_family,
                                            get_in_addr((struct sockaddr*) &remoteaddr),
                                            remoteIP, INET6_ADDRSTRLEN)
                << " on socket " << newfd << endl;
    }

    return newfd;

}


int loop_client_fork(Options opt) {
    // can be either blocking or non-blocking

}

int loop_client_nofork(Options opt) {
    // must be non-blocking otherwise simultaneous connections can't be handled

}
