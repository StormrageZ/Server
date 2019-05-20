
set(SRCS
    EventLoop/Channel.cpp
    Encapsulate/EpollPoller.cpp
    EventLoop/EventLoop.cpp
    EventLoop/EventLoopThread.cpp
    EventLoop/EventLoopThreadPool.cpp
    Server/ParseWeb.cpp
    Main.cpp
    Server/Server.cpp
    #ThreadPool.cpp
    Encapsulate/Timer.cpp
    Encapsulate/Util.cpp
    Log/AsyncLogging.cpp 
    CountDownLatch.cpp 
    Log/FileUtil.cpp 
    Log/LogFile.cpp 
    Log/Logging.cpp 
    Log/LogStream.cpp 
    EventLoop/Thread.cpp 


)


add_executable(WebServer ${SRCS})
target_link_libraries(WebServer pthread)
