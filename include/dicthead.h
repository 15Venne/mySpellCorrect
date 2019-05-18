
#ifndef __DICTHEAD_H__
#define __DICTHEAD_H__

#include"mylog.h"
#include"cppjieba.h"

#include<string.h>
#include<unistd.h>
#include<stdio.h>
#include<errno.h>
#include<sys/types.h>
#include<dirent.h>
#include<pthread.h>
#include<stdlib.h>

#include<iostream>
#include<vector>
#include<map>
#include<string>
#include<memory>
#include<set>
#include<fstream>
#include<sstream>
#include<unordered_map>
#include<algorithm>
#include<queue>


using std::cout;
using std::endl;
using std::map;
using std::vector;
using std::string;
using std::shared_ptr;
using std::set;
using std::pair;
using std::unordered_map;
using std::cin;
using std::ofstream;
using std::ifstream;


//---------------------------------------------------------------------------------------
//conf
class Configuration
{
public:
    static Configuration *getInstance()
    {
        if(!_pInstance)
        {
            _pInstance = new Configuration();
        }
        return _pInstance;
    }
    vector<string> &getEngFileList()
    {
        return _engfileList;
    }
    vector<string> &getChnFileList()
    {
        return _chnfileList;
    }
    string ip()
    {
        return _ip;
    }
    unsigned short port()
    {
        return atoi(_port.c_str());
    }
    string ditcPath()
    {
        return _mydict;
    }
    string indexPath()
    {
        return _index;
    }
private:
    Configuration();//1
    ~Configuration(){}
    static Configuration *_pInstance;
private:
    string _ip;
    string _port;
    string _mydict;
    string _index;
    string _engdict;
    string _chndict;
    vector<string> _engfileList;
    vector<string> _chnfileList;
};//

Configuration::Configuration()
{
    std::ifstream is;
    is.open("../conf/my.conf");
    if(!is.good())
    {
        LogError("open file failed");
        return;
    }
    string line, key, value;
    while(getline(is, line))
    {
        std::istringstream iss(line);
        while(iss >> key, iss >> value)
        {
            switch(atoi(key.c_str()))
            {
            case 1:
                _ip = value; break;
            case 2:
                _port = value; break;
            case 3:
                _mydict = value; break;
            case 4:
                _index = value; break;
            case 5:
                _engdict = value; break;
            case 6:
                _chndict = value; break;
                
            default: break;
            }
        }
    }
    struct dirent *p;
    DIR *dir1 = opendir(_engdict.c_str());
    if(dir1 == NULL)
    {
        LogError("opendir engdict failed");
    }
    while((p = readdir(dir1)) != NULL)
    {
        if('.' == p->d_name[0])
        {
            continue;
        }
        string tmpname(_engdict);
        tmpname.append(1,'/');
        tmpname.append(string(p->d_name));
        _engfileList.push_back(tmpname);
        cout << tmpname << endl;
    }
    closedir(dir1);
    DIR *dir2 = opendir(_chndict.c_str());
    if(dir2 == NULL)
    {
        LogError("opendir chndict failed");
    }
    while((p = readdir(dir2)) != NULL)
    {
        if('.' == p->d_name[0])
        {
            continue;
        }
        string tmpname(_chndict);
        tmpname.append(1,'/');
        tmpname.append(string(p->d_name));
        _chnfileList.push_back(tmpname);
        cout << tmpname << endl;
    }
    closedir(dir2);
    //cout << _ip << endl;
    //cout << _port << endl;
    //cout << _mydict << endl;
    //cout << _index << endl;
}

Configuration *Configuration::_pInstance = Configuration::getInstance();
//----------------------------------------------------------
//Dictionary字典
class Dictionary
{
    friend class Task;
public:
    Dictionary(Configuration *);//1

    int wordCnt()
    {
        return _wordCnt;
    }
    void reSetDictAndIndex()
    {
        _dic.clear();
        _dict.clear();
        _indexTable.clear();
        readConf(_p);
        makeDict();
        makeIndexTable();
        writeDictAndIndexInFile();
    }
private:
    void readConf(Configuration *);//2
    void readEngFile(string &);//3
    void readChnFile(string &);//4
    void makeDict();//5
    void makeIndexTable();//6
    void stringDeal(string &);//7
    void chnStringDeal(string &);//8
    void writeDictAndIndexInFile();//9
    void readDictAndIndexDat();//10
private:
    map<string, int> _dic; //统计
    int _wordCnt;
    vector<pair<string, int>> _dict;//字典
    unordered_map<string, set<int>> _indexTable;//索引表
    Configuration *_p;
};//

Dictionary::Dictionary(Configuration *p)
: _dic()
, _wordCnt(0)
, _p(p)
{
    readDictAndIndexDat();
    //readConf(p);
    //makeDict();
    //makeIndexTable();
}//1

void Dictionary::readConf(Configuration *p) 
{
    for(auto &filename : p->getEngFileList())
    {
        readEngFile(filename);
        //cout << filename << endl;
    }
    for(auto & f : p->getChnFileList())
    {
        readChnFile(f);
    }
}//2

void Dictionary::readEngFile(string &filename) 
{
    std::ifstream is;
    is.open(filename);
    if(!is.good())
    {
        LogError("open file failed");
        return;
    }
    string text;
    while(getline(is, text))
    {
        stringDeal(text);
        string word;
        std::istringstream line(text);
        while(line >> word)
        {
            _dic[word]++;
            _wordCnt++;
            //cout << word << ": " << _dic[word] << endl;
        }
    }
}//3

void Dictionary::readChnFile(string &filename)
{
    std::ifstream is;
    is.open(filename);
    if(!is.good())
    {
        LogError("open file failed");
        return;
    }
    string text;
    while(getline(is, text))
    {
        chnStringDeal(text);
        string word;
        std::istringstream line(text);
        while(line >> word)
        {
            if(!is_chinese(word))
            {
                continue;
            }
            _dic[word]++;
            _wordCnt++;
            cout << word << ": " << _dic[word] << endl;
        }
    }

}//4

void Dictionary::makeDict()
{
    for(auto &e : _dic)
    {
        _dict.push_back(make_pair(e.first, e.second));
    }
}//5

void Dictionary::makeIndexTable()
{
    int i, j;
    for(j = 0; j < _dict.size(); ++j ) //vector<pair<string, int>>
    {
        if(is_chinese(_dict[j].first)) //如果是中文
        {
            char chword[3] = {0};
            int k = 0;
            while(_dict[j].first[k])
            {
                chword[0] = _dict[j].first[k];
                chword[1] = _dict[j].first[k+1];
                chword[2] = _dict[j].first[k+2];
                string indch(chword);
                _indexTable[indch].insert(j);
                k = k+3;
            }
        }else{ //如果是英文
            string ind("a");
            for(i = 0; i < _dict[j].first.size(); ++i)
            {
                ind[0] = _dict[j].first[i];
                _indexTable[ind].insert(j);    //unordered_map<string, set<int>> _indexTable;       
            }
        }
    }
}//6

void Dictionary::stringDeal(string &s)
{
    int i = 0;
    while(s[i])
    {
        if(isalpha(s[i]))
        {
            if(s[i] >= 'A' && s[i] <= 'Z')
            {
                s[i] += 32;
            }
        }else{
            s[i] = ' ';
        }
        ++i;
    }
}//7

void Dictionary::chnStringDeal(string &s)
{
    string tmp = cutCword(s);
    s = tmp;
}//8

void Dictionary::writeDictAndIndexInFile()
{
    ofstream ofs;
    ofs.open(_p->ditcPath());
    if(!ofs.good())
    {
        LogError("open dict.dat failed");
        return;
    }
    for(auto & w : _dict)
    {
        ofs << w.first.c_str() << " " << w.second << endl;
    }
    ofs.close();

    ofstream ofs2;
    ofs2.open(_p->indexPath());
    if(!ofs2.good())
    {
        LogError("open index.dat failed");
        return;
    }
    for(auto & i : _indexTable)
    {
        ofs2 << i.first.c_str() << " ";
        for(auto & s : i.second)
        {
            ofs2 << s << " ";
        }
        ofs2 << endl;
    }
    ofs2.close();
}//9
void Dictionary::readDictAndIndexDat()
{
    ifstream ifs;
    ifs.open(_p->ditcPath());
    if(!ifs.good())
    {
        LogError("open dict.dat failen");
    }
    string line,key,value;
    while(getline(ifs, line))
    {
        std::istringstream iss(line);
        while(iss >> key)
        {
            iss >> value;
            string tmpkey = key;
            int tmpvalue = atoi(value.c_str());
            _dict.push_back(std::make_pair(tmpkey, tmpvalue));
        }
    }
    ifs.close();
    ifstream ifs2;
    ifs2.open(_p->indexPath());
    if(!ifs2.good())
    {
        LogError("open index.dat failed");
    }
    while(getline(ifs2, line))
    {
        std::istringstream iss(line);
        while(iss >> key)
        {
            string tmpkey = key;
            set<int> tmpvalue;
            while(iss >> value)
            {
                tmpvalue.insert(atoi(value.c_str()));
            }
            _indexTable[key] = tmpvalue;
        }
    }

}//10
//--------------------------------------------------------------------------
//候选词结果 QueryResult
class QueryResult
{
    friend class Task;
    
public:
    QueryResult(int dis, string word, int fre)
    : _distance(dis)
    , _word(word)
    , _frequency(fre)
    {  }
    
    int getDistance() const
    {
        return _distance;
    }
    int getFrequency() const
    {
        return _frequency;
    }

private:
    int _distance;
    string _word;
    int _frequency;
};//

//---------------------------------------------------------------------------








#endif






