#include"RabbitMq.h"
#include"HttpClient.h"
#include"HttpsClient.h"
#include<boost/json.hpp>
#include"ThreadPool.h"
#include<fstream>
#include"Config.h"
#include"DnsClient.h"
using namespace boost;
boost::asio::io_context ioc;
std::ofstream error_log{"error.log"};
std::ofstream sums_log{"sums.log"};
std::ofstream success_log{"success.log"};
unsigned long long success_nums;
unsigned long long nums_2;
unsigned long long nums_3;
unsigned long long nums_4;
unsigned long long scan_sums;
RabbitMq::RabbitMq():timer{ioc}
{
    auto& config=Config::getInstance();
    AMQP::Login login{config.user,config.password};
    consumeQueue=config.consume_queue;
    publishQueue=config.publish_routingkey;
    exchange=config.publish_exchange;
    log_per_times=config.log_per_times;
    address=new AMQP::Address{config.ip,config.port,login,config.virtual_host,false};
    handler=std::make_unique<AMQP::LibBoostAsioHandler >(ioc);
    connection=std::make_unique<AMQP::TcpConnection>(handler.get(),*address);  
    ChannelConsume=std::make_unique<AMQP::TcpChannel>(connection.get());
    ChannelConsume->setQos(200);
    Channelpublish=std::make_unique<AMQP::TcpChannel>(connection.get());
}

RabbitMq::~RabbitMq()
{
    delete address;
}

void RabbitMq::run()
{
    ChannelConsume->onError(std::bind(&RabbitMq::OnError_con,shared_from_this(),std::placeholders::_1));
    Channelpublish->onError(std::bind(&RabbitMq::OnError_pub,shared_from_this(),std::placeholders::_1));
    ChannelConsume->consume(consumeQueue).onReceived(
        std::bind(&RabbitMq::OnReceived,shared_from_this(),std::placeholders::_1,std::placeholders::_2,std::placeholders::_3)
    );

    timer.expires_from_now(std::chrono::minutes{log_per_times});
    timer.async_wait(std::bind(&RabbitMq::onWait,shared_from_this(),std::placeholders::_1));
    boost::asio::io_context::work work{ioc};
    ioc.run();

}

void RabbitMq::onWait(const std::error_code& ec)
{
    if(ec)
    {
        timer.expires_from_now(std::chrono::minutes{log_per_times});
        timer.async_wait(std::bind(&RabbitMq::onWait,shared_from_this(),std::placeholders::_1));
    }
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    double percentage=((double)success_nums)/scan_sums;
    sums_log<<tm->tm_year+1900<<'/'<<tm->tm_mon+1<<'/'<<tm->tm_mday<<' '<<tm->tm_hour<<':'<<tm->tm_min<<" scan sums:"<<scan_sums<<" percentage:"<<percentage<<std::endl;
    scan_sums=0;
    success_log<<tm->tm_year+1900<<'/'<<tm->tm_mon+1<<'/'<<tm->tm_mday<<' '<<tm->tm_hour<<':'<<tm->tm_min
    <<" success sums:"<<success_nums<<" 2**nums:"<<nums_2<<" 3**nums:"<<nums_3<<" 4**nums:"<<nums_4<<std::endl;
    ThreadPool::getInstance().zero_nums();
    timer.expires_from_now(std::chrono::minutes{log_per_times});
    timer.async_wait(std::bind(&RabbitMq::onWait,shared_from_this(),std::placeholders::_1));
}


void RabbitMq::Publish(const std::string& message)
{
    if(Channelpublish.get()!=nullptr)
    {
        ioc.post(
            [message,this]
            {
                if(Channelpublish.get()!=nullptr)Channelpublish->publish(exchange,publishQueue,message);
            }
        );
    }
}

void RabbitMq::OnReceived(const AMQP::Message& message, uint64_t deliveryTag, bool redelivered)
{
    try
    {
        auto obj = boost::json::parse(std::string_view{message.body(),message.bodySize()}).as_object();
        if(obj["type"]==1)
        {
            std::string suffix;
            suffix=obj["suffix"].as_string();
		    auto urlList = obj["urlList"].as_array();
            auto ib=urlList.end();
            for(auto it=urlList.begin();it!=ib;it++)
            {
                std::string url;
                url=it->as_string();
                if(suffix=="domain_")
                {
                    auto dns=std::make_shared<DnsClient>(ThreadPool::getInstance().getIocontext(),url,std::bind(&RabbitMq::Publish,shared_from_this(),std::placeholders::_1),suffix);
                    dns->run();
                    continue;
                }
                scan_sums++;
                bool is_tls;
                std::string ip;
                unsigned short port;
                auto size=url.size();
                size_t i=1,j=1;
                for(;i<size;i++)
                {
                    if(url[i]==':')
                    {
                        if(url[i-1]=='s')is_tls=true;
                        else is_tls=false;
                        j=i+3;
                        i=j;
                        break;
                    }
                }   
                for(;i<size;i++)
                {
                    if(url[i]==':')
                    {
                        ip=url.substr(j,i-j);
                        break;
                    }
                }
                port=std::stoul(url.substr(i+1,size-1));
                //std::cout<<is_tls<<' '<<ip<<' '<<port<<std::endl;

                if(is_tls)
                {
                    auto https=std::make_shared<HttpsClient>(ThreadPool::getInstance().getIocontext(),ip,port,std::bind(&RabbitMq::Publish,shared_from_this(),std::placeholders::_1),suffix,"",-1);
                    https->run();
                }
                else
                {
                    auto http=std::make_shared<HttpClient>(ThreadPool::getInstance().getIocontext(),ip,port,std::bind(&RabbitMq::Publish,shared_from_this(),std::placeholders::_1),suffix,"",-1);
                    http->run();
                }
            }
        }
    }
    catch(std::exception& e)
    {
        ThreadPool::getInstance().getLogIC().post([e]{ error_log<<e.what()<<'\n';});     
    }
    ChannelConsume->ack(deliveryTag);
}

void RabbitMq::OnError_con(const char* message)
{
    connection.reset(new AMQP::TcpConnection{handler.get(),*address});
    ChannelConsume.reset(new AMQP::TcpChannel{connection.get()});
    ChannelConsume->onError(std::bind(&RabbitMq::OnError_con,shared_from_this(),std::placeholders::_1));
    ChannelConsume->setQos(200);
    ChannelConsume->consume(consumeQueue).onReceived(
        std::bind(&RabbitMq::OnReceived,shared_from_this(),std::placeholders::_1,std::placeholders::_2,std::placeholders::_3)
    );
    Channelpublish.reset(new AMQP::TcpChannel{connection.get()});
    Channelpublish->onError(std::bind(&RabbitMq::OnError_pub,shared_from_this(),std::placeholders::_1));
    std::string mes{message};
    ThreadPool::getInstance().getLogIC().post([mes]{error_log<<mes<<'\n';});
}

void RabbitMq::OnError_pub(const char* message)
{
    connection.reset(new AMQP::TcpConnection{handler.get(),*address});
    ChannelConsume.reset(new AMQP::TcpChannel{connection.get()});
    ChannelConsume->onError(std::bind(&RabbitMq::OnError_con,shared_from_this(),std::placeholders::_1));
    ChannelConsume->setQos(200);
    ChannelConsume->consume(consumeQueue).onReceived(
        std::bind(&RabbitMq::OnReceived,shared_from_this(),std::placeholders::_1,std::placeholders::_2,std::placeholders::_3)
    );
    Channelpublish.reset(new AMQP::TcpChannel{connection.get()});
    Channelpublish->onError(std::bind(&RabbitMq::OnError_pub,shared_from_this(),std::placeholders::_1));
    std::string mes{message};
    ThreadPool::getInstance().getLogIC().post([mes]{error_log<<mes<<'\n';});
}