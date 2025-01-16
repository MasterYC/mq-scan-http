# 实习项目报告

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

程序主要功能是向Rabbitmq消费数据，数据格式如[1.json](./bin/1.json)

{

 "suffix": "2022_11",

 "type": 1,

 "urlList": [

  "http://5.161.87.253:80",

  "http://8.142.88.21:9000",

  "http://8.142.94.159:9002",

  "http://5.161.87.250:5985",

  "http://5.161.64.120:80",

  "http://8.142.76.238:80",

  "http://8.142.94.159:9002",

  "http://8.142.64.55:8888"

 ]

}

所示，suffix代表日期；type代表该json下的urlList里面的url是否需要http访问，1代码需要访问；urlList里面的url是需要发起http连接的url。然后该程序会根据url的不同发起http或者https连接，将访问的结果格式化为如[2.json](./bin/2.json)所示

{
    "url": "http://116.211.180.239:80/",
    "ip_str": "116.211.180.239",
    "scan_time": "2023-02-17 12:29:37",
    "port": 80,
    "ip_isp": "电信",
    "suffix": "domain_",
    "protocol": "http",
    "ip_region": "湖北省恩施州",
    "has_ssl_cert": false,
    "domain": "www.163.com",
    "response": {
        "body": "<html>\r\n<head><title>404 Not Found</title></head>\r\n<body>\r\n<center><h1>404 Not Found</h1></center>\r\n<hr><center>dx-hubei-wuhan-6-116-211-180-141</center>\r\n<hr><center>nginx</center>\r\n</body>\r\n</html>\r\n",
        "headers": "Server:nginx Date:Fri, 17 Feb 2023 04:29:38 GMT Content-Type:text/html Content-Length:201 Connection:keep-alive X-Frame-Options:SAMEORIGIN ",
        "url": "http://116.211.180.239:80/",
        "statusCode": 404,
        "has_ssl_cert": false,
        "server": "nginx",
        "title": "404 Not Found",
        "statusLine": "HTTP/1.1 404 Not Found"
    }
}

。然后再上传到Rabbitmq的服务器中，各个字段具体是什么功能就不用再说了，看名字就能看出来。

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

## 主要模块代码说明

###### RabbitMq

该模块主要是消费RabbitMq，将消费下来的数据进行JSON反序列化，将其中提取到的url建立Http/Https连接，并将该任务提交到线程池进行，之后将处理好的数据上传到RabbitMq。其他功能还包括记录消费的总数和访问成功的数量以及成功率等，每隔一段时间进行打印。

![image-20230401190539888](C:\Users\yc\AppData\Roaming\Typora\typora-user-images\image-20230401190539888.png)

###### HttpClient

该模块主要处理Http的连接，根据获得的url和port进行访问，然后将访问之后的结果提取关键信息序列化为JSON，提交给RabbitMq模块的上传任务队列，由它来处理上传。其他功能还包括错误日志的打印，错误重传以及超时重传。对于HttpsClient模块，我下面不再介绍，多了ssl/tls的握手和ssl/tls证书的信息提取。

![image-20230401192420247](C:\Users\yc\AppData\Roaming\Typora\typora-user-images\image-20230401192420247.png)

###### ThreadPool

该模块主要实现了线程池的功能，每个线程有一个任务调度器，通过将任务提交给任务调度器，任务调度器会维护一个任务队列来顺序执行，同时该模块还需要维护总的并发量，以确保连接不会失败太多，每次提交连接任务的时候都会检查并发量是否超过最大值，超过则阻塞消费线程，每次连接任务结束时会检查并发量是否小于最小值，如果小于则唤醒消费Rabbitmq的线程。

![image-20230401200847393](C:\Users\yc\AppData\Roaming\Typora\typora-user-images\image-20230401200847393.png)

###### Config

该模块主要是处理配置文件的输入，以及配置文件的解析和配置变量的保存，该模块较为简单，但也很重要，这里简单叙述一下。

###### 还有其他一些模块，可以在源代码中查看。

## 实现过程中主要遇到的问题

###### 内存访问异常的问题

在一开始写代码的时候，更多的是关注了一些库的使用以及软件整体的架构，所以对于一些细节问题给忽略了。第一次测试就出现内存访问异常的问题，这是因为有个成员变量的初始化延后，但是建立连接的回调函数已经执行了却没有找到该变量的内存。经过分模块多方排查才找出问题所在。

###### RabbitMq连接读写冲突的问题

第一次测试不仅仅出现了内存访问异常的问题，还有多线程的冲突问题，多线程同时读写一个连接是有问题的，因为一个tcp连接（Rabbitmq是基于tcp协议的）的读写缓冲区是一个临界区。我没有加锁来控制多线程的互斥，所以导致一直出现异常。最后我是通过只使用一个线程来解决的，因为经过测试对于RabbitMQ来说只需要一个线程来消费和上传就够了，这不是程序运行速度的瓶颈。对于Http的连接是没有这个问题的，因为一个http的生命周期都在一个线程的任务调度器上面。

###### 生产环境运行程序的问题

当我第一次上生产环境（远程Linux服务器）运行时发生了无法运行的问题，即使我将所需要用到的动态库复制到同一路径下也不行，但是对于Windows来说是可行的。然后我尝试使用静态编译，虽然编译成功但是有警告信息，这是静态编译带来的glibc版本问题，一开始的时候在生产环境运行没什么问题，但是当我需要使用DNS解析的时候，我发现就无法运行了，警告信息也提示我，对于gethostbyname这个系统调用会产生意想不到的问题（由于我写代码的环境和生产环境glibc版本不一致），所以最终还是要回到动态编译上面。经过各种查找资料发现可以在编译选项上添加参数来设置动态库的搜寻路径，对于Linux来说，默认的搜索路径是通过ldconfig来设置的，但是我不能随便改这个，因为很多其他程序可能也依赖于某个动态库。所以我需要在编译选项上面手动设置动态链接器的路径以及动态库的路径。

```
set_target_properties(Main PROPERTIES LINK_FLAGS "-Wl,-rpath,./lib,-dynamic-linker,./lib/ld-linux-x86-64.so.2")
```

###### 程序运行速度太慢的问题

当我第一次开始测试我的程序时，发现了程序运行缓慢的问题，程序会在一开始十分快速的运行，通过观察Rabbitmq的消费速度，以及日志输出的成功个数可以观察出来。但是当运行了不到一分钟之后程序速度变的十分缓慢。这个问题排查了很久也没发现到底哪里有问题，最后老板带我测试了一下，发现了问题所在。原来是大量的tcp连接都没有断开，处于TIME_WAIT状态。后续查资料发现，原来对于TCP连接，在客户端主动断开连接之后会进入一个TIME_WAIT的状态（因为我一个http连接处理完肯定就不用了，直接回收内存了），这个时间会持续1-4分钟，相当于所有连接都会等待这么长时间才能释放，我才能重新建立连接，导致整个程序运行缓慢。最后的解决方法是设置socket属性，当我断开连接时立马释放连接和端口。

###### DNS解析性能瓶颈的问题

在后续的需求中又增加了域名的Http访问功能，对于域名访问，首先需要解析域名获取ip地址，之后再进行之前实现的http访问即可，但是速度十分缓慢，很明显是DNS解析速度太慢的问题。发现本地DNS客户端流量速度很慢，试过修改DNS服务器但是几乎没有效果。最后想到可以用UDP实现DNS协议，这样我还可以指定DNS服务器，然后我又从网上搜寻了很多DNS服务器的IP地址放到DNS的数组里面来随机使用，这样下来我的程序速度又上去了。

###### 死锁的问题

在一开始处理DNS解析的需求的时候，我DNS执行的线程也是使用的线程池的线程。在我第一次测试的时候又发现了问题，程序会在运行一小会之后直接不动了，也就是Rabbitmq不消费任何数据了，也没有任何流量了。经排查发现是代码设计的问题。因为在没有DNS解析需求之前，提交HTTP任务都是Rabbitmq消费线程来提交的，这样当并发数到达一个峰值就会阻塞也是阻塞的是该线程，当连接完成时（连接用的线程池的线程），会检查并发数是否小于最小并发数，如果小于则唤醒消费线程继续消费，但是如果提交HTTP任务的是线程池的线程会怎么样（DNS执行在线程池中），线程池阻塞了，也就没有人来唤醒自己了，这就会导致死锁。最后的解决方法是将提交HTTP连接这个行为让MQ线程来提交（相当于把这个行为提交到MQ的任务调度器上面，我的代码中每个线程都有一个任务调度器，不只是线程池有）。

###### 当然，还有其他小问题，更多的学习过程中遇到的问题，很多库都是第一次接触所以需要查询很多资料来查找正确使用方法以及其中的坑，但是中文资料也很少，需要看很多英文资料。
