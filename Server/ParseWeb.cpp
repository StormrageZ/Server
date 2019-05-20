#include "ParseWeb.h"
#include "../EventLoop/Channel.h"
#include "../Encapsulate/Util.h"
#include "../EventLoop/EventLoop.h"
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <iostream>
using namespace std;

pthread_once_t MimeType::once_control = PTHREAD_ONCE_INIT;//仅执行一次
std::unordered_map<std::string, std::string> MimeType::mime;
const __uint32_t DEFAULT_EVENT = EPOLLIN | EPOLLET | EPOLLONESHOT;//只监听一次
const int DEFAULT_EXPIRED_TIME = 2000; // ms
const int DEFAULT_KEEP_ALIVE_TIME = 5 * 60 * 1000; // ms

char favicon[555] = {//图片数据没有特殊含义
  
};

void MimeType::init()//初始化mime对应hash表
{
    mime[".html"] = "text/html";
    mime[".avi"] = "video/x-msvideo";
    mime[".bmp"] = "image/bmp";
    mime[".c"] = "text/plain";
    mime[".doc"] = "application/msword";
    mime[".gif"] = "image/gif";
    mime[".gz"] = "application/x-gzip";
    mime[".htm"] = "text/html";
    mime[".ico"] = "image/x-icon";
    mime[".jpg"] = "image/jpeg";
    mime[".png"] = "image/png";
    mime[".txt"] = "text/plain";
    mime[".mp3"] = "audio/mp3";
    mime["default"] = "text/html";
}

std::string MimeType::getMime(const std::string &suffix)//匹配hash表，key value 
{
    pthread_once(&once_control, MimeType::init);//只执行一次
    if (mime.find(suffix) == mime.end())
        return mime["default"];
    else
        return mime[suffix];
}


ClientRequest::ClientRequest(EventLoop *loop, int connfd):
        loop_(loop),//注册事件循环
        channel_(new Channel(loop, connfd)),//读写事件IO    
        fd_(connfd),//当前的客户端连接
        error_(false),//初始化无错误
        connectionState_(H_CONNECTED),//初始化状态为已连接
        method_(METHOD_GET),//初始化方法为GET
        HTTPVersion_(HTTP_11),//支持HTTP1.1
        nowReadPos_(0), //当前读位置
        curState(STATE_PARSE_URI), //开始处于解析URI状态
        hState_(H_START),//解析开始
        keepAlive_(false)//关闭长连接
{
    //loop_->queueInLoop(bind(&ClientRequest::setHandlers, this));
    channel_->setReadHandler(bind(&ClientRequest::handleRead, this));
    channel_->setWriteHandler(bind(&ClientRequest::handleWrite, this));//交给channel处理
    channel_->setConnHandler(bind(&ClientRequest::handleConn, this));
}

void ClientRequest::reset()//重置连接
{
    //inBuffer_.clear();
    fileName_.clear();
    path_.clear();
    nowReadPos_ = 0;
    curState = STATE_PARSE_URI;
    hState_ = H_START;
    headers_.clear();
    //keepAlive_ = false;
    if (timer_.lock())
    {
        shared_ptr<TimerNode> my_timer(timer_.lock());
        my_timer->clearReq();
        timer_.reset();
    }
}

void ClientRequest::seperateTimer()//分离计时器
{
    //cout << "seperateTimer" << endl;
    if (timer_.lock())
    {
        shared_ptr<TimerNode> my_timer(timer_.lock());
        my_timer->clearReq();
        timer_.reset();
    }
}

void ClientRequest::handleRead()//读请求
{
    int &events_ = channel_->getEvents();
    do
    {
        bool zero = false;
        int read_num = readn(fd_, inBuffer_, zero);//读取到的大小
        LOG << "Request: " << inBuffer_;//日志记录请求
        if (connectionState_ == H_DISCONNECTING)
        {
            inBuffer_.clear();//如果正在断开请求，清空缓存
            break;
        }
        //cout << inBuffer_ << endl;
        if (read_num < 0)//读取出错
        {
            perror("1");
            error_ = true;
            handleError(fd_, 400, "Bad Request");//
            break;
        }
        // else if (read_num == 0)
        // {
        //     error_ = true;
        //     break; 
        // }
        else if (zero)
        {
            // 有请求出现但是读不到数据，可能是Request Aborted，或者来自网络的数据没有达到等原因
            // 最可能是对端已经关闭了，统一按照对端已经关闭处理
            //error_ = true;
            connectionState_ = H_DISCONNECTING;
            if (read_num == 0)
            {
                //error_ = true;
                break;
            }
           
        }

        if (curState == STATE_PARSE_URI)
        {
            URIState flag = this->parseURI();
            if (flag == PARSE_URI_AGAIN)
                break;
            else if (flag == PARSE_URI_ERROR)
            {
                perror("2");
                LOG << "FD = " << fd_ << "," << inBuffer_ << "******";
                inBuffer_.clear();
                error_ = true;
                handleError(fd_, 400, "Bad Request");
                break;
            }
            else
                curState = STATE_PARSE_HEADERS;
        }
        if (curState == STATE_PARSE_HEADERS)
        {
            HeaderState flag = this->parseHeaders();
            if (flag == PARSE_HEADER_AGAIN)
                break;
            else if (flag == PARSE_HEADER_ERROR)
            {
                perror("3");
                error_ = true;
                handleError(fd_, 400, "Bad Request");
                break;
            }
            if(method_ == METHOD_POST)
            {
                // POST方法准备
                curState = STATE_RECV_BODY;
            }
            else 
            {
                curState = STATE_ANALYSIS;
            }
        }
        if (curState == STATE_RECV_BODY)
        {
            int content_length = -1;
            if (headers_.find("Content-length") != headers_.end())
            {
                content_length = stoi(headers_["Content-length"]);
            }
            else
            {
                //cout << "(curState == STATE_RECV_BODY)" << endl;
                error_ = true;
                handleError(fd_, 400, "Bad Request: Lack of argument (Content-length)");
                break;
            }
            if (static_cast<int>(inBuffer_.size()) < content_length)
                break;
            curState = STATE_ANALYSIS;
        }
        if (curState == STATE_ANALYSIS)
        {
            AnalysisState flag = this->analysisRequest();
            if (flag == ANALYSIS_SUCCESS)
            {
                curState = STATE_FINISH;
                break;
            }
            else
            {
                //cout << "curState == STATE_ANALYSIS" << endl;
                error_ = true;
                break;
            }
        }
    } while (false);
    //cout << "curState=" << curState << endl;
    if (!error_)
    {
        if (outBuffer_.size() > 0)
        {
            handleWrite();
            //events_ |= EPOLLOUT;
        }
        // error_ may change
        if (!error_ && curState == STATE_FINISH)
        {
            this->reset();
            if (inBuffer_.size() > 0)
            {
                if (connectionState_ != H_DISCONNECTING)
                    handleRead();
            }

            // if ((keepAlive_ || inBuffer_.size() > 0) && connectionState_ == H_CONNECTED)
            // {
            //     this->reset();
            //     events_ |= EPOLLIN;
            // }
        }
        else if (!error_ && connectionState_ != H_DISCONNECTED)
            events_ |= EPOLLIN;
    }
}

void ClientRequest::handleWrite()
{
    if (!error_ && connectionState_ != H_DISCONNECTED)
    {
        int &events_ = channel_->getEvents();
        if (writen(fd_, outBuffer_) < 0)
        {
            perror("writen");
            events_ = 0;
            error_ = true;
        }
        if (outBuffer_.size() > 0)
            events_ |= EPOLLOUT;
    }
}

void ClientRequest::handleConn()
{
    seperateTimer();
    int &events_ = channel_->getEvents();
    if (!error_ && connectionState_ == H_CONNECTED)
    {
        if (events_ != 0)
        {
            int timeout = DEFAULT_EXPIRED_TIME;
            if (keepAlive_)
                timeout = DEFAULT_KEEP_ALIVE_TIME;
            if ((events_ & EPOLLIN) && (events_ & EPOLLOUT))
            {
                events_ = __uint32_t(0);
                events_ |= EPOLLOUT;
            }
            //events_ |= (EPOLLET | EPOLLONESHOT);
            events_ |= EPOLLET;
            loop_->updatePoller(channel_, timeout);

        }
        else if (keepAlive_)
        {
            events_ |= (EPOLLIN | EPOLLET);
            //events_ |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
            int timeout = DEFAULT_KEEP_ALIVE_TIME;
            loop_->updatePoller(channel_, timeout);
        }
        else
        {
            //cout << "close normally" << endl;
            // loop_->shutdown(channel_);
            // loop_->runInLoop(bind(&ClientRequest::handleClose, shared_from_this()));
            events_ |= (EPOLLIN | EPOLLET);
            //events_ |= (EPOLLIN | EPOLLET | EPOLLONESHOT);
            int timeout = (DEFAULT_KEEP_ALIVE_TIME >> 1);
            loop_->updatePoller(channel_, timeout);
        }
    }
    else if (!error_ && connectionState_ == H_DISCONNECTING && (events_ & EPOLLOUT))
    {
        events_ = (EPOLLOUT | EPOLLET);
    }
    else
    {
        //cout << "close with errors" << endl;
        loop_->runInLoop(bind(&ClientRequest::handleClose, shared_from_this()));
    }
}


URIState ClientRequest::parseURI()
{
    string &str = inBuffer_;
    string cop = str;
    // 读到完整的请求行再开始解析请求
    size_t pos = str.find('\r', nowReadPos_);//找到第一个出现\r的位置
    if (pos < 0)
    {
        return PARSE_URI_AGAIN;
    }
    // 去掉请求行所占的空间，节省空间
    string request_line = str.substr(0, pos);//取出请求
    if (str.size() > pos + 1)
        str = str.substr(pos + 1);
    else 
        str.clear();
    // Method
    int posGet = request_line.find("GET");
    int posPost = request_line.find("POST");
    int posHead = request_line.find("HEAD");//解析方法

    if (posGet >= 0)
    {
        pos = posGet;
        method_ = METHOD_GET;
    }
    else if (posPost >= 0)
    {
        pos = posPost;
        method_ = METHOD_POST;
    }
    else if (posHead >= 0)
    {
        pos = posHead;
        method_ = METHOD_HEAD;
    }
    else
    {
        return PARSE_URI_ERROR;//解析错误
    }

    // filename
    pos = request_line.find("/", pos);//找到URI的开头
    if (pos < 0)
    {
        fileName_ = "index.html";
        HTTPVersion_ = HTTP_11;
        return PARSE_URI_SUCCESS;//????????????????????????
    }
    else
    {
        size_t _pos = request_line.find(' ', pos);//找到第二个空格
        if (_pos < 0)
            return PARSE_URI_ERROR;//
        else
        {
            if (_pos - pos > 1)
            {
                fileName_ = request_line.substr(pos + 1, _pos - pos - 1);
                size_t __pos = fileName_.find('?');
                if (__pos >= 0)
                {
                    fileName_ = fileName_.substr(0, __pos);
                }
            }
                
            else
                fileName_ = "index.html";
        }
        pos = _pos;
    }
    //cout << "fileName_: " << fileName_ << endl;
    // HTTP 版本号
    pos = request_line.find("/", pos);
    if (pos < 0)
        return PARSE_URI_ERROR;
    else
    {
        if (request_line.size() - pos <= 3)
            return PARSE_URI_ERROR;
        else
        {
            string ver = request_line.substr(pos + 1, 3);
            if (ver == "1.0")
                HTTPVersion_ = HTTP_10;
            else if (ver == "1.1")
                HTTPVersion_ = HTTP_11;
            else
                return PARSE_URI_ERROR;
        }
    }
    return PARSE_URI_SUCCESS;
}

HeaderState ClientRequest::parseHeaders()//解析请求头
{
    string &str = inBuffer_;
    int key_start = -1, key_end = -1, value_start = -1, value_end = -1;
    int now_read_line_begin = 0;
    bool notFinish = true;
    size_t i = 0;
    for (; i < str.size() && notFinish; ++i)
    {
        switch(hState_)
        {
            case H_START:
            {
                if (str[i] == '\n' || str[i] == '\r')
                    break;
                hState_ = H_KEY;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
            case H_KEY:
            {
                if (str[i] == ':')
                {
                    key_end = i;
                    if (key_end - key_start <= 0)
                        return PARSE_HEADER_ERROR;
                    hState_ = H_COLON;
                }
                else if (str[i] == '\n' || str[i] == '\r')
                    return PARSE_HEADER_ERROR;
                break;  
            }
            case H_COLON:
            {
                if (str[i] == ' ')
                {
                    hState_ = H_SPACES_AFTER_COLON;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;  
            }
            case H_SPACES_AFTER_COLON:
            {
                hState_ = H_VALUE;
                value_start = i;
                break;  
            }
            case H_VALUE:
            {
                if (str[i] == '\r')
                {
                    hState_ = H_CR;
                    value_end = i;
                    if (value_end - value_start <= 0)
                        return PARSE_HEADER_ERROR;
                }
                else if (i - value_start > 255)
                    return PARSE_HEADER_ERROR;
                break;  
            }
            case H_CR:
            {
                if (str[i] == '\n')
                {
                    hState_ = H_LF;
                    string key(str.begin() + key_start, str.begin() + key_end);
                    string value(str.begin() + value_start, str.begin() + value_end);
                    headers_[key] = value;
                    now_read_line_begin = i;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;  
            }
            case H_LF:
            {
                if (str[i] == '\r')
                {
                    hState_ = H_END_CR;
                }
                else
                {
                    key_start = i;
                    hState_ = H_KEY;
                }
                break;
            }
            case H_END_CR:
            {
                if (str[i] == '\n')
                {
                    hState_ = H_END_LF;
                }
                else
                    return PARSE_HEADER_ERROR;
                break;
            }
            case H_END_LF:
            {
                notFinish = false;
                key_start = i;
                now_read_line_begin = i;
                break;
            }
        }
    }
    if (hState_ == H_END_LF)
    {
        str = str.substr(i);
        return PARSE_HEADER_SUCCESS;
    }
    str = str.substr(now_read_line_begin);
    return PARSE_HEADER_AGAIN;
}

AnalysisState ClientRequest::analysisRequest()
{
    if (method_ == METHOD_POST)
    {
        // ------------------------------------------------------
        // My CV stitching handler which requires OpenCV library
        // ------------------------------------------------------
        // string header;
        // header += string("HTTP/1.1 200 OK\r\n");
        // if(headers_.find("Connection") != headers_.end() && headers_["Connection"] == "Keep-Alive")
        // {
        //     keepAlive_ = true;
        //     header += string("Connection: Keep-Alive\r\n") + "Keep-Alive: timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
        // }
        // int length = stoi(headers_["Content-length"]);
        // vector<char> data(inBuffer_.begin(), inBuffer_.begin() + length);
        // Mat src = imdecode(data, CV_LOAD_IMAGE_ANYDEPTH|CV_LOAD_IMAGE_ANYCOLOR);
        // //imwrite("receive.bmp", src);
        // Mat res = stitch(src);
        // vector<uchar> data_encode;
        // imencode(".png", res, data_encode);
        // header += string("Content-length: ") + to_string(data_encode.size()) + "\r\n\r\n";
        // outBuffer_ += header + string(data_encode.begin(), data_encode.end());
        // inBuffer_ = inBuffer_.substr(length);
        // return ANALYSIS_SUCCESS;
    }
    else if (method_ == METHOD_GET || method_ == METHOD_HEAD)
    {
        string header;
        header += "HTTP/1.1 200 OK\r\n";
        if(headers_.find("Connection") != headers_.end() && (headers_["Connection"] == "Keep-Alive" || headers_["Connection"] == "keep-alive"))
        {
            keepAlive_ = true;
            header += string("Connection: Keep-Alive\r\n") + "Keep-Alive: timeout=" + to_string(DEFAULT_KEEP_ALIVE_TIME) + "\r\n";
        }
        int dot_pos = fileName_.find('.');
        string filetype;
        if (dot_pos < 0) 
            filetype = MimeType::getMime("default");
        else
            filetype = MimeType::getMime(fileName_.substr(dot_pos));
        

        // echo test
        if (fileName_ == "hello")
        {
            outBuffer_ = "HTTP/1.1 200 OK\r\nContent-type: text/plain\r\n\r\nHello World";
            return ANALYSIS_SUCCESS;
        }
        if (fileName_ == "favicon.ico")
        {
            header += "Content-Type: image/png\r\n";
            header += "Content-Length: " + to_string(sizeof favicon) + "\r\n";
            header += "Server: Zhong's Web Server\r\n";

            header += "\r\n";
            outBuffer_ += header;
            outBuffer_ += string(favicon, favicon + sizeof favicon);;
            return ANALYSIS_SUCCESS;
        }

        struct stat sbuf;
        if (stat(fileName_.c_str(), &sbuf) < 0)
        {
            header.clear();
            handleError(fd_, 404, "Not Found!");
            return ANALYSIS_ERROR;
        }
        header += "Content-Type: " + filetype + "\r\n";
        header += "Content-Length: " + to_string(sbuf.st_size) + "\r\n";
        header += "Server: ZHONG's Web Server\r\n";
        // 头部结束
        header += "\r\n";
        outBuffer_ += header;
        
        if (method_ == METHOD_HEAD)
            return ANALYSIS_SUCCESS;

        int src_fd = open(fileName_.c_str(), O_RDONLY, 0);
        if (src_fd < 0)
        {
          outBuffer_.clear();
          handleError(fd_, 404, "Not Found!");
          return ANALYSIS_ERROR;
        }
		void *mmapRet = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, src_fd, 0);
		close(src_fd);
		if (mmapRet == (void *)-1)
		{
			munmap(mmapRet, sbuf.st_size);
			outBuffer_.clear();
			handleError(fd_, 404, "Not Found!");
			return ANALYSIS_ERROR;
		}
        char *src_addr = static_cast<char*>(mmapRet);
        outBuffer_ += string(src_addr, src_addr + sbuf.st_size);;
        munmap(mmapRet, sbuf.st_size);
        return ANALYSIS_SUCCESS;
    }
    return ANALYSIS_ERROR;
}

void ClientRequest::handleError(int fd, int err_num, string short_msg)
{
    short_msg = " " + short_msg;
    char send_buff[4096];
    string body_buff, header_buff;
    body_buff += "<html><title>哎~出错了</title>";
    body_buff += "<body bgcolor=\"ffffff\">";
    body_buff += to_string(err_num) + short_msg;
    body_buff += "<hr><em> zhong's Web Server</em>\n</body></html>";

    header_buff += "HTTP/1.1 " + to_string(err_num) + short_msg + "\r\n";
    header_buff += "Content-Type: text/html\r\n";
    header_buff += "Connection: Close\r\n";
    header_buff += "Content-Length: " + to_string(body_buff.size()) + "\r\n";
    header_buff += "Server: zhong's Web Server\r\n";;
    header_buff += "\r\n";
    // 错误处理不考虑writen不完的情况
    sprintf(send_buff, "%s", header_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
    sprintf(send_buff, "%s", body_buff.c_str());
    writen(fd, send_buff, strlen(send_buff));
}

void ClientRequest::handleClose()
{
    connectionState_ = H_DISCONNECTED;
    shared_ptr<ClientRequest> guard(shared_from_this());
    loop_->removeFromPoller(channel_);
}


void ClientRequest::newEvent()
{
    channel_->setEvents(DEFAULT_EVENT);
    loop_->addToPoller(channel_, DEFAULT_EXPIRED_TIME);
}