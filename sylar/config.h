/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/15 14:57
* @version: 1.0
* @description: 
********************************************************************************/


#ifndef SYLAR_CINFIG_H
#define SYLAR_CINFIG_H

#include <memory>
#include <string>
#include <sstream>
#include <boost/lexical_cast.hpp>
#include <yaml-cpp/yaml.h>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <functional>   // 可以封装成指针，函数指针，lambda表达式，成员，伪装，把一个函数签名不一样的包装成需要的接口

#include "log.h"
#include "thread.h"

namespace sylar {

/**
 * @brief 配置变量基类
 * 虚拟的基类
 */
class ConfigVarBase {
public:
    typedef std::shared_ptr<ConfigVarBase> ptr;

    ConfigVarBase(const std::string &name, const std::string &description = "")
            : m_name(name) // 转换成小写
            , m_description(description) {
        std::transform(m_name.begin(), m_name.end(), m_name.begin(), ::tolower);
    }

    virtual ~ConfigVarBase() {} // 虚析构函数

    const std::string &getName() const { return m_name; }

    const std::string &getDescription() const { return m_description; }

    virtual std::string toString() = 0;

    virtual bool fromString(const std::string &val) = 0;    // 解析，初始化到自己的成员里面去
    virtual std::string getTypeName() const = 0;

protected:
    std::string m_name;
    std::string m_description;
};

// 基础类型的转换
// F from_type, T to_type
template<class F, class T>
class LexicalCast {
public:
    T operator()(const F &v) {
        return boost::lexical_cast<T>(v);
    }
};

// todo 常用容器的 偏特化
template<class T>
class LexicalCast<std::string, std::vector<T>> {
public:
    std::vector<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);    // 解析成 yaml, 一个数组
        typename std::vector<T> vec;    // typename 用于告诉编译器，这是一个类型，而不是一个变量
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str())); // 递归调用
        }
        return vec;
    }
};

// vector -> string
template<class T>
class LexicalCast<std::vector<T>, std::string> {
public:
    std::string operator()(const std::vector<T> &v) {
        YAML::Node node;
        for(auto &i: v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i))); // 递归调用, 把它变成一个 yaml 的结点
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::list<T>> {
public:
    std::list<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::list<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.push_back(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::list<T>, std::string> {
public:
    std::string operator()(const std::list<T> &v) {
        YAML::Node node;
        for(auto &i: v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::set<T>> {
public:
    std::set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::set<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::set<T>, std::string> {
public:
    std::string operator()(const std::set<T> &v) {
        YAML::Node node;
        for(auto &i: v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::unordered_set<T>> {
public:
    std::unordered_set<T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_set<T> vec;
        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i) {
            ss.str("");
            ss << node[i];
            vec.insert(LexicalCast<std::string, T>()(ss.str()));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::unordered_set<T>, std::string> {
public:
    std::string operator()(const std::unordered_set<T> &v) {
        YAML::Node node;
        for(auto &i: v) {
            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::map<std::string, T>> {
public:
    std::map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::map<std::string, T> vec;
        std::stringstream ss;
        for(auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::map<std::string, T>, std::string> {
public:
    std::string operator()(const std::map<std::string, T> &v) {
        YAML::Node node;
        for(auto &i: v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

template<class T>
class LexicalCast<std::string, std::unordered_map<std::string, T>> {
public:
    std::unordered_map<std::string, T> operator()(const std::string &v) {
        YAML::Node node = YAML::Load(v);
        typename std::unordered_map<std::string, T> vec;
        std::stringstream ss;
        for(auto it = node.begin(); it != node.end(); ++it) {
            ss.str("");
            ss << it->second;
            vec.insert(std::make_pair(it->first.Scalar(), LexicalCast<std::string, T>()(ss.str())));
        }
        return vec;
    }
};

template<class T>
class LexicalCast<std::unordered_map<std::string, T>, std::string> {
public:
    std::string operator()(const std::unordered_map<std::string, T> &v) {
        YAML::Node node;
        for(auto &i: v) {
            node[i.first] = YAML::Load(LexicalCast<T, std::string>()(i.second));
        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

// 实现序列化与反序列化, 实现复杂类型的转换
// FromStr T operator()(const std::string& v)
// ToStr std::string operator()(const T& v)
// todo 特例化，实现基础类型的转换
template<class T, class FromStr = LexicalCast<std::string, T>, class ToStr = LexicalCast<T, std::string>>
class ConfigVar : public ConfigVarBase {
public:
    typedef RWMutex RWMutexType;    // 配置写少读多，使用读写锁
    typedef std::shared_ptr<ConfigVar> ptr;
    // todo 当一个配置更改的时候，需要监听者知道他原来的配置是什么，新的配置是什么
    typedef std::function<void(const T &old_value, const T &new_value)> on_change_cb;  // 使用的时候与智能指针一样

    ConfigVar(const std::string &name, const T &default_value, const std::string &description = "")
            : ConfigVarBase(name, description), m_val(default_value) {
    }

    std::string toString() override {
        try {
//                return boost::lexical_cast<std::string>(m_val);
            RWMutexType::ReadLock lock(m_mutex);
            return ToStr()(m_val);
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::toString exception "
                                              << e.what() << " convert: " << typeid(m_val).name() << " to string";
        }
        return "";
    }

    bool fromString(const std::string &val) override {
        try {
//                m_val = boost::lexical_cast<T>(val);
            setValue(FromStr()(val));
            return true;
        } catch (std::exception &e) {
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "ConfigVar::FromString exception "
                                              << e.what() << " convert: string to " << typeid(m_val).name()
                                              << " -\n" << val;
        }
        return false;
    }

    const T getValue() {
        RWMutexType::ReadLock lock(m_mutex);
        return m_val;
    }

    void setValue(const T &v) {
        {   // 前面这一段用读锁
            RWMutexType::ReadLock lock(m_mutex);
            if(v == m_val) {   // 如果新值与旧值相同，不需要通知
                return;
            }
            for(auto &i: m_cbs) { // 通知所有的监听者
                i.second(m_val, v);
            }
        }// 下面用写锁
        RWMutexType::WriteLock lock(m_mutex);
        m_val = v;
    }

    std::string getTypeName() const override { return typeid(T).name(); }

    // 添加变更回调函数
    uint64_t addListener(on_change_cb cb) {
        static uint64_t s_fun_id = 0;
        RWMutexType::WriteLock lock(m_mutex);
        ++s_fun_id;
        m_cbs[s_fun_id] = cb;
        return s_fun_id;
    }

    // 删除变更回调函数
    void delListener(u_int64_t key) {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.erase(key);
    }

    // 获取对应 key 的回调函数
    on_change_cb getListener(u_int64_t key) {
        RWMutexType::ReadLock lock(m_mutex);
        auto it = m_cbs.find(key);
        return it == m_cbs.end() ? nullptr : it->second;
    }

    // 清空所有的回调函数
    void clearListener() {
        RWMutexType::WriteLock lock(m_mutex);
        m_cbs.clear();
    }

private:
    RWMutexType m_mutex;
    T m_val;

    // 变更回调函数组
    // todo 为什么要用map，而不是vector,因为functionl 没有比较函数，
    // 所以在删除的时候，没办法判断是否是同一个函数，所以用在外围进行包装，使用map来定义它
    // 传入的时候传入一个 key，然后在删除的时候，传入这个key，就可以删除了
    // u_int64_t 是key，要求是唯一的，一般可以用hash值来表示
    std::map<u_int64_t, on_change_cb> m_cbs;
};

class Config {
public:
    typedef RWMutex RWMutexType;    // 读多写少
    typedef std::unordered_map<std::string, ConfigVarBase::ptr> ConfigVarMap;

    template<class T>
    static typename ConfigVar<T>::ptr
    Lookup(const std::string &name, const T &default_value, const std::string &description = "") {   // 如果有的话是返回，没有的话是创建
        {   // 原配置、这里使用全局写锁的话、下面会调用读锁，导致死锁
            RWMutexType::ReadLock lock(GetMutex());
            auto it = GetDatas().find(name);
            if(it != GetDatas().end()) {
                auto tmp = std::dynamic_pointer_cast<ConfigVar<T>>(
                        it->second); // 利用dynamic_pointer_cast进行类型转换，如果同名的不是T类型的，就会返回空
                if(tmp) {
                    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name " << name << " exists";
                    return tmp;
                } else {
                    SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name " << name << " exists but type not "
                                                      << typeid(T).name() << " real_type=" << it->second->getTypeName()
                                                      << "\n" << it->second->toString();
                    return nullptr;
                }
            }
        }
        auto tmp = Lookup<T>(name);
        {
            RWMutexType::WriteLock lock(GetMutex());
            if(tmp) {
                SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "Lookup name " << name << " exists";
                return tmp;
            }

            if(name.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789") != std::string::npos) {
                SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Lookup name invalid " << name;
                throw std::invalid_argument(name);
            }

            typename ConfigVar<T>::ptr v(new ConfigVar<T>(name, default_value, description));
            GetDatas()[name] = v;
            return v;
        }
    }

    template<class T>
    static typename ConfigVar<T>::ptr Lookup(const std::string &name) {
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        if(it == GetDatas().end()) {   // 没有找到
            return nullptr;
        }
        return std::dynamic_pointer_cast<ConfigVar<T>>(it->second); // 找到了，转成对应的智能指针
    }

    static void LoadFromYaml(const YAML::Node &root);

    static ConfigVarBase::ptr LookupBase(const std::string &name);

    static void Visit(std::function<void(ConfigVarBase::ptr)> cb);  // 获取所有的 map 中的数据，观察者模式
private:
    static ConfigVarMap &GetDatas() {
        // 初始化的顺序，可能会引起 bug ，现在使用静态函数来返回，s_datas 一定先初始化
        // 编译器并没有规定全局变量、静态变量谁先初始化
        static ConfigVarMap s_datas;
        return s_datas;
    }

    // 全局变量、全局变量的初始化其实没有一个严格的顺序，所以这里使用函数来返回，变量比函数先初始化，防止出现内存错误
    static RWMutexType &GetMutex() {
        static RWMutexType s_mutex;
        return s_mutex;
    }
};
}

#endif //SYLAR_CINFIG_H
