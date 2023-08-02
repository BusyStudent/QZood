#include "sqlite.hpp"

#include <stdlib.h>
#include <iostream>
#include <sqlite3.h>

#include "./myGlobalLog.hpp"

#ifdef _WIN32
#include <windows.h>
#define sleep Sleep
#elif
#include <unistd.h>
#endif

SqliteDatabase::SqliteDatabase() {
    mDb = nullptr;
}

SqliteDatabase::~SqliteDatabase() {
    close();
}

sqlite3* SqliteDatabase::db() {
    return mDb;
}

static int searchTableCallback(void* ptr,int rowLens,char** rows, char** cols) {
    if (rowLens == 0) {
        return 0;
    }
    auto result = static_cast<TableResult*>(ptr);
    for (int i = 0;i < rowLens; ++i) {
        (*result)[QString::fromUtf8(cols[i])].push_back(QString::fromUtf8(rows[i] ? rows[i] : "NULL"));
    }
    return 0;
}

int SqliteDatabase::open(const char* filename) {
    auto rc = sqlite3_open(filename, &mDb);
    if(rc != SQLITE_OK){
        std::cerr << "Error open database: " << sqlite3_errmsg(mDb) << std::endl;
    }
    return rc == SQLITE_OK;
}

void SqliteDatabase::close() {
    if (nullptr != mDb) {
        while(sqlite3_close(mDb) == SQLITE_BUSY) {
            sleep(100);
        }
    }
    mDb = nullptr;
}

int SqliteDatabase::queryTables(QStringList &tables) {
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name!='sqlite_sequence';";
    char** result;
    int nrow, ncolumn;
    char* errmsg;
    LOG(INFO) << "exec sql command: " << sql;
    auto rc = sqlite3_get_table(mDb, sql, &result, &nrow, &ncolumn, &errmsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error executing query: " << errmsg << std::endl;
        sqlite3_free(errmsg);
    } else {
        tables.resize(nrow);
        for (int i = 0;i < nrow; ++i) {
            tables[i] = QString(result[i + 1]);
        }
        sqlite3_free(errmsg);
        sqlite3_free_table(result);
    }
    return rc == SQLITE_OK;
}

int SqliteDatabase::createTable(const char* tableName, const QStringList heads) {
    QString sql = QString("%1%2%3").arg("CREATE TABLE IF NOT EXISTS ", tableName, " (");
    for (int i = 0;i < heads.size(); ++i) {
        sql += heads[i] + ((i != (heads.size() - 1)) ? "," : ");");
    }
    LOG(INFO) << "exec sql command: " << sql;
    auto rc = sqlite3_exec(mDb, sql.toUtf8(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Error executing query: " << sqlite3_errmsg(mDb) << std::endl;
    }

    return rc == SQLITE_OK;
}

int SqliteDatabase::insertValues(const char* tableAndHead, const QStringList values) {
    QString sql = "INSERT INTO ";
    sql += tableAndHead;
    sql += " VALUES (";
    for (int i = 0;i < values.size(); ++i) {
        sql += values[i] + ((i != (values.size() - 1)) ? "," : ");");
    }
    LOG(INFO) << "exec sql command: " << sql;
    auto rc = sqlite3_exec(mDb, sql.toUtf8(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare SQL statement: " << sqlite3_errmsg(mDb) << std::endl;
    }
    return rc == SQLITE_OK;
}

Result<TableResult> SqliteDatabase::queryValues(const char* tableName, QStringList heads, QString filter, QString groupBy, QString having, QString orderBy) {
    if (heads.size() == 0 || nullptr == tableName) {
        return Result<TableResult>();
    }
    QString sql = "SELECT ";
    for (auto head : heads) {
        sql.push_back(head + ",");
    }
    sql.remove(sql.length() - 1, 1);
    sql += " FROM ";
    sql += tableName;
    if (!filter.isEmpty()) {
        sql += " WHERE " + filter;
    }
    if (!groupBy.isEmpty()) {
        sql += " GROUP BY " + groupBy;
    }
    if (!having.isEmpty()) {
        sql += " HAVING " + having;
    }
    if (!orderBy.isEmpty()) {
        sql += " ORDER BY " + orderBy; 
    }
    sql += ";";
    LOG(INFO) << "exec sql command: " << sql;
    TableResult result;
    auto rc = sqlite3_exec(mDb, sql.toUtf8(), searchTableCallback, &result, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare SQL statement: " << sqlite3_errmsg(mDb) << std::endl;
        return Result<TableResult>();
    }
    return Result<TableResult>(result);
}
