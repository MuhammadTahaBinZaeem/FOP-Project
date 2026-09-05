#include "pocket_engineer/engine.hpp"

#include <algorithm>
#include <array>
#include <charconv>
#include <cctype>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
using Socket = SOCKET;
constexpr Socket invalid_socket = INVALID_SOCKET;
void close_socket(Socket socket) { closesocket(socket); }
#else
#include <arpa/inet.h>
#include <cerrno>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <csignal>
using Socket = int;
constexpr Socket invalid_socket = -1;
void close_socket(Socket socket) { close(socket); }
#endif

namespace {
constexpr std::size_t header_budget = 16384, body_budget = 32768;
struct Request {
    std::string method, target, body;
    std::map<std::string, std::string> headers;
};
std::string lowercase(std::string value) {
    for(char& c:value)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}
std::string trim(std::string value) {
    const auto first=value.find_first_not_of(" \t");
    return first==std::string::npos?"":value.substr(first,value.find_last_not_of(" \t")-first+1);
}
unsigned number(std::string_view text) {
    unsigned result=0;const auto parsed=std::from_chars(text.data(),text.data()+text.size(),result);
    if(parsed.ec!=std::errc{}||parsed.ptr!=text.data()+text.size())throw std::runtime_error("Invalid unsigned number");
    return result;
}
void timeout(Socket socket) {
#ifdef _WIN32
    const DWORD milliseconds=3000;
    setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,reinterpret_cast<const char*>(&milliseconds),sizeof(milliseconds));
    setsockopt(socket,SOL_SOCKET,SO_SNDTIMEO,reinterpret_cast<const char*>(&milliseconds),sizeof(milliseconds));
#else
    const timeval interval{3,0};
    setsockopt(socket,SOL_SOCKET,SO_RCVTIMEO,&interval,sizeof(interval));
    setsockopt(socket,SOL_SOCKET,SO_SNDTIMEO,&interval,sizeof(interval));
#endif
}
void response(Socket client,std::string_view status,std::string_view type,const std::string& body) {
    const std::string headers="HTTP/1.1 "+std::string(status)+"\r\nContent-Type: "+std::string(type)
        +"\r\nContent-Length: "+std::to_string(body.size())
        +"\r\nX-Content-Type-Options: nosniff\r\nReferrer-Policy: no-referrer"
        +"\r\nCache-Control: no-cache\r\nConnection: close\r\n\r\n";
    for(const std::string_view part:{std::string_view(headers),std::string_view(body)}) {
        std::size_t offset=0;
        while(offset<part.size()) {
            const int chunk=static_cast<int>(std::min<std::size_t>(part.size()-offset,65536));
            const int sent=send(client,part.data()+offset,chunk,0);
            if(sent<=0)return;
            offset+=static_cast<std::size_t>(sent);
        }
    }
}
Request read_request(Socket client) {
    std::array<char,4096> buffer{};std::string wire;
    std::size_t separator=std::string::npos;
    while((separator=wire.find("\r\n\r\n"))==std::string::npos) {
        const int received=recv(client,buffer.data(),static_cast<int>(buffer.size()),0);
        if(received<=0)throw std::runtime_error("Incomplete or timed-out request");
        wire.append(buffer.data(),static_cast<std::size_t>(received));
        if(wire.size()>header_budget+body_budget)throw std::runtime_error("Request too large");
        if(wire.find("\r\n\r\n")==std::string::npos&&wire.size()>header_budget)throw std::runtime_error("Headers too large");
    }
    if(separator>header_budget)throw std::runtime_error("Headers too large");
    Request request;std::istringstream lines(wire.substr(0,separator));std::string line,version,extra;
    if(!std::getline(lines,line))throw std::runtime_error("No request line");
    if(!line.empty()&&line.back()=='\r')line.pop_back();
    std::istringstream first(line);
    if(!(first>>request.method>>request.target>>version)||(first>>extra)||(version!="HTTP/1.1"&&version!="HTTP/1.0"))
        throw std::runtime_error("Malformed request line");
    while(std::getline(lines,line)) {
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        const auto colon=line.find(':');
        if(colon==std::string::npos||colon==0)throw std::runtime_error("Malformed header");
        if(!request.headers.emplace(lowercase(line.substr(0,colon)),trim(line.substr(colon+1))).second)
            throw std::runtime_error("Duplicate header");
    }
    if(request.headers.contains("transfer-encoding"))throw std::runtime_error("Chunked requests are unsupported");
    const auto length=request.headers.contains("content-length")?number(request.headers.at("content-length")):0u;
    if(length>body_budget)throw std::runtime_error("Body exceeds 32768 bytes");
    request.body=wire.substr(separator+4);
    while(request.body.size()<length) {
        const int received=recv(client,buffer.data(),static_cast<int>(std::min<std::size_t>(buffer.size(),length-request.body.size())),0);
        if(received<=0)throw std::runtime_error("Incomplete request body");
        request.body.append(buffer.data(),static_cast<std::size_t>(received));
    }
    if(request.body.size()!=length)throw std::runtime_error("Unexpected trailing request bytes");
    return request;
}
std::string mime(const std::filesystem::path& path) {
    const auto extension=path.extension().string();
    if(extension==".html")return "text/html; charset=utf-8";
    if(extension==".css")return "text/css; charset=utf-8";
    if(extension==".js")return "application/javascript; charset=utf-8";
    if(extension==".wasm")return "application/wasm";
    if(extension==".webmanifest")return "application/manifest+json";
    if(extension==".json")return "application/json";
    if(extension==".png")return "image/png";
    if(extension==".webp")return "image/webp";
    return "application/octet-stream";
}
void handle(Socket client,const std::filesystem::path& root,unsigned port) {
    try {
        const auto request=read_request(client);
        const std::string host1="127.0.0.1:"+std::to_string(port),host2="localhost:"+std::to_string(port);
        const auto host=request.headers.find("host");
        if(host==request.headers.end()||(host->second!=host1&&host->second!=host2)) {
            response(client,"403 Forbidden","text/plain","Use the printed localhost address");return;
        }
        if(const auto origin=request.headers.find("origin");origin!=request.headers.end()
            &&origin->second!="http://"+host1&&origin->second!="http://"+host2) {
            response(client,"403 Forbidden","text/plain","Cross-origin solver access is disabled");return;
        }
        if(request.method=="POST"&&request.target=="/api/solve") {
            const char* raw=pocket_engineer::pe_solve_json(request.body.c_str());
            if(!raw)throw std::runtime_error("Solver allocation failed");
            const std::string result(raw);pocket_engineer::pe_free_string(raw);
            response(client,"200 OK","application/json",result);return;
        }
        if(request.method=="POST"&&request.target=="/api/identify") {
            const auto problem=pocket_engineer::parse_request(request.body);
            if(problem.input.size()>4096)throw std::runtime_error("Identification input too long");
            response(client,"200 OK","application/json",pocket_engineer::Engine{}.identify(problem.input).to_json());return;
        }
        if(request.method!="GET") {response(client,"405 Method Not Allowed","text/plain","Use GET or a supported POST API");return;}
        if(request.target=="/api/catalog") {response(client,"200 OK","application/json",pocket_engineer::catalog_json());return;}
        if(request.target=="/api/capabilities") {response(client,"200 OK","application/json",pocket_engineer::Engine{}.capabilities_json());return;}
        auto target=request.target.substr(0,request.target.find('?'));
        if(target.empty()||target.front()!='/'||target.find('%')!=std::string::npos||target.find('\\')!=std::string::npos
            ||target.find('\0')!=std::string::npos)throw std::runtime_error("Unsupported URL path");
        if(target=="/")target="/index.html";
        const auto file=std::filesystem::weakly_canonical(root/target.substr(1));
        const auto relative=file.lexically_relative(root);
        if(relative.empty()||*relative.begin()=="..") {response(client,"403 Forbidden","text/plain","Path is outside the website directory");return;}
        if(!std::filesystem::is_regular_file(file)){response(client,"404 Not Found","text/plain","Not found");return;}
        if(std::filesystem::file_size(file)>32*1024*1024)throw std::runtime_error("Static file is too large");
        std::ifstream stream(file,std::ios::binary);if(!stream)throw std::runtime_error("Cannot read static file");
        std::ostringstream body;body<<stream.rdbuf();response(client,"200 OK",mime(file),body.str());
    } catch(const std::exception& error) {
        response(client,"400 Bad Request","text/plain",error.what());
    }
}
} // namespace

int main(int argc,char** argv) {
    try {
        const unsigned port=argc>1?number(argv[1]):8080;
        if(port==0||port>65535)throw std::runtime_error("Port must be 1–65535");
        auto site=argc>2?std::filesystem::path(argv[2]):std::filesystem::path("www");
        // CPack archives are relocatable; launching the executable from another
        // working directory still finds the installed website next to bin/.
        if(argc<=2&&!std::filesystem::is_directory(site))
            site=std::filesystem::absolute(argv[0]).parent_path().parent_path()/"share/pocket-engineer/www";
        const auto root=std::filesystem::canonical(site);
#ifdef _WIN32
        WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data)!=0)throw std::runtime_error("WSAStartup failed");
#else
        std::signal(SIGPIPE,SIG_IGN);
#endif
        const Socket server=socket(AF_INET,SOCK_STREAM,0);
        if(server==invalid_socket)throw std::runtime_error("Cannot create socket");
        const int reuse=1;
#ifdef _WIN32
        setsockopt(server,SOL_SOCKET,SO_REUSEADDR,reinterpret_cast<const char*>(&reuse),sizeof(reuse));
#else
        setsockopt(server,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
#endif
        sockaddr_in address{};address.sin_family=AF_INET;
        address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
        address.sin_port=htons(static_cast<std::uint16_t>(port));
        if(bind(server,reinterpret_cast<sockaddr*>(&address),sizeof(address))!=0||listen(server,16)!=0) {
            close_socket(server);throw std::runtime_error("Cannot bind localhost port");
        }
        std::cout<<"Pocket Engineer: http://127.0.0.1:"<<port<<"\nServing "<<root<<std::endl;
        for(;;) {
            const Socket client=accept(server,nullptr,nullptr);
            if(client==invalid_socket)break;
            timeout(client);handle(client,root,port);close_socket(client);
        }
        close_socket(server);
#ifdef _WIN32
        WSACleanup();
#endif
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
