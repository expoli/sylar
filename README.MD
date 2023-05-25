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

## 线程库封装

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
- 整合配置
    - 读多写少的情况下，读写锁性能更好
    - 要写、要读、遍历配置的时候，可以修改配置
    - 我们可以将配置输出到文件里、修改完之后再反序列化回去

## 协程库封装

定义协程接口
ucontext_t.
制定一些自己的宏、方便调试

```c++
Fiber::GetThis()
   如果当前线程还没有协程的时候，都会创建一个主协程
   主协程 -> swapin -> 子协程
            Yield <-
Thread->main_fiber <----> sub_fiber
            ^
            |
            V
        sub_fiber
        
主协程创建子协程、子协程执行完之后回到主协程
```

协程调度模块 scheduler

```c++
        1 : N     1 : M
scheduler -> thread -> fiber

1. 线程池，分配一组线程
2. 协程调度器，将协程，指定到相应的线程上去执行

m_threads
<function<void()>>, fiber, threadid> m_fibers // 支持在特定线程上执行
schedule(fubnc/fiber)   // 把一个函数扔进去，让他以协程的方式执行

start()
stop()  // 等待协程调度器所有的任务执行完毕
run()
1. 设置当前线程的 scheduler
2. 设置当前线程的 run, fiber
3. 协程调度循环 while(true)
    1. 协程消息队列里面是否有任务
    2. 无任务执行，执行 idle 协程
```

```c++
IOManager (epoll) --> Scheduler
      |
      |
      V
     idle (epoll_wait) 
// 基于 Schedule 实现 idle 方法

    信号量
PutMessage(msg,) +信号量1
message_queue
    |
    |----Thread
    |----Thread
        wait()-信号量1，RecvMessage(msg)
        
异步 IO，等待数据返回。在 epoll_wait 中等待。
socket_pair pipe，从 epoll_wait 中唤醒，然后再去读取数据。处理实时消息队列请求。

epoll_create, epoll_ctl, epoll_wait

基于 epoll 来做线程池的调度。
- 当线程没有事情可做的时候，或者要做的事情做完了，
或者要做的事情是一个异步IO的时候，就会陷入 epoll_wait ，
如果这个时候外部有事件通知到我们的话，往句柄里面写数据，就会唤醒 epoll_wait ，让其从消息队列里面拿出消息执行。

这样的话，就不用使用信号量阻塞线程，因为一旦信号量阻塞线程，那么那些异步的回调方法是没办法接收的。
```


## socket 函数库

## http 协议开发

## 分布协议

## 推荐系统
