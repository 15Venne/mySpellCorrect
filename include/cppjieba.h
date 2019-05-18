#ifndef __CPPJIEBA_H__
#define __CPPJIEBA_H__


#include<stdio.h>
#include <iostream>
#include "../libs/cppjieba/include/cppjieba/Jieba.hpp"
#include <string>

bool is_chinese(const std::string& str)
{  
    unsigned char utf[4] = {0};
    unsigned char unicode[3] = {0};
    bool res = false;
    for (int i = 0; i < str.length(); i++)
    { 
        if ((str[i] & 0x80) == 0) 
        {   //ascii begin with 0      
            res = false;
        } 
        else /*if ((str[i] & 0x80) == 1) */
        {
            utf[0] = str[i];
            utf[1] = str[i + 1]; 
            utf[2] = str[i + 2]; 
            i++;   
            i++;
            unicode[0] = ((utf[0] & 0x0F) << 4) | ((utf[1] & 0x3C) >>2);  
            unicode[1] = ((utf[1] & 0x03) << 6) | (utf[2] & 0x3F);
            //      printf("%x,%x\n",unicode[0],unicode[1]);
            //      printf("aaaa %x,%x,%x\n\n",utf[0],utf[1],utf[2]);
            if(unicode[0] >= 0x4e && unicode[0] <= 0x9f)
            {  
                if (unicode[0] == 0x9f && unicode[1] >0xa5)    
                    res = false; 
                else 
                    res = true; 
            }else    
                res = false; 
        } 
    } 
    return res;
}

//来源：CSDN 
//原文：https://blog.csdn.net/u010317005/article/details/77511416 
//版权声明：本文为博主原创文章，转载请附上博文链接！



void otherToSpaces(std::string &s)
{
    //std::cout << is_chinese(s) << std::endl;
    int i = 0;
    while(i < s.size())
    {
        if((unsigned short)s[i] > 128)
        {
            i += 2;
        }else{
            s[i++] = ' ';
        }
    }
}

std::string cutCword(std::string &s1)
{
    otherToSpaces(s1);
    const char* const DICT_PATH = "../libs/cppjieba/dict/jieba.dict.utf8";
    const char* const HMM_PATH = "../libs/cppjieba/dict/hmm_model.utf8";
    const char* const USER_DICT_PATH = "../libs/cppjieba/dict/user.dict.utf8";
    const char* const IDF_PATH = "../libs/cppjieba/dict/idf.utf8";
    const char* const STOP_WORD_PATH = "../libs/cppjieba/dict/stop_words.utf8";

    cppjieba::Jieba jieba(DICT_PATH,
        HMM_PATH,
        USER_DICT_PATH,
        IDF_PATH,
        STOP_WORD_PATH);
    std::vector<std::string> words;
    std::vector<cppjieba::Word> jiebawords;
    std::string s=s1;
    std::string result;

    jieba.Cut(s, words, true);
    return limonp::Join(words.begin(), words.end(), " ");
}

#endif
