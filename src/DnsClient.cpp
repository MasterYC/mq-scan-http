#include"DnsClient.h"
#include"HttpClient.h"
#include"HttpsClient.h"
#include"ThreadPool.h"
#include"Config.h"
#include<fstream>
#include<iostream>
extern std::ofstream error_log;
extern unsigned long long scan_sums;
extern boost::asio::io_context ioc;
DnsClient::DnsClient(boost::asio::io_context& io_context,const std::string& u,const std::function<void(const std::string&)>& call,const std::string suffi)
:ic{io_context},resolve{ic},url{u},callback{call},suffix{suffi},timer{ic}
{
    
}

DnsClient::~DnsClient()
{
    ThreadPool::getInstance().CompleteTask(-1);
}

void DnsClient::run()
{
    //timer.expires_from_now(std::chrono::seconds{Config::getInstance().time_out});
    //timer.async_wait(std::bind(&DnsClient::onWait,shared_from_this(),std::placeholders::_1));
    resolve.async_resolve(boost::asio::ip::tcp::v4(),url,"http",boost::asio::ip::tcp::resolver::address_configured,std::bind(&DnsClient::onResolve_http,shared_from_this(),std::placeholders::_1,std::placeholders::_2));
}

void DnsClient::onWait(const std::error_code& ec)
{
    if(ec)return;
    resolve.cancel();
}

void DnsClient::onResolve_http(const std::error_code& ec,const boost::asio::ip::tcp::resolver::results_type& result)
{
    //timer.cancel();

    if(ec||result.empty())
    {
        ThreadPool::getInstance().getLogIC().post(
            [ec,url=url]
            {
                error_log<<ec.value()<<':'<<ec.message()<<" http://"<<url<<" dns resolve\n";
            }  
        );
        std::cout<<"resolve error"<<std::endl;
        //timer.expires_from_now(std::chrono::seconds{Config::getInstance().time_out});
        //timer.async_wait(std::bind(&DnsClient::onWait,shared_from_this(),std::placeholders::_1));
        resolve.async_resolve(boost::asio::ip::tcp::v4(),url,"https",boost::asio::ip::tcp::resolver::address_configured,std::bind(&DnsClient::onResolve_https,shared_from_this(),std::placeholders::_1,std::placeholders::_2));
        return;
    }
    ioc.post(
        [result=result,callback=callback,suffix=suffix,url=url]
        {
            scan_sums++;
            auto http=std::make_shared<HttpClient>(ThreadPool::getInstance().getIocontext(),result->endpoint().address().to_string(),result->endpoint().port(),callback,suffix,url,-1);
            http->run();
        }
    );
    std::cout<<result->endpoint().address()<<std::endl;
    //timer.expires_from_now(std::chrono::seconds{Config::getInstance().time_out});
    //timer.async_wait(std::bind(&DnsClient::onWait,shared_from_this(),std::placeholders::_1));
    resolve.async_resolve(boost::asio::ip::tcp::v4(),url,"https",boost::asio::ip::tcp::resolver::address_configured,std::bind(&DnsClient::onResolve_https,shared_from_this(),std::placeholders::_1,std::placeholders::_2));
}

void DnsClient::onResolve_https(const std::error_code& ec,const boost::asio::ip::tcp::resolver::results_type& result)
{
    //timer.cancel();
    if(ec||result.empty())
    {
        std::cout<<"resolve error"<<std::endl;
        ThreadPool::getInstance().getLogIC().post(
      [ec,url=url]
        {
            error_log<<ec.value()<<':'<<ec.message()<<" https://"<<url<<" dns resolve\n";
        });  
        return;
    }
    std::cout<<result->endpoint().address()<<std::endl;
    ioc.post(
        [result=result,callback=callback,suffix=suffix,url=url]
        {
            scan_sums++;
            auto https=std::make_shared<HttpsClient>(ThreadPool::getInstance().getIocontext(),result->endpoint().address().to_string(),result->endpoint().port(),callback,suffix,url,-1);
            https->run();
        }
    );
}