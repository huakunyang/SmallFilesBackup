/************************************************************
FileName: ConfigureParser.h
Description: header for md5 calculate
Author: Huakun.yang
Version: 1.0
Date: 2019, Jan, 22
Function:

History:
<author>        <time>       <version>       <description>
Huakun.yang    2019,Jan,22   1.0            Initial Creation
************************************************************/

#ifndef _LIST_FILES_WITH_MD5S_
#define _LIST_FILES_WITH_MD5S_

#include <string>
#include <queue>
#include <NetPackProto.h>

struct FILE_ELEMENT
{
    std::string strFilePath_;
    FILE_INFO fileInfo_;
};

bool calcFileMd5(char * strFile, unsigned char * strMd5, int & fileSize);
bool listFilesWithMd5s(const std::string & strFileFolder, std::queue< FILE_ELEMENT > & queueFileList);

#endif
