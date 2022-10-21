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

<div align = "center">
	<img src = "https://user-images.githubusercontent.com/59964272/156934296-32dc27bd-ac2c-48d0-8d8d-752726ec65d6.JPG" height=318 width=491>
	<img src = "https://user-images.githubusercontent.com/59964272/156934314-0be63b5b-2e43-4e97-bdcd-266ea9b5ba65.JPG" height=318 width=491>
	<img src = "https://user-images.githubusercontent.com/59964272/156934355-4fca18fd-295e-47b0-b4e4-30d6b68faf57.JPG" height=318 width=491>
	<img src = "https://user-images.githubusercontent.com/59964272/156934361-3023b150-273d-41a8-9459-9f24bfaf66d4.JPG" height=318 width=491>
	<img src = "https://user-images.githubusercontent.com/59964272/156934371-849e3b00-faba-46a7-ad00-cff049d11718.JPG" height=318 width=491>
	<img src = "https://user-images.githubusercontent.com/59964272/156934697-e92d0d3b-6ddf-4a59-b1aa-665d85fe257f.JPG" height=318 width=491>
</div>
<br>

## Developer
**Sushant Kumar**
	

