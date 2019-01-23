/************************************************************
FileName: ListFilesWithMd5.cc
Description: Calcute the Md5 for the files in the specified folder
Author: Huakun.yang
Version: 1.0
Date: 2019, Jan, 22
Function:

History:
<author>        <time>       <version>       <description>
Huakun.yang    2019,Jan,22   1.0            Initial Creation
************************************************************/

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <memory>
#include <string.h>
#include <stdio.h>
#include "ListFilesWithMd5.h"
#include "md5.h"
#include "NetPackProto.h"

/* This function calcute the Md5 sum for one file*/
bool calcFileMd5(char * strFile, unsigned char * strMd5, int & fileSize)
{
    /* open the file */
    FILE *fp = fopen(strFile, "rb");
    if(fp == NULL)
    {
        return false;
    }

    struct stat st;
    if(-1 == stat(strFile, &st))
    {
        return false;
    }

    std::shared_ptr<char> data(new char[st.st_size], std::default_delete<char[]>());  
    if(data.get() == NULL)
    {
        return false;
    }    

    /*read the file into buffer*/
    int ret = fread(data.get(), 1, st.st_size, fp);
    if(ret != st.st_size)
    {
        return false;
    } 

    /*calcute its md5 sum*/
    unsigned char digest[16] = {0};
    MD5_CTX context;
    MD5Init(&context);
    MD5Update(&context, (unsigned char *)data.get(), st.st_size);
    MD5Final(&context, digest);
    fclose(fp);
    
    memcpy(strMd5, digest,16);    
    fileSize = st.st_size;
    
    return true;
}

/*This function calcute the md5 sum of the files in one folder*/
bool listFilesWithMd5s(const std::string & strFileFolder, std::queue< FILE_ELEMENT > & queueFileList)
{
    /*Initialize the directory strucure */
    dirent * dirt;
    DIR * dir;;
    std::shared_ptr<char> s[1024];
    int i=0,n=1;
    s[0]= std::shared_ptr<char>(new char[1024], std::default_delete<char[]>());
    memset(s[0].get(),0,1024);
    sprintf(s[0].get(),"%s",strFileFolder.c_str());    
    do
    {
        dir = opendir(s[i].get());        
        if(dir!=NULL)
        {
            while((dirt=readdir(dir))!=NULL)
            {
                if(strcmp(dirt->d_name,".")==0||strcmp(dirt->d_name,"..")==0)
                    continue;   //skip current and parent dir
                
                if(dirt->d_type == 8) //regular file
                {
                    /*for each directory item, if it is regular file, caluculate its Md5 sum and put to queue list*/
                    char strFileName[1024]={0};
                    /*compose the file path*/
                    snprintf(strFileName,1024,"%s/%s",s[i].get(),dirt->d_name);
                    unsigned char md5[16];
                    int fileSize = 0;
                    if(true == calcFileMd5(strFileName, md5, fileSize))
                    {
                        /*if the md5 sum is calcuated successfully*/
                        FILE_ELEMENT fileElement;
                        fileElement.strFilePath_ = strFileName;
                        memset(&fileElement.fileInfo_,0,sizeof(FILE_INFO));                        
                        strncpy(fileElement.fileInfo_.fileName_, dirt->d_name, 256);
                        memcpy(fileElement.fileInfo_.md5_, md5, 16);
                        fileElement.fileInfo_.fileSize_ = fileSize;
                        queueFileList.push(fileElement);
                    }
                }
                else
                {
                    /*if the dir entry is sub dir, add it to the array for further looking up*/
                    s[n] = std::shared_ptr<char>(new char[1024], std::default_delete<char[]>()); 
                    memset(s[n].get(),0,1024);
                    sprintf(s[n].get(),"%s/%s",s[i].get(),dirt->d_name);
                    n++;
                }

            }
        }
        i++;
    } while(i<n);

    return true;
}
