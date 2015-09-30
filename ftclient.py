"""File: ftclient.py
Author: TYLER MCGINNIS
Email: mcginnit@onid.oregonstate.edu
Description:
    The client side of a simple FTP application
Usage:
    To run, type the following command at the terminal:
        python ftclient.py <HOST> <PORT> <COMMAND> <DATA_PORT>

    <HOST> is the IP address of the FTP server
    <PORT> is the port number the FTP server is listening on

    The following are valid for <COMMAND>:
    '-l'            get a list of the .txt files available from the
                    FTP server
    '-g <filename>' get the file specified by <filename> from the FTP
                    server

    <DATA_PORT> the local port for the client to listen on for data from
                the FTP server

Citations:
    This implementation of a Python chat client relied heavily on the
    Python socket library documentation, found at:
    https://docs.python.org/2.7/library/socket.html#
"""


import os
import sys
import socket
import random
import string


def initiateContact(client_socket, host, port):
    """connects a socket instance to a server/host at a specific port"""
    client_socket.connect((host, port))


def makeRequest(client_socket, user_input):
    """sends command to ftp server"""
    client_socket.send("%s\n" % user_input)


def receiveMessage(client_socket):
    """receives incoming message from a server host for a client_socket,
    an instance of a socket object
    """
    return client_socket.recv(4096)


def receiveData(server_socket, command, msg, host, port, filename):
    conn, addr = server_socket.accept()
    data = receiveMessage(conn)
    conn.close()
    if data:
        if data == msg:
            pass  # in this case, file was not found
        else:
            if command == '-l':
                print "Receiving directory structure from"
                print "%s:%s\n" % (host, port)
                print data
            elif command == '-g':
                print "Receiving \"%s\" from %s:%s" % (filename, host, port)
                receiveFile(data, filename)
                print "File Transfer complete."


def receiveFile(data, filename):
    # if file exists, add random string of digits to end of filename
    while os.path.isfile(filename):
        f = filename.split('.')
        filename = f[0] + random.choice(string.digits) + '.' + f[1]
    with open(filename, 'w') as f:
        f.write(data)


def main(argv):

    # check that correct number of command line arguments provided
    if len(argv) < 4 or len(argv) > 5:
        print "Usage: python ftclient.py <SERVER_HOST> <SERVER_PORT> <COMMAND> <DATA_PORT>"
        exit(1)

    HOST, PORT, COMMAND, DATA_PORT = argv[:4]

    FILENAME = "x"  # placeholder
    if COMMAND == "-g":  # if getting a file, assign args accordingly
        FILENAME = argv[3]
        DATA_PORT = argv[4]

    # confirm PORT is an integer
    try:
        PORT = int(PORT)
    except ValueError:
        print "<SERVER_PORT> must be an integer"
        exit(1)

    # confirm DATA_PORT is an integer
    try:
        DATA_PORT = int(DATA_PORT)
    except ValueError:
        print "<DATA_PORT> must be an integer"
        exit(1)

    try:
        sock_p = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock_d = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock_d.bind(('localhost', DATA_PORT))
        sock_d.listen(1)

        # Connect to server and send data
        initiateContact(sock_p, HOST, PORT)

        # formulate message and make request to server
        msg = "%s %s %s" % (COMMAND, FILENAME, DATA_PORT)
        makeRequest(sock_p, msg)

        # receive data from data connection
        receiveData(sock_d, COMMAND, msg, HOST, PORT, FILENAME)
        received = receiveMessage(sock_p)

        # print any messages from control connection
        if received:
            print "%s:%s says" % (HOST, PORT)
            print received

        sock_p.close()
        sock_d.close()
    except socket.error, exception:
        print "Caught exception socket.error: %s" % exception


if __name__ == '__main__':
    main(sys.argv[1:])
