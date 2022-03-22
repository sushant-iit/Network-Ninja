#define FILE_CHUNK_SIZE 256
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_RESET "\x1b[0m"

#include <iostream>
#include <stdio.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cstring>
#include <queue>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h> 
#include <mutex> 
#include <sys/stat.h>
#include <time.h>
#include <fstream>

using namespace std;

void* handleClient(void *arg);
vector <string> splitWord(string &s, char delimiter);
void handleAuthentication(int client_socket);
string getTimestamp();
void initialiseTheDataStructures();
void saveDataStructures();
string execute(string cmd );


//Global State Controllers Variable:
mutex mtx;
queue <int> waitingClient;
unordered_map <string, string> authDetails;
unordered_set <string> activeClients;
unordered_map <int, string> sockMap;


// This class implemented by me proivdes me a high level interface to network as a server:
// Updated constructs to allow multiple clients:
class Server{
    int  sockfd;
    struct sockaddr_in sin;
    int sin_len = sizeof(sin);
    int port = 0;

public:
    bool constructNow(int server_port){

        //Initialise the socket:
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if(sockfd < 0){
            cout<< getTimestamp()  << "Error creating the socket"<<endl;
            exit(EXIT_FAILURE);
        }

        //Set up the address:
        memset(&sin, '\0', sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
        sin.sin_port = htons(server_port);

        //Bind the socket to the local address:
        if( bind(sockfd,(sockaddr *)&sin, sizeof(sin)) < 0 ){
            cout << getTimestamp() << "Error Binding to the address" << endl;
            exit(EXIT_FAILURE);
        }

        port = server_port;

        return true;
    }

    void listenNow(){
        //Start Listening
        if(listen(sockfd, 10) < 0){
            cout << getTimestamp() << "Error Listening" << endl;
            exit(EXIT_FAILURE);
        }

        cout << getTimestamp() << "Server Listening on port: " << port << endl;
    }

    int acceptNow(){
        int new_socket;
        if((new_socket=accept(sockfd, (sockaddr *)&sin, (socklen_t *)&sin_len)) < 0){
            cout << getTimestamp() << "Error Accepting" << endl;
            exit(EXIT_FAILURE);
        }
        return new_socket;
    }

    string readMsg(int client_socket){
        string msg = "";
        char last_character = '-';

        //This is a part of my protocol
        do{
            char buffer[2048] = {0};
            read(client_socket , buffer, 2048);
            string s(buffer);
            msg += s;
            last_character = s[s.size()-1];
        }while(last_character!='%');
        
        msg = msg.substr(0, msg.size()-1);
        lookForSpecialCommands(client_socket, msg);        //It is a kind of middleware, which awares server in the event of unusual exit by the client;
        return msg;
    }

    bool sendMsg(int client_socket, string msg){

        //Earlier Method:
        // char *msg_pointer = &msg[0];
        // send(client_socket, msg_pointer, sizeof(msg), 0);

        //This protocol is used to send the message as the message is not sent fully by the underlying layer:
        //The message is sent in parts:
        msg = msg + "%";         //Special Terminating Character:
        char buffer[msg.size()];
        strcpy(buffer, &msg[0]);

        int bytes_sent_total = 0;
        int bytes_sent_now = 0;
        int message_len = msg.length();
        while (bytes_sent_total < message_len)
        {
            bytes_sent_now = send(client_socket, &buffer[bytes_sent_total], message_len - bytes_sent_total, 0);
            if (bytes_sent_now == -1)
            {
                // Handle error
                cout << getTimestamp() <<"Send Msg Error in Server Protocol" << endl;
                return false;
            }
            bytes_sent_total += bytes_sent_now;
        }
        return true;
    }

    void closeConnection(int client_socket){
        close(client_socket);
    }


    void sendFile(int client_socket){
        //Get the mode name:
        string mode = readMsg(client_socket);
        if(mode.compare("-a")==0)
            sendMsg(client_socket, "(server): Transferring in ascii mode\n");
        else
            sendMsg(client_socket, "(server): Transferring in binary mode\n");

        struct stat stats;
        //Get the file name which client requests:
        string fileName = readMsg(client_socket);
        string dirName = "./storage/"+sockMap[client_socket];
        string fileNameWithPath = dirName + "/" + fileName;

        //Get the file information:
        if(stat(&fileNameWithPath[0], &stats) == -1){
            cout << getTimestamp() << "Client ID: " << client_socket << " Can't get file info" << endl;
            sendMsg(client_socket, "fail\n");
            return;
        }
    
        //Send success msg to the receiver
        sendMsg(client_socket, "(server): File name received\n");
        readMsg(client_socket);     //Useless just to complete send-recv cycle

        // //Open the file for reading:    
        FILE* fp;
        if(mode.compare("-a")==0)
            fp = fopen(&fileNameWithPath[0], "r");
        else
            fp = fopen(&fileNameWithPath[0], "rb");

        if(fp==NULL){
            cout << getTimestamp() << "Client ID: " << client_socket << "Couldn't open file" << endl;
            sendMsg(client_socket, "fail\n");
            return;
        }
        sendMsg(client_socket, "(server): File opened\n");
        readMsg(client_socket);   //Useless just to complete send-recv cycle

        //Send the file size information to the receiver for proper processing:
        long long remaining_file_size = (long long) stats.st_size;
        sendMsg(client_socket, to_string(remaining_file_size));
        if(readMsg(client_socket).compare("fail\n")==0){
            cout << getTimestamp() << "Client ID: " << client_socket << "File info couldn't be sent" << endl;
            fclose(fp);
            return;
        }

        while(remaining_file_size > 0){
            char buffer[FILE_CHUNK_SIZE] = {0};
            long long send_size = remaining_file_size < FILE_CHUNK_SIZE ? remaining_file_size : FILE_CHUNK_SIZE;
            long long read_size = fread(buffer, 1, send_size, fp);
            if(read_size <= 0)
                break;
            int bytes_send_now = send(client_socket, buffer, read_size, 0);
            if(bytes_send_now == -1){
                cout << "Sending File Failed" <<endl;
                fclose(fp);
                exit(1);
            }
            remaining_file_size -= read_size;
        }

        fclose(fp);
        cout << getTimestamp() << "Client ID: " << client_socket << " File Sent successfully..."<< endl;
        return;

    }


    void recvFile(int client_socket, string dest_path){
        //Get the mode name:
        string mode = readMsg(client_socket);
        if(mode.compare("-a")==0)
            sendMsg(client_socket, "(server): Transferring in ascii mode\n");
        else
            sendMsg(client_socket, "(server): Transferring in binary mode\n");
        

        //Get the file size from the sender and send the confirmation for the proper processing:
        long long remaining_file_size = stoll(readMsg(client_socket));
        sendMsg(client_socket, "(server): file size recevied");
        
        //Get the file name from the sender and send the confirmation for the proper processing:
        string fileName = readMsg(client_socket);
        sendMsg(client_socket, "(server): file name recevied");

        //Make the directory for the user if not exists:
        string dirName = "./storage/"+sockMap[client_socket];
        mkdir(&dirName[0], 0777);
        string fullFilePathWithName = dirName +"/"+ fileName;


        //Open the file for writing:    
        FILE* fp;
        if(mode.compare("-a")==0)
            fp = fopen(&fullFilePathWithName[0], "w");
        else
            fp = fopen(&fullFilePathWithName[0], "wb");

        if(fp==NULL){
            cout << getTimestamp() << "Couldn't open file for writing" << endl;
            return;
        }

        while(remaining_file_size > 0){
            char buffer[FILE_CHUNK_SIZE] = {0};
            long long recv_size = remaining_file_size < FILE_CHUNK_SIZE ? remaining_file_size : FILE_CHUNK_SIZE;
            long long bytes_recv_now = read(client_socket, buffer, recv_size);
            if(bytes_recv_now == -1){
                cout << "Receiving File Failed" <<endl;
                fclose(fp);
                return;
            }
            long long bytes_written_now = fwrite(buffer, 1, bytes_recv_now, fp);
            if(bytes_written_now < 0){
                cout << "Receiving File Failed" <<endl;
                fclose(fp);
                return;
            }
            remaining_file_size -= bytes_recv_now;
        }

        fclose(fp);
        return;
    }

    void deleteFile(int client_socket){
        string fileNamePath = "./storage/" + sockMap[client_socket] + "/"+ readMsg(client_socket);
        int result = remove(&fileNamePath[0]);
        string res;
        if(result == -1){
            res = ANSI_RED;
            res += "Couldn't delete file...\n";
            res += ANSI_RESET;
        }
        else{
            res = ANSI_GREEN;
            res += "File Deleted successfully...\n";
            res += ANSI_RESET;
        }
        sendMsg(client_socket, res);
    }

private:
    void lookForSpecialCommands(int client_socket, string msg){
        //If at any moment client exits, when server will try to listen to client, it will exit him:
        if(msg.compare("close")==0){
            closeConnection(client_socket);
            activeClients.erase(sockMap[client_socket]);
            sockMap.erase(client_socket);
            cout << getTimestamp() << "Client ID: " <<client_socket << " disconnected" << endl;
            pthread_exit(NULL);
        }
    }

};

Server myserver = Server();

//Main Business Logic Functions:--------------------------------------------------------------------------------
void* handleClient(void *arg){
    //Choose the earliest client to serve
    mtx.lock();
    int client_socket = waitingClient.front();
    waitingClient.pop();
    mtx.unlock();

    cout << getTimestamp() << "Client ID: " <<client_socket << " connected" << endl;

    //Authentication:
    handleAuthentication(client_socket);

    //Make user directory if not there:
    string user_directory = "./storage/"+ sockMap[client_socket];
    mkdir(&user_directory[0], 0777);

    //Now listen to the actual requests:
    string req = "";
    while(req.compare("exit")!=0){
        req = myserver.readMsg(client_socket);
        cout <<getTimestamp() << "Client ID: " <<client_socket  << " ServiceReq: "<<req << endl;
        vector <string> parsedCommands = splitWord(req, ' ');
        if(parsedCommands[0].compare("PUT")==0){
            myserver.sendMsg(client_socket, "(server): Ready for file transfer");
            myserver.recvFile(client_socket, "");
        }else if(parsedCommands[0].compare("GET")==0){
            myserver.sendMsg(client_socket, "(server): Ready for file transfer");
            myserver.sendFile(client_socket);
        }else if(parsedCommands[0].compare("LS")==0){
            myserver.sendMsg(client_socket, execute("ls ./storage/" + sockMap[client_socket]));
        }else if(parsedCommands[0].compare("DEL")==0){
            myserver.sendMsg(client_socket, "Deleting...\n");
            myserver.deleteFile(client_socket);
        }
    }




    activeClients.erase(sockMap[client_socket]);
    sockMap.erase(client_socket);
    myserver.closeConnection(client_socket);
    cout << getTimestamp() << "Client ID: " <<client_socket << " disconnected" << endl;
    pthread_exit(NULL); 
}

//Helper Functions ----------------------------------------------------------------------------------
void handleAuthentication(int client_socket){
    bool isAuthenticated = false;

    while(!isAuthenticated){
        string authInfo = myserver.readMsg(client_socket);
        cout<< getTimestamp() << "Client ID: " <<client_socket  << " AuthReq: "<<authInfo << endl;
        vector <string> clientInfo = splitWord(authInfo, ':');

        if(clientInfo.size()!=3){
            string errMsg = ANSI_RED;
            errMsg += "Authentication Failure\n";
            errMsg += "Please enter the details correctly. Make sure there is no ':' character in user_name or password\n";
            errMsg += ANSI_RESET;
            myserver.sendMsg(client_socket, errMsg);
            continue;
        }

        //SignUp or login based on the request:
        if(clientInfo[2].compare("y")==0){

            if(authDetails.find(clientInfo[0])!=authDetails.end()){
                string errMsg = ANSI_RED;
                errMsg += "Create user failure\n";
                errMsg += "There is already a user with that name in the database\n";
                errMsg += ANSI_RESET;
                myserver.sendMsg(client_socket, errMsg);
                continue;
            }

            //Create the user and log him in:
            authDetails[clientInfo[0]] = clientInfo[1];
            string resMsg = ANSI_GREEN;
            resMsg += "LogIn Success";
            resMsg += ANSI_RESET;
            myserver.sendMsg(client_socket, resMsg);
            isAuthenticated = true;
            activeClients.insert(clientInfo[0]);
            sockMap[client_socket] = clientInfo[0];

        }else{

            if(activeClients.find(clientInfo[0])!=activeClients.end()){
                string errMsg = ANSI_RED;
                errMsg += "Login user failure\n";
                errMsg += "Client Active In Another Session\n";
                errMsg += ANSI_RESET;
                myserver.sendMsg(client_socket, errMsg);
                continue;
            }

            if(authDetails.find(clientInfo[0])==authDetails.end() || authDetails[clientInfo[0]].compare(clientInfo[1])!=0){
                string errMsg = ANSI_RED;
                errMsg += "Login user failure\n";
                errMsg += "Invalid user_name or password\n";
                errMsg += ANSI_RESET;
                myserver.sendMsg(client_socket, errMsg);
                continue;
            }

            //Log the user in:
            string resMsg = ANSI_GREEN;
            resMsg += "LogIn Success";
            resMsg += ANSI_RESET;
            myserver.sendMsg(client_socket, resMsg);
            isAuthenticated = true;
            activeClients.insert(clientInfo[0]);
            sockMap[client_socket] = clientInfo[0];
        }
    }
}

//Utility functions--------------------------------------------------------------------------------
void initialiseTheDataStructures(){
    //Make the important folders if they are not there;

    mkdir("meta-data", 0777);
    mkdir("storage", 0777);

    ifstream configFile;
    configFile.open("./meta-data/config.txt");
    string user;

    while(getline(configFile, user)){
        vector <string> userInfo = splitWord(user, ':');
        authDetails[userInfo[0]] = userInfo[1];
    }

    configFile.close();

    return;
}

//Saves the user states before closing:
void saveDataStructures(){
    ofstream conifgFile;
    conifgFile.open("./meta-data/config.txt");
    for(auto it: authDetails){
        conifgFile << it.first <<":"<<it.second << endl;
    }
    conifgFile.close();
    return;
}

string execute(string cmd )
{
    string file_name = "./meta-data/temp.txt" ;
    system( ( cmd + " > " + file_name ).c_str() ) ; // redirect output to file

    // open file for input, return string containing characters in the file
    ifstream file(file_name) ;
    string output = ANSI_BLUE, temp;
    while(getline(file, temp)){
        output += temp + " ";
    }
    output += "\n";
    output += ANSI_RESET;

    return  output;
}

vector <string> splitWord(string &s, char delimiter){
    vector <string> res;
    string curr;
    for(auto x : s){
        if(x==delimiter){
            res.push_back(curr);
            curr = "";
        }else
        curr += x;
    }
    res.push_back(curr);
    return res;
}


string getTimestamp()
{
    time_t ltime;
    ltime=time(NULL);
    string res = string(asctime( localtime(&ltime) ));
    return res.substr(0, res.size()-1)+"\t";
}

//Signal Handlers:---------------------------------------------------------------------------------
void exit_handler(int sig){
    cout << getTimestamp() <<"Shutting down the server...\nSaving the data...\n";
    saveDataStructures();
    exit(0);
    return;
}

//Main Functions:----------------------------------------------------------------------------------

int main(int argc, char **argv){

    initialiseTheDataStructures();

    try{
        //Construct the server:
        myserver.constructNow(stoi(argv[1]));
        
    }catch(exception e){
        cout << getTimestamp() << "Please provide command in the following format:\n";
        cout << getTimestamp() << "./server port\n";
        exit(0);
    }

    //Register signal handlers:
    signal(SIGINT, exit_handler); 

    myserver.listenNow();
    cout << getTimestamp() << "Server started successfully..." << endl; 
    cout << "All the server events are stored in ./meta-data/logs.txt\nPlease use that for debugging purposes\n";
    freopen("./meta-data/logs.txt","a",stdout);
    cout << getTimestamp() << "Server started successfully..." << endl; 

    //Accept connections in infinite loop
    while(true){
        //Accept the connections and let the separate thread handle each client;
        int client_socket = myserver.acceptNow();
        waitingClient.push(client_socket);
        pthread_t tid;
        pthread_create(&tid, NULL, handleClient, NULL);
    }

    return 0;
}

