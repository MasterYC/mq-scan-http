# Readme

## 如何使用

只需要将bin里面的如下介绍全部复制到Linux系统中即可直接运行

###### [Main](./bin/Main)

主程序，下面所示的文件都必须和它是同一路径

```shell
./Main
```

运行

###### [config.json](./bin/config.json) 配置文件，解释如下

thread_nums	http/https访问的线程数量

time_out	http/https访问的超时时间

retry_nums	http/https发生错误时的重试次数

min_tasks	http/https唤醒继续建立连接的最小连接数量

max_tasks	http/https最大连接数量，超过此数量会阻塞消费，从而阻塞连接的建立

ip	Rabbitmq服务器的ip地址

port	Rabbitmq服务器的端口号

virtual_host	Rabbitmq服务器的virtualhost路径

user	Rabbitmq服务器的登录用户名

password	Rabbitmq服务器的登录密码

consume_queue	该程序需要消费的队列名称

publish_routingkey	该程序需要将http响应信息回传的routingkey

publish_exchange	该程序信息回传的交换机

status_log_open	访问成功的连接是否需要打印日志，包括url 端口和http响应代码

log_per_times	消费连接总数和连接访问成功总数日志的打印时间间隔，单位是分钟

json_log_open	上传的json是否需要打印的开关

crack_url	暴力破解路径的数组集合

crack_open	暴力破解路径的开关	

###### [QQwry.dat](./bin/QQwry.dat)

这是查询ip归属地和ip的运营商的数据库

###### [lib](./bin/lib)

该文件夹是程序所需要的动态库，放在和程序的路径相同的地方即可

## 程序功能说明

程序主要功能是向Rabbitmq消费数据，数据格式如[1.json](./bin/1.json)所示，suffix代表日期；type代表该json下的urlList里面的url是否需要http访问，1代码需要访问；urlList里面的url是需要发起http连接的url。然后该程序会根据url的不同发起http或者https连接，将访问的结果格式化为如[2.json](./bin/2.json)所示。然后再上传到Rabbitmq的服务器中，各个字段具体是什么功能就不用再说了，看名字就能看出来

## 如何编译

###### 我的开发环境是Arch Linux，clang 15.07 libc++，具体使用什么环境编译基本都可以，提前需要编译安装的库如下

[CopernicaMarketingSoftware/AMQP-CPP: C++ library for asynchronous non-blocking communication with RabbitMQ (github.com)](https://github.com/CopernicaMarketingSoftware/AMQP-CPP)

这是处理mq连接消费和上传的C++库，具体编译安装网页已经写的很详细了，这里不再赘述。

[Boost C++ Libraries](https://www.boost.org/)

boost库，需要用到里面的beast(http/https库)，asio(异步IO，网络连接处理、定时器、任务循环)，json(json的序列化与反序列化)。

[openssl/openssl: TLS/SSL and crypto library (github.com)](https://github.com/openssl/openssl)

openssl库，以便于支持https的连接和ssl/tls证书的信息处理。

###### 这里面除了第一个库需要自己编译，其他两个可以直接通过包管理器安装，安装命令如下

```shell
pacman -S boost
pacman -S openssl
```

###### 然后就可以编译该程序了，具体编译方法如下(cmake文件默认使用了clang和libc++，如果需要修改，可以修改cmake文件，我这里没有做判断)

```shell
mkdir build
cd build
cmake ../
make
```

## 如何阅读和修改代码

阅读之前需要熟悉使用上面介绍的库，要不然会一头雾水。整体代码以[Rabbitmq.cpp](./src/Rabbitmq.cpp)为主要架构的，里面包含消费和上传，以及发起HTTP和HTTPS请求，[ThreadPool.cpp](./src/ThreadPool.cpp)处理连接数量的控制功能，具体情况还是见代码里面吧，感觉这里不太好描述，但整体架构就是这样。
