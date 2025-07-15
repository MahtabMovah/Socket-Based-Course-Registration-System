#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <iostream>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
#include <vector>
using namespace std;
string username_array="";
char username_buffer[50];
#define TCP_port_main 25289 
#define IP_addr_local "127.0.0.1" //IP_addr_local address
#define MAXSIZEOFDATA 30000 
#define MAXSIZEOFDATA_res 4
char auth_result[MAXSIZEOFDATA_res];
char fin_result[MAXSIZEOFDATA];
char inputData[MAXSIZEOFDATA]; 
char Enter_username[MAXSIZEOFDATA]; 
char Enter_password[MAXSIZEOFDATA];

int sockfd_client; //socket for client
struct sockaddr_in client_addr; //address for clientA

//initialize Socket
// source Beej's guide: https://beej.us/guide/bgnet/html/
void initializeSocket() {
    sockfd_client = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd_client == -1) {
        perror("Client  Socket Failed!");
        close(sockfd_client);
        exit(1);
    }
    memset(client_addr.sin_zero, '\0', sizeof  client_addr.sin_zero);
    client_addr.sin_family = AF_INET;
    client_addr.sin_port = htons(TCP_port_main);
    client_addr.sin_addr.s_addr = inet_addr(IP_addr_local);
    if (connect(sockfd_client, (struct sockaddr*) &client_addr, sizeof(client_addr)) == -1) {
        perror("Client  Connect Failed!");
        close(sockfd_client);
        exit(1);
    }
    printf("The client is up and running.\n");

}






int main() {
    int number_of_trials=0;
    int remaining=3;
    while(number_of_trials<3){
    initializeSocket();
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if (getsockname(sockfd_client, (struct sockaddr *)&sin, &len) == -1)
    perror("getsockname");
    else
    printf("port number %d\n", ntohs(sin.sin_port));
    int simp= ntohs(sin.sin_port);
 //client gets a request to enter te username
    int Ask_for_username;
    if ((Ask_for_username = recv(sockfd_client, Enter_username, MAXSIZEOFDATA - 1, 0)) == -1) {
        perror("Received from Client  Failed!");
        close(sockfd_client);
        exit(1);
    }
    
    Enter_username[Ask_for_username] = '\0'; //receive processed result from central server
    printf(" %s",Enter_username);

 //client enters the username
    string username_array="";
    getline(cin,username_array);
    int size_username=username_array.length();
    char username_buffer[size_username];
    strcpy(username_buffer,username_array.c_str());


 //    char username_buffer[50];
 //    fgets(username_buffer,50,stdin);

 //client sends the username to the main server
    
    if (send(sockfd_client, username_buffer, strlen(username_buffer), 0) == -1) {
        perror("Client Send Username Failed!");
        close(sockfd_client);
        exit(1);
    }
 //   printf("The client sent username: %s to the Central server.\n", username_buffer);
 // client gets a request to enter password
    int Askforpass;
    if (( Askforpass = recv(sockfd_client, Enter_password, MAXSIZEOFDATA - 1, 0)) == -1) {
        perror("Username receive from client Failed!");
        close(sockfd_client);
        exit(1);
    }
    Enter_password [Askforpass] = '\0'; //receive processed result from central server
    printf(" %s",Enter_password);


 //client enters password
    string password_array="";
    getline(cin,password_array);
    int size_pass=password_array.length();
    char password_buffer[size_pass];
    strcpy(password_buffer,password_array.c_str());

 //   char password_buffer[50];
   // fgets(password_buffer,50,stdin);
 //client sends the password to the main server
    
    if (send(sockfd_client, password_buffer, strlen(password_buffer), 0) == -1) {
        perror("Client Send password Failed!");
        close(sockfd_client);
        exit(1);
    }
  //  printf("The client sent password: %s to the Central server.\n", password_buffer);
 /////authentication request is sent////

    printf("%s  sent an authentication request to the main server.\n",username_buffer);    
 ////get the auth esult from the main server
    int Auth_res_bytes;
    if ((Auth_res_bytes = recv(sockfd_client,auth_result, MAXSIZEOFDATA_res - 1, 0)) == -1) {
        perror("Received from Client Failed!");
        close(sockfd_client);
        exit(1);
    }
    
    auth_result[Auth_res_bytes] = '\0'; //receive processed result from central server

    string true_T="TT";
    string f_user="FU";
    string f_pass="FP";

    int res_comp = true_T.compare(auth_result);
    int res_user=f_user.compare(auth_result);
    int res_pass=f_pass.compare(auth_result);
    if (res_comp==0){
        printf("%s received the result of authentication using TCP over port %d.\n",username_buffer, simp);   
        cout<<"Authentication is successful"<<endl;
        //cout<<"Please enter the course code to query: "<<endl;
        number_of_trials=3;

            while(true){
       // if (number_of_trials!=3){cout<<"Please enter the course code to query: "<<endl;}
    cout<<"Please enter the course code to query: "<<endl;
    string course_name="";
    getline(cin,course_name);
    int size_course_name=course_name.length();
    char course_name_buffer[size_course_name];
    strcpy(course_name_buffer,course_name.c_str());
    if (send(sockfd_client, course_name_buffer, strlen(course_name_buffer), 0) == -1) {
        perror("Client Send course_name Failed!");
        close(sockfd_client);
        exit(1);
    //printf("The client sent course name: %s to the Central server.\n", course_name_buffer);

    }
    cout<<"Please enter the category (Credit / Professor / Days / CourseName):"<<endl;

    string q_name="";
    getline(cin,q_name);
    int size_q_name=q_name.length();
    char q_name_buffer[size_q_name];
    strcpy(q_name_buffer,q_name.c_str());
    if (send(sockfd_client, q_name_buffer, strlen(q_name_buffer), 0) == -1) {
        perror("Client Send request_name Failed!");
        close(sockfd_client);
        exit(1);
    //printf("The client sent query name: %s to the Central server.\n", q_name_buffer);

    }
   // printf("%s sent a request to the main server.\n",username_array);

    cout<< username_buffer << " sent a request to the main server"<<endl;

///////lets receive the information finally///////
    int final_res_bytes;
    if ((final_res_bytes = recv(sockfd_client,fin_result, MAXSIZEOFDATA - 1, 0)) == -1) {
        perror("Received info from Client  Failed!");
        close(sockfd_client);
        exit(1);
    }  
    fin_result[final_res_bytes] = '\0';
   // cout<<"something received"<<endl;
 //   cout<<fin_result<<endl;

    printf("The client received the response from the Main server using TCP over port %d \n", simp);
    cout<< q_name_buffer<<" of "<<course_name_buffer<< " is "<<fin_result<<endl;
   // The <category> of <course code> is <information>.


    }
    }
    else{
        printf("%s received the result of authentication using TCP over port %d.\n",username_buffer,  simp);  
        cout<<"Authentication failed"<<endl;
        remaining=remaining-1;
        if (res_user==0){ cout<<"Password does not match"<<endl;}
        if (res_pass==0){ cout<<"Username Does not exis"<<endl;}
         cout<<"Attempts remaining:"<<remaining<<endl;
        number_of_trials=number_of_trials+1;
        if (number_of_trials==3){
                 cout<<"Authentication Failed for 3 attempts. Client will shut down"<<endl;
                 close(sockfd_client);
                 


        
        }

    }

    

    



    }
      close(sockfd_client);
    return 0;
}