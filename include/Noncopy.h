
#ifndef __NONCOPY_H__
#define __NONCOPY_H__



class Noncopyable
{
protected:
    Noncopyable(){}
    ~Noncopyable(){}
    
    Noncopyable(const Noncopyable&) = delete;
    Noncopyable &operator=(const Noncopyable&) = delete;
};//


#endif
