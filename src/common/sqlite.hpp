#include <QString>
#include <QStringList>

#include "promise.hpp"

class sqlite3;

typedef QMap<QString, QVector<QString>> TableResult;

class SqliteDatabase {
public:
    SqliteDatabase();
    ~SqliteDatabase();

    [[nodiscard]] int open(const char* filename);
    void close();
    [[nodiscard]] int queryTables(QStringList &tables);
    [[nodiscard]] int createTable(const char* tableName, const QStringList heads);
    [[nodiscard]] int insertValues(const char* tableAndHead, const QStringList values);
    Result<TableResult> queryValues(const char* tableName, QStringList heads = {"*"}, QString filter = "",QString groupBy = "",QString having = "", QString orderBy = "");

    sqlite3* db();
private:
    sqlite3* mDb;

};