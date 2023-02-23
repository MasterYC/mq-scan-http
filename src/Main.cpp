#include"RabbitMq.h"
int main()
{
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    // boost::asio::io_context ic;
    // auto dns=std::make_shared<DnsClient>(ic,"www.baidu.com",f,"123");
    // dns->run();
    // ic.run();

    
    std::cout<<"start"<<std::endl;
    auto mq=std::make_shared<RabbitMq>();
    mq->run();
    std::cout<<"end"<<std::endl;
}

