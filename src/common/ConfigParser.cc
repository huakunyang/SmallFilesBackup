/************************************************************
FileName: ConfigParser.cc
Description: The implementation of config parser
Author: Huakun.yang
Version: 1.0
Date: 2019, Jan, 22
Function:

History:
<author>        <time>       <version>       <description>
Huakun.yang    2019,Jan,22   1.0            Initial Creation
************************************************************/

#include "ConfigParser.h"
#include <fstream>
#include<iostream>

/*parse the config file, each configuration item likes follows
sever_port = 50051
*/
ConfigParser::ConfigParser(const std::string & configFilePath)
{
    std::ifstream configFile;
    configFile.open(configFilePath.c_str());
    std::string str_line;
    if (configFile.is_open())
    {
        while (!configFile.eof())
        {
            getline(configFile, str_line);
            /*bypass the comment in the configration file that begins with "#" */
            if ( str_line.find('#') == 0 ) 
            {
                continue;
            }    
            size_t pos = str_line.find('=');
            std::string str_key = str_line.substr(0, pos);
            std::string str_value = str_line.substr(pos + 1);
            /*inster the configutation item to the map for query*/
            mapConfigItem_.insert(std::pair<std::string, std::string>(str_key, str_value));
        }
    }
    else
    {    
        fprintf(stderr,"Cannot open config file %s\n",configFilePath.c_str());
        exit(-1);
    }
}

ConfigParser::~ConfigParser()
{

}

/*retrieve configuration item value by looking up the map*/
bool ConfigParser::getValue(const std::string & key, std::string & valueOut)
{
    auto iter = mapConfigItem_.find(key);
    if(iter != mapConfigItem_.end())
    {
        valueOut = iter->second;
        return true;
    }
    return false;
}

