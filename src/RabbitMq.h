#pragma once
#include<boost/asio.hpp>
#include <amqpcpp.h>
#include<amqpcpp/libboostasio.h>


class RabbitMq:public std::enable_shared_from_this<RabbitMq>
{
public:
    RabbitMq();
    ~RabbitMq();
    void run();
    void Publish(const std::string& message);

private:
    std::unique_ptr<AMQP::LibBoostAsioHandler> handler;
    std::unique_ptr<AMQP::TcpConnection> connection;
    std::unique_ptr<AMQP::TcpChannel> ChannelConsume;
    std::unique_ptr<AMQP::TcpChannel> Channelpublish;

    std::string publishQueue;
    std::string consumeQueue;
    std::string exchange;
    AMQP::Address* address;
    boost::asio::steady_timer timer;
    unsigned int log_per_times;

    void onWait(const std::error_code& ec);
    void OnReceived(const AMQP::Message& message, uint64_t deliveryTag, bool redelivered);
    void OnError_con(const char* message);
    void OnError_pub(const char* message);
};