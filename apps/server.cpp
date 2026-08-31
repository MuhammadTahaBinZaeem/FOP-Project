#include "pocket_engineer/engine.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
using socket_handle = SOCKET;
constexpr socket_handle invalid_socket_handle = INVALID_SOCKET;
void close_socket(socket_handle value) { (void)closesocket(value); }
#else
#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
using socket_handle = int;
constexpr socket_handle invalid_socket_handle = -1;
void close_socket(socket_handle value) { (void)close(value); }
#endif

namespace {
constexpr std::size_t max_request_bytes=1024U*1024U;

std::string read_file(const std::filesystem::path& path) {
    std::ifstream file(path,std::ios::binary);
    std::ostringstream out;
    out<<file.rdbuf();
    return out.str();
}

std::string header(std::string_view status,std::string_view type,std::size_t length) {
    return "HTTP/1.1 "+std::string(status)+"\r\nContent-Type: "+std::string(type)+"\r\nContent-Length: "+std::to_string(length)+"\r\nAccess-Control-Allow-Origin: *\r\nConnection: close\r\n\r\n";
}

bool send_all(socket_handle client,std::string_view text) {
    std::size_t sent{};
    while(sent<text.size()) {
        const auto remaining=text.size()-sent;
        const auto chunk=(std::min)(remaining,static_cast<std::size_t>(std::numeric_limits<int>::max()));
        const int count=send(client,text.data()+sent,static_cast<int>(chunk),0);
        if(count<=0) return false;
        sent+=static_cast<std::size_t>(count);
    }
    return true;
}

void send_response(socket_handle client,std::string_view status,std::string_view type,const std::string& body) {
    const auto response=header(status,type,body.size())+body;
    (void)send_all(client,response);
}

std::string field(std::string_view text,std::string_view key) {
    const auto tag="\""+std::string(key)+"\"";
    auto pos=text.find(tag);
    if(pos==std::string_view::npos) return {};
    pos=text.find(':',pos+tag.size());
    if(pos==std::string_view::npos) return {};
    ++pos;
    while(pos<text.size()&&std::isspace(static_cast<unsigned char>(text[pos]))) ++pos;
    if(pos>=text.size()||text[pos]!='\"') return {};
    std::string value;
    bool escaped=false;
    for(++pos;pos<text.size();++pos) {
        const char c=text[pos];
        if(escaped) {
            switch(c) {
                case 'n': value+='\n';break;
                case 'r': value+='\r';break;
                case 't': value+='\t';break;
                default: value+=c;break;
            }
            escaped=false;
        } else if(c=='\\') {
            escaped=true;
        } else if(c=='\"') {
            return value;
        } else {
            value+=c;
        }
    }
    return {};
}

std::string content_type(const std::filesystem::path& path) {
    const auto extension=path.extension().string();
    if(extension==".html") return "text/html; charset=utf-8";
    if(extension==".css") return "text/css; charset=utf-8";
    if(extension==".js") return "application/javascript";
    if(extension==".json"||extension==".webmanifest") return "application/manifest+json";
    if(extension==".png") return "image/png";
    return "application/octet-stream";
}

bool enable_reuse(socket_handle server) {
    int yes=1;
#ifdef _WIN32
    return setsockopt(server,SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&yes),sizeof(yes))==0;
#else
    return setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(yes))==0;
#endif
}
}

int main(int argc,char** argv) {
#ifdef _WIN32
    WSADATA socket_data{};
    if(WSAStartup(MAKEWORD(2,2),&socket_data)!=0) {
        std::cerr<<"WSAStartup failed\n";
        return 1;
    }
#endif
    const int port=argc>1?std::stoi(argv[1]):8080;
    const auto root=std::filesystem::path(argc>2?argv[2]:"www");
    const socket_handle server=socket(AF_INET,SOCK_STREAM,0);
    if(server==invalid_socket_handle) {
        std::cerr<<"socket failed\n";
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    (void)enable_reuse(server);
    sockaddr_in address{};
    address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_ANY);
    address.sin_port=htons(static_cast<std::uint16_t>(port));
    if(bind(server,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0||listen(server,16)!=0) {
        std::cerr<<"Cannot listen on port "<<port<<"\n";
        close_socket(server);
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    std::cout<<"Pocket Engineer offline site: http://127.0.0.1:"<<port<<"\n";
    for(;;) {
        const socket_handle client=accept(server,nullptr,nullptr);
        if(client==invalid_socket_handle) {
#ifndef _WIN32
            if(errno==EINTR) continue;
#endif
            break;
        }
        std::string request;
        std::array<char,8192> buffer{};
        bool too_large=false;
        for(;;) {
            const int count=recv(client,buffer.data(),static_cast<int>(buffer.size()),0);
            if(count<=0) break;
            request.append(buffer.data(),static_cast<std::size_t>(count));
            if(request.size()>max_request_bytes) {
                too_large=true;
                break;
            }
            const auto split=request.find("\r\n\r\n");
            if(split==std::string::npos) continue;
            const std::string marker="Content-Length: ";
            const auto content_length_at=request.find(marker);
            std::size_t content_length{};
            if(content_length_at!=std::string::npos) content_length=static_cast<std::size_t>(std::stoul(request.substr(content_length_at+marker.size())));
            if(request.size()>=split+4+content_length) break;
        }
        if(too_large) {
            send_response(client,"413 Payload Too Large","text/plain","Request exceeds 1 MiB limit");
            close_socket(client);
            continue;
        }
        const auto line_end=request.find("\r\n");
        const auto line=request.substr(0,line_end);
        const auto body_at=request.find("\r\n\r\n");
        const auto body=body_at==std::string::npos?"":request.substr(body_at+4);
        if(line.rfind("POST /api/identify ",0)==0) {
            auto input=field(body,"input");
            if(input.empty()) input=field(body,"question");
            send_response(client,"200 OK","application/json",pocket_engineer::Engine{}.identify(input).to_json());
        } else if(line.rfind("POST /api/solve ",0)==0) {
            pocket_engineer::ProblemSpec problem{field(body,"domain"),field(body,"topic"),field(body,"input")};
            send_response(client,"200 OK","application/json",pocket_engineer::Engine{}.solve(problem).to_json());
        } else if(line.rfind("GET /api/capabilities ",0)==0) {
            send_response(client,"200 OK","application/json",pocket_engineer::Engine{}.capabilities_json());
        } else if(line.rfind("OPTIONS ",0)==0) {
            send_response(client,"204 No Content","text/plain","");
        } else {
            const auto begin=line.find(' ');
            const auto end=begin==std::string::npos?std::string::npos:line.find(' ',begin+1);
            auto path=end==std::string::npos?"/":line.substr(begin+1,end-begin-1);
            if(path=="/") path="/index.html";
            if(path.find("..")!=std::string::npos) {
                send_response(client,"403 Forbidden","text/plain","Forbidden");
            } else {
                const auto file=root/path.substr(1);
                if(!std::filesystem::is_regular_file(file)) send_response(client,"404 Not Found","text/plain","Not found");
                else send_response(client,"200 OK",content_type(file),read_file(file));
            }
        }
        close_socket(client);
    }
    close_socket(server);
#ifdef _WIN32
    WSACleanup();
#endif
}
