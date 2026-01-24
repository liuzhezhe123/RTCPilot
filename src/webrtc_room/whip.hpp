#ifndef WHIP_HPP
#define WHIP_HPP
#include "net/http/http_server.hpp"
#include <string>
#include <memory>

namespace cpp_streamer
{

class Whip
{
public:
    Whip(uv_loop_t* loop, const std::string& ip, uint16_t port, Logger* logger);
    Whip(uv_loop_t* loop, const std::string& ip, uint16_t port, const std::string& key_file, const std::string& cert_file, Logger* logger);
    virtual ~Whip();

public:
    static bool ssl_enable_;
private:
    uv_loop_t* loop_ = nullptr;
    Logger* logger_ = nullptr;
    std::unique_ptr<HttpServer> http_server_;
};

}
#endif
