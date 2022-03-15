#define PORT 8083
#define MAX_SIZE 20480

#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h> 
#include <vector>

using namespace std;

void processSpecialCommands(string msgRead);
vector <string> splitWord(string &s, char delimiter);

// This class implemented by me proivdes me a high level interface to network as a client:
class Client{
    int  sockfd;
    struct sockaddr_in saddr;

public:

    bool constructNow(string server_address,int server_port){
        //Initialise the socket:
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if(sockfd < 0){
            cout << "Error creating the socket"<<endl;
            exit(EXIT_FAILURE);
        }
        
        //Set the address:
        memset(&saddr, '\0', sizeof(sockaddr));
        saddr.sin_family = AF_INET;
        saddr.sin_port = htons(server_port);

        if(inet_pton(AF_INET, &server_address[0], &saddr.sin_addr)<=0){
            cout << "Invalid Address"<< endl;
            exit(EXIT_FAILURE);
        }

        return true;
    }

    void connectnow(){
        //Start Connection:
        if(connect(sockfd, (sockaddr *)&saddr, sizeof(sockaddr)) < 0){
            cout << "Connection Failed" << endl;
            exit(EXIT_FAILURE);
        }
    }


    string readMsg(){
        string msg = "";
        char last_character = '-';

        //This is a part of my protocol
        do{
            char buffer[2048] = {0};
            read( sockfd , buffer, 2048);
            string s(buffer);
            msg += s;
            last_character = s[s.size()-1];
        }while(last_character!='%');

        return msg.substr(0, msg.size()-1);
    }

    bool sendMsg(string msg){
        // char *msg_pointer = &msg[0];
        // send(sockfd, msg_pointer, sizeof(msg), 0);

        //This protocol is used to send the message as the message is not sent fully by the underlying layer:
        //The message is sent in parts:
        msg += "%";         //Special Terminating Character:
        char buffer[msg.size()+1];
        strcpy(buffer, &msg[0]);

        int bytes_sent_total = 0;
        int bytes_sent_now = 0;
        int message_len = msg.length();
        while (bytes_sent_total < message_len)
        {
            bytes_sent_now = send(sockfd, &buffer[bytes_sent_total], message_len - bytes_sent_total, 0);
            if (bytes_sent_now == -1)
            {
                // Handle error:
                cout <<"Send Msg Error in Sending Protocol" << endl;
                return false;
            }
            bytes_sent_total += bytes_sent_now;
        }
        return true;
    }

    void closenow(){
        close(sockfd);
    }
};

//Global controller variables:
Client myclient = Client();
pthread_t recv_t, send_t;

//Main client logic:
void* receiveMsg(void *arg){
    while(true){
        string msgRead = myclient.readMsg();
        cout << msgRead << endl;
        processSpecialCommands(msgRead);
    }
}

void* sendMsg(void *arg){
    while(true){
        string msg;
        getline(cin, msg);
        myclient.sendMsg(msg);
        processSpecialCommands(msg);
    }
}

//Helper Functions:
void processSpecialCommands(string msgRead){
    vector<string> parsedCommand = splitWord(msgRead, ' ');
    if(parsedCommand.size()>0 && (parsedCommand[0].compare("close")==0 || parsedCommand[0].compare("\x1b[31mClose:")==0))
        exit(0);
}

//Utility Functions:
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


//Signal Handlers:
void exit_handler(int sig){
    myclient.sendMsg("close");
    myclient.closenow();
    exit(0);
}



int main(int argc, char** argv){

    try{
        string ip_address(argv[1]);

        //Construct the client:
        myclient.constructNow(ip_address, stoi(argv[2]));
    }catch(exception e){
        cout << "The correct format is:\n";
        cout <<  "./client server_ip_address server_port\n";
        exit(0);
    }


    //Register signal handlers:
    signal(SIGINT, exit_handler); 

    myclient.connectnow();
    cout << "Connected to server..." << endl;
    cout << "Enter your username: ";
    
    string myName;
    getline(cin, myName);
    myclient.sendMsg(myName);

    //Just to confirm that the connection is established:

    pthread_create(&recv_t, NULL, receiveMsg, NULL);
    pthread_create(&send_t, NULL, sendMsg, NULL);

    pthread_join(recv_t, NULL);
    pthread_join(send_t, NULL);

    myclient.closenow();
    return 0;
}