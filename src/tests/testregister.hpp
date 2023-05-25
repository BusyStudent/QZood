#pragma once

#include <functional>
#include <QString>
#include <QElapsedTimer>

enum TestType {
    WIDGET,
    WIDGET_W,
    CMD
};

#define TestFlag(id) TestFlags()[id]

#define INIT_TEST_START(module_name,test_name,type)   TestFlag(id) = true;                                    \
    ZoodLogString(QString("[==========] Test %1 will be run").arg(#test_name));                               \
    ZoodLogString(QString("[----------] Runing test type is %1").arg(#type));                                 \
    ZoodLogString(QString("[ RUN      ] %1.%2").arg(#module_name,#test_name));                                \
    QElapsedTimer time;                                                                                       \
    time.start();                                                                                             \

#define INIT_TEST_END(module_name,test_name)  auto ms = time.elapsed();                                       \
    ZoodLogString(QString("[==========] %1.%2 run in : %3 ms").                                               \
        arg(#module_name,#test_name,QString::number(ms)));                                                    \
    if (TestFlag(id)) {                                                                                       \
        ZoodLogString(QString("[  PASSED  ] %1.%2").arg(#module_name,#test_name));                            \
    } else {                                                                                                  \
        ZoodLogString(QString("[  FAILED  ] %1.%2").arg(#module_name,#test_name));                            \
    }                                                                                                         \

#define ZOOD_TEST_BODY(module_name,test_name,type)                                                            \
    static void *test__##module_name ## _##test_name (const int id);                                          \
    static bool test__init_##module_name ## _##test_name = []() {                                             \
        ZoodRegisterTest(#module_name, #test_name, type, test__##module_name ## _##test_name);                \
        return true;                                                                                          \
    }();                                                                                                      \
    static void *test__##module_name ## _##test_name ##_imp (const int id);                                   \
    static void *test__##module_name ## _##test_name (const int id) {                                         \
        INIT_TEST_START(module_name,test_name,type);                                                          \
        auto result = test__##module_name ## _##test_name ## _imp(id);                                        \
        INIT_TEST_END(module_name,test_name);                                                                 \
        return result;                                                                                        \
    }                                                                                                         \
    static void *test__##module_name ## _##test_name ## _imp (const int id)

#define ZOOD_TEST(module_name,test_name) ZOOD_TEST_BODY(module_name,test_name,TestType::WIDGET)
      
#define ZOOD_TEST_W(module_name,test_name) ZOOD_TEST_BODY(module_name,test_name,TestType::WIDGET_W)

#define ZOOD_TEST_C(module_name,test_name) static void *test__##module_name ## _##test_name (const int id);                                          \
    static bool test__init_##module_name ## _##test_name = []() {                                             \
        ZoodRegisterTest(#module_name, #test_name, TestType::CMD, test__##module_name ## _##test_name);       \
        return true;                                                                                          \
    }();                                                                                                      \
    static void test__##module_name ## _##test_name ##_imp (const int id);                                    \
    static void *test__##module_name ## _##test_name (const int id) {                                         \
        INIT_TEST_START(module_name,test_name,TestType::CMD);                                                          \
        test__##module_name ## _##test_name ## _imp(id);                                                      \
        INIT_TEST_END(module_name,test_name);                                                                 \
        return nullptr;                                                                                       \
    }                                                                                                         \
    static void test__##module_name ## _##test_name ## _imp (const int id)

#define LOG_VAL(val) ZoodLogString(QString("%1 : %2").arg(#val, QString::number(val)))

#define EXPECT_EQ(val1, val2) if (!((val1) == (val2))) {                                                     \
    TestFlag(id) = false;                                                                                    \
    LOG_VAL(val1); LOG_VAL(val2);                                                                            \
    ZoodLogString(QString("%1 != %2 and the diff is : %3").arg(                                              \
            #val1, #val2, QString::number((val2) - (val1))));                                                \
} void(0)

#define EXPECT_NE(val1, val2) if (!((val1) != (val2))) {                                                     \
    TestFlag(id) = false;                                                                                    \
    LOG_VAL(val1); LOG_VAL(val2);                                                                            \
    ZoodLogString(QString("%1 == %2").arg(#val1, #val2));                                                    \
} void(0)

#define EXPECT_LT(val1, val2) if (!((val1) < (val2))) {                                                      \
    TestFlag(id) = false;                                                                                    \
    LOG_VAL(val1); LOG_VAL(val2);                                                                            \
    ZoodLogString(QString("%1 >= %2").arg(#val1, #val2));                                                    \
} void(0)

#define EXPECT_LE(val1, val2) if (!((val1) <= (val2))) {                                                     \
    TestFlag(id) = false;                                                                                    \
    LOG_VAL(val1); LOG_VAL(val2);                                                                            \
    ZoodLogString(QString("%1 > %2").arg(#val1, #val2));                                                     \
} void(0)

#define EXPECT_GT(val1, val2) if (!((val1) > (val2))) {                                                      \
    TestFlag(id) = false;                                                                                    \
    LOG_VAL(val1); LOG_VAL(val2);                                                                            \
    ZoodLogString(QString("%1 <= %2").arg(#val1, #val2));                                                    \
} void(0)

#define EXPECT_GE(val1, val2) if (!((val1) >= (val2))) {                                                     \
    TestFlag(id) = false;                                                                                    \
    LOG_VAL(val1); LOG_VAL(val2);                                                                            \
    ZoodLogString(QString("%1 < %2").arg(#val1, #val2));                                                     \
} void(0)

#define EXPECT_TRUE(val1) if (!(val1)) {                                                                     \
    TestFlag(id) = false;                                                                                    \
    ZoodLogString(QString("%1 : %2").arg(#val1, "False"));                                                   \
} void(0)

#define EXPECT_FALSE(val1) if ((val1)) {                                                                     \
    TestFlag(id) = false;                                                                                    \
    ZoodLogString(QString("%1 : %2").arg(#val1, "True"));                                                    \
} void(0)


#define ASSERT_EQ(val1, val2) if (!((val1) == (val2))) {                                                     \
    TestFlag(id) = false;                                                                                    \
    LOG_VAL(val1); LOG_VAL(val2);                                                                            \
    ZoodLogString(QString("%1 != %2 and the diff is : %3").arg(                                              \
        #val1, #val2, QString::number((val2) - (val1))));                                                    \
    return nullptr;                                                                                          \
} void(0)

#define ASSERT_NE(val1, val2) if (!((val1) != (val2))) {                                                     \
    TestFlag(id) = false;                                                                                    \
    LOG_VAL(val1); LOG_VAL(val2);                                                                            \
    ZoodLogString(QString("%1 == %2").arg(#val1, #val2));                                                    \
    return nullptr;                                                                                          \
} void(0)

#define ASSERT_LT(val1, val2) if (!((val1) < (val2))) {                                                      \
    TestFlag(id) = false;                                                                                    \
    LOG_VAL(val1); LOG_VAL(val2);                                                                            \
    ZoodLogString(QString("%1 >= %2").arg(#val1, #val2));                                                    \
    return nullptr;                                                                                          \
} void(0)

#define ASSERT_LE(val1, val2) if (!((val1) <= (val2))) {                                                     \
    TestFlag(id) = false;                                                                                    \
    LOG_VAL(val1); LOG_VAL(val2);                                                                            \
    ZoodLogString(QString("%1 > %2").arg(#val1, #val2));                                                     \
    return nullptr;                                                                                          \
} void(0)

#define ASSERT_GT(val1, val2) if (!((val1) > (val2))) {                                                      \
    TestFlag(id) = false;                                                                                    \
    LOG_VAL(val1); LOG_VAL(val2);                                                                            \
    ZoodLogString(QString("%1 <= %2").arg(#val1, #val2));                                                    \
    return nullptr;                                                                                          \
} void(0)

#define ASSERT_GE(val1, val2) if (!((val1) >= (val2))) {                                                     \
    TestFlag(id) = false;                                                                                    \
    LOG_VAL(val1); LOG_VAL(val2);                                                                            \
    ZoodLogString(QString("%1 < %2").arg(#val1, #val2));                                                     \
    return nullptr;                                                                                          \
} void(0)

#define ASSERT_TRUE(val1) if (!(val1)) {                                                                     \
    TestFlag(id) = false;                                                                                    \
    ZoodLogString(QString("%1 : %2").arg(#val1, "False"));                                                   \
    return nullptr;                                                                                          \
} void(0)

#define ASSERT_FALSE(val1) if ((val1)) {                                                                     \
    TestFlag(id) = false;                                                                                    \
    ZoodLogString(QString("%1 : %2").arg(#val1, "True"));                                                    \
    return nullptr;                                                                                          \
} void(0)

void ZoodRegisterTest(const QString &mudule_name, const QString &name,const TestType type, const std::function<void*(const int id)> &task);
void ZoodLogString(const QString &text);
QMap<int, bool>& TestFlags();