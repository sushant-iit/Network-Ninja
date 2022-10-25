# Multithreaded Chat Server with Encryption
## Allows multiple clients to tallk to each other

Using this application, multiple clients can talk to each other by connecting to the server and watch each others' statuses  - BUSY or FREE, and based on availability connect to the particular client.


**Update:** \
Now, the clients can also encrypt their messages using RC4 encryption to send messages securely. On the connection set up, a key is shared between client pairs using **Diffie-Hellman-Key-Exchange** (key range can be increased in the code by altering public key n and g, for testing purposes they are kept small). \
The common key established is then used for **RC4 Encryption**.

<div align="center"> 
<img src="https://user-images.githubusercontent.com/59964272/197150754-28475938-8dde-4055-9ec7-0c4637e352c7.gif" alt="Gif showing usage of this program">
</div>

## Usage
1. Download server_q2.cpp and client_q2.cpp (client program should be compiled in all the PCs that want to talk.)
2. Open terminal in that directory.
3. Compile the server program using the command: \
    `g++ server_q2.cpp -o server -lpthread`
4. Compile the client program using the command: \
    `g++ client_q2.cpp -o client -lpthread` \
The client programs should be run at those computers which want to talk. The server and all the clients should be in the same network to chat.
5. Run the server using command: \
    `./server port_number` \
    For example: \
     `./server 8083` \
     You wil get message saying "Server started successfully".
6. Open a new terminal on server computer and find the ip address of server computer using the command: \
     `ifconfig` \
     	<div align = "center">
         <img src = "https://user-images.githubusercontent.com/59964272/158458097-ad34d2e1-3c56-4ee3-aa23-96d734064fa5.JPG" height=318 width=491>
	</div>
	Note the inet address (10.10.77.210) in this case.
7. Similary,  run the client on all the PCs that want to chat using command: (they should be in the same subnet as server) \
    `./client server_ip_address server_port` \
    For example: \
    `./client 10.10.77.210 8083`
8. Choose any user_name for one client (all clients should have different names).
9. Enter the following command to see which users are free: \
    `status`
10. Repeat the same steps from 7 on other PCs to create more clients so that they can chat. 
11. Then type the following command to connect to that user: \
    `connect <other_client_user_name>` \
If you get the output: \
    `(server): You are now connected to <other_client_user_name>` \
then you are now connected and can begin the chat.
12. All the strings except command words will be sent to other user and can be received from other user. \
**Note:** To send messages securely (in encrypted format), preprend message with: \
`secure: <message>` *(secure followed by colon followed by space)* 
13. If you wish to close the active chat session with a user type: \
    `goodbye`
14. Go to step 8 if you want to talk to other users.
15. To disconnect the client from the chat server, type: \
    `close`

## Available commands and behaviours:
	The supported commands are:
	1. status               : List all the users status
	2. connect <username>   : Connect to username and start chatting
	3. goodbye              : Ends current chatting session
	4. close                : Disconnects you from the server
	5. <message>            : Sends the message to other client in plain text format.
	6. secure: <message>    : Sends the message in secure format to other client.
	7. Ctrl+C (client side) : Ends current chatting session (if present) and disconnects and terminates the client.
	8. Ctrl+C (server side) : Ends all the chatting sessions and disconnects all the clients and terminates the server.
	
**Note:** In the active chat session, any text except the above are treated as normal message. If you want to send encrypted messages, prepend message with *secure: \<message\>*


## Diffie-Hellman-Key-Exchange Process
To perform the handshake the following protocol is used:
1. Here, the public keys n (=1999) and g (=1777) is fixed and known to both the clients.
2. Client1 sends connection request for client2 to the server.
3. Server on successful connection sends GEN_KEYS to both the clients.
4. Both the clients generate private key x (say client1), y (say client2) using random generator and send A = g<sup>x</sup>mod n, B = g <sup>y</sup>mod n to the server. The server then forwards A and B to the partner clients using key_param message. \
*Client1 gets B and client2 gets A after this step.*
5. On receiving key_param message, both the clients set their keys. Client1 computes key=B<sup>x</sup>mod n = (g <sup>y</sup>)<sup>x</sup> mod n and client2 computes key using key = A<sup>y</sup> mod n = (g<sup>x</sup>)<sup>y</sup> mod n.

Hence, a common key is established between them.

**Note:** In the code, there are no x and y.. Code is written in symmetric way. Both the clients refer their keys as x only (They are different variables in different computers).

**Note:** In our code, whenever there is a change in connection, say you disconnect and reconnect, a new common key is established.

<div align = "center">
	<img src = "https://user-images.githubusercontent.com/59964272/197687852-23d83c0c-0eee-4397-8818-91b7b2d7f698.JPG" height=318 width=491>
	<img src = "https://user-images.githubusercontent.com/59964272/197687890-e6d72950-9639-4f48-b4d9-5be9869dd54b.JPG" height=318 width=491>
	<img src = "https://user-images.githubusercontent.com/59964272/197687991-6ffa053e-4f77-4255-80cc-a4d1f4871b82.JPG" height=318 width=491>
	<img src = "https://user-images.githubusercontent.com/59964272/197688036-19907f63-8aa3-4c8b-8cd6-6185b253cc88.JPG" height=318 width=491>
	<img src = "https://user-images.githubusercontent.com/59964272/197688099-a65f9a76-0ef8-4777-90b4-05b2fb56f437.JPG" height=318 width=491>
	<img src = "https://user-images.githubusercontent.com/59964272/197688146-40c05d57-8f3d-4a2a-84c5-77aa1f1b433f.JPG" height=318 width=491>
</div>
<br>


## Developer
**Sushant Kumar**
	

