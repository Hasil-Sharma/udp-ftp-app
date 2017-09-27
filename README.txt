# For final code version refer to code-merge branch

The program implements realiable file transfer protocol over UDP. Realiability is achieved by Stop-And-Wait realiability protocol which waits for one packet to be successfully processed before processing the next packet.

USAGE:
- Compile server.c and client.c in their respective folder using the makefile provided
- Start the server by `./server <port-num>` and start the client by `./client <server-ip-address> <port-num>` in their respective folder
- When client starts successfully it starts the prompt `>>` for the user to give commands

COMMANDS:
- GET
  * USAGE:
    get <FILE_NAME>, get followed by space and name of the file user wishes to fetch from the remote server

  * IMPLEMENTATION:
    After user enters get command with filename the client sends READ request to the server with filename. Server replies to this request packet with DATA packet, client saves the data packet to a file on server side with the filename same as sent to server, and replies ACK for Data packet received, and the process continues till the file is completely transferred. DATA packet received by client and ACK packet sent by client have same seq_id, and after receiving ACK packet for latest DATA packet sent server replies next DATA packet. If for any reason server doesn't receive an ACK for the DATA packet sent it'll resend the DATA packet again. In any case server receives a ACK packet for datagram it wasn't expecting and client receives a DATA packet it wasn't expecting, the packets are simply dropped.

    Client relies on offset in DATA Packets to know whether or not the file transfer has been completed. In case, file size is a whole multiple of packet payload size server sends out an empty DATA packet for client to know that file transfer has been completed.

- PUT
  * USAGE:
    put <FILE_NAME>, put followed by space and name of the file user want to send to the remote server

  * IMPLEMENTATION:
    After user enters put command with filename client sends WRITE request to the server with filename. Server replies with ACK for this request, client replies with the first DATA packet and the process continues similarly, client resending DATA packet in case it doesn't get ACK in a given time perid, as it does in case of get.

- LS
  * USAGE:
    ls : Prints the files in the server's directory

  * IMPLEMENTATION:
    Client send LS Request to the server and waits for reply from the server. It resends the request in case it doesn't get a reply within given time period.

- DELETE
  * USAGE:
    delete <FILE_NAME>, delete followed by space and name of the file user wants to delete from the server

  * IMPLEMENTATION:
    Client sends DELETE request to the server with file name in the payload and waits for ACK from the server. It resends the request till the time an ACK it received from the server.
- EXIT
  * USAGE:
    exit : server exits the infinite loop

  * IMPLEMENTATION:
    Client send EXIT request to the server and waits for an ACK. It resends the request till the time an ACK is received from the server.

Encryption: All the packets are XOR key encrypted before sending and are decrypted immediately receiving on both the client and server side.
