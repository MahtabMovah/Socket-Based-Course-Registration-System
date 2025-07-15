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
#include <vector>
#include <iostream>
#include <cmath>
#include <limits>
#include <algorithm>
using namespace std;

#define local_IP "127.0.0.1" 
#define UDP_server_c 21289 //UDP of server C
#define UDP_server_CS 22289 //UDP of server CS
#define UDP_server_EE 23289 //UDP of server EE
#define UDP_MAIN 24289 //UDP of serverMain
#define TCP_port_client 25289
#define MAXSIZEDATAS 50 
#define BUFFMAXSIZE 30000 
#define MAZXBYUSENAME 30000 


int socckfd_client  ; 
struct sockaddr_in addr_of_client; 
int sockfd_UDP_main; 
struct sockaddr_in addr_of_UDP_main; 
int socket_child_client ; 
struct sockaddr_in addr_child_client; 
struct sockaddr_in addr_of_EE; 
struct sockaddr_in addr_of_cs; 
struct sockaddr_in addr_of_c; 
char Username_buffer[MAXSIZEDATAS];
char coursecode_buffer[MAXSIZEDATAS];

char ee_q_res[BUFFMAXSIZE];
char cs_q_res[BUFFMAXSIZE];

struct sockaddr_in serverc_Address; 
struct sockaddr_in servercs_Address; 
struct sockaddr_in serverEE_Address; 
socklen_t serverc_size; 
socklen_t serverEE_Address_size; 
socklen_t servercs_Address_size;
socklen_t servercs_size; //
socklen_t serveree_size; //

//initialize Socket for clientA
// source Beej's guide: https://beej.us/guide/bgnet/html/
void initializeSocket_client() {
    socckfd_client  = socket(PF_INET, SOCK_STREAM, 0);
    if (socckfd_client  == -1) {
        perror("Client  Socket Failed!");
        exit(1);
    }
    memset(addr_of_client.sin_zero, '\0', sizeof  addr_of_client.sin_zero);
    addr_of_client.sin_family = AF_INET;
    addr_of_client.sin_port = htons(TCP_port_client);
    addr_of_client.sin_addr.s_addr = inet_addr(local_IP);
    if (::bind(socckfd_client , (struct sockaddr*) &addr_of_client, sizeof(addr_of_client)) == -1) {
        perror("Client  Bind Failed!");
        exit(1);
    }
    if (listen(socckfd_client , 5) == -1) {
        perror("Client  Listen Failed!");
        exit(1);
    }
}

void initializeSocket_UDP() {
    sockfd_UDP_main = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockfd_UDP_main == -1) {
        perror("UDP Socket Failed!");
        exit(1);
    }
    memset(addr_of_UDP_main.sin_zero, '\0', sizeof  addr_of_UDP_main.sin_zero);
    addr_of_UDP_main.sin_family = AF_INET;
    addr_of_UDP_main.sin_port = htons(UDP_MAIN);
    addr_of_UDP_main.sin_addr.s_addr = inet_addr(local_IP);
    if(::bind(sockfd_UDP_main, (struct sockaddr*) &addr_of_UDP_main, sizeof(addr_of_UDP_main)) == -1) {
        perror("UDP Bind Failed!");
        exit(1);
    }
}

void setUpAddresses() {
    memset(addr_of_EE.sin_zero, '\0', sizeof  addr_of_EE.sin_zero);
    addr_of_EE.sin_family = AF_INET;
    addr_of_EE.sin_port = htons(UDP_server_EE);
    addr_of_EE.sin_addr.s_addr = inet_addr(local_IP);
    memset(addr_of_cs.sin_zero, '\0', sizeof  addr_of_cs.sin_zero);
    addr_of_cs.sin_family = AF_INET;
    addr_of_cs.sin_port = htons(UDP_server_CS);
    addr_of_cs.sin_addr.s_addr = inet_addr(local_IP);
    memset(addr_of_c.sin_zero, '\0', sizeof  addr_of_c.sin_zero);
    addr_of_c.sin_family = AF_INET;
    addr_of_c.sin_port = htons(UDP_server_c);
    addr_of_c.sin_addr.s_addr = inet_addr(local_IP);
}





int main() {
    //initialization
    initializeSocket_client();
    initializeSocket_UDP();
    printf("The main server is up and running.\n");
    setUpAddresses();
    int trial=0;
    while (trial<3) {
        socklen_t  clientA_TCP_len = sizeof(addr_child_client);

        socket_child_client  = accept(socckfd_client , (struct sockaddr*) &addr_child_client, &clientA_TCP_len);
        if (socket_child_client  == -1) {
            perror("Accept Client Failed!");
            exit(1);
        }
        char AskForUsername[50];
        bzero(AskForUsername,50);
        strcpy(AskForUsername,"Please enter the username: ");
        if (send(socket_child_client , AskForUsername,strlen(AskForUsername), 0) == -1) {
            perror("send to client failed Failed!");
            exit(1);
        }
        
        int username;
        //recv username from client
        if ((username = recv(socket_child_client , Username_buffer, MAXSIZEDATAS - 1, 0)) == -1) {
            perror("Received from Client Failed!");
            exit(1);
        }

        Username_buffer[username] = '\0';
        printf("The main server received the authentication for %s using TCP over port %d.\n", Username_buffer, TCP_port_client);
       // printf("usernamesize is %d",username);
       // string(sizeof(Username_buffer),Username_buffer);
 
    // username encripted
    string string_username(Username_buffer);
    int ind;
    int v;
    int i;
    char encript_user[string_username.length()]={};
    for (i=0;i<string_username.length();i++){
    int v= string_username[i]+0;
    if (v<119 && v>96){
        encript_user[i]=string_username[i]+4;
        }
        else if (v<123 && v>118){

            encript_user[i]=string_username[i]-22;
        }
        else if (v>64 && v<87){
            encript_user[i]=string_username[i]+4;
            }
        else if (v>86 && v<91 ){encript_user[i]=string_username[i]-22;}
        else if (v<54 && v>47){encript_user[i]=string_username[i]+4;}
        else if (v<58 && v>53){encript_user[i]=string_username[i]-6;}
        else{encript_user[i]=string_username[i]+0;}


   }
   encript_user[string_username.length()] = '\0';
 //   printf("this is the encript_user: %s",encript_user);
      
//      Send a password request message
        char AskForPass[100];
        bzero(AskForPass,100);
        strcpy(AskForPass,"Please enter the password: ");
        if (send(socket_child_client , AskForPass,strlen(AskForPass), 0) == -1) {
            perror("send to client failed Failed!");
            exit(1);
        }
//      receive password from client
        char password_buffer[50];
        int password_of_client;
        if ((password_of_client = recv(socket_child_client , password_buffer, MAXSIZEDATAS- 1, 0)) == -1) {
            perror("Received from Client Failed!");
            exit(1);
        }
        password_buffer[password_of_client] = '\0';
       // printf("The main server received password=\"%s\" from the client using TCP over port %d.\n", password_buffer, TCP_port_client);    
//      encripte the password:
    string string_pass(password_buffer);
   //     cout<<"this is the password I received"<<endl;
   // cout<<password_buffer<<endl;
  //  int ind;
    int vv;
    int ii;
    char encript_pass[string_pass.length()]={};
    for (ii=0;ii<string_pass.length();ii++){
    int vv= string_pass[ii]+0;
    if (vv<119 && vv>96){
        encript_pass[ii]=string_pass[ii]+4;
        }
        else if (vv<123 && vv>118){

            encript_pass[ii]=string_pass[ii]-22;
        }
        else if (vv>64 && vv<87){
            encript_pass[ii]=string_pass[ii]+4;
            }
        else if (vv>86 && vv<91 ){encript_pass[ii]=string_pass[ii]-22;}
        else if (vv<54 && vv>47){encript_pass[ii]=string_pass[ii]+4;}
        else if (vv<58 && vv>53){encript_pass[ii]=string_pass[ii]-6;}
        else{encript_pass[ii]=string_pass[ii]+0;}


   }
   encript_pass[string_pass.length()] = '\0';
 //   printf("this is the encript_user: %s",encript_user);
//    printf("this is the encript_password: %s",encript_pass);
//send encripted username & password to server C
    if (sendto(sockfd_UDP_main, encript_user, sizeof( encript_user) , 0, (struct sockaddr *) &addr_of_c, sizeof(addr_of_c)) == -1) {
        perror("Send Data to server T Failed!");
        exit(1);
    }
  //  printf("I send auth_user as :  %s" , encript_user);

    if (sendto(sockfd_UDP_main, encript_pass, sizeof( encript_pass) , 0, (struct sockaddr *)&addr_of_c , sizeof(addr_of_c)) == -1) {
        perror("Send Data to server T Failed!");
        exit(1);
    
    }
    cout<<"The main server sent an authentication request to serverC"<<endl;
    //printf("I send auth_user as :  %s" , encript_pass);
    serverc_size = sizeof( serverc_Address);
    int bytes_res;
    char auth_res[MAXSIZEDATAS];
    if ((bytes_res = recvfrom(sockfd_UDP_main, auth_res, sizeof(auth_res), 0, (struct sockaddr *)& serverc_Address, &serverc_size)) == -1) {
            perror("Receive From Server T Failed!");
            exit(1);
        }
        auth_res[bytes_res] = '\0';
    printf("The main server received the result of the authentication request from ServerC using UDP over port %d .\n", UDP_server_c); 
    string true_T="TT";
    string true_user="T";
    string false_user="F";
  //  cout<<auth_res<<endl ; 
  //  cout<<true_T<<endl ;   
    int res_comp = true_T.compare(auth_res);
    int res_comp_user = true_user.compare(auth_res);
    int res_comp_pass = false_user.compare(auth_res);
   // cout<<res_comp<<endl;
    char auth_res_client[1];
//    string auth_res_client ="F";

    if (res_comp==0){
        trial=4;
//    cout<<"eee!!! shod!" <<endl;
 //   string auth_res_client ="F";

    strcpy(auth_res_client,"TT");
///-----send the authentication result to client)
    if (send(socket_child_client , auth_res_client,strlen(auth_res_client), 0) == -1) {
        perror("send to client failed Failed!");
        exit(1);
    }
     cout<<"The main server sent the authentication result to the client"<<endl;
   
    }
    else{ cout<<"The main server sent the authentication result to the client"<<endl;
    if (res_comp_user==0){ 
        // cout<<"pass ghalate" <<endl;   
        strcpy(auth_res_client,"FP");
        if (send(socket_child_client , auth_res_client,strlen(auth_res_client), 0) == -1) {
        perror("send to client failed Failed!");
        exit(1);

    }}
        if (res_comp_pass==0){ 
        // cout<<"user ghalate" <<endl;   
        strcpy(auth_res_client,"FU");
        if (send(socket_child_client , auth_res_client,strlen(auth_res_client), 0) == -1) {
        perror("send to client failed Failed!");
        exit(1);

    }}

    
    trial=trial+1;

    }
    }
///////////////////////////////get the course code from the clint///////////////////////////////////////////
    while(true){
    int coursecode_bytes;
    char coursecode_buffer[MAXSIZEDATAS];
    if (( coursecode_bytes= recv(socket_child_client ,coursecode_buffer , MAXSIZEDATAS - 1, 0)) == -1) {
        perror("Received from Client course code Failed!");
        exit(1);
    }
    coursecode_buffer[coursecode_bytes] = '\0';
 //   printf("The main server received username=\"%s\" from the client using TCP over port %d.\n",coursecode_buffer, TCP_port_client);

///////////////////////////////get the query from the client////////////////////////////////////////////////
    int qcode_bytes;
    char qcode_buffer[MAXSIZEDATAS];
    if (( qcode_bytes= recv(socket_child_client ,qcode_buffer , MAXSIZEDATAS - 1, 0)) == -1) {
        perror("Received from Client query code Failed!");
        exit(1);
    }
    qcode_buffer[qcode_bytes] = '\0';
  //  printf("The main server received username=\"%s\" from the client using TCP over port %d.\n",qcode_buffer, TCP_port_client);

    printf("The main server received from %s to query course %s about %s using TCP over port %d .\n",Username_buffer, coursecode_buffer,qcode_buffer,TCP_port_client );



    string str_course(coursecode_buffer);
    char elec[3]="EE";
    char css[3]="CS";
    if (elec == str_course.substr(0, 2)) {
         cout<<"The main server sent a request to serverEE"<< endl;
        if (sendto(sockfd_UDP_main, coursecode_buffer, sizeof(coursecode_buffer), 0, (struct sockaddr *) &addr_of_EE, sizeof(addr_of_EE)) == -1) {
        perror("Send codename Failed!");
        exit(1);
        }
        if (sendto(sockfd_UDP_main, qcode_buffer, sizeof(qcode_buffer), 0, (struct sockaddr *) &addr_of_EE, sizeof(addr_of_EE)) == -1) {
        perror("Send codename Failed!");
        exit(1);
        }

        int servereebytes;
        serverEE_Address_size=sizeof(serverEE_Address);
        if ((servereebytes = recvfrom(sockfd_UDP_main, ee_q_res, sizeof(ee_q_res), 0, (struct sockaddr *)&serverEE_Address, &serverEE_Address_size)) == -1) {
        perror("Receive From Server T Failed!");
        exit(1);}
        ee_q_res[servereebytes] = '\0';       

        /////send final result to the client///////
        if (send(socket_child_client , ee_q_res,strlen(ee_q_res), 0) == -1) {
            perror("send to client failed Failed!");
            exit(1);
        }

     }
    if (css== str_course.substr(0, 2))   { cout<<"The main server sent a request to serverCS"<< endl;
        if (sendto(sockfd_UDP_main, coursecode_buffer,sizeof(coursecode_buffer), 0, (struct sockaddr *) &addr_of_cs, sizeof(addr_of_cs)) == -1) {
        perror("Send codename Failed!");
        exit(1);
        }
        if (sendto(sockfd_UDP_main, qcode_buffer, sizeof(qcode_buffer), 0, (struct sockaddr *) &addr_of_cs, sizeof(addr_of_cs)) == -1) {
        perror("Send codename Failed!");
        exit(1);}
        cout<<"sent them to csser";
        int servercsbytes;
        servercs_Address_size=sizeof(servercs_Address);
        if ((servercsbytes = recvfrom(sockfd_UDP_main, cs_q_res, sizeof(cs_q_res), 0, (struct sockaddr *)&servercs_Address, &servercs_Address_size)) == -1) {
        perror("Receive From Server T Failed!");
        exit(1);}
        printf("The main server received the response from serverCS using UDP over port %d .\n",UDP_server_CS);
        cs_q_res[servercsbytes] = '\0';   
            

        /////send final result to the client///////
        if (send(socket_child_client , cs_q_res,strlen(cs_q_res), 0) == -1) {
            perror("send to client failed Failed!");
            exit(1);
        }
       
       
     
     }
        if (css != str_course.substr(0, 2) && elec != str_course.substr(0, 2))  { 
        if (sendto(sockfd_UDP_main, coursecode_buffer, sizeof(coursecode_buffer), 0, (struct sockaddr *) &addr_of_EE, sizeof(addr_of_EE)) == -1) {
        perror("Send codename Failed!");
        exit(1);
        }
        if (sendto(sockfd_UDP_main, qcode_buffer, sizeof(qcode_buffer), 0, (struct sockaddr *) &addr_of_EE, sizeof(addr_of_EE)) == -1) {
        perror("Send codename Failed!");
        exit(1);
        }

        int servereebytes;
        serverEE_Address_size=sizeof(serverEE_Address);
        if ((servereebytes = recvfrom(sockfd_UDP_main, ee_q_res, sizeof(ee_q_res), 0, (struct sockaddr *)&serverEE_Address, &serverEE_Address_size)) == -1) {
        perror("Receive From Server T Failed!");
        exit(1);}
        ee_q_res[servereebytes] = '\0';  
        printf("The main server received the response from serverEE using UDP over port %d .\n",UDP_server_EE);     

        /////send final result to the client///////
        cout<< "The main server sent the query information to the client."<<endl;
        if (send(socket_child_client , ee_q_res,strlen(ee_q_res), 0) == -1) {
            perror("send to client failed Failed!");
            exit(1);
        }
       

     }
     
    





    }
    

    //close sockets
    close(sockfd_UDP_main);
    close(socket_child_client );
    return 0;
};

