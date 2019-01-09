/********************************************************************************/
/* Author: Andrew Walls                                                         */
/* Major: Computer Science                                                      */
/* Creation Date: October 30, 2018                                              */
/* Due Date: Decemebr 8, 2018                                                   */
/* Course: CSC328-010                                                           */
/* Professor Name: Dr. Frye                                                     */
/* Assignment: Download Server Phase 2                                          */
/* Filename: downloadServer.cpp                                                 */
/* Language: C++, g++ (GCC) 4.4.7 20120313 (Red Hat 4.4.7-18)                   */
/* Compilation Statement: g++ downloadServer.cpp -o server                      */
/* Execution Command: ./server <opt port #>                                     */
/* Purpose: Concurrent, Connection based download server                        */
/********************************************************************************/
#include<iostream>    
#include<string.h>   
#include <unistd.h> 
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/wait.h>

#define DEFAULTPORT 1333
#define MAXLEN 512

using namespace std;
 
//Function Prototypes, Documentaiton Above Implementation Below
int getMessage(int newsockfd, char* clientIP);
string GetStdoutFromCommand(string cmd) ;
int changeDir(const char* path);

/** 
 * Function name: main() 
 * Description: Control flow for the program, creates a socket and listens for incoming
 *                       connections, which are assigned a new process by fork()
 * Parameters: NA
 * Return Value: int - signal to terminate program
 */
int main(int argc,char* argv[]) {
  const char *hello = "HELLO<^>" ;//Greeting Message, All use <^> special char seq to mark end
  int port, backlog, sockfd, newsockfd, numChild, pid = 0;
  backlog = 10; //Num of clients we will keep waiting
  socklen_t size = sizeof(struct sockaddr_in);
  if (argc > 1 && atoi(argv[1]) >= 1024 && atoi(argv[1]) <= 49151)
    port = atoi(argv[1]); //Set port from command line arg
  else
    port = DEFAULTPORT; //Default Port Number
  cout<<"Using Port # "<<port<<endl;
  struct sockaddr_in server = {AF_INET, htons(port)};
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
    perror("Socket Error: ");
    return -1;
  }
  server.sin_addr.s_addr = INADDR_ANY;
  if (bind(sockfd, (struct sockaddr*) &server, sizeof(server)) == -1){
    perror("Bind Error: ");
    return -1;
  }
  if (listen(sockfd, backlog) == -1){
    perror("Listen Error: ");
    return -1;
  }
  for (;;){ //Daemon Process
    if((newsockfd = accept(sockfd, (struct sockaddr*) &server, &size)) == -1){
      perror("Accept Error: ");
      return -1;
    }
    char * clientIP = inet_ntoa(server.sin_addr);
	if ((pid = fork()) == -1){
	  perror("fork failed");
	  exit(-1);
	} 
    if (pid == 0){ //Process Request
      cout<<"\nConnected to "<<clientIP<<endl; 
      if (send(newsockfd, hello,strlen(hello),0) == -1){ //Send Hello Message
        perror("Send Error: ");
        return -1;
      }
      int acceptMessages = 0; 
      while(acceptMessages != 23){ //23 means client sent "BYE<^>" Message
        acceptMessages = getMessage(newsockfd, clientIP); //Fetch message 
      }
      close(newsockfd);
      exit(0);
    }   // end child process
      close(newsockfd); // parent doesn't need newsockfd
      numChild++;
    }   // end for
  while (numChild > 0) { // basic wait for all the children created
    if (wait(NULL) < -1) 
      perror("Error in wait: ");
    else
      numChild--;
  }  // end while
  return 0; 
}
/** 
 * Function name: getMesage() 
 * Description: Listens for and responds to individual messages from clients 
 * Parameters: int newsockfd - socket file descriptor for client 
 *                       char* clientIP - client's IP address just for logging purposes 
 * Return Value: int - 0 if successful, -1 if not
 */
int getMessage(int newsockfd, char* clientIP){
  string message, path, ls = "";
  char buffer[1024] = "";
  if(recv(newsockfd, &buffer,1024,0) > 0){ 
    cout<<"\n buffer is "<<buffer<<endl;
	for(int i = 0; i < 1024; i++){
      if(buffer[i] == '<' && i < 1022 && buffer[i+1]  == '^' && buffer[i+2] == '>')
        i = 1025; //Exit loop 
      else 
         message.push_back(buffer[i]);
     }
  }
  cout<<endl<<"Message = "<<message<<endl;
  if(message == "BYE" || message == "Bye" || message =="bye"){
    close(newsockfd); //close socket
	return 23; //Signals Graceful Termination
  }
  else if(message == "PWD" || message == "Pwd" || message =="pwd"){
    path = GetStdoutFromCommand("pwd");
    path.append("<^>");
    const char * p = path.c_str();
    if (send(newsockfd, p, strlen(p),0) == -1){
	  perror("Send Error: ");
	  return -1; //Signals send error on getMesage
	}
	return 0; //signals normal exit
  }
  else if(message == "DIR" || message == "Dir" || message =="dir"){
    ls = GetStdoutFromCommand("ls -F -l");
    ls.append("<^>");
    const char * d = ls.c_str();
    if (send(newsockfd, d, strlen(d),0) == -1){
	  perror("Send Error: ");
      return -1;
	}
    return 0; //signals normal exit
  }
  else if(message.substr(0,2) == "CD" || message.substr(0,2)  == "Cd" || message.substr(0,2)  =="cd"){
    cout<<"\n got CD message from "<<clientIP<<endl;
    string dest = "cd ";
    string res = "ERROR<^>";//Assume cd didnt work
    string dPath = message.substr(3,message.length()-3);
    dest.append(dPath);
    const char* y = dPath.c_str();
    int cdResult = changeDir(y);
    if(cdResult == 0)
      res = "SUCCESS<^>";//CD was successful
    cout<<endl<<"SENDING THIS FROM CD: "<<res<<endl;
    const char * z = res.c_str();
    if (send(newsockfd, z, strlen(z),0) == -1){
	  perror("Send Error: ");
      return -1;
	}
	return 0; //signals normal exit
  }
  else if(message.substr(0,8)  == "DOWNLOAD" || message.substr(0,8) == "Download" || message.substr(0,8) =="download"){
    cout<<"\n got DOWNLOAD message from "<<clientIP<<endl;
    string cat = "cat ";
    string resp = "";
    cat.append(message.substr(9,message.length()-9));
    string sendCat = GetStdoutFromCommand(cat);
    if (sendCat.substr(0,5) == "cat: ") {//Means there was an error finding file except in the 1 in a million chance file starts with "cat: "
      const char* f =  "File Not Found<^>";
      if (send(newsockfd, f, strlen(f),0) == -1){
        perror("Send Error: ");
        return -1;
      }	  
    }
    else{
      const char * r = "READY<^>";
      if (send(newsockfd, r, strlen(r),0) == -1){
        perror("Send Error: ");
        return -1;
      }
      while(recv(newsockfd, &buffer,1024,0) > 0){ 
        for(int i = 0; i < 1024; i++){
          if(buffer[i] == '<' && i < 1022 && buffer[i+1]  == '^' && buffer[i+2] == '>')
            i = 1025; //Exit loop 
          else 
            resp.push_back(buffer[i]);
        }
	  }
      if(resp == "READY"){ 
        sendCat.append("<^>");
        const char * c = sendCat.c_str();
        if (send(newsockfd, c, strlen(c),0) == -1){
	      perror("Send Error: ");
          return -1;
        }
        cout<<"\n File "<<message.substr(9,message.length()-9)<<" downloaded by "<<clientIP<<endl;
        return 0; //signals normal exit
     }
    }
  }
  else{
     cout<<"\nUnrecognized Message Received\n";
     return -2; //Signal unrecognized message
  }
}
/** 
 * Function name: GetStdoutFromCommand() 
 * Description: Got this funciton from the source below:
 *              https://www.jeremymorgan.com/tutorials/c-programming/how-to-capture-the-output-of-a-linux-command-in-c/                     
 * Parameters: string cmd - any unix termial command
 * Return Value: string - console outputof said command
 */
string GetStdoutFromCommand(string cmd) {
  string data;
  FILE * stream;
  const int max_buffer = 256;
  char buffer[max_buffer];
  cmd.append(" 2>&1");
  stream = popen(cmd.c_str(), "r");
  if (stream) {
    while (!feof(stream))
      if (fgets(buffer, max_buffer, stream) != NULL) data.append(buffer);
        pclose(stream);
    }
  return data;
}
/** 
 * Function name: changeDir() 
 * Description: From Dr. Frye's examples in public csc328 directory with slight modifications
 * Parameters: const char* path - what directory for server to cd to
 * Return Value: int - 0 if successful, -1 if not
 */
int changeDir(const char* path){
  char *dirname=(char *)calloc(MAXLEN, sizeof(char));    // allocate memory                                                                              
  char *currdir=(char *)calloc(MAXLEN, sizeof(char));    // allocate memory and set        
  char *errmsg=(char *)calloc(MAXLEN, sizeof(char));    // allocate memory and set                                          
  strcpy(dirname, path);
 /* get current directory and print */
 if (getcwd(currdir, MAXLEN) == NULL) {
   perror("Error getting current working directory: ");
   return -1;
 }   // end if getcwd                                                               
 if (chdir(dirname) == -1) {
   sprintf(errmsg, "Error changing to directory %s: ");
   perror(errmsg);
   return -1;
 }  // end if chdir                                                                 
 /* get current directory and print */
 if (getcwd(currdir, MAXLEN) == NULL) {
   perror("Error getting current working directory: ");
   return -1;
 }   // end if getcwd                                                               
 return 0;
}
