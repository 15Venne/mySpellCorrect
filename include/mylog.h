
#ifndef __MYLOG_H__
#define __MYLOG_H__


#include<iostream>
#include<log4cpp/Category.hh>
#include<log4cpp/OstreamAppender.hh>
#include<log4cpp/Priority.hh>
#include<log4cpp/Appender.hh>
#include<log4cpp/FileAppender.hh>
#include<log4cpp/PatternLayout.hh>
//输出的日志信息能同时输出到终端和文件
using namespace log4cpp;

using std::string;
using std::cout;
using std::endl;
using std::ostringstream;

#define prefix(msg)  string("[").append(__FILE__)\
	.append(":").append(__FUNCTION__)\
    .append(":").append(std::to_string(__LINE__))\
	.append("] ").append(msg).c_str()

#define LogError(msg)  Mylogger::getInstance()->error(prefix(msg))
#define LogInfo(msg)  Mylogger::getInstance()->info(prefix(msg))
#define LogWarn(msg)  Mylogger::getInstance()->warn(prefix(msg))
#define LogDebug(msg)  Mylogger::getInstance()->debug(prefix(msg))

Category *getFileCategory()
{
    PatternLayout *pLayoutFile = new PatternLayout();
    pLayoutFile->setConversionPattern("%d: [%p] %c %x: %m%n");
    
   // PatternLayout *pLayoutCout = new PatternLayout();
    //pLayoutCout->setConversionPattern("%d: [%p] %c %x: %m%n");

    Appender *pfileAppender = new FileAppender("pfileAppender", "../log/my.log");
    pfileAppender->setLayout(pLayoutFile);

    //OstreamAppender *pOsAppender = new OstreamAppender("pOsAppender", &cout);
    //pOsAppender->setLayout(pLayoutCout);

    Category &root = Category::getRoot();
    root.setPriority(Priority::DEBUG);

    Category &subFile = root.getInstance("fileLog");
    subFile.addAppender(pfileAppender);
    //subFile.addAppender(pOsAppender);
    subFile.setPriority(Priority::DEBUG);
    return &subFile;
}

class Mylogger
{
public:
    static Mylogger *getInstance();
    
    void warn(const char*);
    void error(const char*);
    void debug(const char*);
    void info(const char*);
private:
    Mylogger()
    : _pCategory(getFileCategory())
    {  }
    ~Mylogger()
    {  }
    static Mylogger *_pmylogger;
    Category *_pCategory;
};

Mylogger *Mylogger::_pmylogger = nullptr;

Mylogger *Mylogger::getInstance()
{
    if(!_pmylogger)
    {
        _pmylogger = new Mylogger();
    }
    return _pmylogger;
}

void Mylogger::warn(const char *msg)
{
    _pCategory->warn(msg);
}
void Mylogger::error(const char *msg)
{
    _pCategory->error(msg);
}
void Mylogger::debug(const char *msg)
{
    _pCategory->debug(msg);
}
void Mylogger::info(const char *msg)
{
    _pCategory->info(msg);
}



/* int main() */
/* { */
/*     cout << "hello,world" << endl; */

/*     char s1[] = "helloworld"; */
    /* int num = 5; */
    /* loginfo("msg %s %d msg", s1, num); */
    
    /* return 0; */
/* } */

#endif
