# Project 1

## Warm-Up: __Sockets, Echo Client-Server, and Transferring a File__

For this part, I used [Linux Sockets Tutorial] as a reference.

A socket is an abstract model of one end of a communication channel. The warm-up for project one starts out with a client-server pair; therefore, there are two ends to the channel, so two files will be made: server and client.

### Design

For both the transfer and echo client, it will:

1. Create a socket via system call - socket()
2. Connect the socket to the address of the server via system call - connect()
3. Send and receive data to the server socket - recv() and send()

And the server will:

1. Create a socket via system call - socket()
2. Bind the socket to an address via system call - bind()
3. Listen to connections from clients - listen()
4. Accept a connection - accept()
5. Send and receive data to the client socket - recv() and send()

Subtle differences between the two programs sending and receiving data order:

![Echo Diagram](illustrations/echo_diagram.png)
![Transfer Diagram](illustrations/transfer_diagram.png)

I have decided to use the Internet domain as the Socket Type, and that data type sockaddr\_in) is provided in **<netinet/in.h>**. I will use stream sockets (TCP) instead of datagram sockets (UDP) for reliability for the marginal cost of larger packets sent through the socket.

### Testing Procedure

My testing procedure for this part was minimalistic. I entered the respective project directories, and I started a tmux session and split the screen in half. The left-half of the screen was dedicated to the server, and right-half was dedicated to the client.

I had gedit as a text editor, so I made frequent debug messages with fprintf(), saved, and recompiled the server or client with Make on the appropriate tmux side of the screen. I ran the server first, and then the client, and I used no options to rely on the default values. Once I had a working program on both sides, I submitted each assignment.

To sum it all up, I performed integration testing, but not unit testing. 

### Flow of Control / How to Use

For both transfer and echo programs, the server process needs to start first. Navigate to the server executable and start a new process by executing the compiled artifact made by make/gcc (./\*server). Passing -h to either server program will give you the options you can pass, but luckily, they each have defaults, so this isn't entirely necessary. 

Once the server process has started, the server socket is created and binded to an address and then listens on the server address for any client's attempting to make a connection.

Next, start a new process by executing the compiled client artifact (./\*client). Again, passing -h will give you options, but since the server is using the default port, don't pass a different port to the client. Once the client process has started, the server process will accept the new client and store its address and length.

For the echo portion of this warm-up, the client sends a string (default is "Hello World!") in a buffer to the server, the server receives this message into a buffer and sends back the message in the buffer, and then the client receives this message and prints it out on the console. 

For the transfer portion, the client sends no data to the server. Instead, it waits to receive a stream of data from the server. This data stream are strings from a file, so the server puts lines of characters from the file into the buffer and sends it back to the client. Instead of outputting the received strings to the console, the client continuously writes the data to a file. **Note**: Make sure to create a dummy file for the server to read/transfer to the client. The default file it looks for is called "6200.txt".

## References
[Linux Sockets Tutorial]
[Linux Sockets Tutorial]: http://www.linuxhowtos.org/C_C++/socket.htm
[Stack Overflow Sending File Help]
[Stack Overflow Sending File Help]: https://stackoverflow.com/questions/30440188/sending-files-from-client-to-server-using-sockets-in-c
