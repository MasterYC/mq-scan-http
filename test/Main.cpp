#include<amqpcpp.h>
#include<amqpcpp/libboostasio.h>
#include<iostream>
#include<thread>
#include<boost/asio.hpp>
#include<fstream>
using namespace std;
using namespace boost;
asio::io_context ic;
AMQP::LibBoostAsioHandler handle{ic};
AMQP::TcpConnection connection{&handle,"amqp://yc:yc@192.168.1.4/"};
//AMQP::TcpChannel chan{&connection};
ifstream ifs{"1.json"};
void f(const string& mes)
{
    //chan.publish("","123",mes);
    //this_thread::sleep_for(1ms);
    ic.post(bind(f,mes));
}
int main()
{
    // string mes;
    // ifs>>mes;
    // ic.post(bind(f,mes));
    ic.run();
}