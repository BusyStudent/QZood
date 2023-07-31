#include "promise.hpp"

NetPromiseHelper::NetPromiseHelper(QObject *p) : QObject(p) {

}

NetPromiseHelper::~NetPromiseHelper() {

}

void NetPromiseHelper::doNotify(const void *value) {
    emit notify(value);
}