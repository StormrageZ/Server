
set(SRCS
    Channel.cpp
    Epoll.cpp
    EventLoop.cpp
    EventLoopThread.cpp
    EventLoopThreadPool.cpp
    ParseWeb.cpp
    Main.cpp
    Server.cpp
    #ThreadPool.cpp
    Timer.cpp
    Util.cpp
    AsyncLogging.cpp 
    CountDownLatch.cpp 
    FileUtil.cpp 
    LogFile.cpp 
    Logging.cpp 
    LogStream.cpp 
    Thread.cpp 


)


add_executable(WebServer ${SRCS})
target_link_libraries(WebServer pthread)
