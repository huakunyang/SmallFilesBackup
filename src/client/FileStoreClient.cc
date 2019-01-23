/************************************************************
FileName: FileStoreClient.cc
Description: Transfer files to server for backup in the secified location
Author: Huakun.yang
Version: 1.0
Date: 2019, Jan, 22
Function:

History:
<author>        <time>       <version>       <description>
Huakun.yang    2019,Jan,22   1.0            Initial Creation
************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include "TransferClient.h"
#include "ConfigParser.h"


#define CONF_SERVER_IP_KEY     "server_ip"
#define CONF_SERVER_PORT_KEY   "server_port"
#define CONF_FOLDER_PATH_KEY   "folder_path"

int main(int argc, char *argv[])
{
    /*read configurations*/
    ConfigParser configParser("./client.conf");
    std::string strServerIP, strFolderPath, strServerPort;

    /*read server ip*/    
    if(false == configParser.getValue(CONF_SERVER_IP_KEY, strServerIP))
    {
        fprintf(stderr, "failed to get server ip from config file \n");
        return 1;
    } 

    /*read folder path for monitor*/
    if(false == configParser.getValue(CONF_FOLDER_PATH_KEY, strFolderPath))
    {
        fprintf(stderr, "failed to get folder Path from config file \n");
        return 1;
    } 

    /* read sever port */
    if(false == configParser.getValue(CONF_SERVER_PORT_KEY, strServerPort))
    {
        fprintf(stderr, "failed to get server port from config file \n");
        return 1;
    } 

    char * end;
    unsigned short portNum = static_cast<int>(strtol(strServerPort.c_str(),&end,10));

    /*start the client to transfer the files to folder*/
    TransferClient transferClient;
    transferClient.run(strFolderPath, strServerIP, portNum);

    return 0;
}

