#include "Encapsulate/Util.h"
#include "EventLoop/EventLoop.h"
#include "Server/Server.h"
#include "Log/Logging.h"
#include <getopt.h>
#include <string>

int main(int argc, char *argv[])
{    
    int threadNum = 4;
    int port = 80;
     std::string logPath = "Client.log";
    int opt;
    const char *str = "p:t:l:";
    while ((opt = getopt(argc, argv, str))!= -1)
    {
        switch (opt)
        {   
            case 'p': 
            {
                port = atoi(optarg);
                break;
            }
            case 't':
            {
                threadNum = atoi(optarg);/* Convert a string to an integer.  */
                break;
            }
            case 'l':
            {
                logPath = optarg;
                if (logPath.size() < 2 || optarg[0] != '/')
                {
                    printf("logPath should start with \"/\"\n");
                    abort();
                }
                break;
            }
            default: break;
        }
    }
    Logger::setLogFileName(logPath);//确定log日志存放路径
    cout<<"初始化主线程\n";
    EventLoop Main;//非new定义对象，在栈中分配存储空间
   cout<<"初始化子线程\n";
    Server HTTPServer(&Main, threadNum, port);
    cout<<"开启线程池\n";
    HTTPServer.start();
    cout<<"开启主线程\n";
    Main.loop();
    return 0;
}
