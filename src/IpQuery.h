#pragma once
#include<string>
#include<tuple>
class IpQuery 
{
public:
    IpQuery(const IpQuery&)=delete;
    IpQuery(IpQuery&&)=delete;
    IpQuery& operator=(const IpQuery&)=delete;
    IpQuery& operator=(IpQuery&&)=delete;
    static IpQuery& getInstance();
    
    std::tuple<bool, std::string, std::string> operator()(std::string _ip);
private:
    unsigned long getValue(unsigned long start, int length);
    std::string getString(unsigned long start);
    std::tuple<std::string, std::string> getAddress(unsigned long start);
    void getHead();
    unsigned long searchIP(unsigned long index_start,unsigned long index_end, unsigned long ip);
    bool beNumber(char c);
    unsigned long getIP(std::string &&ip_addr);

    bool state{false};
    std::string m_bytes;
    uint32_t m_index_head = 0;
    uint32_t m_index_tail = 0;
    static IpQuery instance;
    IpQuery(std::string _file);
};
