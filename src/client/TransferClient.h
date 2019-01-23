/************************************************************
FileName: TransferClient.h
Description: The header of transfering files to server 
for backup in the secified location
Author: Huakun.yang
Version: 1.0
Date: 2019, Jan, 22
Function:

History:
<author>        <time>       <version>       <description>
Huakun.yang    2019,Jan,22   1.0            Initial Creation
************************************************************/

#ifndef _TRANSFER_CLIENT_H_
#define _TRANSFER_CLIENT_H_

#include <string>
#include <thread>
#include <mutex>
#include "NetPackProto.h"
#include "ListFilesWithMd5.h"

class TransferClient
{

public:
    TransferClient();
    ~TransferClient();
    void run(const std::string & strFolderPath, const std::string & strServerIP, unsigned short shortServerPort);

    std::mutex queueMutex_;
    std::queue< FILE_ELEMENT > queueFileList_;    

};

#endif
