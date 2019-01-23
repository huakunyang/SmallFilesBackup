/************************************************************
FileName: ConfigureParser.h
Description: header for data receiving and sending
Author: Huakun.yang
Version: 1.0
Date: 2019, Jan, 22
Function:

History:
<author>        <time>       <version>       <description>
Huakun.yang    2019,Jan,22   1.0            Initial Creation
************************************************************/

#ifndef _DATA_SEND_RECV_H_
#define _DATA_SEND_RECV_H_

bool recvData(int socket, char *buf, int size);
bool sendData(int socket, char * buf, int size);

#endif
