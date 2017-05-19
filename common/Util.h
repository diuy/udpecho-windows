#pragma once
#include <string>
#include <iostream>
std::string nowTimeStr();
std::string nowDateStr();
extern int StringToInt(const char* str,int defaultValue);


#define COUT(V) std::cout<<nowTimeStr()<<": "<<V<<endl
#define CERR(V) std::cerr<<nowTimeStr()<<": "<<V<<endl

