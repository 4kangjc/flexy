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
//    << port << ") error: " << mysql_error(mysql);

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
    poolSize_ = GetParamValue(params_, "poll", 5);
    mysql_.reset(m, mysql_close);
    return true;
}

IStmt::ptr MySQL::prepare(const std::string &sql) {
//    return MySQLStmt::Create(shared_from_this(), sql);
    return nullptr;
}

ITransaction::ptr MySQL::openTransaction(bool auto_commit) {
    return nullptr;
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

//MySQLStmt::ptr MySQLStmt::Create(const MySQL::ptr& db, const std::string& stmt) {
//    auto st = mysql_stmt_init(db)
//}

MySQLRes::MySQLRes(MYSQL_RES *res, int eno, const char *estr) {

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


} // namespace flexy