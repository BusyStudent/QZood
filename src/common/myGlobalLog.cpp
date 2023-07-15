#include "myGlobalLog.hpp"

MyDebug::MyDebug(const LogType& type,const QString &prex, const std::function<void(const QString &)> func) : mType(type) {
    new (&mBuf) QDebug(&mMsg);
    switch (mType) {
    case LogType::DEBUG:
        mPrex = "[DEBUG] " + prex;
        break;
    case LogType::INFO:
        mPrex = "[INFO] " + prex;
        break;
    case LogType::WARNING:
        mPrex = "[WARNING] " + prex;
        break;
    case LogType::FATAL:
        mPrex = "[FATAL] " + prex;
        break;
    default:
        mPrex = "";
        break;
    }
    if (func != nullptr) {
        this->func = func;
    }
}

MyDebug::~MyDebug() {
    ((QDebug*)&mBuf)->~QDebug();
    if (!mMsg.isEmpty()) {
        switch (mType) {
        case LogType::ABANDON:
            break;
        default:
            func(mPrex + mMsg);
        }
    }
}