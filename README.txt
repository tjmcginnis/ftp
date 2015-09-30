File: README.txt
Author: TYLER MCGINNIS
Email: mcginnit@onid.oregonstate.edu
CS 372, Spring 2015
Project 2


FILES:
    Makefile
    ftserve.cpp
    ftclient.py
    README.txt


BUILD INSTRUCTIONS:

    For the ftp server:
        On the command line, just type 'make' and hit enter.

        To get a fresh build, just type 'make clean'
        **Most likely don't need to worry about this

    For ftclient.py:
        No build/compilation necessary


USAGE:

    ftserve:
        To start the server, type the following into the command line,
        followed by enter:

        ./ftserve <SERVER_PORT>

        Where <SERVER_PORT> is the port which you want the server to
        listen on.

        This server will respond to two commands from a client:
        -l               send a list of the .txt files contained in the
                         directory the server program is running in
        -g <filename>    send the file specified by <filename>

        To stop server, enter a SIGINT, such as CTRL+C


    ftclient:
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
