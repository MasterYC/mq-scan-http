#include"HttpsClient.h"
#include<fstream>
#include"ThreadPool.h"
#include"IpQuery.h"
#include"Config.h"
using namespace boost;

extern boost::asio::io_context ioc;
extern unsigned long long scan_sums;
extern std::ofstream error_log;
extern std::ofstream status_log;
extern std::ofstream json_log;

HttpsClient::HttpsClient(asio::io_context& io_context,const std::string& ip,unsigned short port,const std::function<void(const std::string&)>& call,const std::string& suffi,const std::string& d_url,size_t _crack_current_url):
ctx{asio::ssl::context::tlsv12_client},endpoint{asio::ip::address::from_string(ip),port},request{beast::http::verb::get,"/",11},callback{call},ic{io_context},error_nums{0},timer{ic},suffix{suffi},dns_url(d_url)
,crack_url_current(_crack_current_url)
{

    request.set(beast::http::field::host,ip); 
    ctx.set_default_verify_paths();
    //ctx.set_verify_mode(asio::ssl::verify_peer); 
    socket=std::make_unique<beast::ssl_stream<beast::tcp_stream>>(io_context,ctx);
}

HttpsClient::~HttpsClient()
{
    socket->next_layer().close();
}

void HttpsClient::run()
{
    if(crack_url_current==-1)request.target("/");
    else request.target(Config::getInstance().crack_url[crack_url_current]);
    socket->next_layer().async_connect(endpoint,std::bind(&HttpsClient::onConnect,shared_from_this(),std::placeholders::_1));
    setTimer();
}

void HttpsClient::onWait(const std::error_code& ec)
{
    if(ec)return;
    socket->next_layer().cancel();
}

void HttpsClient::onConnect(const std::error_code& ec)
{
    timer.cancel();
    if(ec)
    {
        Log(ec);
        if(Error_nums())return;
        onError();
        return;
    }
    socket->next_layer().socket().set_option(boost::asio::ip::tcp::socket::linger{true,0});
    socket->next_layer().socket().set_option(boost::asio::ip::tcp::socket::reuse_address{true});
    setTimer();
    socket->async_handshake(asio::ssl::stream_base::client,std::bind(&HttpsClient::onShake,shared_from_this(),std::placeholders::_1));
}

void HttpsClient::onShake(const std::error_code& ec)
{
    timer.cancel();
    if(ec)
    {
        Log(ec);
        if(Error_nums())return;
        if(ec.value()==ECANCELED)
        {
            setTimer();
            socket->async_handshake(asio::ssl::stream_base::client,std::bind(&HttpsClient::onShake,shared_from_this(),std::placeholders::_1));
            return;
        }
        onError();
        return;
    }
    setTimer();
    beast::http::async_write(*socket.get(),request,std::bind(&HttpsClient::onWrite,shared_from_this(),std::placeholders::_1,std::placeholders::_2));
}

void HttpsClient::onWrite(const std::error_code& ec,std::size_t size)
{
    timer.cancel();
    if(ec)
    {
        Log(ec);
        if(Error_nums())return;
        if(ec.value()==ECANCELED)
        {
            setTimer();
            beast::http::async_write(*socket.get(),request,std::bind(&HttpsClient::onWrite,shared_from_this(),std::placeholders::_1,std::placeholders::_2));
            return;
        }
        onError();
        return;
    }
    setTimer();
    beast::http::async_read(*socket.get(),buffer,response,std::bind(&HttpsClient::onRead,shared_from_this(),std::placeholders::_1,std::placeholders::_2));
}

void HttpsClient::onRead(const std::error_code& ec,std::size_t size)
{
    timer.cancel();
    if(ec)
    {
        Log(ec);
        if(Error_nums())return;
        if(ec.value()==ECANCELED)
        {
            setTimer();
            beast::http::async_read(*socket.get(),buffer,response,std::bind(&HttpsClient::onRead,shared_from_this(),std::placeholders::_1,std::placeholders::_2));
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
    if(!dns_url.empty())url="https://"+dns_url;
    else url="https://"+ip_str+':'+std::to_string(endpoint.port())+std::string{request.target()}; 
    auto now = std::chrono::system_clock::now();
    auto tt = std::chrono::system_clock::to_time_t(now);
    struct tm* tmNow = localtime(&tt);
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

    auto ssl=socket->native_handle();
    auto x509=SSL_get_peer_certificate(ssl);

    json::object obj;
    json::object res;
    json::object cert;
    json::array subject_alternative_names;

    res["body"]=body;
    res["headers"]=headers.str();
    res["url"]=url;
    res["statusCode"]=statusCode;
    res["has_ssl_cert"]=true;
    res["server"]=response[beast::http::field::server];
    res["title"]=title;
    res["statusLine"]=statusLine;

    cert["certificate"]=get_pem(x509);
    cert["serial_number"]=get_ASN1_INT(X509_get_serialNumber(x509));
    cert["issuerDN"]=get_name(X509_get_issuer_name(x509));
    cert["signature_algorithm"]=OBJ_nid2ln(X509_get_signature_nid(x509));
    cert["subjectDN"]=get_name(X509_get_subject_name(x509));
    cert["not_before"]=get_date(X509_get_notBefore(x509));
    cert["not_after"]=get_date(X509_get_notAfter(x509));
    get_subject_alt_names(x509,subject_alternative_names);
    cert["subject_alternative_names"]=subject_alternative_names;
    cert["version"]=X509_get_version(x509)+1;
    cert["publicKey"]=get_publickey(x509);
    cert["subjectDN_domain"]=get_subject_domain(x509);
    cert["cert_fingerprint"]=get_fingerprint(x509);

    obj["url"]=url;
    obj["ip_str"]=ip_str;
    obj["scan_time"]=scan_time;
    obj["port"]=endpoint.port();
    obj["ip_isp"]=std::get<2>(ip_message);
    obj["suffix"]=suffix;
    obj["protocol"]="https";
    obj["ip_region"]=std::get<1>(ip_message);
    obj["has_ssl_cert"]=true;
    if(!dns_url.empty())obj["domain"]=dns_url;
    
    obj["response"]=res;
    obj["cert"]=cert;

    if(Config::getInstance().status_log_open)ThreadPool::getInstance().getLogIC().post([url,statusCode]{status_log<<url<<' '<<statusCode<<'\n';});
    if(Config::getInstance().json_log_open)ThreadPool::getInstance().getLogIC().post([obj]{json_log<<obj<<'\n';});
    ThreadPool::getInstance().CompleteTask(statusCode/100);
    callback(json::serialize(obj));

    if(!Config::getInstance().crack_open)return;
    crack_url_current++;
    if(crack_url_current<Config::getInstance().crack_url_size)
    {
        ioc.post(
            [ip_str,endpoint=this->endpoint,callback=this->callback,suffix=this->suffix,dns_url=this->dns_url,crack_url_current=this->crack_url_current]
            {
                scan_sums++;
                auto https=std::make_shared<HttpsClient>(ThreadPool::getInstance().getIocontext(),ip_str,endpoint.port(),callback,suffix,dns_url,crack_url_current);
                https->run();
            }
        );
    }

}

void HttpsClient::setTimer()
{
    timer.expires_from_now(std::chrono::seconds{Config::getInstance().time_out});
    timer.async_wait(std::bind(&HttpsClient::onWait,shared_from_this(),std::placeholders::_1));
}

bool HttpsClient::Error_nums()
{
    error_nums++;
    if(error_nums==Config::getInstance().retry_nums)
    {
        ThreadPool::getInstance().CompleteTask(-1);
        std::string url="https://"+endpoint.address().to_string()+':'+std::to_string(endpoint.port());
        ThreadPool::getInstance().getLogIC().post([url]{ error_log<<url<<' '<<"task failed\n";});
        return true;
    }
    return false;
}

void HttpsClient::onError()
{
    socket->next_layer().close();
    socket->next_layer().async_connect(endpoint,std::bind(&HttpsClient::onConnect,shared_from_this(),std::placeholders::_1));
    setTimer();
}

void HttpsClient::Log(const std::error_code& ec)
{
    ThreadPool::getInstance().getLogIC().post(
      [ec]
      {
        error_log<<ec.value()<<':'<<ec.message()<<'\n';
      }  
    );
}

std::string HttpsClient::get_pem(X509* x509)
{
    BIO* bio = BIO_new(BIO_s_mem());
	PEM_write_bio_X509(bio, x509);
	BUF_MEM* buf;
	BIO_get_mem_ptr(bio, &buf);
	std::string result(buf->data, buf->length);
	BIO_free(bio);
	return result;
}

std::string HttpsClient::get_fingerprint(X509* x509)
{
    static const char hexbytes[] = "0123456789ABCDEF";
	unsigned int md_size;
	unsigned char md[EVP_MAX_MD_SIZE];
	const EVP_MD* digest = EVP_get_digestbyname("sha1");
	X509_digest(x509, digest, md, &md_size);
    std::string result;
    result.reserve(50);
	for (unsigned i = 0; i < md_size; i++)
	{
        result.push_back(hexbytes[(md[i] & 0xf0) >> 4]);
        result.push_back(hexbytes[(md[i] & 0x0f) >> 0]);
	}
	return result;
}

std::string HttpsClient::get_subject_domain(X509* x509)
{
    auto name=X509_get_subject_name(x509);
    std::string result;
    result.reserve(40);
    int size=X509_NAME_entry_count(name);
    for(int i=0;i<size;i++)
    {
        auto entry=X509_NAME_get_entry(name,i);
        auto value=X509_NAME_ENTRY_get_data(entry);
        auto obj=X509_NAME_ENTRY_get_object(entry);
        auto key=OBJ_nid2sn(OBJ_obj2nid(obj));
        if(std::string_view(key)=="CN")
        {
            result.append(get_ASN1_STRING(value));
            break;
        }

    }
    return result;
}

std::string HttpsClient::get_publickey(X509* x509)
{
    auto bio=BIO_new(BIO_s_mem());

    auto pkey=X509_get_pubkey(x509);
    std::string result=EVP_PKEY_get0_type_name(pkey);
    result.reserve(3000);
    result.push_back(' ');
    EVP_PKEY_print_public(bio,pkey,0,NULL);
    EVP_PKEY_free(pkey);

    BUF_MEM* buf;
    BIO_get_mem_ptr(bio,&buf);
    result.append(buf->data,buf->length);
    BIO_free(bio);
    return result;
}

void HttpsClient::get_subject_alt_names(X509* x509,boost::json::array& array)
{   
    auto names=(const stack_st_GENERAL_NAME*)X509_get_ext_d2i(x509,NID_subject_alt_name,NULL,NULL);
    for(int i=0;i<sk_GENERAL_NAME_num(names);i++)
    {
        auto gen=sk_GENERAL_NAME_value(names,i);
        if(gen->type==GEN_URI||gen->type==GEN_DNS||gen->type==GEN_EMAIL)
        {
            auto str=gen->d.uniformResourceIdentifier;
            std::string_view value((char*)ASN1_STRING_get0_data(str),ASN1_STRING_length(str));
            array.emplace_back(value);
        }
        else if(gen->type==GEN_IPADD)
        {
            auto data=gen->d.ip->data;
            if(gen->d.ip->length==4)
            {
                std::string value;
                value.reserve(20);
                for(int j=0;j<4;j++)value.append(std::to_string((int)data[j]));
                array.emplace_back(value);
            }
        }
    }
}

std::string HttpsClient::get_date(ASN1_TIME* time)
{
    std::string result;
    result.reserve(20);
    auto t=time->data;
    int i=0;
    int year=0,month=0,day=0,hour=0,minute=0,second=0;
	if (time->type == V_ASN1_UTCTIME) 
    {
		year = (t[i++] - '0') * 10;
		year += (t[i++] - '0');
		year += (year < 70 ? 2000 : 1900);
	}
	else if (time->type == V_ASN1_GENERALIZEDTIME) 
    {
		year = (t[i++] - '0') * 1000;
		year += (t[i++] - '0') * 100;
		year += (t[i++] - '0') * 10;
		year += (t[i++] - '0');
	}
    result.append(std::to_string(year));
    result.push_back('-');
    result.push_back(t[i++]);
    result.push_back(t[i++]);
    result.push_back('-');
    result.push_back(t[i++]);
    result.push_back(t[i++]);
    result.push_back(' ');
    result.push_back(t[i++]);
    result.push_back(t[i++]);
    result.push_back(':');
    result.push_back(t[i++]);
    result.push_back(t[i++]);
    result.push_back(':');
    result.push_back(t[i++]);
    result.push_back(t[i++]);
    return result;
}

std::string HttpsClient::get_name(X509_NAME* name)
{
    std::string result;
    result.reserve(40);
    int size=X509_NAME_entry_count(name);
    for(int i=0;i<size;i++)
    {
        auto entry=X509_NAME_get_entry(name,i);
        auto value=X509_NAME_ENTRY_get_data(entry);
        auto obj=X509_NAME_ENTRY_get_object(entry);
        auto key=OBJ_nid2sn(OBJ_obj2nid(obj));
        result.append(key);
        result.push_back('=');
        result.append(get_ASN1_STRING(value));
        if(i==size-1)break;
        result.append(", ");
    }
    return result;
}

std::string HttpsClient::get_ASN1_INT(ASN1_INTEGER* ai)
{  
    static const char hexbytes[] = "0123456789ABCDEF";
    std::string result;
    result.reserve(40);
    for(int i=0; i<ai->length; i++)
    {
        result.push_back(hexbytes[(ai->data[i]&0xf0)>>4]);
        result.push_back(hexbytes[(ai->data[i]&0x0f)>>0]);
    }
    return result;
}

std::string HttpsClient::get_ASN1_STRING(ASN1_STRING* as)
{
    std::string result;
	if (ASN1_STRING_type(as) != V_ASN1_UTF8STRING) 
    {
		unsigned char* utf8;
		int length = ASN1_STRING_to_UTF8(&utf8, as);
		result = std::string((char*)utf8, length);
		OPENSSL_free(utf8);
	}
	else 
    {
		result = std::string((char*)ASN1_STRING_get0_data(as), ASN1_STRING_length(as));
	}
	return result;
}