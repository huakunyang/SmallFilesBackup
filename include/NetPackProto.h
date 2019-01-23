/************************************************************
FileName: ConfigureParser.h
Description: header for data pack definition between client and 
server, and the status definition
Author: Huakun.yang
Version: 1.0
Date: 2019, Jan, 22
Function:

History:
<author>        <time>       <version>       <description>
Huakun.yang    2019,Jan,22   1.0            Initial Creation
************************************************************/

#ifndef _NET_PACK_PROTO_
#define _NET_PACK_PROTO_

struct FILE_INFO
{
    char fileName_[256];
    char md5_[16];
    int fileSize_;
};

#define SERVER_STATUS_FILE_EXIST           0
#define SERVER_STATUS_FILE_NOT_EXIST       1
#define SERVER_STATUS_FILE_WRITE_SUCCESS   2
#define SERVER_STATUS_FILE_WRITE_ERROR     3

#endif
