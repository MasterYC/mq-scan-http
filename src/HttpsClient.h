#pragma once
#include<boost/beast.hpp>
#include<boost/beast/ssl.hpp>
#include <string>
#include<boost/json.hpp>

class HttpsClient :public std::enable_shared_from_this<HttpsClient>
{
public:
    HttpsClient(boost::asio::io_context& io_context,const std::string& ip,unsigned short port,const std::function<void(const std::string&)>& call,const std::string& suffi,const std::string& d_url,size_t _crack_current_url);
    ~HttpsClient();
    void run();
private:
    boost::asio::ssl::context ctx;
    boost::asio::io_context& ic;
    std::unique_ptr<boost::beast::ssl_stream<boost::beast::tcp_stream>> socket;
    boost::asio::ip::tcp::endpoint endpoint;
    boost::beast::multi_buffer buffer;
    boost::beast::http::request<boost::beast::http::dynamic_body> request;
    boost::beast::http::response<boost::beast::http::dynamic_body> response;
    std::function<void(const std::string&)> callback;
    boost::asio::steady_timer timer;
    unsigned error_nums;
    std::string suffix;
    std::string dns_url;
    size_t crack_url_current;

    void onConnect(const std::error_code& ec);
    void onShake(const std::error_code& ec);
    void onWrite(const std::error_code& ec,size_t size);
    void onRead(const std::error_code& ec, size_t size);
    void onWait(const std::error_code& ec);
    void onError();
    bool Error_nums();
    void setTimer();
    void Log(const std::error_code& ec);

    std::string get_name(X509_NAME* name);
    std::string get_ASN1_INT(ASN1_INTEGER* ai);
    std::string get_ASN1_STRING(ASN1_STRING* as);
    std::string get_date(ASN1_TIME* time);
    void get_subject_alt_names(X509* x509,boost::json::array& array);
    std::string get_publickey(X509* x509);
    std::string get_subject_domain(X509* x509);
    std::string get_fingerprint(X509* x509);
    std::string get_pem(X509* x509);
};