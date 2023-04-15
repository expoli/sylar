# sylar

## 开发环境

* ArchLinux
* cmake
* gcc
* boost

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

Config --> Yaml

