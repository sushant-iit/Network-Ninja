#define PORT 8083
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
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h> 
#include <vector>
#include <climits>
#include "crypto.hpp"

using namespace std;

//Diffie-Hellman Parameters:
const int n = 1999, g = 1777;
int x, key;

int pow2mod(int number, int power, int mod);
string preProcessMessage(string msgRead, bool isMsgRead = false);
void postProcessSpecialCommands(string msgRead);
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
        msgRead = preProcessMessage(msgRead, true);
        cout << msgRead << endl;
        postProcessSpecialCommands(msgRead);
    }
}

void* sendMsg(void *arg){
    while(true){
        string msg;
        getline(cin, msg);
        msg = preProcessMessage(msg);
        myclient.sendMsg(msg);
        postProcessSpecialCommands(msg);
    }
}

//Helper Functions:
void postProcessSpecialCommands(string msgRead){
    vector<string> parsedCommand = splitWord(msgRead, ' ');
    if(parsedCommand.size()>0 && (parsedCommand[0].compare("close")==0 || parsedCommand[0].compare("\x1b[31mClose:")==0))
        exit(0);
    if(parsedCommand.size()>1 && parsedCommand[1].compare("GEN_KEYS")==0){
        long long seed = (long long)&parsedCommand[0];
        seed = seed%INT_MAX + time(0)%INT_MAX;
        srand(seed);
        x = rand()%n;
        int A = pow2mod(g, x, n);
        myclient.sendMsg("key_exchange "+to_string(A));
    }
}

string preProcessMessage(string msgRead, bool isMsgRead){
    vector<string> parsedCommand = splitWord(msgRead, ' ');
    if(parsedCommand.size() > 0 && parsedCommand[0].compare("key_param")==0){
        if(parsedCommand.size() < 3){
            string errMsg = ANSI_RED;
            errMsg += "Error! Key could not be retrieved\n";
            errMsg += ANSI_RESET;
            cout << errMsg;
        }
        int B = stoi(parsedCommand[1]);
        key = pow2mod(B, x, n);
        string successMsg = ANSI_GREEN;
        successMsg += "\t(server): Diffie-Hellman-Key-Exchange Success, Key: " + to_string(key);
        successMsg += ANSI_RESET;
        return successMsg;
    }
    //Encrypting or Decrypting the message:
    if((parsedCommand.size() > 0 && parsedCommand[0].compare("secure:")==0 )||
        (parsedCommand.size() > 1 && parsedCommand[1].compare("secure:")==0)){
        string sender;
        int i = 0;
        while(i < msgRead.length() && msgRead[i]!=':') i++;
        i++;
        if(isMsgRead){
            //Skipping the first colon:
            sender = msgRead.substr(0, i);
            while(i < msgRead.length() && msgRead[i]!=':') i++;
            i++;
        }
        string msgToBeEncryptedDecrytped;
        if(i+1 < msgRead.length())
            msgToBeEncryptedDecrytped += msgRead.substr(i+1);
        string  encryptedDecryptedText = encrypt_decrypt(msgToBeEncryptedDecrytped, to_string(key));
        string finalMessage;
        if(isMsgRead)
            finalMessage = sender + " secure: " + msgToBeEncryptedDecrytped + " [" + encryptedDecryptedText + "]";
        else
            finalMessage = "secure: " + encryptedDecryptedText;
        return finalMessage;
    }
    return msgRead;
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

int pow2mod(int number, int power, int mod){
    if(power==0) return 1;
    int x = pow2mod(number, power/2, mod);
    x = 1LL * x * x % mod;
    if(power&1)
        x = 1LL * x * number % mod;
    return x;
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