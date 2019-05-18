
#ifndef __LRUCACHE_H__
#define __LRUCACHE_H__

#include<string.h>

#include<iostream>
#include<map>
#include<string>
#include<vector>

using std::cout;
using std::endl;
using std::string;
using std::map;
using std::vector;

typedef struct valueNode
{
    vector<string> value;
    struct valueNode *ppre;
    struct valueNode *pnext;
    int alive;
    string key;
}valueNode, *pvalueNode;

typedef struct valueNodeList
{
    pvalueNode phead;
    pvalueNode ptail;
    int size;
}valueNodeList, *pvalueNodeList;

class LRUCache
{
public:
    LRUCache(int capacityNum)
    : _capacity(capacityNum)
    {
        //cout << "LRUCache(int)" << endl;
        memset(&_valList, 0, sizeof(_valList));
    }
    //尾插法 最常用的在链表尾
    void valuenodeInsert(string key, vector<string> &value)
    {
        _cache[key].alive = 1;
        _cache[key].value = value;
        _cache[key].key = key;
        pvalueNode pcur = &_cache[key];
        if(_valList.size == 0)
        {
            _valList.phead = pcur;
            _valList.ptail = pcur;
        }
        else
        {
            _valList.ptail->pnext = pcur;
            pcur->ppre = _valList.ptail;
            _valList.ptail = pcur;
        }
        pcur = nullptr;
        ++_valList.size;
    }

    void valueDelete(string key)
    {
        pvalueNode pcur = &_cache[key];
        if(pcur == _valList.phead)
        {
            _valList.phead = pcur->pnext;
            if(pcur->pnext)
            {
                pcur->pnext->ppre = nullptr;
            }
        }
        else if(pcur == _valList.ptail)
        {
            pcur->ppre->pnext = nullptr;
            _valList.ptail = pcur->ppre;
        }
        else
        {
            pcur->ppre->pnext = pcur->pnext;
            pcur->pnext->ppre = pcur->ppre;
        }
        _cache.erase(key);
        pcur = nullptr;
        --_valList.size;
        
     //   cout << "删除成功" << endl;
    }

    void valueDelete()
    {
        pvalueNode pcur = _valList.phead;
        _valList.phead = pcur->pnext;
        if(pcur->pnext)
        {
            pcur->pnext->ppre = nullptr;
        }
        _cache.erase(pcur->key);
        pcur = nullptr;
        --_valList.size;
    }
    
    vector<string> get(string key)
    {   
        if(_valList.size == 0)
        {
            cout << "data is empty." << endl;
            return vector<string>();
        }
        if(_cache[key].alive == 0)
        {
            cout << "no key." << endl;
            _cache.erase(key);
            return vector<string>();
        }
        vector<string> tmp = _cache[key].value;
        valueDelete(key);
        valuenodeInsert(key, tmp);
        return tmp;
    }

    void put(string key,vector<string> value)
    {
        if(_cache[key].alive == 1)
        {
         //   cout << "key already exists" << endl;
            _cache[key].value = value;
            get(key);
            return;
        }
        if(_valList.size == _capacity)
        {
            valueDelete();
        }
        valuenodeInsert(key, value);
    }

    ~LRUCache()
    {
        //cout << "~LRUCache()" << endl;
    }
    map<string, valueNode> *getCache()
    {
        return &_cache;
    }
    valueNodeList *getValueNodeLise()
    {
        return &_valList;
    }

    LRUCache &operator=(LRUCache &rhs);//
private:
    int _capacity;
    map<string, valueNode> _cache;
    valueNodeList _valList;
};
//等号重载：以对方更新自身，自身原本的map不变
LRUCache &LRUCache::operator=(LRUCache &rhs)
{
    pvalueNode pcur = rhs.getValueNodeLise()->phead;
    while(pcur)
    {
        string key;
        vector<string> value;
        key = pcur->key;
        value = pcur->value;
        put(key, value);
        pcur = pcur->pnext;
    } 
    pcur = nullptr;
}//

/* int test0() */
/* { */
/*     LRUCache ca = LRUCache(2); */
/*     ca.put(1, 1); */
/*     cout << "已插入1, 1" << endl; */
/*     ca.put(2, 2); */
/*     cout << "已插入2, 2" << endl; */
/*     cout << "测试get(1) " << ca.get(1) << endl; //输出1 */
/*     ca.put(3, 3);//使key2作废 */
/*     cout << "测试put(3)完毕, key2作废" << endl; */
    /* cout << "测试get(2), 应为无数据 " << ca.get(2) << endl; //无数据 */
    /* ca.put(4, 4);//使key1作废 */
    /* cout << "put(4)完毕, key1作废" << endl; */
    /* cout << "get(1): 应为无数据 " << ca.get(1) << endl; //无数据 */
    /* cout << "get(3): " << ca.get(3) << endl; //输出3 */
    /* cout << "get(4): " << ca.get(4) << endl; //输出4 */
    /* return 0; */
/* } */

/* int test1() */
/* { */
/*     LRUCache ca = LRUCache(2); */
/*     ca.put(2, 1); */
/*     ca.put(2, 2); */
/*     cout << ca.get(2) << endl; */
/*     ca.put(1, 1); */
/*     ca.put(4, 1); */
/*     cout << ca.get(2) << endl; */
/* } */

/* int test2() */
/* { */
/*     LRUCache ca = LRUCache(2); */
/*     cout << ca.get(2) << endl; */
/*     ca.put(2, 6); */
    /* cout << ca.get(1) << endl; */
    /* ca.put(1, 5); */
    /* ca.put(1, 2); */
    /* cout << ca.get(1) << endl; */
    /* cout << ca.get(2) << endl; */
/* } */

/* int test3() */
/* { */
/*     LRUCache ca = LRUCache(2); */
/*     ca.put(2, 1); */
/*     ca.put(1, 1); */
/*     ca.put(2, 3); */
/*     ca.put(4, 1); */
/*     cout << ca.get(1) << endl; */
/*     cout << ca.get(2) << endl; */
/* } */









#endif

