#include "mysql.h"
#include "flexy/util/log.h"
#include "flexy/util/config.h"

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");
static auto g_mysql_dbs = Config::Lookup("mysql.dbs",
                                         std::map<std::string, std::map<std::string, std::string>>(), "mysql dbs");

bool mysql_time_to_time_t(const MYSQL_TIME& mt, time_t& ts) {
    struct tm tm;
    ts = 0;
    localtime_r(&ts, &tm);
    tm.tm_year = mt.year - 1900;
    tm.tm_mon = mt.month - 1;
    tm.tm_mday = mt.day;
    tm.tm_hour = mt.hour;
    tm.tm_min = mt.minute;
    tm.tm_sec = mt.second;
    ts = mktime(&tm);
    if(ts < 0) {
        ts = 0;
    }
    return true;
}

bool time_t_to_mysql_time(const time_t& ts, MYSQL_TIME& mt) {
    struct tm tm;
    localtime_r(&ts, &tm);
    mt.year = tm.tm_year + 1900;
    mt.month = tm.tm_mon + 1;
    mt.day = tm.tm_mday;
    mt.hour = tm.tm_hour;
    mt.minute = tm.tm_min;
    mt.second = tm.tm_sec;
    return true;
}

std::optional<time_t> mysql_time_to_time_t(const MYSQL_TIME& mt) {
    struct tm tm;
    time_t ts = 0;
    localtime_r(&ts, &tm);              // 初始化 tm
    tm.tm_year = mt.year - 1900;
    tm.tm_mon = mt.month - 1;
    tm.tm_mday = mt.day;
    tm.tm_hour = mt.hour;
    tm.tm_min = mt.minute;
    tm.tm_sec = mt.second;
    ts = mktime(&tm);
    if (ts < 0) {
        return {};
    }
    return ts;
}

std::optional<MYSQL_TIME> time_t_to_mysql_time(const time_t& ts) {
    struct tm tm;
    localtime_r(&ts, &tm);
    MYSQL_TIME mt;
    mt.year = tm.tm_year + 1900;
    mt.month = tm.tm_mon + 1;
    mt.day = tm.tm_mday;
    mt.hour = tm.tm_hour;
    mt.minute = tm.tm_min;
    mt.second = tm.tm_sec;
    return mt;
}

namespace {

struct MySQLThreadIniter {
    MySQLThreadIniter() {
        mysql_thread_init();
    }

    ~MySQLThreadIniter() {
        mysql_thread_end();
    }
};

}

static MYSQL* mysql_init(std::map<std::string, std::string>& params, int timeout) {
    static thread_local MySQLThreadIniter s_thread_initer;
    
    MYSQL* mysql = ::mysql_init(nullptr);
    if (mysql == nullptr) {
        FLEXY_LOG_ERROR(g_logger) << "mysql_init error";
        return nullptr;
    }

    if (timeout > 0) {
        mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);
    }
    bool close = false;
    mysql_options(mysql, MYSQL_OPT_RECONNECT, &close);
    mysql_options(mysql, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    int port = GetParamValue(params, "port", 0);
    auto host = GetParamValue<std::string>(params, "host");
    auto user = GetParamValue<std::string>(params, "user");
    auto passwd = GetParamValue<std::string>(params, "passwd");
    auto dbname = GetParamValue<std::string>(params, "dbname");



//    FLEXY_LOG_INFO(g_logger) << "mysql_real_connect(" << host
//    << ", " << user << ", " << passwd << ", " << dbname << ", "
//    << port;

    if (mysql_real_connect(mysql, host.c_str(), user.c_str(), passwd.c_str(),
            dbname.c_str(), port, nullptr, 0) == nullptr) {
        FLEXY_LOG_ERROR(g_logger) << "mysql_real_connect(" << host
        << ", " << user << ", " << passwd << ", " << dbname << ", "
        << port << ") error: " << mysql_error(mysql);

        mysql_close(mysql);
        return nullptr;
    }
    return mysql;
}

MySQL::MySQL(const std::map<std::string, std::string>& args)
    : params_(args), lastUsedTime_(0), hasError_(false), poolSize_(10) {

}

bool MySQL::connect() {
    if (mysql_ && !hasError_) {
        return true;
    }
    MYSQL* m = mysql_init(params_, 0);
    if (!m) {
        hasError_ = true;
        return false;
    }
    hasError_ = false;
    poolSize_ = GetParamValue(params_, "pool", 5);
    mysql_.reset(m, mysql_close);
    return true;
}

IStmt::ptr MySQL::prepare(const std::string &sql) {
   return MySQLStmt::Create(shared_from_this(), sql);
}

ITransaction::ptr MySQL::openTransaction(bool auto_commit) {
    return MySQLTransaction::Create(shared_from_this(), auto_commit);
}

int64_t MySQL::getLastInsertId() {
    return mysql_insert_id(mysql_.get());
}

bool MySQL::isNeedCheck() {
    if (time(0) - lastUsedTime_ < 5 && !hasError_) {
        return false;
    }
    return true;
}

bool MySQL::ping() {
    if (!mysql_) {
        return false;
    }
    if (mysql_ping(mysql_.get())) {
        hasError_ = true;
        return false;
    }
    hasError_ = false;
    return true;
}

int MySQL::execute(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rt = execute(fmt, ap);
    va_end(ap);
    return rt;
}

int MySQL::execute(const char *fmt, va_list ap) {
    cmd_ = format(fmt, ap);
    int r = ::mysql_query(mysql_.get(), cmd_.c_str());
    if (r) {
        FLEXY_LOG_ERROR(g_logger) << "cmd = " << cmd() << ", error "
        << getErrStr();
        hasError_ = true;
    } else {
        hasError_ = false;
    }
    return r;
}

int MySQL::execute(const std::string &sql) {
    cmd_ = sql;
    int r = ::mysql_query(mysql_.get(), cmd_.c_str());
    if (r) {
        FLEXY_LOG_ERROR(g_logger) << "cmd = " << cmd() << ", error "
        << getErrStr();
        hasError_ = true;
    } else {
        hasError_ = false;
    }
    return r;
}

std::shared_ptr<MySQL> MySQL::getMySQL() {
    return nullptr;
}

std::shared_ptr<MYSQL> MySQL::getRaw() {
    return mysql_;
}

uint64_t MySQL::getAffectedRows() {
    if (!mysql_) {
        return 0;
    }
    return mysql_affected_rows(mysql_.get());
}

static MYSQL_RES* my_mysql_query(MYSQL* mysql, const char* sql) {
    if (mysql == nullptr) {
        FLEXY_LOG_ERROR(g_logger) << "mysql_query mysql is null";
        return nullptr;
    }

    if (sql == nullptr) {
        FLEXY_LOG_ERROR(g_logger) << "mysql_query sql is null";
        return nullptr;
    }

    if (::mysql_query(mysql, sql)) {
        FLEXY_LOG_ERROR(g_logger) << "mysql_query(" << sql << ") error:"
        << mysql_error(mysql);
        return nullptr;
    }
    MYSQL_RES* res = mysql_store_result(mysql);
    if (res == nullptr) {
        FLEXY_LOG_ERROR(g_logger) << "mysql_store_result() error:"
        << mysql_error(mysql);
    }
    return res;
}

const char* MySQL::cmd() {
    return cmd_.c_str();
}

bool MySQL::use(const std::string &dbname) {
    if (!mysql_) {
        return false;
    }
    if (dbname_ == dbname) {
        return true;
    }
    if (mysql_select_db(mysql_.get(), dbname_.c_str()) == 0) {
        dbname_ = dbname;
        hasError_ = false;
        return true;
    } else {
        dbname_ = "";
        hasError_ = true;
        return false;
    }
}

std::string MySQL::getErrStr() {
    if (!mysql_) {
        return "mysql is null";
    }
    const char* str = mysql_error(mysql_.get());
    if (str) {
        return str;
    }
    return "";
}

int MySQL::getErrno() {
    if (!mysql_) {
        return -1;
    }
    return mysql_errno(mysql_.get());
}

uint64_t MySQL::getInsertId() {
    if (mysql_) {
        return mysql_insert_id(mysql_.get());
    }
    return 0;
}

ISQLData::ptr MySQL::query(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    auto rt = query(fmt, ap);
    va_end(ap);
    return rt;
}

ISQLData::ptr MySQL::query(const char *fmt, va_list ap) {
    cmd_ = format(fmt, ap);
    MYSQL_RES* res = my_mysql_query(mysql_.get(),cmd_.c_str());
    if (!res) {
        hasError_ = true;
        return nullptr;
    }
    hasError_ = false;
    return ISQLData::ptr(new MySQLRes(res, mysql_errno(mysql_.get()),
                                      mysql_error(mysql_.get())));
}

ISQLData::ptr MySQL::query(const std::string &sql) {
    cmd_ = sql;
    MYSQL_RES* res = my_mysql_query(mysql_.get(), cmd_.c_str());
    if (!res) {
        hasError_ = true;
        return nullptr;
    }
    hasError_ = false;
    return ISQLData::ptr(new MySQLRes(res, mysql_errno(mysql_.get()),
                                      mysql_error(mysql_.get())));
}

MySQLRes::MySQLRes(MYSQL_RES *res, int eno, const char *estr)
    : errno_(eno), errstr_(estr), cur_(nullptr), curLength_(nullptr) {
    if (res) {
        data_.reset(res, mysql_free_result);
    }

}

bool MySQLRes::foreach(const MySQLRes::data_cb &cb) {
    MYSQL_ROW row;
    uint64_t fields = getColumnCount();
    int i = 0;
    while ((row = mysql_fetch_row(data_.get()))) {
        if (!cb(row, fields, i++)) {
            return false;
        }
    }
    return true;
}

int MySQLRes::getDataCount() {
    return mysql_num_rows(data_.get());
}

int MySQLRes::getColumnCount() {
    return mysql_num_fields(data_.get());
}

int MySQLRes::getColumnBytes(int idx) {
    return curLength_[idx];
}

int MySQLRes::getColumnType(int idx) {
    return 0;
}

std::string MySQLRes::getColumnName(int idx) {
    auto field = mysql_fetch_field_direct(data_.get(), idx);
    return field->name;
}

bool MySQLRes::next() {
    cur_ = mysql_fetch_row(data_.get());
    curLength_ = mysql_fetch_lengths(data_.get());
    return cur_;
}

bool MySQLRes::isNull(int idx) {
    if (cur_[idx] == nullptr) {
        return true;
    }
    return false;
}

int8_t MySQLRes::getInt8(int idx) {
    return getInt64(idx);
}

uint8_t MySQLRes::getUint8(int idx) {
    return getInt64(idx);
}

int16_t MySQLRes::getInt16(int idx) {
    return getInt64(idx);
}

uint16_t MySQLRes::getUint16(int idx) {
    return getInt64(idx);
}

int32_t MySQLRes::getInt32(int idx) {
    return getInt64(idx);
}

uint32_t MySQLRes::getUint32(int idx) {
    return getInt64(idx);
}

int64_t MySQLRes::getInt64(int idx) {
    return ::atoll(cur_[idx]);
}

uint64_t MySQLRes::getUint64(int idx) {
    return getInt64(idx);
}

float MySQLRes::getFloat(int idx) {
    return getDouble(idx);
}

double MySQLRes::getDouble(int idx) {
    return ::atof(cur_[idx]);
}

std::string MySQLRes::getString(int idx) {
    return std::string(cur_[idx], curLength_[idx]);
}

std::string MySQLRes::getBlob(int idx) {
    return std::string(cur_[idx], curLength_[idx]);
}

time_t MySQLRes::getTime(int idx) {
    if (!cur_[idx]) {
        return 0;
    }
    return StrToTime(cur_[idx]);
}

std::string MySQLRes::getTimeStr(int idx) {
    if (!cur_[idx]) {
        return 0;
    }
    return std::string(cur_[idx], curLength_[idx]);
}

MySQLStmt::ptr MySQLStmt::Create(const MySQL::ptr& db, const std::string& stmt) {
#define XX                                                      \
    auto st = mysql_stmt_init(db->getRaw().get());              \
    if (!st) {                                                  \
       return nullptr;                                          \
    }                                                           \
    if (mysql_stmt_prepare(st, stmt.c_str(), stmt.size())) {    \
       FLEXY_LOG_ERROR(g_logger) << "stmt = " << stmt           \
       << " errno = " << mysql_stmt_errno(st) << " errstr = "   \
       << mysql_stmt_error(st);                                 \
       mysql_stmt_close(st);                                    \
       return nullptr;                                          \
    }

    XX;
    MySQLStmt::ptr rt(new MySQLStmt(db, st)); 
#define YY                                                      \
    int count = mysql_stmt_param_count(st);                     \
    rt->binds_.resize(count);                                   \
    memset(&rt->binds_[0], 0, sizeof(rt->binds_[0]) * count);   \
    return rt       
    YY;
}

MySQLStmt::ptr MySQLStmt::Create(MySQL::ptr&& db, const std::string& stmt) {
    XX;
    MySQLStmt::ptr rt(new MySQLStmt(std::move(db), st));
    YY;
}

#undef XX
#undef YY


MySQLStmt::~MySQLStmt() {
    if (stmt_) {
        mysql_stmt_close(stmt_);
    }

    for (auto& i : binds_) {
        if (i.buffer) {
            free(i.buffer);
        }
    }
}

int MySQLStmt::bind(int idx, int8_t value) {
    return bindInt8(idx, value);
}

int MySQLStmt::bind(int idx, uint8_t value) {
    return bindUint8(idx, value);
}

int MySQLStmt::bind(int idx, int16_t value) {
    return bindInt16(idx, value);
}

int MySQLStmt::bind(int idx, uint16_t value) {
    return bindUint64(idx, value);
}

int MySQLStmt::bind(int idx, int32_t value) {
    return bindInt32(idx, value);
}

int MySQLStmt::bind(int idx, uint32_t value) {
    return bindUint32(idx, value);
}

int MySQLStmt::bind(int idx, int64_t value) {
    return bindInt64(idx, value);
}

int MySQLStmt::bind(int idx, uint64_t value) {
    return bindUint64(idx, value);
}

int MySQLStmt::bind(int idx, float value) {
    return bindFloat(idx, value);
}

int MySQLStmt::bind(int idx, double value) {
    return bindDouble(idx, value);
}

int MySQLStmt::bind(int idx, const std::string &value) {
    return bindString(idx, value);
}

int MySQLStmt::bind(int idx, const char *value) {
    return bindString(idx, value);
}

int MySQLStmt::bind(int idx, const void* value, int len) {
    return bindBlob(idx, value, len);
}

int MySQLStmt::bind(int idx) {
    binds_[--idx].buffer_type = MYSQL_TYPE_NULL;
    return 0;
}

int MySQLStmt::bindNull(int idx) {
    return bind(idx);
}

#define __BIND__(type, ptr, len, signed)        \
    if (idx < 1 || idx > (int)binds_.size())    \
        return -1;                              \
    binds_[--idx].buffer_type = type;           \
    if (binds_[idx].buffer == nullptr) {        \
        binds_[idx].buffer = malloc(len);       \
    }                                           \
    memcpy(binds_[idx].buffer, ptr, len);       \
    binds_[idx].is_unsigned = signed;           \
    binds_[idx].buffer_length = sizeof(value);  \
    return 0

int MySQLStmt::bindInt8(int idx, int8_t value) {
    __BIND__(MYSQL_TYPE_TINY, &value, sizeof(value), false);
}

int MySQLStmt::bindUint8(int idx, uint8_t value) {
    __BIND__(MYSQL_TYPE_TINY, &value, sizeof(value), true);
}

int MySQLStmt::bindInt16(int idx, int16_t value) {
    __BIND__(MYSQL_TYPE_SHORT, &value, sizeof(value), false);
}

int MySQLStmt::bindUint16(int idx, uint16_t value) {
    __BIND__(MYSQL_TYPE_SHORT, &value, sizeof(value), true);
}

int MySQLStmt::bindInt32(int idx, int32_t value) {
    __BIND__(MYSQL_TYPE_LONG, &value, sizeof(value), false);
}

int MySQLStmt::bindUint32(int idx, uint32_t value) {
    __BIND__(MYSQL_TYPE_LONG, &value, sizeof(value), true);
}

int MySQLStmt::bindInt64(int idx, int64_t value) {
    __BIND__(MYSQL_TYPE_LONGLONG, &value, sizeof(value), false);
}

int MySQLStmt::bindUint64(int idx, uint64_t value) {
    __BIND__(MYSQL_TYPE_LONGLONG, &value, sizeof(value), true);
}

int MySQLStmt::bindFloat(int idx, float value) {
    __BIND__(MYSQL_TYPE_FLOAT, &value, sizeof(value), false);
}

int MySQLStmt::bindDouble(int idx, double value) {
    __BIND__(MYSQL_TYPE_DOUBLE, &value, sizeof(value), false);
}

#undef __BIND__

#define __BIND2__(type, ptr, len)                                   \
    if (idx < 1 || idx > (int)binds_.size())                        \
        return -1;                                                  \
    binds_[--idx].buffer_type = type;                               \
    if (binds_[idx].buffer == nullptr) {                            \
        binds_[idx].buffer = malloc(len);                           \
    } else if ((size_t)binds_[idx].buffer_length < (size_t)len) {   \
        free(binds_[idx].buffer);                                   \
        binds_[idx].buffer = malloc(len);                           \
    }                                                               \
    memcpy(binds_[idx].buffer, ptr, len);                           \
    binds_[idx].buffer_length = len;                                \
    return 0

int MySQLStmt::bindString(int idx, const char *value) {
    __BIND2__(MYSQL_TYPE_STRING, value, strlen(value));
}

int MySQLStmt::bindString(int idx, const std::string &value) {
    __BIND2__(MYSQL_TYPE_STRING, value.c_str(), value.size());
}

int MySQLStmt::bindBlob(int idx, const void *value, int64_t size) {
    __BIND2__(MYSQL_TYPE_BLOB, value, size);
}

int MySQLStmt::bindBlob(int idx, const std::string& value) {
    __BIND2__(MYSQL_TYPE_BLOB, value.c_str(), value.size());
}

#undef __BIND2__

int MySQLStmt::bindTime(int idx, time_t value) {
    return bindString(idx, TimeToStr(value));
}

int MySQLStmt::execute() {
    mysql_stmt_bind_param(stmt_, &binds_[0]);
    return mysql_stmt_execute(stmt_);
}

int64_t MySQLStmt::getLastInsertId() {
    return mysql_stmt_insert_id(stmt_);
}

ISQLData::ptr MySQLStmt::query() {
    mysql_stmt_bind_param(stmt_, &binds_[0]);
    return MySQLStmtRes::Create(shared_from_this());
}

MySQLStmtRes::ptr MySQLStmtRes::Create(const std::shared_ptr<MySQLStmt> &stmt) {
    int eno = mysql_stmt_errno(stmt->getRaw());
    const char* errstr = mysql_stmt_error(stmt->getRaw());
    MySQLStmtRes::ptr rt(new MySQLStmtRes(stmt, eno, errstr));
    if (eno) {
        return rt;
    }

    // stmt->execute();

    MYSQL_RES* res = mysql_stmt_result_metadata(stmt->getRaw());
    if (!res) {
        return MySQLStmtRes::ptr(new MySQLStmtRes(stmt, stmt->getErrno(), stmt->getErrStr()));
    }

    int num = mysql_num_fields(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);

    rt->binds_.resize(num);
    memset(&rt->binds_[0], 0, sizeof(rt->binds_[0]) * num);
    rt->datas_.resize(num);

    for (int i = 0; i < num; ++i) {
        rt->datas_[i].type = fields[i].type;
        
        rt->datas_[i].name = fields[i].name;
        switch (fields[i].type) {
#define XX(m, t) \
            case m: \
                rt->datas_[i].alloc(sizeof(t)); \
                break;
            XX(MYSQL_TYPE_TINY, int8_t);
            XX(MYSQL_TYPE_SHORT, int16_t);
            XX(MYSQL_TYPE_LONG, int32_t);
            XX(MYSQL_TYPE_LONGLONG, int64_t);
            XX(MYSQL_TYPE_FLOAT, float);
            XX(MYSQL_TYPE_DOUBLE, double);
            XX(MYSQL_TYPE_TIMESTAMP, MYSQL_TIME);
            XX(MYSQL_TYPE_DATETIME, MYSQL_TIME);
            XX(MYSQL_TYPE_DATE, MYSQL_TIME);
            XX(MYSQL_TYPE_TIME, MYSQL_TIME);
#undef XX
            default:
                rt->datas_[i].alloc(fields[i].length);
                break;
        }

        rt->binds_[i].buffer_type = rt->datas_[i].type;
        rt->binds_[i].buffer = rt->datas_[i].data;
        rt->binds_[i].buffer_length = rt->datas_[i].data_length;
        rt->binds_[i].length = &rt->datas_[i].length;
        rt->binds_[i].is_null = &rt->datas_[i].is_null;
        rt->binds_[i].error = &rt->datas_[i].error;
    }

    if (mysql_stmt_bind_result(stmt->getRaw(), &rt->binds_[0])) {
        return MySQLStmtRes::ptr(new MySQLStmtRes(stmt, stmt->getErrno(),
                                                  stmt->getErrStr()));
    }

    stmt->execute();

    if (mysql_stmt_store_result(stmt->getRaw())) {
        return MySQLStmtRes::ptr(new MySQLStmtRes(stmt, stmt->getErrno(),
                                                  stmt->getErrStr())); 
    }

    return rt;
}

int MySQLStmtRes::getDataCount() {
    return mysql_stmt_num_rows(stmt_->getRaw());
}

int MySQLStmtRes::getColumnCount() {
    return mysql_stmt_field_count(stmt_->getRaw());
}

int MySQLStmtRes::getColumnBytes(int idx) {
    return datas_[idx].length;
}

int MySQLStmtRes::getColumnType(int idx) {
    return datas_[idx].type;
}

std::string MySQLStmtRes::getColumnName(int idx) {
    return datas_[idx].name;
}

const std::string& MySQLStmtRes::getColumnName(int idx) const {
    return datas_[idx].name;
}

bool MySQLStmtRes::isNull(int idx) {
    return datas_[idx].is_null;
}

#define XX(type) \
    return *(type*)datas_[idx].data

int8_t MySQLStmtRes::getInt8(int idx) {
    XX(int8_t);
}

uint8_t MySQLStmtRes::getUint8(int idx) {
    XX(uint8_t);
}

int16_t MySQLStmtRes::getInt16(int idx) {
    XX(int16_t);
}

uint16_t MySQLStmtRes::getUint16(int idx) {
    XX(uint16_t);
}

int32_t MySQLStmtRes::getInt32(int idx) {
    XX(int32_t);
}

uint32_t MySQLStmtRes::getUint32(int idx) {
    XX(uint32_t);
}

int64_t MySQLStmtRes::getInt64(int idx) {
    XX(int64_t);
}

uint64_t MySQLStmtRes::getUint64(int idx) {
    XX(uint64_t);
}

float MySQLStmtRes::getFloat(int idx) {
    XX(float);
}

double MySQLStmtRes::getDouble(int idx) {
    XX(double);
}

#undef XX

std::string MySQLStmtRes::getString(int idx) {
    return std::string(datas_[idx].data, datas_[idx].length);
}

std::string MySQLStmtRes::getBlob(int idx) {
    return std::string(datas_[idx].data, datas_[idx].length);
}

time_t MySQLStmtRes::getTime(int idx) {
    MYSQL_TIME* v = (MYSQL_TIME*)datas_[idx].data;
    auto time_ptr = mysql_time_to_time_t(*v);
    return time_ptr ? *time_ptr : 0;
}

std::string MySQLStmtRes::getTimeStr(int idx) {
    return TimeToStr(getTime(idx));
}


bool MySQLStmtRes::next() {
    return !mysql_stmt_fetch(stmt_->getRaw());
}

MySQLStmtRes::Data::Data() : is_null(false), error(false), type(),
                            length(0), data_length(0), data(nullptr) {
}

MySQLStmtRes::Data::~Data() {
    if (data) {
        delete[] data;
        data = nullptr;
    }
}

void MySQLStmtRes::Data::alloc(size_t size) {
    if (data) {
        delete[] data;
    }
    data = new char[size]();
    length = size;
    data_length = size;
}

MySQLStmtRes::MySQLStmtRes(const std::shared_ptr <MySQLStmt> &stmt, int eno, const std::string &estr)
    : errno_(eno), errstr_(estr), stmt_(stmt) {
}

MySQLStmtRes::MySQLStmtRes(std::shared_ptr <MySQLStmt> &&stmt, int eno, const std::string &errstr)
    : errno_(eno), errstr_(errstr), stmt_(std::move(stmt)) {
}

MySQLStmtRes::~MySQLStmtRes() {
    if (!errno_) {
        mysql_stmt_free_result(stmt_->getRaw());
    }
}

MySQLTransaction::ptr MySQLTransaction::Create(const MySQL::ptr &mysql, bool auto_commit) {
    MySQLTransaction::ptr rt(new MySQLTransaction(mysql, auto_commit));
    if (rt->begin()) {
        return rt;
    }
    return nullptr;
}

MySQLTransaction::ptr MySQLTransaction::Create(MySQL::ptr &&mysql, bool auto_commit) {
    MySQLTransaction::ptr rt(new MySQLTransaction(std::move(mysql), auto_commit));
    if (rt->begin()) {
        return rt;
    }
    return nullptr;
}

MySQLTransaction::~MySQLTransaction() {
    if (autoCommit_) {
        commit();
    } else {
        rollback();
    }
}

int64_t MySQLTransaction::getLastInsertId() {
    return mysql_->getLastInsertId();
}

bool MySQLTransaction::begin() {
    return execute("BEGIN") == 0;
}

bool MySQLTransaction::commit() {
    if (isFinished_ || hasError_) {
        return !hasError_;
    }
    int rt = execute("COMMIT");
    if (rt == 0) {
        isFinished_ = true;
    } else {
        hasError_ = true;
    }
    return rt == 0;
}

bool MySQLTransaction::rollback() {
    if (isFinished_) {
        return true;
    }   
    int rt = execute("ROLLBACK");
    if (rt == 0) {
        isFinished_ = true;
    } else {
        hasError_ = true;
    }
    return rt == 0;
}

int MySQLTransaction::execute(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rt = execute(fmt, ap);
    va_end(ap);
    return rt;
}

int MySQLTransaction::execute(const char *fmt, va_list ap) {
    if (isFinished_) {
        FLEXY_LOG_ERROR(g_logger) << "transaction is finished, format = " << fmt;
        return -1;
    }
    int rt = mysql_->execute(fmt, ap);
    if (rt) {
        hasError_ = true;
    }
    return rt;
}

int MySQLTransaction::execute(const std::string &sql) {
    if (isFinished_) {
        FLEXY_LOG_ERROR(g_logger) << "transaction is finished, sql = " << sql;
        return -1;
    }
    int rt = mysql_->execute(sql);
    if (rt) {
        hasError_ = true;
    }
    return rt;
}

std::shared_ptr<MySQL> MySQLTransaction::getMySQL() {
    return mysql_;
}

MySQLTransaction::MySQLTransaction(const MySQL::ptr &mysql, bool auto_commit)
    : mysql_(mysql), autoCommit_(auto_commit), isFinished_(false), hasError_(false) {
}

MySQLTransaction::MySQLTransaction(MySQL::ptr&& mysql, bool auto_commit)
    : mysql_(std::move(mysql)), autoCommit_(auto_commit), isFinished_(false), hasError_(false) {
}

MySQLManager::MySQLManager() : maxConn_(10) {
    mysql_library_init(0, nullptr, nullptr);
}

MySQLManager::~MySQLManager() {
    mysql_library_end();
    for (auto& [name, mysql_deque] : conns_) {
        for (auto sql_ptr : mysql_deque) {
            delete sql_ptr;
        }
    }
}

MySQL::ptr MySQLManager::get(const std::string &name) {
    unique_lock<decltype(mutex_)> lock(mutex_);
    if (auto it = conns_.find(name); it != conns_.end()) {
        if (it->second.size() - count_[name] != 0) {
            while (!it->second.front()) {
                it->second.pop_front();
                count_[name]--;
            }
            MySQL* rt = it->second.front();
            it->second.pop_front();
            lock.unlock();

            if (!rt->isNeedCheck()) {
                rt->lastUsedTime_ = time(0);
                return MySQL::ptr(rt, [this, name](MySQL* m) {
                    freeMySQL(name, m);
                });
            }

            if (rt->ping()) {
                rt->lastUsedTime_ = time(0);
                return MySQL::ptr(rt, [this, name](MySQL* m) {
                    freeMySQL(name, m);
                });
            } else if (rt->connect()) {
                rt->lastUsedTime_ = time(0);
                return MySQL::ptr(rt, [this, name](MySQL* m) {
                    freeMySQL(name, m);
                });
            } else {
                FLEXY_LOG_WARN(g_logger) << "reconnect " << name << " fail";
                return nullptr;
            }
        }
    }
    std::map<std::string, std::string> args;
    auto& config = g_mysql_dbs->getValue();
    if (auto it = config.find(name); it != config.end()) {
        args = it->second;
    } else {
        it = dbDefines_.find(name);
        if (it != dbDefines_.end()) {
            args = it->second;
        } else {
            return nullptr;
        }
    }
    lock.unlock();
    MySQL* rt = new MySQL(args);
    if (rt->connect()) {
        rt->lastUsedTime_ = time(0);
        return MySQL::ptr(rt, [this, name](MySQL* m) {
            freeMySQL(name, m);
        });
    } else {
        delete rt;
        return nullptr;
    }
}

void MySQLManager::registerMySQL(const std::string &name, const std::map <std::string, std::string> &params) {
    LOCK_GUARD(mutex_);
    dbDefines_[name] = params;
}

void MySQLManager::registerMySQL(const std::string &name, std::map <std::string, std::string>&& params) {
    LOCK_GUARD(mutex_);
    dbDefines_[name] = std::move(params);
}

void MySQLManager::checkConnection(uint64_t sec) {
    uint64_t now = time(0);
    std::vector<MySQL*> conns;
    {
        LOCK_GUARD(mutex_);
        for (auto& [name, mysql_deque] : conns_) {
            for (auto& mysql_ptr : mysql_deque) {
                if (now - mysql_ptr->lastUsedTime_ >= sec) {
                    conns.push_back(mysql_ptr);
                    mysql_ptr = nullptr;
                }
            }
        }
    }

    for (auto ptr : conns) {
        delete ptr;
    }
}

int MySQLManager::execute(const std::string &name, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rt = execute(name, fmt, ap);
    va_end(ap);
    return rt;
}

int MySQLManager::execute(const std::string &name, const char *fmt, va_list ap)  {
    auto conn = get(name);
    if (!conn) {
        FLEXY_LOG_ERROR(g_logger) << "MySQLManager::execute, get(" << name
        << ") fail, format = " << fmt;
        return -1;
    }
    return conn->execute(fmt, ap);
}

int MySQLManager::execute(const std::string &name, const std::string &sql) {
    auto conn = get(name);
    if (!conn) {
        FLEXY_LOG_ERROR(g_logger) << "MySQLManager::execute, get(" << name 
        << ") fail, sql = " << sql;
        return -1;
    }   
    return conn->execute(sql);
}

ISQLData::ptr MySQLManager::query(const std::string &name, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    auto rt = query(name, fmt, ap);
    va_end(ap);
    return rt;
}

ISQLData::ptr MySQLManager::query(const std::string &name, const char *fmt, va_list ap)  {
    auto conn = get(name);
    if (!conn) {
        FLEXY_LOG_ERROR(g_logger) << "MySQLManager::query, get(" << name
        << ") fail, format = " << fmt;
        return nullptr;
    }
    return conn->query(fmt, ap);
}

ISQLData::ptr MySQLManager::query(const std::string &name, const std::string &sql) {
    auto conn = get(name);
    if (!conn) {
        FLEXY_LOG_ERROR(g_logger) << "MySQLManager::query, get(" << name 
        << ") fail, sql = " << sql;
        return nullptr;
    }   
    return conn->query(sql);
}

MySQLTransaction::ptr MySQLManager::openTransaction(const std::string &name, bool auto_commit) {
    auto conn = get(name);
    if (!conn) {
        FLEXY_LOG_ERROR(g_logger) << "MySQLManager::openTransaction, get(" << name 
        << ") fail";
        return nullptr;
    }
    return MySQLTransaction::Create(conn, auto_commit);
}

void MySQLManager::freeMySQL(const std::string &name, MySQL *m) {
    if (m->mysql_) {
        LOCK_GUARD(mutex_);
        if (conns_[name].size() - count_[name] < (size_t)m->poolSize_) {
            conns_[name].push_back(m);
            return;
        }
    }
    delete m;
}


} // namespace flexy