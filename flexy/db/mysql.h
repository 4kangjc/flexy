#pragma once

#include "db.h"
#include "flexy/thread/mutex.h"
#include "flexy/util/singleton.h"
#include <mysql/mysql.h>
#include <functional>
#include <optional>
#include <map>
#include <unordered_map>
#include <deque>

namespace flexy {

class MySQL;
class MySQLStmt;

struct MySQLTime {
    MySQLTime(time_t t) : ts(t) {}
    time_t ts;
};

bool mysql_time_to_time_t(const MYSQL_TIME& mt, time_t& ts);
bool time_t_to_mysql_time(const time_t& ts, MYSQL_TIME& mt);

std::optional<time_t> mysql_time_to_time_t(const MYSQL_TIME& mt);
std::optional<MYSQL_TIME> time_t_to_mysql_time(const time_t& ts);

class MySQLRes : public ISQLData {
public:
    using ptr = std::shared_ptr<MySQLRes>;
    using data_cb = std::function<bool(MYSQL_ROW row, int field_count, int row_no)>;
    MySQLRes(MYSQL_RES* res, int eno, const char* estr);
    MYSQL_RES* get() const { return data_.get(); }

    int getErrno() const override { return errno_; }
    const std::string& getErrStr() const override { return errstr_; }

    bool foreach(const data_cb& cb);

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
    std::string errstr_;
    MYSQL_ROW cur_;
    unsigned long* curLength_;
    std::shared_ptr<MYSQL_RES> data_;
};

class MySQLStmtRes : public ISQLData {
friend class MySQLStmt;
public:
    typedef std::shared_ptr<MySQLStmtRes> ptr;
    static MySQLStmtRes::ptr Create(const std::shared_ptr<MySQLStmt>& stmt);
    ~MySQLStmtRes();

    int getErrno() const override { return errno_;}
    const std::string& getErrStr() const override { return errstr_;}

    int getDataCount() override;
    int getColumnCount() override;
    int getColumnBytes(int idx) override;
    int getColumnType(int idx) override;
    std::string getColumnName(int idx) override;
    const std::string& getColumnName(int idx) const;

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
    MySQLStmtRes(const std::shared_ptr<MySQLStmt>& stmt, int eno, const std::string& errstr);
    MySQLStmtRes(std::shared_ptr<MySQLStmt>&& stmt, int eno, const std::string& errstr);
    struct Data {
        Data();
        ~Data();

        void alloc(size_t size);

        // my_bool is_null;
        bool is_null;
        // my_bool error;
        bool error;
        enum_field_types type;
        unsigned long length;
        int32_t data_length;
        char* data;
        std::string name;
    };
private:
    int errno_;
    std::string errstr_;
    std::shared_ptr<MySQLStmt> stmt_;
    std::vector<MYSQL_BIND> binds_;
    std::vector<Data> datas_;
};

class MySQLManager;
class MySQL : public IDB, public std::enable_shared_from_this<MySQL> {
friend class MySQLManager;
public:
    using ptr = std::shared_ptr<MySQL>;

    MySQL(const std::map<std::string, std::string>& args);

    bool connect();
    bool ping();

    int execute(const char* fmt, ...) override;
    int execute(const char* fmt, va_list ap);
    int execute(const std::string& sql) override;
    int64_t getLastInsertId() override;

    ptr getMySQL();
    std::shared_ptr<MYSQL> getRaw();

    uint64_t getAffectedRows();

    ISQLData::ptr query(const char* fmt, ...) override;
    ISQLData::ptr query(const char* fmt, va_list ap);
    ISQLData::ptr query(const std::string& sql) override;

    ITransaction::ptr openTransaction(bool auto_commit) override;
    IStmt::ptr prepare(const std::string& sql) override;

    template <typename... Args>
    int execStmt(const char* stmt, Args&&... args);

    template <typename... Args>
    ISQLData::ptr queryStmt(const char* stmt, Args&&... args);

    const char* cmd();

    bool use(const std::string& dbname);
    int getErrno() override;
    std::string getErrStr() override;
    uint64_t getInsertId();
private:
    bool isNeedCheck();
private:
    std::map<std::string, std::string> params_;
    std::shared_ptr<MYSQL> mysql_;

    std::string cmd_;
    std::string dbname_;

    uint64_t lastUsedTime_;
    bool hasError_;
    int32_t poolSize_;
};

class MySQLTransaction : public ITransaction {
public:
    using ptr = std::shared_ptr<MySQLTransaction>;

    static MySQLTransaction::ptr Create(const MySQL::ptr& mysql, bool auto_commit);
    static MySQLTransaction::ptr Create(MySQL::ptr&& mysql, bool auto_commit);
    ~MySQLTransaction();

    bool begin() override;
    bool commit() override;
    bool rollback() override;

    int execute(const char* fmt, ...) override;
    int execute(const char* fmt, va_list ap);
    int execute(const std::string& sql) override;
    int64_t getLastInsertId() override;
    std::shared_ptr<MySQL> getMySQL();

    bool isAutoCommit() const { return autoCommit_; }
    bool isFinished() const { return isFinished_; }
    bool isError() const { return hasError_; }

private:
    MySQLTransaction(const MySQL::ptr& mysql, bool auto_commit);
    MySQLTransaction(MySQL::ptr&& mysql, bool auto_commit);

private:
    MySQL::ptr mysql_;
    bool autoCommit_;
    bool isFinished_;
    bool hasError_;
};

class MySQLStmt : public IStmt, public std::enable_shared_from_this<MySQLStmt> {
public:
    using ptr = std::shared_ptr<MySQLStmt>;
    static MySQLStmt::ptr Create(const MySQL::ptr& db, const std::string& stmt);
    static MySQLStmt::ptr Create(MySQL::ptr&& db, const std::string& stmt);

    ~MySQLStmt();
    int bind(int idx, int8_t value);
    int bind(int idx, uint8_t value);
    int bind(int idx, int16_t value);
    int bind(int idx, uint16_t value);
    int bind(int idx, int32_t value);
    int bind(int idx, uint32_t value);
    int bind(int idx, int64_t value);
    int bind(int idx, uint64_t value);
    int bind(int idx, float value);
    int bind(int idx, double value);
    int bind(int idx, const std::string& value);
    int bind(int idx, const char* value);
    int bind(int idx, const void* value, int len);
    int bind(int idx, Blob blob) { return bind(idx, blob.value, blob.len); }
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

    int getErrno() override { return mysql_stmt_errno(stmt_); }
    std::string getErrStr() override { auto e = mysql_stmt_error(stmt_); return e ? e : ""; }

    int execute() override;
    int64_t getLastInsertId() override;
    ISQLData::ptr query() override;

    MYSQL_STMT* getRaw() const { return stmt_; }

private:
    MySQLStmt(const MySQL::ptr& db, MYSQL_STMT* stmt) : mysql_(db), stmt_(stmt) {}
    MySQLStmt(MySQL::ptr&& db, MYSQL_STMT* stmt) : mysql_(std::move(db)), stmt_(stmt) {}

private:
    MySQL::ptr mysql_;
    MYSQL_STMT* stmt_;
    std::vector<MYSQL_BIND> binds_;
};

class MySQLManager{
public:
    MySQLManager();
    ~MySQLManager();

    MySQL::ptr get(const std::string& name);
    void registerMySQL(const std::string& name, const std::map<std::string, std::string>& params);
    void registerMySQL(const std::string& name, std::map<std::string, std::string>&& params);

    void checkConnection(uint64_t sec = 30);

    uint32_t getMaxConn() const { return maxConn_; }
    void setMaxConn(uint32_t v) { maxConn_ = v; }

    int execute(const std::string& name, const char* fmt, ...);
    int execute(const std::string& name, const char* fmt, va_list ap);
    int execute(const std::string& name, const std::string& sql);

    ISQLData::ptr query(const std::string& name, const char* fmt, ...);
    ISQLData::ptr query(const std::string& name, const char* fmt, va_list ap);
    ISQLData::ptr query(const std::string& name, const std::string& sql);

    MySQLTransaction::ptr openTransaction(const std::string& name, bool auto_commit);

    template <class... Args>
    int execStmt(const std::string& name, const char* stmt, Args&&... args);

    template <class... Args>
    ISQLData::ptr queryStmt(const std::string& name, const char* stmt, Args&&... args);

private:
    void freeMySQL(const std::string& name, MySQL* m);

private:
    uint32_t maxConn_;
    mutex mutex_;
    std::map<std::string, std::deque<MySQL*>> conns_;
    std::map<std::string, std::map<std::string, std::string>> dbDefines_;
    std::unordered_map<std::string, int> count_;
};

using MySQLMgr = Singleton<MySQLManager>;

template <class... Args>
int MySQL::execStmt(const char* stmt, Args&&... args) {
    auto st = MySQLStmt::Create(shared_from_this(), stmt);
    if (!st) {
        return -1;
    }
    int idx = 0;
    int rt = (st->bind(++idx, std::forward<Args>(args)) || ...);
    if (rt != 0) {
        return rt;
    }
    return st->execute();
}

template <class... Args>
ISQLData::ptr MySQL::queryStmt(const char* stmt, Args&&... args) {
    auto st = MySQLStmt::Create(shared_from_this(), stmt);
    if (!st) {
        return nullptr;
    }
    int idx = 0;
    int rt = (st->bind(++idx, std::forward<Args>(args)) || ...);
    if (rt != 0) {
        return nullptr;
    }
    return st->query();
}

template <class... Args>
int MySQLManager::execStmt(const std::string& name, const char* stmt, Args&&... args) {
    auto conn = get(name);
    if (!conn) {
        return -1;
    }
    return conn->execStmt(stmt, std::forward<Args>(args)...);
}

template <class... Args>
ISQLData::ptr MySQLManager::queryStmt(const std::string& name, const char* stmt, Args&&... args) {
    auto conn = get(name);
    if (!conn) {
        return nullptr;
    }
    return conn->queryStmt(stmt, std::forward<Args>(args)...);
}

} // namespace flexy