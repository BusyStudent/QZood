#include "../common/sqlite.hpp"
#include "../common/myGlobalLog.hpp"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace testing;

class DatabaseTest : public Test {
public:
    void SetUp() {
        if (db.open(NULL)) {
            auto rc = db.createTable("PlayHistory", {"id TEXT PRIMARY KEY",
                                     "title TEXT NOT NULL",
                                     "playTime INTEGER NOT NULL"});
        }
    }
    void TearDown() {
        db.close();
    }
public:
    SqliteDatabase db;
};

TEST_F(DatabaseTest, createTable) {
    EXPECT_EQ(db.createTable("User", { "id INTEGER PRIMARY KEY",
                "name TEXT NOT NULL",
                "email TEXT NOT NULL UNIQUE",
                "age INTEGER" }), 1);
    QStringList tables;
    EXPECT_EQ(db.queryTables(tables), 1);
    EXPECT_EQ(tables.size(), 2);
    EXPECT_STREQ(tables[0].toLocal8Bit(), "PlayHistory");
    EXPECT_STREQ(tables[1].toLocal8Bit(), "User");
    LOG(DEBUG) << "create table test";
    for (int i = 0;i < tables.size(); ++i) {
        LOG(DEBUG) << tables[i];
    }
    LOG(DEBUG) << "table test done...";
}

TEST_F(DatabaseTest, insertAndSearch) {
    EXPECT_EQ(db.insertValues("PlayHistory (id,title,playTime)", {"'001'", "'刀剑神域'", "0"}), 1);
    EXPECT_EQ(db.insertValues("PlayHistory (id,title,playTime)", { "'002'", "'未闻花名'", "45" }), 1);
    EXPECT_EQ(db.insertValues("PlayHistory (id,title,playTime)", { "'003'", "'末日三问'", "24" }), 1);
    
    auto result = db.queryValues("PlayHistory", {"id", "title", "playTime"});

    ASSERT_TRUE(result.has_value());
    auto map = result.value();
    EXPECT_EQ(map.size(), 3);
    
    EXPECT_STREQ(map["id"][0].toUtf8(), "001");
    EXPECT_STREQ(map["title"][0].toUtf8(), "刀剑神域");
    EXPECT_STREQ(map["playTime"][0].toUtf8(), "0");

    EXPECT_STREQ(map["id"][1].toUtf8(), "002");
    EXPECT_STREQ(map["title"][1].toUtf8(), "未闻花名");
    EXPECT_STREQ(map["playTime"][1].toUtf8(), "45");

    EXPECT_STREQ(map["id"][2].toUtf8(), "003");
    EXPECT_STREQ(map["title"][2].toUtf8(), "末日三问");
    EXPECT_STREQ(map["playTime"][2].toUtf8(), "24");
}