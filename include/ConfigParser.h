/************************************************************
FileName: ConfigureParser.h
Description: header for configure parser
Author: Huakun.yang
Version: 1.0
Date: 2019, Jan, 22
Function:

History:
<author>        <time>       <version>       <description>
Huakun.yang    2019,Jan,22   1.0            Initial Creation
************************************************************/

#ifndef _CONFIG_PARSER_H_
#define _CONFIG_PARSER_H_

#include <map>
#include <string>

class ConfigParser
{
public:
    ConfigParser(const std::string & configFile);
    ~ConfigParser();
    bool getValue(const std::string & key, std::string & valueOut);

private:
    std::map<std::string, std::string> mapConfigItem_;

};

#endif