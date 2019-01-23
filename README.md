# SmallFilesBackup
This program contains the client and server for small files transfer to client for backup. 

# please note that THIS PROGRAM CAN ONLY RUN ON LINUX system. 

# How to build
Just type "make" in the top folder, the Makefile contains the build information which will be read by make

# How to use it
Upon make finish building, in the bin folder, there will be the following 4 files:
- FileStoreClient: the client program
- client.conf:     the config file for client
- FileStoreServer: the server program
- server.conf:     the config file for server

put "FileStoreServer" and "server.conf" in the server in same folder, edit the "server.conf" for the following fields:
- server_port=xxx: indicate the port number on which the server will listen on connection from client
- folder_path=xxx: indicate the folder path in which the files will be put for backup from client

run server by typing "./FileStoreServer" in shell 

put "FileStoreClient" and "client.conf" in the client in same folder, edit the "client.conf" for the following fields:
- server_ip=xx.xx.xx.xx  : indicate the server ip, which runs the program FileStoreServer
- server_port=xxx        : indicate the server port, which is configured in server.conf mentioned above
- folder_path=xxx        : indicate the folder path, the client program will transfer the files in the path to server, and will monitor the file changes in the folder, for each new file created, will be transfer to server also.

run client by typing "./FileStoreClient" in shell

# Multiple Clients vs One Server
Server is designed with Multi-thread, which means that one server can work with multi-clients, if the configuration is correctly filled with ip and port

# doc
The folder contains the simple design doc

# src and include
The code for the server and client program
