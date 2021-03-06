# Multithreaded Chat Server
## Allows multiple clients to tallk to each other

Using this application, multiple clients can talk to each other by connecting to the server and watch each others' statuses  - BUSY or FREE, and based on availability connect to the particular client.

<div align="center"> 
<img src="https://user-images.githubusercontent.com/59964272/158463392-314d9833-17de-4387-bcc7-b64ac536929b.gif" alt="Gif showing usage of this program" height="382px">
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
12. All the strings except command words will be sent to other user and can be received from other user.
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
	5. Ctrl+C (client side) : Ends current chatting session (if present) and disconnects and terminates the client.
	6. Ctrl+C (server side) : Ends all the chatting sessions and disconnects all the clients and terminates the server.

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
	

