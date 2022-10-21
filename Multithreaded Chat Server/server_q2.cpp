#define MAX_SIZE 20480
#define ANSI_RED "\x1b[31m"
#define ANSI_GREEN "\x1b[32m"
#define ANSI_YELLOW "\x1b[33m"
#define ANSI_BLUE "\x1b[34m"
#define ANSI_MAGENTA "\x1b[35m"
#define ANSI_CYAN "\x1b[36m"
#define ANSI_RESET "\x1b[0m"

#include <iostream>
#include <string>
#include <unordered_map>
#include <cstring>
#include <queue>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <signal.h> 
#include <mutex> 

using namespace std;
void* handleClient(void *arg);
bool connectToOtherClient(string my_client_name ,vector <string> parsedMsg);
void sendCurrentStatus(string my_client_name);
vector <string> splitWord(string &s, char delimiter);
string getGeneralInstructions(string my_client_name);
void closeTheSession(string my_client_name);

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
            cout << "Error creating the socket"<<endl;
            exit(EXIT_FAILURE);
        }

        //Set up the address:
        memset(&sin, '\0', sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = htonl(INADDR_ANY);
        sin.sin_port = htons(server_port);

        //Bind the socket to the local address:
        if( bind(sockfd,(sockaddr *)&sin, sizeof(sin)) < 0 ){
            cout << "Error Binding to the address" << endl;
            exit(EXIT_FAILURE);
        }

        port = server_port;

        return true;
    }

    void listenNow(){
        //Start Listening
        if(listen(sockfd, 10) < 0){
            cout << "Error Listening" << endl;
            exit(EXIT_FAILURE);
        }

        cout << "Server Listening on port: " << port << endl;
    }

    int acceptNow(){
        int new_socket;
        if((new_socket=accept(sockfd, (sockaddr *)&sin, (socklen_t *)&sin_len)) < 0){
            cout << "Error Accepting" << endl;
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

        return msg.substr(0, msg.size()-1);
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
                cout <<"Send Msg Error in Server Protocol" << endl;
                return false;
            }
            bytes_sent_total += bytes_sent_now;
        }
        return true;
    }

    void closeConnection(int client_socket){
        close(client_socket);
    }
};

//Global State Controllers Variable:
Server myserver = Server();
queue <int> waitingClient;
unordered_map <string, string> partnerClient;
unordered_map <string, int> sock;
mutex mtx;



//Main Business Logic Functions:--------------------------------------------------------------------------------
void* handleClient(void *arg){
    //Choose the earliest client to serve
    mtx.lock();
    int client_socket = waitingClient.front();
    waitingClient.pop();
    mtx.unlock();
    string client_name = myserver.readMsg(client_socket);

    //Check if already there is a user with that name:
    if(sock.find(client_name)!=sock.end()){
        string errMsg = ANSI_RED;
        errMsg += "Close: There is already a client with that name...\nDisconnecting...\n";
        errMsg += ANSI_RESET;
        myserver.sendMsg(client_socket, errMsg);
        cout << ANSI_RED << "Entered client_name already exists in the database..." << ANSI_RESET << endl;
        return NULL;
    }

    sock[client_name] = client_socket;

    cout << "Client ID: "<< client_name <<" service started"<<endl;
    myserver.sendMsg(client_socket, getGeneralInstructions(client_name));

    string req = "";
    while(true){

        req = myserver.readMsg(client_socket);
        vector <string> parsedMsg = splitWord(req, ' ');

        //If client requests status:
        if(parsedMsg[0].compare("status")==0)
            sendCurrentStatus(client_name);
        //If client wants to connect to other client:
        else if(parsedMsg[0].compare("connect")==0)
            connectToOtherClient(client_name ,parsedMsg);
        else if(parsedMsg[0].compare("close")==0){
            closeTheSession(client_name);
            break;
        }else if(parsedMsg[0].compare("key_exchange")==0){
            usleep(1000);   //Waiting 1ms for tcp to flush ...
            if(parsedMsg.size() < 1){
                string errMsg = ANSI_RED;
                errMsg += "Key Exchange Error: Param Not Found";
                errMsg += ANSI_RESET;
                myserver.sendMsg(sock[partnerClient[client_name]], errMsg);
            }
            string keyExchangeMsg = "key_param "+parsedMsg[1] + " "+ partnerClient[client_name];
            myserver.sendMsg(sock[partnerClient[client_name]], keyExchangeMsg);
        }
        //If this client is already connected to some client then send the read message to that client:
        else if(partnerClient.find(client_name)!=partnerClient.end() ){
            //Checkpoint: If the message is goodbye, close the session
            string msg = "\t";
            msg += "("+client_name+"): "+ req;
            myserver.sendMsg(sock[partnerClient[client_name]], msg);
            if(req.compare("goodbye")==0)
                closeTheSession(client_name);
            continue;
        }
        else{
            //This means illegal command:
            string errMsg = ANSI_RED;
            errMsg += "\t(server): Command Not Found";
            errMsg += ANSI_RESET;
            myserver.sendMsg(client_socket, errMsg);
        }

    }
    sock.erase(client_name);
    myserver.closeConnection(client_socket);
    cout << "Client ID: " << client_name <<" disconnected.."<<endl;
    pthread_exit(NULL); 
}

//Helper Functions ----------------------------------------------------------------------------------
void sendCurrentStatus(string my_client_name){
    vector <string> names;
    string statusMessage;
    statusMessage += ANSI_CYAN; 
    statusMessage += "\t(server):\n";

    for(auto it: sock)
        names.push_back(it.first);

    for(int i=0; i < names.size()-1; i++){
        string hasPartner = partnerClient.find(names[i])==partnerClient.end() ? "FREE" : "BUSY";
        statusMessage += "\t" + names[i] + " " + hasPartner + "\n";
    }

    string hasPartner = partnerClient.find(names[names.size()-1])==partnerClient.end() ? "FREE" : "BUSY";
    statusMessage += "\t" + names[names.size()-1] + " " + hasPartner + "\n";

    statusMessage += ANSI_RESET;
    myserver.sendMsg(sock[my_client_name], statusMessage);
    return;
}

bool connectToOtherClient(string my_client_name ,vector <string> parsedMsg){

    string otherClientName = parsedMsg[1];
    cout << "Session request from "+my_client_name+" to "+otherClientName << endl;

    if(partnerClient.find(my_client_name)!=partnerClient.end()){
        string errMsg = ANSI_RED;
        errMsg += "\t(server): You are already connected to someone.";
        errMsg += ANSI_RESET;
        myserver.sendMsg(sock[my_client_name], errMsg );
        cout << ANSI_RED << "Already connected, connect request rejected" << ANSI_RESET << endl;
        return false;
    }

    if(my_client_name.compare(otherClientName)==0){
        string errMsg = ANSI_RED;
        errMsg += "\t(server): Destination name can't be same as source Name";
        errMsg += ANSI_RESET;
        myserver.sendMsg(sock[my_client_name], errMsg );
        cout << ANSI_RED << "Destination name can't be same as source Name" << ANSI_RESET << endl;
        return false;
    }

    if(sock.find(otherClientName)==sock.end()){
        string errMsg = ANSI_RED;
        errMsg += "\t(server): No client named: "+otherClientName;
        errMsg += ANSI_RESET;
        myserver.sendMsg(sock[my_client_name], errMsg );
        cout << ANSI_RED <<"Couldn't connect: No client named: "<<otherClientName  <<ANSI_RESET <<  endl;
        return false;
    }
    
    if(partnerClient.find(otherClientName)!=partnerClient.end()){
        string errMsg = ANSI_RED;
        errMsg += "\t(server): "+otherClientName+" is BUSY";
        errMsg += ANSI_RESET;
        myserver.sendMsg(sock[my_client_name], ANSI_RED +errMsg);
        cout << ANSI_RED << "Couldn't connect: client "+otherClientName +" is busy" << ANSI_RESET<< endl;
        return false;
    }

    partnerClient[my_client_name] = otherClientName;
    partnerClient[otherClientName] = my_client_name;

    cout <<ANSI_GREEN << "Connected: "+my_client_name<< " and "<<otherClientName << ANSI_RESET << endl;

    //Ask the clients to generate keys for communication:
    string keyReqMsg = ANSI_YELLOW;
    keyReqMsg += "\n\t(Server): GEN_KEYS ";
    myserver.sendMsg(sock[otherClientName], keyReqMsg + ANSI_RESET);
    myserver.sendMsg(sock[my_client_name], keyReqMsg + ANSI_RESET);
    return true;
}

void closeTheSession(string my_client_name){
    //Inform the partner about the session closure;
    if(partnerClient.find(my_client_name)==partnerClient.end())
        return;
    string otherClientName = partnerClient[my_client_name];
    string msg = ANSI_CYAN;
    msg += "\n\t(server): "+my_client_name+" closed the chat session";
    msg += ANSI_RESET;
    myserver.sendMsg(sock[otherClientName],msg);

    //Update the data structures to close the connection:

    if(partnerClient.find(otherClientName)!=partnerClient.end())
        partnerClient.erase(otherClientName);

    if(partnerClient.find(my_client_name)!=partnerClient.end())
        partnerClient.erase(my_client_name);

    //Inform the requesting client about the closure:
    msg = "";
    msg += ANSI_CYAN;
    msg += "\t(server): Session closed";
    msg += ANSI_RESET;
    myserver.sendMsg(sock[my_client_name], msg);

    //Log the data to the server:
    cout << "Disconnected "+my_client_name + " and "+otherClientName << endl;
    
}

string getGeneralInstructions(string my_client_name){
    string msg = ANSI_CYAN;
    msg += "\t\n\t(server): Hi, "  + my_client_name +"\n";
    msg += "\tThe supported commands are:\n";
    msg += "\t1. status             : List all the users status\n";
    msg += "\t2. connect <username> : Connect to username and start chatting\n";
    msg += "\t3. goodbye            : Ends current chatting session\n";
    msg += "\t4. close              : Disconnects you from the server\n";
    msg += "\t5. <message>          : Sends the message to other client in plain text format.\n";
    msg += "\t6. secure: <message>  : Sends the message in secure format to other client.\n";
    msg += ANSI_RESET;
    return msg;
}

//Utility functions--------------------------------------------------------------------------------
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

//Signal Handlers:---------------------------------------------------------------------------------
void exit_handler(int sig){
    cout <<"\nShutting down the server...\nDisconnecting Clients...\n";
    for(auto it: sock){
        myserver.sendMsg(it.second, "close");
        myserver.closeConnection(it.second);
    }
    exit(0);
    return;
}

//Main Functions:----------------------------------------------------------------------------------

int main(int argc, char **argv){

    try{
        //Construct the server:
        myserver.constructNow(stoi(argv[1]));
        
    }catch(exception e){
        cout << "Please provide command in the following format:\n";
        cout << "./server port\n";
        exit(0);
    }

    //Register signal handlers:
    signal(SIGINT, exit_handler); 

    myserver.listenNow();
    cout << "Server started successfully..." << endl; 

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

