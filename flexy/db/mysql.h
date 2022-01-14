#pragma once

#include "db.h"
#include <mysql/mysql.h>
#include <functional>
#include <optional>
#include <map>

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
    bool next() override;
private:
    int errno_;
    std::string errstr_;
    MYSQL_ROW cur_;
    unsigned long* curLength_;
    std::unique_ptr<MYSQL_RES> data_;
};

class MySQLStmtRes : public ISQLData {
friend class MySQLStmt;
public:
    typedef std::shared_ptr<MySQLStmtRes> ptr;
    static MySQLStmtRes::ptr Create(std::shared_ptr<MySQLStmt> stmt);
    ~MySQLStmtRes();

    int getErrno() const override { return errno_;}
    const std::string& getErrStr() const override { return errstr_;}

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
    bool next() override;
private:
    MySQLStmtRes(const std::shared_ptr<MySQLStmt>& stmt, int eno, const std::string& estr);
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

}