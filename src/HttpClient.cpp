#include"HttpClient.h"
#include<fstream>
#include<boost/json/src.hpp>
#include"ThreadPool.h"
#include"IpQuery.h"
#include"Config.h"

using namespace boost;
extern boost::asio::io_context ioc;
extern unsigned long long scan_sums;
extern std::ofstream error_log;
std::ofstream status_log{"status.log"};
std::ofstream json_log{"json.log"};

HttpClient::HttpClient(asio::io_context& io_context,const std::string& ip,unsigned short port,const std::function<void(const std::string&)>& call,const std::string& suffi,const std::string& d_url,size_t _crack_current_url):
socket{io_context},endpoint{asio::ip::address::from_string(ip),port},request{beast::http::verb::get,"/",11},callback{call},ic{io_context},error_nums{0},timer{ic},suffix{suffi},dns_url(d_url),
crack_url_current(_crack_current_url)
{ 
    request.set(beast::http::field::host,ip);
}

HttpClient::~HttpClient()
{
    socket.close();
}

void HttpClient::run()
{
    if(crack_url_current==-1)request.target("/");
    else request.target(Config::getInstance().crack_url[crack_url_current]);
    socket.async_connect(endpoint,std::bind(&HttpClient::onConnect,shared_from_this(),std::placeholders::_1));
    setTimer();
}

void HttpClient::onWait(const std::error_code& ec)
{
    if(ec)return;
    socket.cancel();
}

void HttpClient::onConnect(const std::error_code& ec)
{
    timer.cancel();
    if(ec)
    {
        Log(ec);
        if(Error_nums())return;
        onError();
        return;
    }
    
    socket.socket().set_option(boost::asio::ip::tcp::socket::linger{true,0});    
    socket.socket().set_option(boost::asio::ip::tcp::socket::reuse_address{true});
    beast::http::async_write(socket,request,std::bind(&HttpClient::onWrite,shared_from_this(),std::placeholders::_1,std::placeholders::_2));
    setTimer();
}

void HttpClient::onWrite(const std::error_code& ec,std::size_t size)
{
    timer.cancel();
    if(ec)
    {
        Log(ec);
        if(Error_nums())return;
        if(ec.value()==ECANCELED)
        {
            beast::http::async_write(socket,request,std::bind(&HttpClient::onWrite,shared_from_this(),std::placeholders::_1,std::placeholders::_2));
            setTimer();
            return;
        }
        onError();
        return;
    }
    beast::http::async_read(socket,buffer,response,std::bind(&HttpClient::onRead,shared_from_this(),std::placeholders::_1,std::placeholders::_2));
    setTimer();
}

void HttpClient::onRead(const std::error_code& ec,std::size_t size)
{
    timer.cancel();
    if(ec)
    {
        Log(ec);     
        if(Error_nums())return;
        if(ec.value()==ECANCELED)
        {
            beast::http::async_read(socket,buffer,response,std::bind(&HttpClient::onRead,shared_from_this(),std::placeholders::_1,std::placeholders::_2));
            setTimer();
            return;
        }
        onError();
        buffer.clear();
        return;
    }

    std::string body=beast::buffers_to_string(response.body().data());
    std::string statusLine;
    std::stringstream ss;
    ss<<response.base();
    std::getline(ss,statusLine,'\r');
    std::string ip_str=endpoint.address().to_string();
    std::string url;
    if(!dns_url.empty())url="http://"+dns_url;
    else url="http://"+ip_str+':'+std::to_string(endpoint.port())+std::string{request.target()}; 
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    auto tmNow = localtime(&tt);
    char scan_time[20] = { 0 };
    sprintf(scan_time, "%d-%02d-%02d %02d:%02d:%02d", (int)tmNow->tm_year + 1900, (int)tmNow->tm_mon + 1, (int)tmNow->tm_mday, (int)tmNow->tm_hour, (int)tmNow->tm_min, (int)tmNow->tm_sec);
    std::stringstream headers;
    for(auto it=response.begin();it!=response.end();it++)
    {
        headers<<it->name_string()<<':'<<it->value()<<' ';
    }
    std::string_view title;
    if(body.size()!=0)
    {
        size_t sizes=body.size();
        for(size_t i=0;i<sizes;i++)
        {
            bool break_judge=false;
            if(i+14<sizes&&body[i]=='t'&&body[i+1]=='i'&&body[i+2]=='t'&&body[i+3]=='l'&&body[i+4]=='e'&&body[i+5]=='>')
            {
                for(size_t j=i+6;j<sizes;j++)
                {
                    if(j+7<sizes&&body[j]=='<'&&body[j+1]=='/'&&body[j+2]=='t'&&body[j+3]=='i'&&body[j+4]=='t'&&body[j+5]=='l'&&body[j+6]=='e'&&body[j+7]=='>')
                    {
                        title=std::string_view{body.data()+i+6,j-i-6};
                        break_judge=true;
                        break;
                    }
                }
                if(break_judge)break;
            }
        }
    }
    int statusCode=response.result_int();
    auto ip_message=IpQuery::getInstance()(ip_str);

    json::object obj;
    json::object res;
    res["body"]=body;
    res["headers"]=headers.str();
    res["url"]=url;
    res["statusCode"]=statusCode;
    res["has_ssl_cert"]=false;
    res["server"]=response[beast::http::field::server];
    res["title"]=title;
    res["statusLine"]=statusLine;
    obj["url"]=url;
    obj["ip_str"]=ip_str;
    obj["scan_time"]=scan_time;
    obj["port"]=endpoint.port();
    obj["ip_isp"]=std::get<2>(ip_message);
    obj["suffix"]=suffix;
    obj["protocol"]="http";
    obj["ip_region"]=std::get<1>(ip_message);
    obj["has_ssl_cert"]=false;
    if(!dns_url.empty())obj["domain"]=dns_url;

    obj["response"]=res;

    callback(json::serialize(obj));
    if(Config::getInstance().status_log_open)ThreadPool::getInstance().getLogIC().post([url,statusCode]{status_log<<url<<' '<<statusCode<<'\n';});
    if(Config::getInstance().json_log_open)ThreadPool::getInstance().getLogIC().post([obj]{json_log<<obj<<'\n';});
    ThreadPool::getInstance().CompleteTask(statusCode/100);

    if(!Config::getInstance().crack_open)return;
    crack_url_current++;
    if(crack_url_current<Config::getInstance().crack_url_size)
    {
        ioc.post(
            [ip_str,endpoint=this->endpoint,callback=this->callback,suffix=this->suffix,dns_url=this->dns_url,crack_url_current=this->crack_url_current]
            {
                scan_sums++;
                auto http=std::make_shared<HttpClient>(ThreadPool::getInstance().getIocontext(),ip_str,endpoint.port(),callback,suffix,dns_url,crack_url_current);
                http->run();
            }
        );
    }
}

void HttpClient::setTimer()
{
    timer.expires_from_now(std::chrono::seconds{Config::getInstance().time_out});
    timer.async_wait(std::bind(&HttpClient::onWait,shared_from_this(),std::placeholders::_1));
}

bool HttpClient::Error_nums()
{
    error_nums++;
    if(error_nums==Config::getInstance().retry_nums)
    {
        ThreadPool::getInstance().CompleteTask(-1);
        std::string url="http://"+endpoint.address().to_string()+':'+std::to_string(endpoint.port()); 
        ThreadPool::getInstance().getLogIC().post([url]{ error_log<<url<<' '<<"task failed\n";});
        return true;
    }
    return false;
}

void HttpClient::onError()
{
    socket.close();
    socket.async_connect(endpoint,std::bind(&HttpClient::onConnect,shared_from_this(),std::placeholders::_1));
    setTimer();
}


void HttpClient::Log(const std::error_code& ec)
{
    ThreadPool::getInstance().getLogIC().post(
        [ec]
        {
            error_log<<ec.value()<<':'<<ec.message()<<'\n';
        }
    );
}