#pragma once
#include<string>
#include<vector>
class Config
{
public:
    Config(const Config&)=delete;
    Config& operator=(const Config&)=delete;
    Config(Config&&)=delete;
    Config& operator=(Config&&)=delete;
    ~Config()=default;
    static Config& getInstance();
    unsigned thread_nums;
    unsigned time_out;
    unsigned retry_nums;
    unsigned min_tasks;
    unsigned max_tasks;
    std::string ip;
    unsigned short port;
    std::string virtual_host;
    std::string user;
    std::string password;
    std::string consume_queue;
    std::string publish_routingkey;
    std::string publish_exchange;
    bool status_log_open;
    bool json_log_open;
    unsigned log_per_times;
    std::vector<std::string> crack_url;
    size_t crack_url_size;
    bool crack_open;
private:
    static Config instance;
    Config(const std::string& file);
};