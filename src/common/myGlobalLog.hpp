#pragma once

#include <QDebug>
#include <QString>
#if defined(_WIN32)
#include <Windows.h>

#define SetConsoleColor(color)                                          \
	::SetConsoleTextAttribute(::GetStdHandle(STD_OUTPUT_HANDLE), color);  \
	::SetConsoleTextAttribute(::GetStdHandle(STD_ERROR_HANDLE), color);

#define CONSOLE_RED     FOREGROUND_RED
#define CONSOLE_GREEN   FOREGROUND_GREEN
#define CONSOLE_BLUE    FOREGROUND_BLUE
#define CONSOLE_DEFAULT CONSOLE_RED | CONSOLE_GREEN | CONSOLE_BLUE
#define CONSOLE_YELLOW  CONSOLE_GREEN | FOREGROUND_RED
#define CONSOLE_LIGHTBLUE  CONSOLE_GREEN | FOREGROUND_BLUE
#else
#define SetConsoleColor(color)
#define CONSOLE_RED     
#define CONSOLE_GREEN   
#define CONSOLE_BLUE    
#define CONSOLE_DEFAULT 
#define CONSOLE_YELLOW  
#define CONSOLE_LIGHTBLUE 
#endif

class MyDebug {
    public:
        enum LogType {
        ABANDON,
        DEBUG,
        INFO,
        WARNING,
        FATAL,
        NOTYPE,
    };
    public:
        MyDebug(const LogType& type,const QString &mPrex, const std::function<void(const QString &)> func = nullptr);
        ~MyDebug();
        template <typename T>
        MyDebug &operator <<(T &&arg) {
            ((QDebug*)&mBuf)->operator <<(std::forward<T>(arg));
            return *this;
        }
    private:
        std::function<void(const QString &)> func = [this](const QString &msg){
            switch (mType) {
            case LogType::DEBUG:
                qt_message_output(QtMsgType::QtDebugMsg,QMessageLogContext() ,msg);
                break;
            case LogType::INFO:
                qt_message_output(QtMsgType::QtInfoMsg,QMessageLogContext() ,msg);
                break;
            case LogType::WARNING:
                qt_message_output(QtMsgType::QtWarningMsg,QMessageLogContext() ,msg);
                break;
            case LogType::FATAL:
                qt_message_output(QtMsgType::QtFatalMsg,QMessageLogContext() ,msg);
                break;
            default:
                break;
            }
        };
        LogType mType;
        QString mMsg;
        QString mPrex;
        uint8_t mBuf[sizeof(QDebug)];
};

#define MyComDebug(type,time, file, line, funcmsg, func) MyDebug(type,"[" time "]" "[" file ":" + QString::number(line) + "][" + QString(QT_MESSAGELOG_FUNC) + "] ", func)
#define MCDebug(type, func) MyComDebug(type,__TIME__, __FILE__, __LINE__, __FUNCSIG__, func)
#define MDebug(type) MCDebug(type, nullptr)

#define MDebug_DEBUG() MDebug(MyDebug::DEBUG)
#define MDebug_INFO() MDebug(MyDebug::INFO)
#define MDebug_WARNING() MDebug(MyDebug::WARNING)
#define MDebug_ABANDON() MDebug(MyDebug::ABANDON)
#define MDebug_FATAL() MDebug(MyDebug::FATAL)
#define MDebug_NOTYPE() MDebug(MyDebug::NOTYPE)

#define LOG(TYPE) MDebug_ ##TYPE()