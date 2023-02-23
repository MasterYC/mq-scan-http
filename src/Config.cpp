#include"Config.h"
#include<fstream>
#include<sstream>
#include<boost/json.hpp>
#include<iostream>
Config::Config(const std::string& file)
{
    try 
    {
        std::ifstream ifs{file};
        std::stringstream ss;
        std::string temp;
        while(ifs>>temp)
        {
            ss<<temp;
        }
        auto obj=boost::json::parse(ss.str()).as_object();
        thread_nums=obj["thread_nums"].as_int64();
        time_out=obj["time_out"].as_int64();
        retry_nums=obj["retry_nums"].as_int64();
        min_tasks=obj["min_tasks"].as_int64();
        max_tasks=obj["max_tasks"].as_int64();
        ip=obj["ip"].as_string();
        port=obj["port"].as_int64();
        virtual_host=obj["virtual_host"].as_string();
        user=obj["user"].as_string();
        password=obj["password"].as_string();
        consume_queue=obj["consume_queue"].as_string();
        publish_routingkey=obj["publish_routingkey"].as_string();
        publish_exchange=obj["publish_exchange"].as_string();
        status_log_open=obj["status_log_open"].as_bool();
        log_per_times=obj["log_per_times"].as_int64();
        json_log_open=obj["json_log_open"].as_bool();
        auto crack_url_array=obj["crack_url"].as_array();
        for(auto it=crack_url_array.begin();it!=crack_url_array.end();it++)
        {
            crack_url.emplace_back(it->as_string());
        }
        crack_url_size=crack_url.size();
        crack_open=obj["crack_open"].as_bool();
    } 
    catch (std::exception& e) 
    {
        std::cout<<"config file error"<<std::endl;
    }  
}

Config& Config::getInstance()
{
    return instance;
}