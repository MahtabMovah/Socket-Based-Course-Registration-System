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
#include <set>
#include <algorithm>
#include <vector>
#include <limits>
#include <queue>
#include <cmath>
#include <iostream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include<vector>
#include <list>
using namespace std;

#define localhost "127.0.0.1" //localhost address
#define serCSudp 22289 
#define MainUdpPort 24289 
#define SizeOfDATA 40000 
int sockfd_UDP; 
struct sockaddr_in UDPaddr;
struct sockaddr_in central_Address;
socklen_t central_size;
char EE_code[SizeOfDATA]; 
char CS_q[SizeOfDATA];

char INPbuff[SizeOfDATA]; 
char INPbuffs[SizeOfDATA];


struct sockaddr_in address_C;



void initializeSocket() {
    sockfd_UDP = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd_UDP == -1) {
        perror("Server P UDP Socket Failed!");
        exit(1);
    }
    memset(UDPaddr.sin_zero, '\0', sizeof  UDPaddr.sin_zero);
    UDPaddr.sin_family = AF_INET;
    UDPaddr.sin_port = htons(serCSudp);
    UDPaddr.sin_addr.s_addr = inet_addr(localhost);
    if(::bind(sockfd_UDP, (struct sockaddr*) &UDPaddr, sizeof(UDPaddr)) == -1) {
        perror("Server P UDP Bind Failed!");
        exit(1);
    }
    printf("The ServerCS is up and running using UDP on port %d.\n", serCSudp);
}



int main() {
    //initialize socket
    initializeSocket();
    while (true) {
        central_size = sizeof(central_Address);
        ifstream myfile("cs.txt");
        if(!myfile.is_open()){
        cout<<"failed"<<endl;
        return 0;
        }
        string code,cred,prof,date,location;
        string myStr;
        string line;
        std::vector<string> ccodes,credits,profess,days,namec;
        while(getline(myfile,line)){
        stringstream ss(line);
        getline(ss,code,',');
        getline(ss,cred,',');
        getline(ss,prof,',');
        getline(ss,date,',');
        getline(ss,location,',');
        ccodes.push_back(code);
        credits.push_back(cred);
        days.push_back(date);
        namec.push_back(location);
        profess.push_back(prof);

        }


        //get clientA input from central
        // source Beej's guide: https://beej.us/guide/bgnet/html/
        int bytesReceived;
        if ((bytesReceived = recvfrom(sockfd_UDP, EE_code, sizeof(EE_code), 0, (struct sockaddr *)&central_Address, &central_size)) == -1) {
            perror("Receive From Central Server Failed!");
            exit(1);
        }
      //  EE_code[bytesReceived] = '\0';

     //   printf("code reveived: %s",EE_code);
        // cout<<EE_code<<endl;
        int bytesReceivedq;
        if ((bytesReceivedq = recvfrom(sockfd_UDP, CS_q, sizeof(CS_q), 0, (struct sockaddr *)&central_Address, &central_size)) == -1) {
            perror("Receive From Central Server Failed!");
            exit(1);
        }
        CS_q[bytesReceivedq] = '\0';
        printf("The ServerCS received a request from the Main Server about the %s of %s.", CS_q,EE_code);
        string CS_qq(CS_q);
     //   printf("code reveived: %s",CS_q);
     //   cout<<CS_q<<endl;
        char res_code='A';
        memset(address_C.sin_zero, '\0', sizeof  address_C.sin_zero);
        address_C.sin_family = AF_INET;
        address_C.sin_port = htons(MainUdpPort);
        address_C.sin_addr.s_addr = inet_addr(localhost);
        char Credits[7]="Credit";
        char Professor[10]="Professor";
        char Days[5]="Days";
        char CourseName[11]="CourseName";
        string quer_point="F";
        string quer_disc="Didn’t find the course: ";
        string course_point="F";
        string course_disc="Didn’t find the course: ";

 

        for (int ix=0;ix<ccodes.size();ix++){ 
            if(ccodes[ix]==EE_code){int row=ix;course_point="X";

            if (CS_qq==Credits ){quer_point="X";
                cout<<"The course information has been found: The Credits of "<< EE_code<<" is "<<credits[row]<<endl;
                if (sendto(sockfd_UDP,credits[row].data(), credits[row].size(), 0, (struct sockaddr *) &address_C, sizeof(address_C)) == -1) {
                perror("Send Data to server Central Failed!");
                exit(1);
            }
          //  printf("The course information has been found: The <category> of <course code> is <information>.")
        }
        else if (CS_qq==Professor){quer_point="X";
             cout<<"The course information has been found: The Professor of "<< EE_code<<" is "<<profess[row]<<endl;
            if (sendto(sockfd_UDP,profess[row].data(), profess[row].size(), 0, (struct sockaddr *) &address_C, sizeof(address_C)) == -1) {
                perror("Send Data to server Central Failed!");
                exit(1);}
            
        }
        else if (CS_qq==Days){quer_point="X";
             cout<<"The course information has been found: The Days of "<< EE_code<<" is "<<Days[row]<<endl;
            if (sendto(sockfd_UDP,days[row].data(), days[row].size(), 0, (struct sockaddr *) &address_C, sizeof(address_C)) == -1) {
                perror("Send Data to server Central Failed!");
                exit(1);}
        }
        else if (CS_qq==CourseName){quer_point="X";
             cout<<"The course information has been found: The CourseNme of "<< EE_code<<" is "<<namec[row]<<endl;
            if (sendto(sockfd_UDP,namec[row].data(), namec[row].size(), 0, (struct sockaddr *) &address_C, sizeof(address_C)) == -1) {
                perror("Send Data to server Central Failed!");
                exit(1);}
        }
            
             }

         }
         cout<<"The ServerCS finished sending the response to the Main Server."<<endl;
        if (quer_point=="F"){   
            if (sendto(sockfd_UDP,quer_disc.data(), quer_disc.size(), 0, (struct sockaddr *) &address_C, sizeof(address_C)) == -1) {
                perror("Send Data to server Central Failed!");
                exit(1);}
        }
                if (course_point=="F"){ printf("Didn’t find the course: %s .\n.",EE_code);  
            if (sendto(sockfd_UDP,course_disc.data(), course_disc.size(), 0, (struct sockaddr *) &address_C, sizeof(address_C)) == -1) {
                perror("Send Data to server Central Failed!");
                exit(1);}
        }




    // Open the file and check, if it could be opened

    }

    close(sockfd_UDP);
    return 0;
}