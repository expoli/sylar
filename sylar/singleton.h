/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/14 16:41
* @version: 1.0
* @description: 
********************************************************************************/


#ifndef SYLAR_SINGLETON_H
#define SYLAR_SINGLETON_H
#include <memory>

namespace sylar {

    /**
     * 单例模板类
     *
     * 使用模板
     * 构造的单例类的构造函数不能私有，因为模板类静态函数需要调用
     * 这里使用模板类实现单例，日表管理器本身不是单例，模板类是一个提供的单例构造器
     * 只要静态方法就行了
     *
     * 之所以不在日志管理器里面实现单例模式，因为分布式多线程的情况下，
     * 可能是每个线程一个单例，所以不适合在日志管理器里面实现单例模式。使用模板类可以解决这类问题。
     * @tparam T
     * @tparam X
     * @tparam N
     */
        template<class T, class X = void, int N = 0> // N 是用来区分不同的单例
        class Singleton {
        public:
            static T *GetInstance() {
                static T v;
                return &v;
            }
        };

        template<class T, class X = void, int N = 0>
        class SingletonPtr {
        public:
            static std::shared_ptr<T> GetInstance() {
                static std::shared_ptr<T> v(new T);
                return v;
            }
        };
}

#endif //SYLAR_SINGLETON_H
