#ifndef _NOCOPY_H
#define _NOCOPY_H
//将拷贝构造函数和拷贝赋值函数设置为private，禁止类的拷贝
//产生子类的实例对象时会先调用基类的构造函数以及拷贝构造函数（基类构造函数-》成员对象构造函数-》子类构造函数）
//析构时相反
class NoCopy{
protected:
    NoCopy(){}
    ~NoCopy(){}
private:
    NoCopy(const NoCopy&);
    const NoCopy& operator=(const NoCopy&);
};

#endif