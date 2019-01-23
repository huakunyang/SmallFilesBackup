/************************************************************
FileName: FileStoreServer.cc
Description: The implementation of the server that back up the file
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
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>
#include <algorithm>
#include <string>
#include <errno.h>
#include <unistd.h> 
#include <fcntl.h>
#include <signal.h>
#include <iostream> 
#include <thread>
#include <fstream>
#include <mutex>
#include "md5.h"
#include "ListFilesWithMd5.h"
#include "NetPackProto.h"
#include "DataSendRecv.h"
#include "md5.h"
#include "ConfigParser.h"

/*data structur for pass info to thread*/
struct THREAD_DATA
{
    std::queue< FILE_ELEMENT> queueFileList_;
    std::mutex queueMutex_;
};

/*for each file transfering, one thread will be created, this function is the 
thread function, it takes the following steps for each file transfer request:
 - get file info from client, which contains the file name and md5 sum
 - checking its queue list , if the file name and md5 sum can be found, the 
   file already exists in the server and no need for further data transfering
 - if the file is not found in server by checking file name and md5 sum, send 
   command to client for further data transfer
 - calculate the md5 sum after receiving all file contents, if the md5 sum match the 
    one in file info of step 1, the file content is correct, send successful result 
    to client  */
void clientThread(void * localData, int dataSocket, const std::string & strFileFolder)
{
    THREAD_DATA * threadData = (THREAD_DATA *)localData;

    /*get file info firstly*/
    FILE_INFO fileInfo;
    if(false == recvData(dataSocket, (char*)&fileInfo, sizeof(fileInfo)))
    {
        fprintf(stderr, "client thread: error in recv FILE INFO\n");
        return;
    }

    /*copy fhe queue list for further looking up, mutex is required*/
    std::queue< FILE_ELEMENT > queueFileListTmp;
    {
        std::lock_guard<std::mutex> locker(threadData->queueMutex_);
        queueFileListTmp = threadData->queueFileList_;
    }

    /*look up the queue list for md5 sum and file name*/
    bool foundElement = false;
    bool fileFound = false;
    while(queueFileListTmp.size() > 0)
    {
        FILE_ELEMENT fileElement = queueFileListTmp.front();
        queueFileListTmp.pop();

        if((strncmp(fileElement.fileInfo_.fileName_, fileInfo.fileName_, 256) == 0))
        {
            /*the file with same name found*/
            fileFound = true;
            if(memcmp(fileElement.fileInfo_.md5_, fileInfo.md5_,16) == 0)
            {
                /*and md5 sum also match, mark the found flag*/
                foundElement = true;
                break;
            }
        }
    }

    if(foundElement)
    {
        /*send status to client for no need further data transfering as the file is already in server 
        with same name and md5 sum*/
        char sts = SERVER_STATUS_FILE_EXIST;
        if(false == sendData(dataSocket, &sts, 1))
        {
            fprintf(stderr, "client thread: send exist status error\n");
            close(dataSocket);
            return;            
        }
    }
    else
    {
        /*send status to client for further data transfering request*/
        char sts = SERVER_STATUS_FILE_NOT_EXIST;
        if(false == sendData(dataSocket, &sts, 1))
        {
            fprintf(stderr, "client thread: send not exist status error\n");
            close(dataSocket);
            return;            
        }

        /*receiving the file contents data*/
        std::shared_ptr<char> data(new char[fileInfo.fileSize_], std::default_delete<char[]>());
        if(false == recvData(dataSocket, data.get(), fileInfo.fileSize_))
        {
            fprintf(stderr, "client thread: recv file data error\n");
            close(dataSocket);
            return;            
        }

        /*calculate the file conents md5 sum*/
        unsigned char digest[16] = {0};
        MD5_CTX context;
        MD5Init(&context);
        MD5Update(&context, (unsigned char *)data.get(), fileInfo.fileSize_);
        MD5Final(&context, digest);

        if(memcmp(digest, fileInfo.md5_,16) != 0)
        {
            /*if the md5 sum does not match the one in file info, 
            send transfer error status to client*/
            char sts = SERVER_STATUS_FILE_WRITE_ERROR;
            if(false == sendData(dataSocket, &sts, 1))
            {
                fprintf(stderr, "client thread: send file recv status error\n");
                close(dataSocket);
                return;            
            }            
        }
        else
        {
            /*the md5 sum match, the data is successfully received, write the file to server's
             store folder*/
            bool writeFileSucc = false;

            char fileNameWrite[256] = {0};
            if(false == fileFound)
            {
                /*if server has not such file with the name, create the file with the name in file info*/
                snprintf(fileNameWrite, 256, "%s/%s",strFileFolder.c_str(), fileInfo.fileName_);
            }
            else
            {
                /*in this case, the server has already the file but its md5 sum is different, create the 
                file with "name_md5sum"*/
                snprintf(fileNameWrite, 256, "%s/%s_",strFileFolder.c_str(), fileInfo.fileName_);
                for (int i = 0; i< 16; i++)
                {   
                    char numStr[8] = {0};
                    snprintf(numStr,8, "%02x",digest[i]);
                    strncat(fileNameWrite, numStr, 256);
                }
            }

            /*create the write the file with received data*/
            FILE *fp = fopen(fileNameWrite, "wb");
            if(fp == NULL)
            {
                fprintf(stderr, "client thread: error in create file %s\n", fileNameWrite);
            }
            else
            {
                int ret = fwrite(data.get(), 1, fileInfo.fileSize_, fp);
                if(ret != fileInfo.fileSize_)
                {
                   fprintf(stderr, "client thread: error in wrrite file %s\n", fileNameWrite);
                }
                else
                {
                    /*file successfully create and write, add the item to file queue list to indicate that
                    the file is in server's storage*/
                    writeFileSucc = true;

                    FILE_ELEMENT fileElement;
                    fileElement.strFilePath_ = fileNameWrite;
                    strncpy(fileElement.fileInfo_.fileName_,fileInfo.fileName_, 256),
                    memcpy(fileElement.fileInfo_.md5_, fileInfo.md5_, 16);
                    fileElement.fileInfo_.fileSize_ = fileInfo.fileSize_;

                    std::lock_guard<std::mutex> locker(threadData->queueMutex_);
                    threadData->queueFileList_.push(fileElement);
                    fprintf(stdout,"write file :%s\n",fileElement.strFilePath_.c_str());
                }

                fclose(fp);
            }

            /*send final status to client*/
            char sts = (writeFileSucc == true) ? SERVER_STATUS_FILE_WRITE_SUCCESS: SERVER_STATUS_FILE_WRITE_ERROR;
            if(false == sendData(dataSocket, &sts, 1))
            {
                fprintf(stderr, "client thread: send file recv status error\n");
                close(dataSocket);
                return;            
            } 
        }
    }
    close(dataSocket);  
    return;
}

/*the function create the server socket, bind the port and accept the client connection*/
void serverCreate(const std::string & strFolderPath, unsigned short serverPort)
{
    THREAD_DATA threadData;
    listFilesWithMd5s(strFolderPath, threadData.queueFileList_);

    int sockfd;
    int fdTemp;

    /*create socket*/
    int sockfdServer = socket(AF_INET,SOCK_STREAM,0);  //ipv4,TCP
    if(sockfdServer == -1)
    {
        fprintf(stderr,"serverCreate:: socket error!\n");
        return;    
    }
  
    int opt = 1;
    setsockopt(sockfdServer, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));

    struct sockaddr_in s_addr_in;
    memset(&s_addr_in,0,sizeof(s_addr_in));
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);  
    s_addr_in.sin_port = htons(serverPort);        

    /*bind the port*/
    fdTemp = bind(sockfdServer,reinterpret_cast<const struct sockaddr *>(&s_addr_in),sizeof(s_addr_in));
    if(fdTemp == -1)
    {
        fprintf(stderr,"serverCreate:: bind error! errno = %d\n" , errno);
        close(sockfdServer);
        return;
    }
  
    fdTemp = listen(sockfdServer,128);
    if(fdTemp == -1)
    {
        fprintf(stderr, "serverCreate: listen error! errno = %d\n", errno);
        close(sockfdServer);
        return;
    }

    int clientLength = 0;
    struct sockaddr_in s_addr_client;
    while(1)
    {
        clientLength = sizeof(s_addr_client);
         
        /*Block here. Until server accpets a new connection. */
        sockfd = accept(sockfdServer,(struct sockaddr*)(&s_addr_client),(socklen_t *)(&clientLength));
        if(sockfd == -1)
        {
            fprintf(stderr,"serverCreate: Accept error!\n"); 
            continue;                               //ignore current socket ,continue while loop.
        }
        /* create thread for each connection */
        std::thread client(clientThread, (void *)&threadData, sockfd, strFolderPath);
        client.detach();

    }
    close(sockfdServer);
}

/*parse the configuation and create the server service*/
int main(int argc, char *argv[])
{
    ConfigParser configParser("./server.conf");
    std::string strFolderPath, strServerPort;

    if(false == configParser.getValue("folder_path", strFolderPath))
    {
        fprintf(stderr, "failed to get folder Path from config file \n");
        return 1;
    } 

    if(false == configParser.getValue("server_port", strServerPort))
    {
        fprintf(stderr, "failed to get server port from config file \n");
        return 1;
    } 

    char * end;
    unsigned short portNum = static_cast<int>(strtol(strServerPort.c_str(),&end,10));
    serverCreate(strFolderPath,portNum);

    return 0;
}

