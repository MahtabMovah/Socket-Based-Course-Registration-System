#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <string>
#include <fstream>
#include <algorithm>
#include <vector>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <algorithm>
#include <vector>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include<string.h>
#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
const unsigned int ArraySizeDef = 90;
using namespace std;

#define localhost "127.0.0.1" //localhost address
#define SERVERAUTH 21289 
#define MAINUDPPort 24289 
#define MAXDATARECV 51 
char enc_user[MAXDATARECV];
char enc_pass[MAXDATARECV];
struct sockaddr_in central_Address; 
socklen_t central_size; 

std::string line; 

struct sockaddr_in addr_auth; 

string result; //
int sockfd_UDP;
struct sockaddr_in UDPaddr; //address of serverT UDP


void initializeSocket() {
    sockfd_UDP = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd_UDP == -1) {
        perror("Server UDP Socket Failed!");
        exit(1);
    }
    memset(UDPaddr.sin_zero, '\0', sizeof  UDPaddr.sin_zero);
    UDPaddr.sin_family = AF_INET;
    UDPaddr.sin_port = htons(SERVERAUTH);
    UDPaddr.sin_addr.s_addr = inet_addr(localhost);
    if(::bind(sockfd_UDP, (struct sockaddr*) &UDPaddr, sizeof(UDPaddr)) == -1) {
        perror("Server UDP Bind Failed!");
        exit(1);
    }
    printf("The ServerC is up and running using UDP on port %d.\n", SERVERAUTH);
}




struct Data {
    std::string user;
    std::string ID;
};

int main() {
    //initialization

    initializeSocket();
    while (true) {
        central_size = sizeof(central_Address);
        int bytes_user;
        int bytes_pass;
        //receive input from serverC
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        memset(addr_auth.sin_zero, '\0', sizeof  addr_auth.sin_zero);
        addr_auth.sin_family = AF_INET;
        addr_auth.sin_port = htons(MAINUDPPort);
        addr_auth.sin_addr.s_addr = inet_addr(localhost);



        if ((bytes_user = recvfrom(sockfd_UDP, enc_user, sizeof(enc_user), 0, (struct sockaddr *)&central_Address, &central_size)) == -1) {
            perror("Receive From Central Server Failed!");
            exit(1);
        }
        enc_user[bytes_user] = '\0';
      //  printf("The ServerT received a request from Central to get the topology %s talash .\n", enc_user);
        if ((bytes_pass = recvfrom(sockfd_UDP, enc_pass, sizeof(enc_pass), 0, (struct sockaddr *)&central_Address, &central_size)) == -1) {
            perror("Receive From Central Server Failed!");
            exit(1);
        }
        enc_pass[bytes_pass] = '\0';
        cout<<"The ServerC received an authentication request from the Main Server. "<<endl;
    ////////////////////////read the cred.txt file//////////////////////////////////////////////
 /*   vector<vector<string> > content;
    vector<string> row;
    string line, word;

    fstream file ("cred.txt", ios::in);
    if(file.is_open())
    {
    while(getline(file, line))
    {
    row.clear();

    else

            }
////////////////////////////////////////////////////////////////
    
*/
//    cout<<"enc_pass ke encode kardam ineeee="<<enc_pass;
    string enc_passs(enc_pass);
    result="F";  

    // Define array for our needed data
    Data data[ArraySizeDef];

    // Open the file and check, if it could be opened
    std::ifstream ifs("cred.txt");
    if (ifs.is_open()) {

        unsigned int index = 0;
        // Read all lines and put result into arrays
        while ((index < ArraySizeDef) and 
            (std::getline(std::getline(ifs, data[index].user, ',') >> std::ws, data[index].ID))) {
            // Now we have read a comlete line. Goto next index
            ++index;
        }
        // Show debug output
        for (unsigned int i = 0; i < index; ++i)
          { // std::cout << "User: " << data[i].user << "\tID: " << data[i].ID<< '\n';
          //  printf("ans was= %s " , data[0].ID);
         //   printf("ans shao= %s ", data[0].ID.erase((data[0].ID.size())-1));
             }
          

  //      cout<<"sizeofpassenc: "<<enc_passs.size()<<" suzeofextracted: "<<data[0].ID.size()<<"extracted val : "<< data[0].ID;
        for (int xs=0; xs<index ;xs++){
            
        if((data[xs].ID.substr(0, data[xs].ID.size() - 1))==enc_passs)
            {//cout<<"huraaaaa pass !!"<< endl;
            result="T";
            if(data[xs].user==enc_user){
                result="TT";
             }
            }

            

        }
        
    }
    else
        {std::cout << "\n\n*** Error: Could not open source file\n\n";}



    


////////////////////////////////////////////////////////////////            




    
        if (sendto(sockfd_UDP, result.data(), result.size(), 0, (struct sockaddr *) &addr_auth, sizeof(addr_auth)) == -1) {
            perror("Send Data to server Central Failed!");
            exit(1);
        }
    cout<<" The ServerC finished sending the response to the Main Server. "<< endl;
   // cout<< result << endl;
    }
  



    // source Beej's guide: https://beej.us/guide/bgnet/html/
    close(sockfd_UDP);
    return 0;
}
