#pragma once
#include<boost/asio.hpp>


class DnsClient:public std::enable_shared_from_this<DnsClient>
{

public:
    DnsClient(boost::asio::io_context& io_context,const std::string& u,const std::function<void(const std::string&)>& call,const std::string suffi);
    void run();
    ~DnsClient();
private:
    boost::asio::io_context& ic;
    boost::asio::ip::tcp::resolver resolve;
    std::string url;
    std::function<void(const std::string&)> callback;
    std::string suffix;
    boost::asio::steady_timer timer;

    void onResolve_http(const std::error_code& ec,const boost::asio::ip::tcp::resolver::results_type& result);
    void onResolve_https(const std::error_code& ec,const boost::asio::ip::tcp::resolver::results_type& result);
    void onWait(const std::error_code& ec);
};