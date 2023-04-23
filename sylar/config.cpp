/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/15 15:39
* @version: 1.0
* @description: 
********************************************************************************/

#include "config.h"

namespace sylar {

    ConfigVarBase::ptr  Config::LookupBase(const std::string& name){
        RWMutexType::ReadLock lock(GetMutex());
        auto it = GetDatas().find(name);
        return it == GetDatas().end() ? nullptr : it->second;
    }

    // "A.B", 10
    // A:
    //   B: 10
    //   C: str
    // 使用遍历的方式，将结构打平
    static void ListAllMember(const std::string& prefix,
                              const YAML::Node& node,
                              std::list<std::pair<std::string, const YAML::Node>>& output){
        if(prefix.find_first_not_of("abcdefghijklmnopqrstuvwxyz._0123456789")
                != std::string::npos){
            SYLAR_LOG_ERROR(SYLAR_LOG_ROOT()) << "Config invalid name: " << prefix << " : " << node;
            return;
        }
        output.push_back(std::make_pair(prefix, node));
        if(node.IsMap()){
            for(auto it = node.begin(); it != node.end(); ++it){
                ListAllMember(prefix.empty() ? it->first.Scalar() : prefix + "." + it->first.Scalar(), it->second, output);
            }
        }
    }

    void Config::LoadFromYaml(const YAML::Node& root){
        std::list<std::pair<std::string, const YAML::Node>> all_nodes;
        ListAllMember("", root, all_nodes);

        for(auto& i : all_nodes){
            std::string key = i.first;
            if(key.empty()){
                continue;
            }
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            ConfigVarBase::ptr var = Config::LookupBase(key); // 使用压平后的前缀作为 key 去查找
            if(var){
                if(i.second.IsScalar()){    // 如果是一个 string
                    var->fromString(i.second.Scalar());
                } else {
                    std::stringstream ss;
                    ss << i.second;
                    var->fromString(ss.str());
                }
            }
        }
    }

    void Config::Visit(std::function<void(ConfigVarBase::ptr)> cb) {
        RWMutexType::ReadLock lock(GetMutex());
        ConfigVarMap &m = GetDatas();
        for (auto it = m.begin(); it != m.end(); ++it) {
            cb(it->second);
        }
    }

}