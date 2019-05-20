#ifndef _PARSEWEB_H
#define _PARSEWEB_H
#include "../Encapsulate/Timer.h"
#include <string>
#include <unordered_map>
#include <map>
#include <memory>
#include <sys/epoll.h>
#include <functional>
#include <unistd.h>
class EventLoop;
class TimerNode;
class Channel;
//当前解析hhtp请求的状态
enum CurParseState
{
    STATE_PARSE_URI = 1,
    STATE_PARSE_HEADERS,
    STATE_RECV_BODY,
    STATE_ANALYSIS,
    STATE_FINISH
};
//解析URI的结果
enum URIState
{
    PARSE_URI_AGAIN = 1,
    PARSE_URI_ERROR,
    PARSE_URI_SUCCESS,
};
//解析头的结果
enum HeaderState
{
    PARSE_HEADER_SUCCESS = 1,
    PARSE_HEADER_AGAIN,
    PARSE_HEADER_ERROR
};
//分析请求的结果
enum AnalysisState
{
    ANALYSIS_SUCCESS = 1,
    ANALYSIS_ERROR
};
//解析的状态
enum ParseState
{
    H_START = 0,
    H_KEY,
    H_COLON,
    H_SPACES_AFTER_COLON,
    H_VALUE,
    H_CR,
    H_LF,
    H_END_CR,
    H_END_LF
};
//连接的状态
enum ConnectionState
{
    H_CONNECTED = 0,
    H_DISCONNECTING,
    H_DISCONNECTED    
};
//HTTP请求的方法
enum HttpMethod
{
    METHOD_POST = 1,
    METHOD_GET,
    METHOD_HEAD
};
//HTTP版本
enum HttpVersion
{
    HTTP_10 = 1,
    HTTP_11
};
//消息类型
class MimeType
{
private:
    static void init();//初始化
    static std::unordered_map<std::string, std::string> mime;//HASH表查找
    MimeType();
    MimeType(const MimeType &m);//拷贝构造

public:
    static std::string getMime(const std::string &suffix);//获取消息类型

private:
    static pthread_once_t once_control;//保证只执行一次
};


class ClientRequest: public std::enable_shared_from_this<ClientRequest>
{
public:
    ClientRequest(EventLoop *loop, int connfd);
    ~ClientRequest() { close(fd_); }
    void reset();
    void seperateTimer();
    void linkTimer(std::shared_ptr<TimerNode> mtimer)
    {
        // shared_ptr重载了bool, 但weak_ptr没有
        timer_ = mtimer; 
    }
    std::shared_ptr<Channel> getChannel() { return channel_; }//获取当前Channel
    EventLoop *getLoop() { return loop_; }//获取事件循环
    void handleClose();//
    void newEvent();

private:
    EventLoop *loop_; //所在事件循环
    std::shared_ptr<Channel> channel_;//Channel
    int fd_;//客户端连接的描述符号
    std::string inBuffer_;//读缓冲
    std::string outBuffer_;//写缓冲
    bool error_;//是否出错
    ConnectionState connectionState_;//连接状态
    HttpMethod method_; //请求方法
    HttpVersion HTTPVersion_;//版本号
    std::string fileName_;//文件名
    std::string path_;//路径
    int nowReadPos_;//当前解析位置
    CurParseState curState;
    ParseState hState_;
    AnalysisState analysisRequest();
    bool keepAlive_;//长连接或短连接
    std::map<std::string, std::string> headers_;
    std::weak_ptr<TimerNode> timer_;//计时器

    void handleRead();//处理读
    void handleWrite();//处理写
    void handleConn();//处理新连接
    void handleError(int fd, int err_num, std::string short_msg);//处理错误
    URIState parseURI();//解析URI
    HeaderState parseHeaders();//解析请求头
};

#endif