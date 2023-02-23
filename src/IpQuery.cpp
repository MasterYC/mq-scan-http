#include"IpQuery.h"
#include<fstream>
#define REDIRECT_MODE_1 0x01
#define REDIRECT_MODE_2 0x02
IpQuery IpQuery::instance{"QQwry.dat"};

IpQuery::IpQuery(std::string _file)
{
    std::ifstream ifs(_file, std::ios::binary);
    if (ifs.is_open()) 
    {
        state = true;
        ifs.seekg(0, std::ios::end);
        size_t _fsz = (size_t) ifs.tellg();
        ifs.seekg(0, std::ios::beg);
        m_bytes.resize(_fsz);
        ifs.read(&m_bytes[0], (std::streamsize) _fsz);
        ifs.close();
        getHead();
    }
}

IpQuery& IpQuery::getInstance()
{
    return instance;
}

std::tuple<bool, std::string, std::string> IpQuery::operator()(std::string _ip)
{
    auto ip = getIP(std::move(_ip));
    if (ip != 0) 
    {
        auto current = searchIP(m_index_head, m_index_tail, ip);
        auto[country, location] = getAddress(getValue(current + 4, 3));
        location=location.substr(0,location.size()-1);
        country=country.substr(0,country.size()-1);
        return std::make_tuple(true, country, location);
    } 
    else return std::make_tuple(false, "", "");
}

unsigned long IpQuery::getValue(unsigned long start, int length) 
{
    unsigned long variable = 0;
    long val[length], i;
    for (i = 0; i < length; i++) 
        val[i] = m_bytes[start++] & 0x000000FF;
    for (i = length - 1; i >= 0; i--) 
        variable = variable * 0x100 + val[i];
    return variable;
}

std::string IpQuery::getString(unsigned long start) 
{
    char val;
    std::string string;
    do 
    {
        val = m_bytes[start++];
        string += val;
    } while (val != 0x00);
    return string;
}

std::tuple<std::string, std::string> IpQuery::getAddress(unsigned long start) 
{
    unsigned long redirect_address, counrty_address, location_address;
    char val;
    std::string country, location;
    start += 4;
    val = (m_bytes[start] & 0x000000FF);
    if (val == REDIRECT_MODE_1) 
    {
        redirect_address = getValue(start + 1, 3);
        if ((m_bytes[redirect_address] & 0x000000FF) == REDIRECT_MODE_2) 
        {
            counrty_address = getValue(redirect_address + 1, 3);
            location_address = redirect_address + 4;
            country = getString(counrty_address);
        }
        else 
        {
            counrty_address = redirect_address;
            country = getString(counrty_address);
            location_address = redirect_address + country.length();
        }
    }
    else if (val == REDIRECT_MODE_2) 
    {
        counrty_address = getValue(start + 1, 3);
        location_address = start + 4;
        country = getString(counrty_address);
    } 
    else 
    {
        counrty_address = start;
        country = getString(counrty_address);
        location_address = counrty_address + country.length();
    }
    if ((m_bytes[location_address] & 0x000000FF) == REDIRECT_MODE_2 ||(m_bytes[location_address + 1] & 0x000000FF) == REDIRECT_MODE_1) 
        location_address = getValue(location_address + 1, 3);
    location = getString(location_address);
    return std::make_tuple(country, location);
}

void IpQuery::getHead()
{
    m_index_head = getValue(0L, 4);
    m_index_tail = getValue(4L, 4);
}

unsigned long IpQuery::searchIP(unsigned long index_start,unsigned long index_end, unsigned long ip) 
{
    unsigned long index_current, index_top, index_bottom;
    unsigned long record;
    index_bottom = index_start;
    index_top = index_end;
    index_current = ((index_top - index_bottom) / 7 / 2) * 7 + index_bottom;
    do 
    {
        record = getValue(index_current, 4);
        if (record > ip) 
        {
            index_top = index_current;
            index_current = ((index_top - index_bottom) / 14) * 7 + index_bottom;
        } 
        else 
        {
            index_bottom = index_current;
            index_current = ((index_top - index_bottom) / 14) * 7 + index_bottom;
        }
    } while (index_bottom < index_current);
    return index_current;
}

bool IpQuery::beNumber(char c)
{
    return (c >= '0' && c <= '9');
}

unsigned long IpQuery::getIP(std::string &&ip_addr)
{
    unsigned long ip = 0;
    int i, j = 0;
    for (i = 0; i < ip_addr.length(); i++) 
    {
        if (ip_addr[i] == '.') 
        {
            ip = ip * 0x100 + j;
            j = 0;
        }
        else 
        {
            if (beNumber(ip_addr[i]))
                j = j * 10 + ip_addr[i] - '0';
            else
                return 0;
        }
    }
    ip = ip * 0x100 + j;
    return ip;
}