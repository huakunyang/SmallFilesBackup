/************************************************************
FileName: DataSendRecv.cc
Description: The implementation of socket sending and receiving
for a block data
Author: Huakun.yang
Version: 1.0
Date: 2019, Jan, 22
Function:

History:
<author>        <time>       <version>       <description>
Huakun.yang    2019,Jan,22   1.0            Initial Creation
************************************************************/

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include "DataSendRecv.h"

/*check whether the error happens*/
bool isNetworkError(int recvBytes)
{
    bool connectionDone = false;

    if(recvBytes == 0)
    {
        connectionDone = true;
    }

    if((recvBytes == -1)
      &&(errno < ERANGE)
       &&(errno != EINTR)
       &&(errno != EWOULDBLOCK)
       &&(errno != EAGAIN))
    {
        connectionDone = true;
    }

    return connectionDone;
}

/*send a block of data from specified socket*/
bool sendData(int socket, char * buf, int size)
{
    int remainData = size;
    while(remainData > 0)
    {
        int dataSent = write(socket, buf + size - remainData, remainData);
        if(dataSent > 0)
        {
            remainData -= dataSent;
        }
        else
        {
            if(isNetworkError(dataSent))
            {
                return false;
            }
        }
    }
    return true;
}

/*receive a block of data from specified socket*/
bool recvData(int socket, char *buf, int size)
{
    int remainData = size;
    while(remainData > 0)
    {
        int dataRecv = read(socket, buf + size - remainData, remainData);
        if(dataRecv > 0)
        {
            remainData -= dataRecv;
        }
        else if(isNetworkError(dataRecv))
        {
            return false;
        }
    }
    return true;
}
