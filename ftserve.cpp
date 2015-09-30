/*
 * File: ftserve.cpp
 * Author: TYLER MCGINNIS
 * Email: mcginnit@onid.oregonstate.edu
 * Description:
 *     The server implementation of a simple FTP application
 * Usage:
 *     To build, just type 'make' at the command line and press enter
 *
 *     To start the server, type the following command into the command
 *     line, followed by enter:
 *
 *     ./ftserve <SERVER_PORT>
 *
 *     Where <SERVER_PORT> is the port which you want the server to
 *     listen on.
 *
 *     This server will respond to two commands from a client:
 *     -l               send a list of the .txt files contained in the
 *                      directory the server program is running in
 *     -g <filename>    send the file specified by <filename>
 *
 *     To stop server, enter a SIGINT, such as CTRL+C
 *
 * Full List of Citations:
 *     http://stackoverflow.com/questions/1488775/c-remove-new-line-from-multiline-string
 *     http://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
 *     http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
 *     http://stackoverflow.com/questions/20446201/how-to-check-if-string-ends-with-txt
 *     http://stackoverflow.com/questions/5297248/how-to-compare-last-n-characters-of-a-string-to-another-string-in-c
 *     http://stackoverflow.com/questions/12975341/to-string-is-not-a-member-of-std-says-so-g
 *     http://beej.us/guide/bgnet/output/html/multipage/index.html
 */



#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <algorithm>
#include <stdio.h>
#include <cstdlib>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>


/*
 * Patch for std::to_string
 * Citation:
 *     http://stackoverflow.com/questions/12975341/to-string-is-not-a-member-of-std-says-so-g
 */
namespace patch
{
    template < typename T > std::string to_string( const T& n )
    {
        std::ostringstream stm ;
        stm << n ;
        return stm.str() ;
    }
}


void startup(std::string port, int &sock);
void handleRequest(std::vector<std::string> request, int &sock,
                    std::string client_ip, struct sockaddr_storage theiraddr,
                    socklen_t addr_size);

bool validatePort(char *port);
bool checkSuffix(const char *str, const char *suffix);
std::vector<std::string> getDirectory();
std::string getFile(std::string filename);
void removeTrailingNewline(std::string &str);
void unpackRequest(std::vector<std::string> request, std::string &cmd,
                    std::string &fname, std::string &port);
void sendData(int sockfd, std::string msg);
int establishDataConnection(std::string dest_ip, std::string port);


int main(int argc, char **argv)
{

    std::string port, request, command, filename, client_data_port, temp;
    std::string client_str;
    std::vector<std::string> request_container;
    int sock_fd, new_fd, bytes_received, client;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;


    // check that correct number of command line arguments given
    if (argc != 2) {
        std::cout << "Correct usage: ./ftserve <SERVER_PORT>\n";
        exit(1);
    }

    // check that <SERVER_PORT> is an integer
    if (validatePort(argv[1])) {
        port = argv[1];
    } else {
        std::cout << "<SERVER_PORT> must be an integer.\n";
        exit(1);
    }

    startup(port, sock_fd);

    while (1) {
        char recv_buf[100];

        addr_size = sizeof their_addr;
        new_fd = accept(sock_fd, (struct sockaddr *)&their_addr, &addr_size);

        // get client IP address
        client = getpeername(new_fd, (struct sockaddr *)&their_addr,
                                    &addr_size);
        client_str = patch::to_string(client);

        // get client command
        bytes_received = recv(new_fd, recv_buf, 100, 0);
        request = recv_buf;

        // parse request by whitespace
        // load into request_container
        std::stringstream ss (request);
        while(ss>> temp) {
            request_container.push_back(temp);
        }

        handleRequest(request_container, new_fd, client_str,
                        their_addr, addr_size);

        request_container.clear();

        shutdown(new_fd, 2);
    }


    return 0;
}


/*
 * Function: handleRequest
 * Arguments: std::vector<std::string> request, int &sock,
 *                std::string client_ip,
 *                struct sockaddr_storage theiraddr, socklen_t addr_size
 * Handles incoming requests to FTP server by either sending directory
 * listing, sending requested file, or returning an error message to the
 * control connection.
 */
void handleRequest(std::vector<std::string> request, int &sock,
                    std::string client_ip, struct sockaddr_storage theiraddr,
                    socklen_t addr_size)
{
    int bytes_sent, len, sockfd;
    bool file_found = false;
    // .txt files in current directory
    std::vector<std::string> directory = getDirectory();
    std::string data_port, fname, command;

    unpackRequest(request, command, fname, data_port);

    sockfd = establishDataConnection(client_ip, data_port);

    if (command.compare("-l") == 0) {
        std::string dir_listing;

        // build a string with listing of .txt files from the directory
        // the server is currently running in
        for (std::vector<std::string>::const_iterator i = directory.begin();
                i != directory.end(); ++i) {
            dir_listing += *i + "\n";
        }

        sendData(sockfd, dir_listing);
        shutdown(sockfd, 2);
        return;

    } else if (command.compare("-g") == 0) {

        if (std::find(directory.begin(), directory.end(), fname)
                != directory.end()) {
            sendData(sockfd, getFile(fname));
            shutdown(sockfd, 2);
            return;
        }

        file_found = false;
        sendData(sockfd, command + " " + fname + " " + data_port);
    }

    std::string msg = "Command not recognized.\n";
    if (file_found == false) {
        msg = "FILE NOT FOUND\n";
    }
    len = msg.length();
    bytes_sent = send(sock, msg.c_str(), len, 0);

    shutdown(sockfd, 2);
}


/*
 * Function: sendData
 * Arguments: int sockfd, std::string msg
 * Sends message to socket sockfd
 * Citations:
 *     http://beej.us/guide/bgnet/output/html/multipage/index.html
 */
void sendData(int sockfd, std::string msg) {
    int len = msg.length();
    int bytes_sent = send(sockfd, msg.c_str(), len, 0);
}


/*
 * Function: establishDataConnection
 * Arguments: std::string dest_ip, std::string port
 * Establishes a data connection with socket dest_ip:port
 * Citations:
 *     http://beej.us/guide/bgnet/output/html/multipage/index.html
 */
int establishDataConnection(std::string dest_ip, std::string port)
{
    struct addrinfo hints, *res, *i;
    int sockfd, status;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    const char *receiver = dest_ip.c_str();
    const char *p = port.c_str();

    if ((status = getaddrinfo(receiver, p, &hints, &res)) != 0) {
        std::cerr << "getaddrinfo error: " << *gai_strerror(status)
                  << std::endl;
        exit(1);
    }

    // set up socket file descriptor
    for (i = res; i != NULL; i = i->ai_next) {
        if ((sockfd = socket(i->ai_family, i->ai_socktype,
                i->ai_protocol)) == -1) {
            std::cerr << "server: socket error\n";
            continue;
        }

        break;
    }
    connect(sockfd, res->ai_addr, res->ai_addrlen);

    return sockfd;
}


/*
 * unpackRequest
 * Arguments: std::vector<std::string> request, std::string &cmd,
 *             std::string &fname, std::string &port
 * Unpack request variables from request vector and strip trailing
 * newlines
 */
void unpackRequest(std::vector<std::string> request, std::string &cmd,
                    std::string &fname, std::string &port)
{
    // int len;
    std::string msg;

    if (request.size() > 3 || request.size() < 2) {
        // msg = "Command not recognized.\n";
        // len = msg.length();
        // bytes_sent = send(sock, msg.c_str(), len, 0);

        // return;
    }

    cmd = request[0];
    fname = request[1];
    port = request[2];

    removeTrailingNewline(cmd);
    removeTrailingNewline(fname);
    removeTrailingNewline(port);
}


/*
 * Function: removeTrailingNewline
 * Arguments: std::string &str
 * Removes trailing Newline from str
 * Citations
 *     http://stackoverflow.com/questions/1488775/c-remove-new-line-from-multiline-string
 */
void removeTrailingNewline(std::string &str)
{
    str.erase(std::remove(str.begin(), str.end(), '\n'), str.end());
}


/*
 * Function: startup
 * Arguments: std::string port, int &sock
 * Creates socket and starts server running locally on port
 * Citations:
 *     http://beej.us/guide/bgnet/output/html/multipage/index.html
 */
void startup(std::string port, int &sock)
{
    int status, backlog = 10, yes = 1;
    struct addrinfo *servinfo, *p, hints;

    // set up structures
    memset(&hints, 0, sizeof hints);  // confirm hints is empty
    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;  // TCP stream socket
    hints.ai_flags = AI_PASSIVE;      // use localhost as IP

    if ((status = getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0) {
        std::cerr << "getaddrinfo error: " << *gai_strerror(status)
                  << std::endl;
        exit(1);
    }

    // set up socket file descriptor
    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((sock = socket(p->ai_family, p->ai_socktype,
                p->ai_protocol)) == -1) {
            std::cerr << "server: socket error\n";
            continue;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes,
                sizeof(int)) == -1) {
            std::cerr << "setsockopt error\n";
            exit(1);
        }

        if (bind(sock, p->ai_addr, p->ai_addrlen) == -1) {
            shutdown(sock, 2);
            std::cerr << "server: bind error\n";
            continue;
        }

        break;
    }

    // bind to port on localhost
    bind(sock, servinfo->ai_addr, servinfo->ai_addrlen);

    freeaddrinfo(servinfo);  // free servinfo linked-list

    std::cout << "Server open on port " << port << std::endl;
    listen(sock, backlog);
}


/*
 * Function: validatePort
 * Arguments: char *port
 * Returns false if port does not contain only valid integers, otherwise
 * returns true
 */
bool validatePort(char *port)
{
    while (*port) {
        if (!isdigit(*port)) {
            return false;
        } else {
            ++port;
        }

    }

    return true;
}


/*
 * Function: checkSuffix
 * Arguments: const char *filename, const char *suffix
 * Returns true if filename suffix equals suffix
 * Example: checkSuffix("hello.txt", ".txt") would return true
 * Citations:
 *     http://stackoverflow.com/questions/20446201/how-to-check-if-string-ends-with-txt
 *     http://stackoverflow.com/questions/5297248/how-to-compare-last-n-characters-of-a-string-to-another-string-in-c
 */
bool checkSuffix(const char *filename, const char *suffix)
{
    int len = strlen(filename);
    const char *file_suffix = &filename[len-4];
    return strlen(filename) >= strlen(suffix) &&
        strncmp(file_suffix, suffix, 4) == 0;
}


/*
 * Function: getDirectory
 * Arguments: None
 * Returns a vector of strings with the names of all .txt files in the
 * directory the server is currently running in
 * Citations:
 *     http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
 */
std::vector<std::string> getDirectory()
{
    DIR *d;
    struct dirent *dir;
    std::vector<std::string> dir_listing;

    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            if (checkSuffix(dir->d_name, ".txt")) {
                std::string f(dir->d_name);
                dir_listing.push_back(f);
            }
        }

        closedir(d);
    }

    return dir_listing;
}


/*
 * Function: getFile
 * Arguments: std::string filename
 * Returns a string containing the contents of the file named filename
 * Citations:
 *     http://stackoverflow.com/questions/2602013/read-whole-ascii-file-into-c-stdstring
 */
std::string getFile(std::string filename)
{
    std::ifstream in(filename.c_str());
    std::string file((std::istreambuf_iterator<char>(in)),
                      std::istreambuf_iterator<char>());

    return file;
}
