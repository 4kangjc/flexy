#pragma once

#include <sqlite3.h>
#include <deque>
#include <map>
#include <memory>
#include <unordered_map>
#include "db.h"
#include "flexy/thread/mutex.h"
#include "flexy/util/singleton.h"

namespace flexy {

class SQLite3Manager;
class SQLite3 : public IDB, public std::enable_shared_from_this<SQLite3> {
    friend class SQLite3Manager;

public:
    enum Flags {
        READONLY = SQLITE_OPEN_READONLY,
        READWRITE = SQLITE_OPEN_READWRITE,
        CREATE = SQLITE_OPEN_CREATE
    };
    using ptr = std::shared_ptr<SQLite3>;
    static SQLite3::ptr Create(sqlite3* db) {
        return SQLite3::ptr(new SQLite3(db));
    }
    static SQLite3::ptr Create(const std::string& dbname,
                               int flags = READWRITE | CREATE);
    ~SQLite3() { close(); }

    IStmt::ptr prepare(const std::string& stmt) override;

    int getErrno() override { return sqlite3_errcode(db_); }
    std::string getErrStr() override { return sqlite3_errmsg(db_); }

    int execute(const char* fmt, ...) override;
    int execute(const char* fmt, va_list ap);
    int execute(const std::string& sql) override;
    int64_t getLastInsertId() override;
    ISQLData::ptr query(const char* fmt, ...) override;
    ISQLData::ptr query(const std::string& sql) override;

    ITransaction::ptr openTransaction(bool auto_commit = false) override;

    template <class... Args>
    int execStmt(const char* stmt, Args&&... args);

    template <class... Args>
    ISQLData::ptr queryStmt(const char* stmt, Args&&... args);

    int close();

    sqlite3* getDB() { return db_; }

private:
    SQLite3(sqlite3* db) : db_(db) {}

private:
    sqlite3* db_;
    uint64_t lastUsedTime_ = 0;
};

class SQLite3Stmt;
class SQLite3Data : public ISQLData {
public:
    using ptr = std::shared_ptr<SQLite3Data>;
    SQLite3Data(const std::shared_ptr<SQLite3Stmt>& stmt, int err,
                const char* errstr);
    SQLite3Data(std::shared_ptr<SQLite3Stmt>&& stmt, int err,
                const char* errstr);
    int getErrno() const override { return errno_; }
    const std::string& getErrStr() const override { return errstr_; }

    int getDataCount() override;
    int getColumnCount() override;
    int getColumnBytes(int idx) override;
    int getColumnType(int idx) override;

    std::string getColumnName(int idx) override;

    bool isNull(int idx) override;
    int8_t getInt8(int idx) override;
    uint8_t getUint8(int idx) override;
    int16_t getInt16(int idx) override;
    uint16_t getUint16(int idx) override;
    int32_t getInt32(int idx) override;
    uint32_t getUint32(int idx) override;
    int64_t getInt64(int idx) override;
    uint64_t getUint64(int idx) override;
    float getFloat(int idx) override;
    double getDouble(int idx) override;
    std::string getString(int idx) override;
    std::string getBlob(int idx) override;
    time_t getTime(int idx) override;
    std::string getTimeStr(int idx) override;
    bool next() override;

private:
    int errno_;
    bool first_;
    std::string errstr_;
    std::shared_ptr<SQLite3Stmt> stmt_;
};

class SQLite3Stmt : public IStmt,
                    public std::enable_shared_from_this<SQLite3Stmt> {
    friend class SQLite3Data;

public:
    using ptr = std::shared_ptr<SQLite3Stmt>;
    enum Type { COPY = 1, REF = 2 };
    static SQLite3Stmt::ptr Create(const SQLite3::ptr& db, const char* stmt);

    int prepare(const char* stmt);
    ~SQLite3Stmt() { finish(); }
    int finish();

    int bind(int idx, int32_t value);
    int bind(int idx, uint32_t value);
    int bind(int idx, double value);
    int bind(int idx, int64_t value);
    int bind(int idx, uint64_t value);
    int bind(int idx, const char* value, Type type = COPY);
    int bind(int idx, const void* value, int len, Type type = COPY);
    int bind(int idx, Blob blob, Type type = COPY) {
        return bind(idx, blob.value, blob.len, type);
    }
    int bind(int idx, const std::string& value, Type type = COPY);
    int bind(int idx);

    int bindInt8(int idx, int8_t value) override;
    int bindUint8(int idx, uint8_t value) override;
    int bindInt16(int idx, int16_t value) override;
    int bindUint16(int idx, uint16_t value) override;
    int bindInt32(int idx, int32_t value) override;
    int bindUint32(int idx, uint32_t value) override;
    int bindInt64(int idx, int64_t value) override;
    int bindUint64(int idx, uint64_t value) override;
    int bindFloat(int idx, float value) override;
    int bindDouble(int idx, double value) override;
    int bindString(int idx, const char* value) override;
    int bindString(int idx, const std::string& value) override;
    int bindBlob(int idx, const void* value, int64_t size) override;
    int bindBlob(int idx, const std::string& value) override;
    int bindTime(int idx, time_t value) override;
    int bindNull(int idx) override;

    int bind(const char* name, int32_t value);
    int bind(const char* name, uint32_t value);
    int bind(const char* name, double value);
    int bind(const char* name, int64_t value);
    int bind(const char* name, uint64_t value);
    int bind(const char* name, const char* value, Type type = COPY);
    int bind(const char* name, const void* value, int len, Type type = COPY);
    int bind(const char* name, const std::string& value, Type type = COPY);
    int bind(const char* name);

    int step() { return sqlite3_step(stmt_); }
    int reset() { return sqlite3_reset(stmt_); }

    ISQLData::ptr query() override;
    int execute() override;
    int64_t getLastInsertId() override;

    int getErrno() override { return db_->getErrno(); }
    std::string getErrStr() override { return db_->getErrStr(); }

protected:
    SQLite3Stmt(const SQLite3::ptr& db) : db_(db), stmt_(nullptr) {}
    SQLite3Stmt(SQLite3::ptr&& db) : db_(std::move(db)), stmt_(nullptr) {}

protected:
    SQLite3::ptr db_;
    sqlite3_stmt* stmt_;
};

class SQLite3Transaction : public ITransaction {
public:
    enum Type { DEFERRED = 0, IMMEDIATE = 1, EXCLUSIVE = 2 };
    SQLite3Transaction(const SQLite3::ptr& db, bool auto_commit = false,
                       Type type = DEFERRED);
    SQLite3Transaction(SQLite3::ptr&& db, bool auto_commit = false,
                       Type type = DEFERRED);
    ~SQLite3Transaction();
    bool begin() override;
    bool commit() override;
    bool rollback() override;

    int execute(const char* fmt, ...) override;
    int execute(const std::string& sql) override;
    int64_t getLastInsertId() override;

private:
    SQLite3::ptr db_;
    Type type_;
    int8_t status_;
    bool auto_commit_;
};

class SQLite3Manager {
public:
    SQLite3Manager() : maxConn_(10) {}
    ~SQLite3Manager();

    SQLite3::ptr get(const std::string& name);
    void registerSQLite3(const std::string& name,
                         const std::map<std::string, std::string>& params);
    void registerSQLite3(const std::string& name,
                         std::map<std::string, std::string>&& params);

    void checkConnection(uint64_t sec = 30);

    uint32_t getMaxConn() const { return maxConn_; }
    void setMaxConn(uint32_t v) { maxConn_ = v; }

    int execute(const std::string& name, const char* fmt, ...);
    int execute(const std::string& name, const char* fmt, va_list ap);
    int execute(const std::string& name, const std::string& sql);

    ISQLData::ptr query(const std::string& name, const char* fmt, ...);
    ISQLData::ptr query(const std::string& name, const char* fmt, va_list ap);
    ISQLData::ptr query(const std::string& name, const std::string& sql);

    template <class... Args>
    int execStmt(const std::string& name, const char* stmt, Args&&... args);

    template <class... Args>
    ISQLData::ptr queryStmt(const std::string& name, const char* stmt,
                            Args&&... args);

    SQLite3Transaction::ptr openTransaction(const std::string& name,
                                            bool auto_commit);

private:
    void freeSQLite3(const std::string& name, SQLite3* m);

private:
    uint32_t maxConn_;
    mutex mutex_;
    std::map<std::string, std::deque<SQLite3*>> conns_;
    std::map<std::string, std::map<std::string, std::string>> dbDefines_;
    std::unordered_map<std::string, int>
        counts_;  // conns_删除的个数 采用延迟删除
};

using SQLite3Mgr = Singleton<SQLite3Manager>;

template <typename... Args>
int SQLite3::execStmt(const char* stmt, Args&&... args) {
    auto st = SQLite3Stmt::Create(shared_from_this(), stmt);
    if (!st) {
        return -1;
    }
    int i = 0;
    int rt = (st->bind(++i, std::forward<Args>(args)) || ...);
    if (rt != SQLITE_OK) {
        return getErrno();
    }
    return st->execute();
}

template <typename... Args>
ISQLData::ptr SQLite3::queryStmt(const char* stmt, Args&&... args) {
    auto st = SQLite3Stmt::Create(shared_from_this(), stmt);
    if (!st) {
        return nullptr;
    }
    int i = 0;
    int rt = (st->bind(++i, std::forward<Args>(args)) || ...);
    if (rt != SQLITE_OK) {
        return nullptr;
    }
    return st->query();
}

template <class... Args>
int SQLite3Manager::execStmt(const std::string& name, const char* stmt,
                             Args&&... args) {
    auto conn = get(name);
    if (!conn) {
        return -1;
    }
    return conn->execStmt(stmt, std::forward<Args>(args)...);
}

template <class... Args>
ISQLData::ptr SQLite3Manager::queryStmt(const std::string& name,
                                        const char* stmt, Args&&... args) {
    auto conn = get(name);
    if (!conn) {
        return nullptr;
    }
    return conn->queryStmt(stmt, std::forward<Args>(args)...);
}

}  // namespace flexy