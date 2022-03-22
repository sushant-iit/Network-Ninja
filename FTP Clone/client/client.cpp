#define PORT 8083
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
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h> 
#include <vector>
#include <sys/stat.h>
#include <sys/time.h>

using namespace std;

vector <string> splitWord(string &s, char delimiter);

// This class implemented by me proivdes me a high level interface to network as a client:-----------------------------------------------
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

    void sendFile(string fileNameWithPath, string mode){
       //Send the transfer mode to the server:
       sendMsg(mode);
       cout << readMsg();

        struct stat stats;
        string fileName;

        //Get the file information:
        if(stat(&fileNameWithPath[0], &stats) == -1){
            cout << ANSI_RED << "Can't get file info" << ANSI_RESET << endl;
            return;
        }
        vector <string> splitFilePath = splitWord(fileNameWithPath, '/');
        if(splitFilePath.size()==0 || splitFilePath[splitFilePath.size()-1].size()==0){
            cout << ANSI_RED << "File Name couldn't be found" << ANSI_RESET << endl;
            return;
        }
        fileName = splitFilePath[splitFilePath.size()-1];

        //Open the file for reading:    
        FILE* fp;
        if(mode.compare("-a")==0)
            fp = fopen(&fileNameWithPath[0], "r");
        else
            fp = fopen(&fileNameWithPath[0], "rb");

        if(fp==NULL){
            cout << ANSI_RED << "Couldn't open file" << ANSI_RESET << endl;
            return;
        }

        //Send the file information to the receiver for proper processing:
        long long remaining_file_size = (long long) stats.st_size;
        long long sent_size = 0;

        //Check from the server if file size received:
        sendMsg(to_string(remaining_file_size));
        cout << readMsg() << endl;

        
        //Check from the server if file name received:
        sendMsg(fileName);
        cout << readMsg() << endl;

        //For calculating speed of the transfer and displaying progress bar:
        struct timeval ts, te;
        gettimeofday(&ts, NULL); 
        string progress = "";
        long long tot_size = remaining_file_size;
        float prev_per_complete = 0, curr_per_complete = 0;

        while(remaining_file_size > 0){
            char buffer[FILE_CHUNK_SIZE] = {0};
            long long send_size = remaining_file_size < FILE_CHUNK_SIZE ? remaining_file_size : FILE_CHUNK_SIZE;
            long long read_size = fread(buffer, 1, send_size, fp);
            if(read_size <= 0)
                break;
            int bytes_send_now = send(sockfd, buffer, read_size, 0);
            if(bytes_send_now == -1){
                cout << ANSI_RED << "Sending File Failed" << ANSI_RESET <<endl;
                fclose(fp);
                exit(1);
            }
            remaining_file_size -= read_size;

            //For displayig the progress:
            sent_size += read_size;
            gettimeofday(&te, NULL); 
            curr_per_complete = (sent_size*100) / (double)tot_size;
            if(curr_per_complete - prev_per_complete > 10){
                progress += ".";
                prev_per_complete = curr_per_complete;
                cout << "\rSent till now: " << (double)sent_size/1000 << "kb Speed: "<< ((double)sent_size*1000)/(te.tv_usec - ts.tv_usec) << "kb/s Progress:" ;
                cout << progress;
            }
        }

        cout << endl;
        fclose(fp);
        cout << ANSI_GREEN << "File Sent successfully...\n"<<ANSI_RESET << endl;

        return;
  
    }

   void recvFile(string fileName, string mode){
       //Send the transfer mode to the server:
       sendMsg(mode);
       cout << readMsg();

       //Send the fileName to the sender for the retrieval and get confirmation:
       sendMsg(fileName);
       string response = readMsg();
       if(response.compare("fail\n")==0){
           cout << ANSI_RED << "Either server didn't receive file name or the given fileName doesn't exist on the server\n" << ANSI_RESET << endl;
            return;
       }
        cout << response;

        sendMsg("USELESS\n");        //It has logically no use but just to complete the cycle and synchronisation.

        //Check if the sender is able to open the file:
        response = readMsg();
        if(response.compare("fail\n")==0){
            cout << ANSI_RED << "Server couldn't read the file properly\n" << ANSI_RESET << endl;
            return;
       }
        cout << response;

        sendMsg("USELESS\n");        //It has logically no use but just to complete the cycle and synchronisation.

        //Get the file size from the sender and send the confirmation for the proper processing:
        long long remaining_file_size  = 0, received_size = 0;

        try{
            remaining_file_size = stoll(readMsg());
        }catch(exception err){
            sendMsg("fail\n");
            return;
        }
        sendMsg("success\n");

        string fullFilePathWithName = "./storage/" + fileName;

        //Open the file for writing:    
        FILE* fp;
        if(mode.compare("-a")==0)
            fp = fopen(&fullFilePathWithName[0], "w");
        else
            fp = fopen(&fullFilePathWithName[0], "wb");

        if(fp==NULL){
            cout << ANSI_RED << "Couldn't open file for writing" << ANSI_RESET << endl;
            return;
        }

        //For calculating speed of the transfer and displaying progress bar:
        struct timeval ts, te;
        gettimeofday(&ts, NULL); 
        string progress = "";
        long long tot_size = remaining_file_size;
        float prev_per_complete = 0, curr_per_complete = 0;

        while(remaining_file_size > 0){
            char buffer[FILE_CHUNK_SIZE] = {0};
            long long recv_size = remaining_file_size < FILE_CHUNK_SIZE ? remaining_file_size : FILE_CHUNK_SIZE;
            long long bytes_recv_now = read(sockfd, buffer, recv_size);
            if(bytes_recv_now == -1){
                cout << ANSI_RED << "Receiving File Failed\n" << ANSI_RESET <<endl;
                fclose(fp);
                return;
            }
            long long bytes_written_now = fwrite(buffer, 1, bytes_recv_now, fp);
            if(bytes_written_now < 0){
                cout << ANSI_RED << "Receiving File Failed\n" << ANSI_RESET <<endl;
                fclose(fp);
                return;
            }
            remaining_file_size -= bytes_recv_now;

            //For displayig the progress:
            received_size += bytes_recv_now;
            gettimeofday(&te, NULL); 
            curr_per_complete = (received_size*100) / (double)tot_size;
            if(curr_per_complete - prev_per_complete > 10){
                progress += ".";
                prev_per_complete = curr_per_complete;
                cout << "\rReceived till now: " << (double)received_size/1000 << "kb Speed: "<< ((double)received_size*1000)/(te.tv_usec - ts.tv_usec) << "kb/s Progress:" ;
                cout << progress;
            }
        }
        cout << endl;

        fclose(fp);
        cout << ANSI_GREEN <<"File received successfully...\n" << ANSI_RESET << endl;
        return;
    }

    void deleteFile(string fileName){
        sendMsg(fileName);

        //Get the response from the server if the file is actually deleted:
        cout << readMsg() << endl;

    }

private:

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

};

//Helper functions------------------------------------------------------------------------------------------------
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

bool checkForExistenceOfFile(string fileNameWithPath){
    FILE* fp = fopen(&fileNameWithPath[0], "rb");
    if(fp==NULL)
        return false;
    fclose(fp);
    return true;
}

//Global controller variables:------------------------------------------------------------------------------------
Client myclient = Client();


//Signal Handlers:------------------------------------------------------------------------------------------------
void exit_handler(int sig){
    myclient.sendMsg("close");
    myclient.closenow();
    cout << endl;
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

    //Make the directory for storage:
    mkdir("storage", 0777);


    //Register signal handlers:
    signal(SIGINT, exit_handler); 

    myclient.connectnow();
    cout << "Connected to the ftp server..." << endl;

    //Get the authentication details:
    string serverResponse = "";

    while(serverResponse.compare("\x1b[32mLogIn Success\x1b[0m")!=0){
        string username, password;
        string newUser="";
        while(newUser.compare("y")!=0 && newUser.compare("n")!=0 && newUser.compare("e")!=0){
            cout << "Are you a new user?\n y: yes\n n: no\n e: exit\t";
            getline(cin, newUser);
        }
        if(newUser.compare("e")==0)
            exit_handler(SIGKILL);
        cout << "Enter the user name: ";
        getline(cin, username);
        cout << "Enter the password: ";
        getline(cin, password);

        //Send the auth:
        string authDetails = username + ':' + password + ':' + newUser;
        myclient.sendMsg(authDetails);

        //Get the response from the server:
        serverResponse = myclient.readMsg();
        cout << serverResponse << endl << endl;
    }

    while(true){
        cout << "ftp-> ";
        string req;
        getline(cin, req);
        
        //Check for the commands and do the appropriate tasks:
        vector <string> parsedCommand = splitWord(req, ' ');

        if(parsedCommand.size() <  1){
            cout << ANSI_RED << "Command Not Found\n" << ANSI_RESET;
            continue;
        }

        if(parsedCommand[0].compare("PUT")==0){

            if(parsedCommand.size() < 2){
                cout << ANSI_RED << "No file path entered\n The appropriate syntax is:\n";
                cout << "PUT path_relative_to_client_storage_directory" << ANSI_RESET << endl;
                continue;
            }

            //To make it relative to my storage directory of client:
            string filePathName = "./storage/" + parsedCommand[1];

            if(checkForExistenceOfFile(filePathName)==false){
                cout << ANSI_RED << "File Not Found\n The appropriate syntax is:\n";
                cout << "PUT path_relative_to_client_storage_directory\n" << ANSI_RESET << endl;
                continue;
            }

            myclient.sendMsg("PUT");
            cout << myclient.readMsg() << endl;
            if(parsedCommand.size()>=3 && parsedCommand[2].compare("-a")==0)
                myclient.sendFile(filePathName, "-a");
            else
                myclient.sendFile(filePathName, "-bin");

        }else if(parsedCommand[0].compare("GET")==0){

            if(parsedCommand.size() < 2){
                cout << ANSI_RED << "No file path entered\n The appropriate syntax is:\n";
                cout << "GET filename" << ANSI_RESET << endl;
                continue;
            }

            string fileName = parsedCommand[1];

            //Try to start the transfer:
            myclient.sendMsg("GET");
            cout << myclient.readMsg() << endl;
            if(parsedCommand.size()>=3 && parsedCommand[2].compare("-a")==0)
                myclient.recvFile(fileName, "-a");
            else
                myclient.recvFile(fileName, "-bin");
                
        }else if(parsedCommand[0].compare("LS")==0){
            myclient.sendMsg("LS");
            cout << myclient.readMsg() << endl;
        }else if(parsedCommand[0].compare("EXIT")==0)
            exit_handler(SIGKILL);
        else if (parsedCommand[0].compare("DEL")==0){

            if(parsedCommand.size() < 2 || parsedCommand[1].size()==0){
                cout << ANSI_RED << "No file_name entered\n The appropriate syntax is:\n";
                cout << "DEL <file_name>" << ANSI_RESET << endl;
                continue;
            }

            string fileName = parsedCommand[1];

            //Try to delete the file:

            myclient.sendMsg("DEL");
            cout << myclient.readMsg() << endl;
            myclient.deleteFile(fileName);
        }
        else{
            cout << ANSI_RED << "Command Not Found" << ANSI_RESET << endl;
        }

        
    }
    
    myclient.closenow();
    return 0;
}