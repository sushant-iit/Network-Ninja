# FTP Clone
## Allows multiple clients to transfer files to and from the server

Using this application, multiple clients can push their files to the ftp server in their respected allocated personal spaces, download them as and when needed. They can also delete their personal files from the server.

## Directory Structure:
When server program is run the following directory structure is automatically created as and when needed: \
```
    server                
    ├── meta-data         
    │   ├── config.txt    (stores user_name and password)
    │   ├── logs.txt      (stores logs of the server)
    │   └── temp.txt      (temporary area required by server to process some functions)
    ├── server            (generated executable server for ubuntu) 
    ├── server.cpp        (the server program) 
    └── storage           (the storage area.. every user is allocated a special directory) 
        ├── user1          
        │   └── audio.mp3  
        ├── user2          
        │   ├── sample.txt 
        │   └── video.mp4  
        └── user3          
            └── audio.mp3  
 ```
            
When client program is run the following directory structure is automatically created as and when needed:
```
     client
     ├── client            (generated executable client for ubuntu)
     ├── client.cpp        (the client program)
     └── storage           (the storage area of the client, all the GET requests are saved here and PUT request files are fetched relative to this directory)
         ├── audio.mp3
         ├── sample.pdf
         ├── sample.txt
         └── video.mp4
```

## Usage:
1. Download server.cpp on the pc which is to be made an ftp server.
2. Open terminal in that directory.
3. Compile the server program using the command: \
	`g++ server.cpp -o server -lpthread --std=c++17`
4. Run the server using command: \
    `./server port_number` \
    For example: \
     `./server 8081` \
     You wil get message saying "(currrent timestamp) Server started successfully...". \
	 Note that some folders and files will be automatically created as shown in the directory structure.\
   All the logs will be stored in `./meta-data/logs.txt` file.
5. Download client.cpp on the pc which is to be used as client to send or receive the files.
6. Open the terminal in that directory.
7. Compile the client program using the command: \
    `g++ client.cpp -o client --std=c++17` 
8. Open a new terminal on server computer and find the ip address of server computer using the command: \
     `ifconfig` \
     	<div align = "center">
         <img src = "https://user-images.githubusercontent.com/59964272/158458097-ad34d2e1-3c56-4ee3-aa23-96d734064fa5.JPG" height=318 width=491>
	</div>
	Note the inet address (10.10.77.210) in this case.
9. Run the client using the command (they should be in the same subnet as server): \
    `./client server_ip_address server_port` \
    For example: \
    `./client 10.10.77.210 8081` \
	Note that a new directory will be created as described above.
10. If you are first time user, enter 'y' for sign up else enter 'n' for login.
11. Enter the username and password. (A new user will be created if you).
12. Put whatever you want to send in storage folder of the client which is automatically created.
13. Send the file to the server using the command: \
	`PUT <filename>` \
	Note that default mode of sending is binary mode. \
	If you want to send using ascii mode, use the command: \
	`PUT <filename> -a` 
14. You can look into your directory using the command: \
	`LS` \
	It will return blank if no file is there
15. To download the file from the server use the command: \
	`GET <filename>` \
	Note that default mode of receiving is binary mode. \
	If you want to send using ascii mode, use the command: \
	`GET <filename> -a` 
16. To delete a file from the server, type: \
	`DEL <filename>` 
17. To exit the ftp application, run the command: \
	`EXIT`

## Available commands and behaviours:
	The supported commands are:
	1. PUT <fileName>              : Send the file fileName to the server in binary mode
	2. PUT <fileName> -a           : Send the file fileName to the server in ASCII mode
	3. GET <fileName>              : Receive the file fileName from the server in binary mode
	4. GET <fileName> -a           : Receive the file fileName from the server ASCII mode
	5. LS                          : List the server directory contents of the logged in client
	6. EXIT                        : Closes the connection from the server and exits the ftp interface on the client side.

## Developer
**Sushant Kumar**
