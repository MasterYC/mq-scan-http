#pragma once
#include<boost/beast.hpp>
#include <string>

class HttpClient :public std::enable_shared_from_this<HttpClient>
{
public:
    HttpClient(boost::asio::io_context& io_context,const std::string& ip,unsigned short port,const std::function<void(const std::string&)>& call,const std::string& suffi,const std::string& d_url,size_t _crack_current_url);
    ~HttpClient();
    void run();
private:

    boost::asio::io_context& ic;
    unsigned int error_nums;
    std::string suffix;
    std::function<void(const std::string&)> callback;
    boost::beast::tcp_stream socket;
    boost::asio::ip::tcp::endpoint endpoint;
    boost::beast::multi_buffer buffer;
    boost::beast::http::request<boost::beast::http::dynamic_body> request;
    boost::beast::http::response<boost::beast::http::dynamic_body> response;
    boost::asio::steady_timer timer;
    std::string dns_url;
    size_t crack_url_current;

    void onConnect(const std::error_code& ec);
    void onWrite(const std::error_code& ec,size_t size);
    void onRead(const std::error_code& ec, size_t size);
    void onWait(const std::error_code& ec);
    void onError();
    bool Error_nums();
    void setTimer();
    void Log(const std::error_code&ec);
};
