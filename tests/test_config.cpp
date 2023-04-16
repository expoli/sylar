/********************************************************************************
* @author: Cuyu Tang
* @email: me@expoli.tech
* @website: www.expoli.tech
* @date: 2023/4/15 15:38
* @version: 1.0
* @description: 
********************************************************************************/

#include "../sylar/config.h"
#include <yaml-cpp/yaml.h>
// 基础类型的转换
sylar::ConfigVar<int>::ptr g_int_value_config =
        sylar::Config::Lookup("system.port", (int)8080, "system port");

sylar::ConfigVar<float>::ptr g_float_value_config =
        sylar::Config::Lookup("system.value", (float)10.2f, "system value");

sylar::ConfigVar<std::vector<int>>::ptr g_int_vec_value_config =
        sylar::Config::Lookup("system.int_vec", std::vector<int>{1, 2}, "system int vec");

void print_yaml(const YAML::Node& node, int level) {
    if (node.IsScalar()) { // 2 是 scalar
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << node.Scalar() << " - " << node.Type() << " - " << level;
    } else if (node.IsNull()) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << "NULL - " << node.Type() << " NULL " << level;
    } else if (node.IsMap()) { // 2，3 是 map
        for (auto it = node.begin(); it != node.end(); ++it) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << it->first << " - " << it->second.Type() << " - " << level;
            print_yaml(it->second, level + 1);
        }
    } else if (node.IsSequence()) { // 4 是 sequence
        for (size_t i = 0; i < node.size(); ++i) {
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << std::string(level * 4, ' ') << i << " - " << node[i].Type() << " - " << level;
            print_yaml(node[i], level + 1);
        }
    }
}

void test_yaml() {
    YAML::Node root = YAML::LoadFile("/mnt/d/DEV/Clion/sylar_new/bin/conf/log.yaml");

    print_yaml(root, 0);
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << root;
}

void test_config() {
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before: " << g_float_value_config->getValue();
    auto v = g_int_vec_value_config->getValue();
    for (auto& i : v) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "before int_vec: " << i;
    }


    YAML::Node root = YAML::LoadFile("/mnt/d/DEV/Clion/sylar_new/bin/conf/log.yaml");
    sylar::Config::LoadFromYaml(root);

    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_int_value_config->getValue();
    SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after: " << g_float_value_config->getValue();

    auto v2 = g_int_vec_value_config->getValue();
    for (auto& i : v2) {
        SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "after int_vec: " << i;
    }
}

int main(int argc, char** argv ) {
//    test_yaml();
    test_config();
    return 0;
}