# sylar

## 开发环境

* ArchLinux
* cmake
* gcc
  * boost
  * yaml-cpp

## 项目结构

* bin: 二进制文件
* build: 中间文件路径
* cmkake: cmake函数文件夹
* CmakeLists.txt: cmake配置文件
* lib: 库文件输出路径
* Makefile: make配置文件
* sylar: 项目源码
* tests: 测试代码

## 日志系统

- log4j
  - Logger（定义日志类别）
    - Appender（定义日志输出地方）
      - Formatter（定义日志输出格式）
      - FileAppender（文件输出）
      - StdoutAppender（标准输出）
      - StderrAppender（标准错误输出）
      - LogStreamAppender（日志流输出）
    - Formatter（定义日志输出格式）

## 配置系统

* Config --> Yaml
  * yaml-cpp
    * 第三方库，GitHub上有
* 使用方式
  * 读取配置文件
  * 读取配置文件中的某个节点
  * 读取配置文件中的某个节点的某个属性
  * 读取配置文件中的某个节点的某个属性的值
  * 读取配置文件中的某个节点的某个属性的值的数组
  * 读取配置文件中的某个节点的某个属性的值的数组的某个值
```c++
YAML::Node root = YAML::LoadFile("config.yml");
for(auto i = root.begin(); i != root.end(); ++i) {
    std::cout << i->first << " : " << i->second << std::endl;
}

node.IsSequence() // 判断是否是数组
for(size_t i = 0; i < node.size(); ++i) {
    std::cout << node[i].as<std::string>() << std::endl;
}

node.IsScalar() // 判断是否是标量
```

- 配置系统的原则：约定优于配置 
  - 项目真正跑起来的时候，需要修改的配置量不多，可能只有一些比较少的比较重要的配置。

```c++
template<T, FromStr, ToStr>
class ConfigVar;

template<F, T>
LexicalCast;

// 容器偏特化，目前支持vector
// list, set, unordered_set, map, unordered_map
// map/unordered_set 支持 key = std::string
// 遗留问题 Config::Lookup(key), key 相同，类型不同的，不会有报错。这个需要处理一下。
```

- 自定义类型，需要实现 syalr::LexicalCast, 偏特化
  - 实现后，就可以支持 config 解析自定义类型了，自定义类型可以和常规 stl 容器一起使用。
  - （实现是通过递归实现的，不管嵌套多少层，都能正常解析）

### 配置的事件机制
当一个配置项发生修改的时候，可以反向通知对应的代码，回调函数的方式。

### 日志系统整合配置系统

```yaml
logs:
  - name: root
    level: (debug, info, warn, error, fatal)
    formatter: "%d%T%p%T%t%m%n"
    appenders:
      - type: StdoutLogAppender
        level: (debug, info, warn, error, fatal)
      - type: FileLogAppender
        file: ./logs/xxx.log
  - name: test
    level: debug
    formatter: "%d%T%p%T%m%n"
    appenders:
      - type: StdoutLogAppender
      - type: FileLogAppender
        file: ./logs/test.txt
```

- 保证 logger 的唯一性

```c++
static Logger::ptr g_log = SYLAR_LOG_NAME("system");
//m_root, m_system-> m_root
当 logger 的 appenders 为空，使用 root 写 logger
```

- 定义 LogDefine LogAppendDefine 偏特化 LogDefine
- 实现日志配置解析
- 实现日志配置的热更新
- fix 日志文件未以追加方式打开的 bug
- 不同 log 使用不同的配置
  - （fix 原始根据文件定义 appender 的日志级别为 unknown 所以无法输出日志的 bug）

## 协程库封装

Thead, Mutex, RWMutex

pthread pthread_create

互斥量 mutex
信号量 semaphore

- 整合 log
  - Logger, appender, manager
- 锁的性能
  - 自旋 12M 
  - 空锁 30M 
  - 互斥 5M
  - CAS 6M

## socket 函数库

## http 协议开发

## 分布协议

## 推荐系统
