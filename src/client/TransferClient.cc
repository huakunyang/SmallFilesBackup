/************************************************************
FileName: TransferClient.cc
Description: The implementation of transfering files to server 
for backup in the secified location
Author: Huakun.yang
Version: 1.0
Date: 2019, Jan, 22
Function:

History:
<author>        <time>       <version>       <description>
Huakun.yang    2019,Jan,22   1.0            Initial Creation
************************************************************/

#include <queue>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <memory.h>
#include "TransferClient.h"
#include "ListFilesWithMd5.h"
#include "NetPackProto.h"
#include "DataSendRecv.h"
#include "stdio.h"

/* connect to server with socket return*/
bool connectServer(const std::string & strServerIP, unsigned short shortServerPort, int & socketOut)
{
    /*create socket*/
    int dataSocket = socket(AF_INET,SOCK_STREAM,0);
    if(dataSocket == -1)
    {
        return false;
    }
            
    struct sockaddr_in s_addr_in;
    memset(&s_addr_in,0,sizeof(s_addr_in));

    s_addr_in.sin_addr.s_addr = inet_addr(strServerIP.c_str());      //trans char * to in_addr_t
    s_addr_in.sin_family = AF_INET;
    s_addr_in.sin_port = htons(shortServerPort);                   
    
    /*connect to server*/
    if(connect(dataSocket,(struct sockaddr *)(&s_addr_in),sizeof(s_addr_in)) == -1)
    {
        fprintf(stderr,"connect failed: %s, %d\n", strServerIP.c_str(), shortServerPort);
        return false;
    }

    /*return socketed that connected to server*/
    socketOut = dataSocket;
    return true;
}

/*send one file to server via the socket*/
bool sendFileData(int dataSocket, const std::string & strFilePath, int fileSize)
{
    FILE *fp = fopen(strFilePath.c_str(), "rb");
    if(fp == NULL)
    {
        return false;
    }

    std::shared_ptr<char> data(new char[fileSize], std::default_delete<char[]>());  
    if(data.get() == NULL)
    {
        return false;
    }    

    int ret = fread(data.get(), 1, fileSize, fp);
    if(ret != fileSize)
    {
        return false;
    }
                        
    if(false == sendData(dataSocket, data.get(), fileSize))
    {
        return false;
    }
    return true;                    
}

/** 
 * this is thread function for transfering files to server
 * - The while loop in the thread will never exit except error happens
 * - In the loop, it checks the new elements in the file list queue with the following steps taken:
 *      1) send the name and md5 sum of the file to server, server will check whether the file already exists 
 *          via the md5 sum and name
 *      2) read the feedback from server
 *      3) if the feedback from the server says that the file does not exist in server, send the file data to server,
 *          otherwise, if the file exists in server, the thread will remove the file element from queue without
 *          file data sending 
 *      4) server will calculate the md5 sum of the file data after it receives all data of the file, 
 *         if the md5 sum matches the one in the 1) steps, the server will send the good feedback to client
 *      5) with good feedback from server received, remove the file element from queue
 */
void TransferThread(void * thisObj, const std::string & strServerIP, unsigned short shortServerPort)
{
    TransferClient *obj = (TransferClient *)thisObj;
   
    while(1)
    {
        bool hasFile = true;
        FILE_ELEMENT fileElement;
        {
            /*get the top element from queue, mutex is required as the queue is shared by two threads*/
            std::lock_guard<std::mutex> locker(obj->queueMutex_);
            if(obj->queueFileList_.size() == 0)
            {
                hasFile = false;
            }
            else
            {
                fileElement = obj->queueFileList_.front();
            }
        }

        int dataSocket = -1;
        /*if there is element in the queue and connect to server successfully*/
        if((true == hasFile) && (true == connectServer(strServerIP, shortServerPort, dataSocket)))
        {
            /*send the file name and md5 sum to server*/
            if(false == sendData(dataSocket,(char *)(&fileElement.fileInfo_), sizeof(FILE_INFO)))
            {
                close(dataSocket);
                continue;
            }
            
            /*receive the feedback from server on whether the server has already hold the file*/
            char infoRecv = -1;
            if(false == recvData(dataSocket, &infoRecv, 1))
            {
                close(dataSocket);
                continue;
            }
            
            /*if the file is not in server, do the data sending for the file*/
            if(SERVER_STATUS_FILE_NOT_EXIST == infoRecv)
            {
                /*send the file contents*/
                if(false == sendFileData(dataSocket, fileElement.strFilePath_, fileElement.fileInfo_.fileSize_))
                {
                    close(dataSocket);
                    continue;
                }
    
                /*read the feedback from server on whether the file contents has correctly sent to server*/
                infoRecv = -1;        
                if(false == recvData(dataSocket, &infoRecv, 1))
                {
                    close(dataSocket);
                    continue;
                }

                /*if the data sending is not correct, do not remove the element from the queue wth "continue",
                then the file element will get re-try in next time of the loop */
                if(SERVER_STATUS_FILE_WRITE_SUCCESS != infoRecv)
                {
                    close(dataSocket);
                    continue;
                }
            }
        
            /*remove the file element from queue list, mutex is required*/
            std::lock_guard<std::mutex> locker(obj->queueMutex_);
            obj->queueFileList_.pop();
     
            close(dataSocket);
            fprintf(stdout,"send file :%s to server\n",fileElement.fileInfo_.fileName_);
        }
        usleep(1000);
    }
} 

/**
 * This thread will monitor the file changes in the folder, if new file is created or copied to the folder,
 * it will be added to the file list queue for sending to server
*/
void FileCheckThread(void * classObj, const std::string & strFolderPath)
{
    TransferClient * transObj = (TransferClient *) classObj;

    /*using Linux inotify mechanism for monitor one folder*/
    int fd = inotify_init();
    if( fd < 0 )
    {
        fprintf(stderr, "inotify_init failed\n");
        return;
    }

    int wd = inotify_add_watch(fd, strFolderPath.c_str(),  IN_CLOSE_WRITE);
    if(wd < 0)
    {
        fprintf(stderr, "inotify_add_watch %s failed\n", strFolderPath.c_str());
        return ;
    }

    char buf[1024*10];
    memset(buf, 0, 1024*10);
    
    int len = 0;
    int nread = 0;
    struct inotify_event *event;
    while( (len = read(fd, buf, sizeof(buf) - 1)) > 0 )
    {
        nread = 0;
        while( len > 0 )
        {
            event = (struct inotify_event *)&buf[nread];
            if(event->mask & IN_CLOSE_WRITE)
            {
                /*new file is created with write finished*/
                if(event->len > 0)
                {
                    char filePath[1024] = {0};
                    sprintf(filePath, "%s/%s", strFolderPath.c_str(), event->name);
                    
                    unsigned char md5[16]={0};
                    int fileSize = 0;
                    /*add the file to list with file name and md5 sum*/
                    if(true == calcFileMd5(filePath, md5, fileSize))
                    {
                        FILE_ELEMENT fileElement;
                        fileElement.strFilePath_ = filePath;
                        
                        memset(&fileElement.fileInfo_, 0, sizeof(FILE_INFO));

                        strncpy(fileElement.fileInfo_.fileName_, event->name, 256);
                        memcpy(fileElement.fileInfo_.md5_, md5,16);
        
                        fileElement.fileInfo_.fileSize_ = fileSize;
                        
                        /*mutex is required as the queue is shared by two threads*/
                        std::lock_guard<std::mutex> locker(transObj->queueMutex_);
                        transObj->queueFileList_.push(fileElement);                    
                    }
                }
            }
            nread = nread + sizeof(struct inotify_event) + event->len;
            len = len - sizeof(struct inotify_event) - event->len;
        }
    }

}

TransferClient::TransferClient()
{

}

TransferClient::~TransferClient()
{

}

void TransferClient::run(const std::string & strFolderPath, const std::string & strServerIP, unsigned short shortServerPort)
{
    /*init the file queue list with existing files and md5 sums*/
    listFilesWithMd5s(strFolderPath, queueFileList_);
    
    /*create the two threads*/
    std::thread threadFileTransfer(TransferThread,(void *)this, strServerIP, shortServerPort);
    std::thread threadFileCheck(FileCheckThread, (void *)this, strFolderPath);    

    threadFileCheck.join();
    threadFileTransfer.join();
}
