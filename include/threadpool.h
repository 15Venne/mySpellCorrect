
#ifndef __THREADPOOL_H__
#define __THREADPOOL_H__

#include"Noncopy.h"
#include"lruCache.h"
#include"mylog.h"

#include<unistd.h>
#include<pthread.h>
#include<stdio.h>
#include<errno.h>
#include<stdlib.h>

#include<functional>
#include<queue>
#include<memory>
#include<vector>
#include<fstream>
#include<sstream>

using std::function;
using std::queue;
using std::vector;
using std::unique_ptr;
using std::ifstream;
using std::ofstream;
using std::istringstream;
using std::shared_ptr;


//--------------------------------------------------------------------------
//锁
class MutexLock
: public Noncopyable
{
public:
    MutexLock()
        : _isLocking(false)
    {
        if(pthread_mutex_init(&_mutex, NULL))
        {
            perror("pthread_mutex_init");
        }
    }
    ~MutexLock()
    {
        if(pthread_mutex_destroy(&_mutex))
        {
            perror("pthread_mutex_destroy");
        }
    }
    void lock()
    {
        if(pthread_mutex_lock(&_mutex))
        {
            perror("pthread_mutex_lock");
            return;
        }
        _isLocking = true;
    }
    void unlock()
    {
        if(pthread_mutex_unlock(&_mutex))
        {
            perror("pthread_mutex_unlock");
            return;
        }
        _isLocking = false;
    }
    pthread_mutex_t *getMutexPtr()
    {
        return &_mutex;
    }
private:
    bool _isLocking;
    pthread_mutex_t _mutex;
};//

class MutexLockGuard
{
public:
    MutexLockGuard(MutexLock &mu)
        : _mutex(mu)
    { 
        _mutex.lock();
    }
    ~MutexLockGuard()
    {
        _mutex.unlock();
    }
private:
    MutexLock &_mutex;
};//
//-------------------------------------------------------------------------------------
//Condition
class Condition
{
public:
    Condition(MutexLock &mu)
        : _mutex(mu)
    {  
        if(pthread_cond_init(&_cond, NULL))
        {
            perror("pthread_cond_init");
        }
    }
    ~Condition()
    {
        if(pthread_cond_destroy(&_cond))
        {
            perror("pthread_cond_destroy");
        }
    }
    void wait()
    {
        if(pthread_cond_wait(&_cond, _mutex.getMutexPtr()))
        {
            perror("pthread_cond_wait");
        }
    }
    void notify()
    {
        if(pthread_cond_signal(&_cond))
        {
            perror("pthread_cond_signal");
        }
    }
    void notifyAll()
    {
        if(pthread_cond_broadcast(&_cond))
        {
            perror("pthread_cond_broadcast");
        }
    }
private:
    MutexLock &_mutex;
    pthread_cond_t _cond;
};//
//-----------------------------------------------------------------------------------
class Threadpool;
class Task;
//Thread 线程
class Thread
: Noncopyable
{
    friend class Threadpool;
    friend class Task;
public:
    Thread()
    : _pthid(0)
    , _cb()
    , _isRunning(false)
    , _cache(100)
    , _writeCache(false)
    { 
        ifstream is;
        is.open("../cache/my.cache");
        if(!is.good())
        {
            LogError("open my.cache failed");
            return;
        }
        string line;
        while(getline(is, line))
        {
            string word;
            string key;
            vector<string> value;
            istringstream tmpline(line);
            if(tmpline >> word)
            {
                key = word;
            }
            while(tmpline >> word)
            {
                value.push_back(word);
            }
            _cache.put(key, value);
        }
        cout << "内存缓存建立成功" << endl;
        
    }
    void start()
    {
        pthread_create(&_pthid, NULL, threadFunc, this);
        _isRunning = true;
    }
    void join()
    {
        if(_isRunning)
        {
            pthread_join(_pthid, NULL);
        }
    }
    ~Thread()
    {
        if(_isRunning)
        {
            pthread_detach(_pthid);
        }
    }
    void setcb(function<void(shared_ptr<Thread>)> &&cb)
    {
        _cb = std::move(cb);
    }
private:
    static void *threadFunc(void *);//1
private:
    pthread_t _pthid;
    function<void(shared_ptr<Thread>)> _cb;
    bool _isRunning;
    LRUCache _cache;
    bool _writeCache;
};

void *Thread::threadFunc(void *p)
{
    Thread *pt = static_cast<Thread*>(p);
    if(pt)
    {
        pt->_cb(shared_ptr<Thread>(pt));
    }
    return nullptr;
}
//---------------------------------------------------------------------------------
//TaskQueue
using Taskfun = std::function<string(shared_ptr<Thread>&)>;
class TaskQueue
{
public:
    TaskQueue(size_t sz);//1
    bool empty() const;//2
    bool full() const;//3
    void push(Taskfun);//4
    Taskfun pop();//5
    void wakeup();//6
private:
    size_t _sz;
    queue<Taskfun> _que;
    MutexLock _mutex;
    Condition _notEmpty;
    Condition _notFull;
    bool _flag;
};//

TaskQueue::TaskQueue(size_t sz)
: _sz(sz)
, _que()
, _mutex()
, _notEmpty(_mutex)
, _notFull(_mutex)
, _flag(true)
{  }//1

bool TaskQueue::empty() const
{
    return _que.empty();
}//2
bool TaskQueue::full() const
{
    return (_que.size() == _sz);
}//3

void TaskQueue::push(Taskfun t)
{
    {
        MutexLockGuard autoLock(_mutex);
        while(full())
        {
            _notFull.wait();
        }
        _que.push(t);
    }
    _notEmpty.notify();
}//4

Taskfun TaskQueue::pop()
{
    Taskfun task;
    {
        MutexLockGuard autoLock(_mutex);
        while(_flag && empty())
        {
            _notEmpty.wait();
        }
        if(_flag)
        {
            task = _que.front();
            _que.pop();
            _notFull.notify();
        }else{
            return nullptr;
        }
    }
    return task;
}//5
void TaskQueue::wakeup()
{
    _flag = false;
    _notEmpty.notifyAll();
}//6
//------------------------------------------------------------------------------------
//线程池Threadpool
class Threadpool
{
public:
    Threadpool(size_t, size_t);//1
    void start();//2
    void stop();//3
    void addTask(Taskfun &&);//4
private:
    void threadFunc(shared_ptr<Thread>);//5
    Taskfun getTask();//6
    void combineCache(LRUCache &, LRUCache &);//7
    static void *tellWriteCacheThreadFunc(void *);//8
private:
    size_t _threadNum;
    size_t _queSize;
    TaskQueue _taskQue;
    bool _isExit;
    vector<unique_ptr<Thread>> _threads;
};//
Threadpool::Threadpool(size_t threadNum, size_t queSize)
: _threadNum(threadNum)
, _queSize(queSize)
, _taskQue(_queSize)
, _isExit(false)
{ 
    _threads.reserve(_threadNum) ;  
}//1

void Threadpool::start()
{
    for(size_t idx = 0; idx < _threadNum; ++idx)
    {
        //Thread tmp(std::bind(&Threadpool::threadFunc, this, &tmp));
        unique_ptr<Thread> pthread(new Thread());
        //unique_ptr<Thread> pthread(tmp);
        pthread->setcb(std::bind(&Threadpool::threadFunc, this, shared_ptr<Thread>(pthread.get())));
        _threads.push_back(std::move(pthread));
    }
    for(auto & t : _threads)
    {
        t->start();
    }
    
    pthread_t ptwctthid;
    pthread_create(&ptwctthid, NULL, tellWriteCacheThreadFunc, this);

}//2

void *Threadpool::tellWriteCacheThreadFunc(void *p)
{
    Threadpool *pt = static_cast<Threadpool*>(p);
    //以下轮询让线程同步他们的缓存(打开开关)
    int i = 0;
    while(!pt->_isExit)
    {
        for(auto & t : pt->_threads)
        {
            t->_writeCache = false;
        }
        pt->_threads[i]->_writeCache = true; // 一段时间内只有一个线程拥有写磁盘的权限
        sleep(5);
        i = (i + 1) % pt->_threadNum;
    }

}//8


void Threadpool::stop()
{
    if(!_isExit)
    {
        while(!_taskQue.empty())
        {
            ::sleep(1);
        }

        _isExit = true;
        _taskQue.wakeup();

        for(auto & t : _threads)
        {
            t->join();
        }
    }
}//3
void Threadpool::addTask(Taskfun &&t)
{
    _taskQue.push(t);
}//4
void Threadpool::threadFunc(shared_ptr<Thread> p)
{
    while(!_isExit)
    {
        Taskfun task = getTask();
        string s;
        if(task)
        {
            s = task(p);
        }
        //插入/更新 缓存
        istringstream iss(s);
        string word;
        string key;
        vector<string> value;
        if(iss >> word)
        {
            key = word;
        }
        while(iss >> word)
        {
            value.push_back(word);
        }
        p->_cache.put(key, value);
        
        cout << "已更新内存缓存, key: " << key << endl;

      //以下操作需要保证写回磁盘这段时间其他线程的_writecache没有打开
        
        if(p->_writeCache)// 该线程缓存同步到其他线程
                            //(如果同步缓存这一步想要绝对的线程安全
                            //貌似只能加锁了，不过对于内存缓存来讲，即使不安全也无关紧要吧
        {
            for(auto & t : _threads)
            {
                combineCache(t->_cache, p->_cache);
            }
            //以下缓存回写磁盘, 回写磁盘与同步缓存并非一定要绑在一起，更多时候只同步不回写
            //回写磁盘的时间只要小于主线程轮流打开writeCache开关的轮询(sleep)时间，
            //就能做到线程安全
            ofstream ofs;
            ofs.open("../cache/my.cache");
            if(!ofs.good())
            {
                LogError("open my.cache failed");
                return;
            }

            pvalueNode pcur = p->_cache.getValueNodeLise()->phead;
            while(pcur)
            {
                ofs << pcur->key.c_str() << " ";
                auto it = p->_cache.getCache()->find(pcur->key);
                if(it != p->_cache.getCache()->end())
                {
                    for(auto & w : it->second.value)
                    {
                        ofs << w.c_str() << " ";
                    }
                }
                pcur = pcur->pnext;
                ofs << '\n';
            }
            p->_writeCache = false;
            cout << "内存缓存已写回磁盘" << endl;
        } 
    }
}//5
Taskfun Threadpool::getTask()
{
    return _taskQue.pop();
}//6

void Threadpool::combineCache(LRUCache &lcache, LRUCache &rcache)
{
    if(lcache.getCache() == rcache.getCache())
    {
        return;
    }

    LRUCache tmp(100);
    pvalueNode plcur = lcache.getValueNodeLise()->phead;
    pvalueNode prcur = rcache.getValueNodeLise()->phead;

    while(plcur && prcur)
    {
        string lkey, rkey;
        vector<string> lvalue, rvalue;
        lkey = plcur->key;
        lvalue = plcur->value;
        rkey = prcur->key;
        rvalue = prcur->value;
        tmp.put(lkey, lvalue);
        tmp.put(rkey, rvalue);
        plcur = plcur->pnext;
        prcur = prcur->pnext;
    }
    while(plcur)
    {
        string lkey;
        vector<string> lvalue;
        lkey = plcur->key;
        lvalue = plcur->value;
        tmp.put(lkey, lvalue);
        plcur = plcur->pnext;
    }
    while(prcur)
    {
        string rkey;
        vector<string> rvalue;
        rkey = prcur->key;
        rvalue = prcur->value;
        tmp.put(rkey, rvalue);
        prcur = prcur->pnext;
    }
    lcache = tmp;
    rcache = tmp;
}//7
//-------------------------------------------------------------------------------------










#endif
