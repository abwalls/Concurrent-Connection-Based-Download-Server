Author: Andrew Walls                                                                                                                                                
Filename: downloadServer.cpp                                                 
Language: C++, g++ (GCC) 4.4.7 20120313 (Red Hat 4.4.7-18)                   
Compilation Statement: g++ downloadServer.cpp -o server                      
Execution Command: ./server <opt port #>                                     
Purpose: Concurrent, Connection based download server    
Supported Client Messages: BYE - Terminates client connection,
                           PWD - Gets present working directory on server,
                           DIR - Gets directory contents ('ls'),
                           DOWNLOAD - Download file,
                           CD - Change directory,
                           READY - Signals client is ready to begin download


