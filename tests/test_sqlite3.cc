#include <flexy/db/sqlite3.h>
#include <flexy/util/log.h>
#include <flexy/util/file.h>
#include <iostream>


static auto&& g_logger = FLEXY_LOG_ROOT();

int main() {
    std::string dbname = "test.db";
    dbname = flexy::FS::AbsolutePath(dbname);
    auto db = flexy::SQLite3::Create(dbname, flexy::SQLite3::READWRITE);

    if (!db) {
        FLEXY_LOG_INFO(g_logger) << "dbname = " << dbname << " not exits";
        db = flexy::SQLite3::Create(dbname, flexy::SQLite3::READWRITE | flexy::SQLite3::CREATE);
        if (!db) {
            FLEXY_LOG_ERROR(g_logger) << "dbname = " << dbname << " create error";
            return 0;
        }
#define XX(...) #__VA_ARGS__
        int rt = db->execute(
XX(create table user (
            id integer primary key autoincrement,
            name varchar(50) not null default "",
            age int not null default 0,
            create_time datetime
           )));
#undef XX

        if (rt != SQLITE_OK) {
            FLEXY_LOG_ERROR(g_logger) << "create table error "
            << db->getErrno() << " - " << db->getErrStr();
            return 0;
        }
    }
    db->execStmt("insert into user (name, age) values (?, ?)", "stmt_1", 1);
    auto stmt = flexy::SQLite3Stmt::Create(db, "insert into user(name, age, create_time) values(?, ?, ?)");
    if (!stmt) {
        FLEXY_LOG_ERROR(g_logger) << "create statement error " << db->getErrno() << " - " << db->getErrStr();
    }
    int64_t now = time(0);
    for (int i = 2; i < 10; ++i) {
        stmt->bind(1, "stmt_" + std::to_string(i));
        stmt->bind(2, i);
        stmt->bind(3, now + rand() % 100);
        if (stmt->execute() != SQLITE_OK) {
            FLEXY_LOG_ERROR(g_logger) << "execute statement error " << i << " "
            << db->getErrno() << " - " << db->getErrStr();
        }
        stmt->reset();
    }

    if (auto res = db->query("select * from user")) {
        std::cout << "---------------------------------------------\n";
        int col = res->getColumnCount();
        for (int j = 0; j < col; ++j) {
            std::cout << res->getColumnName(j) << '\t';
        }
        std::cout << std::endl;

        while (res->next()) {
            for (int j = 0; j < col; ++j) {
                std::cout << res->getString(j) << '\t';
            }
            std::cout << std::endl;
        }
        std::cout << "---------------------------------------------\n";
    }

}
