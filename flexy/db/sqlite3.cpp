#include "sqlite3.h"
#include "flexy/util/log.h"
#include "flexy/util/config.h"
#include "flexy/env/env.h"

namespace flexy {

static auto g_logger = FLEXY_LOG_NAME("system");

static auto g_sqlite3_dbs = Config::Lookup("sqlite3.dbs", std::map<std::string, 
                            std::map< std::string, std::string>>(), "sqlite3 dbs");

SQLite3::ptr SQLite3::Create(const std::string &dbname, int flags) {
    sqlite3* db;
    if (sqlite3_open_v2(dbname.c_str(), &db, flags, nullptr) == SQLITE_OK) {
        return SQLite3::ptr (new SQLite3(db));
    }
    return nullptr;
}

ITransaction::ptr SQLite3::openTransaction(bool auto_commit) {
    ITransaction::ptr trans(new SQLite3Transaction(shared_from_this(), auto_commit));
    if (trans->begin()) {
        return trans;
    }
    return nullptr;
}

IStmt::ptr SQLite3::prepare(const std::string &stmt) {
    return SQLite3Stmt::Create(shared_from_this(), stmt.c_str());
}

int SQLite3::execute(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rt = execute(fmt, ap);
    va_end(ap);
    return rt;
}

int SQLite3::execute(const char *fmt, va_list ap) {
    std::shared_ptr<char> sql(sqlite3_vmprintf(fmt, ap), sqlite3_free);
    return sqlite3_exec(db_, sql.get(), nullptr, nullptr, nullptr);
}

int SQLite3::execute(const std::string &sql) {
    return sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, nullptr);
}

ISQLData::ptr SQLite3::query(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    std::shared_ptr<char> sql(sqlite3_vmprintf(fmt, ap), sqlite3_free);
    va_end(ap);
    auto stmt = SQLite3Stmt::Create(shared_from_this(), sql.get());
    return stmt ? stmt->query() : nullptr;
}

ISQLData::ptr SQLite3::query(const std::string &sql) {
    auto stmt = SQLite3Stmt::Create(shared_from_this(), sql.c_str());
    return stmt ? stmt->query() : nullptr;
}

int64_t SQLite3::getLastInsertId() {
    return sqlite3_last_insert_rowid(db_);
}

int SQLite3::close() {
    int rc = SQLITE_OK;
    if (db_) {
        rc = sqlite3_close(db_);
        if (rc == SQLITE_OK) {
            db_ = nullptr;
        }
    }
    return rc;
}

SQLite3Data::SQLite3Data(const std::shared_ptr<SQLite3Stmt> &stmt, int err, const char *errstr)
        : errno_(err), first_(true), errstr_(errstr), stmt_(stmt) {
}

SQLite3Data::SQLite3Data(std::shared_ptr<SQLite3Stmt> &&stmt, int err, const char *errstr)
    : errno_(err), first_(true), errstr_(errstr), stmt_(std::move(stmt)) {

}

int SQLite3Data::getDataCount() {
    return -1;
}

int SQLite3Data::getColumnCount() {
    return sqlite3_column_count(stmt_->stmt_);
}

int SQLite3Data::getColumnBytes(int idx) {
    return sqlite3_column_bytes(stmt_->stmt_, idx);
}

int SQLite3Data::getColumnType(int idx) {
    return sqlite3_column_type(stmt_->stmt_, idx);
}

std::string SQLite3Data::getColumnName(int idx) {
    const char* name = sqlite3_column_name(stmt_->stmt_, idx);
    return name ? name : "";
}

bool SQLite3Data::isNull(int idx) {
    return false;
}

int8_t SQLite3Data::getInt8(int idx) {
    return getInt32(idx);
}

uint8_t SQLite3Data::getUint8(int idx) {
    return getInt32(idx);
}

int16_t SQLite3Data::getInt16(int idx) {
    return getInt32(idx);
}

uint16_t SQLite3Data::getUint16(int idx) {
    return getInt32(idx);
}

int32_t SQLite3Data::getInt32(int idx) {
    return sqlite3_column_int(stmt_->stmt_, idx);
}

uint32_t SQLite3Data::getUint32(int idx) {
    return getInt32(idx);
}

int64_t SQLite3Data::getInt64(int idx) {
    return sqlite3_column_int64(stmt_->stmt_, idx);
}

uint64_t SQLite3Data::getUint64(int idx) {
    return getInt64(idx);
}

float SQLite3Data::getFloat(int idx) {
    return getDouble(idx);
}

double SQLite3Data::getDouble(int idx) {
    return sqlite3_column_double(stmt_->stmt_, idx);
}

std::string SQLite3Data::getString(int idx) {
    auto v = (const char*)sqlite3_column_text(stmt_->stmt_, idx);
    return v ? std::string(v, getColumnBytes(idx)) : "";
}

std::string SQLite3Data::getBlob(int idx) {
    auto v = (const char*) sqlite3_column_blob(stmt_->stmt_, idx);
    return v ? std::string(v, getColumnBytes(idx)) : " ";
}

time_t SQLite3Data::getTime(int idx) {
    auto str = getString(idx);
    return StrToTime(str.c_str());
}

bool SQLite3Data::next() {
    int rt = stmt_->step();
    if (first_) {
        errno_ = stmt_->getErrno();
        errstr_ = stmt_->getErrStr();
        first_ = false;
    }
    return rt == SQLITE_ROW;
}

SQLite3Stmt::ptr SQLite3Stmt::Create(const SQLite3::ptr &db, const char *stmt) {
    SQLite3Stmt::ptr rt(new SQLite3Stmt(db));
    if (rt->prepare(stmt) != SQLITE_OK) {
        return nullptr;
    }
    return rt;
}

int64_t SQLite3Stmt::getLastInsertId() {
    return db_->getLastInsertId();
}

int SQLite3Stmt::bindInt8(int idx, int8_t value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindUint8(int idx, uint8_t value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindInt16(int idx, int16_t value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindUint16(int idx, uint16_t value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindInt32(int idx, int32_t value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindUint32(int idx, uint32_t value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindInt64(int idx, int64_t value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindUint64(int idx, uint64_t value) {
    return bind(idx, (int32_t)value);
}

int SQLite3Stmt::bindFloat(int idx, float value) {
    return bind(idx, (double)value);
}

int SQLite3Stmt::bindDouble(int idx, double value) {
    return bind(idx, value);
}

int SQLite3Stmt::bindString(int idx, const char *value) {
    return bind(idx, value);
}

int SQLite3Stmt::bindString(int idx, const std::string &value) {
    return bind(idx, value);
}

int SQLite3Stmt::bindBlob(int idx, const void *value, int64_t size) {
    return bind(idx, value, size);
}

int SQLite3Stmt::bindBlob(int idx, const std::string &value) {
    return bind(idx, (void*)value.c_str(), value.size());
}

int SQLite3Stmt::bindTime(int idx, time_t value) {
    return bind(idx, TimeToStr(value));
}

int SQLite3Stmt::bindNull(int idx) {
    return bind(idx);
}

int SQLite3Stmt::bind(int idx, int32_t value) {
    return sqlite3_bind_int(stmt_, idx, value);
}

int SQLite3Stmt::bind(int idx, uint32_t value) {
    return sqlite3_bind_int(stmt_, idx, value);
}

int SQLite3Stmt::bind(int idx, int64_t value) {
    return sqlite3_bind_int64(stmt_, idx, value);
}

int SQLite3Stmt::bind(int idx, uint64_t value) {
    return sqlite3_bind_int64(stmt_, idx, value);
}

int SQLite3Stmt::bind(int idx, double value) {
    return sqlite3_bind_double(stmt_, idx, value);
}

int SQLite3Stmt::bind(int idx, const char *value, Type type) {
    return sqlite3_bind_text(stmt_, idx, value, strlen(value),
                             type == COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
}

int SQLite3Stmt::bind(int idx, const void *value, int len, Type type) {
    return sqlite3_bind_blob(stmt_, idx, value, len,
                             type == COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
}

int SQLite3Stmt::bind(int idx, const std::string &value, Type type) {
    return sqlite3_bind_text(stmt_, idx, value.c_str(), value.size(),
                             type == COPY ? SQLITE_TRANSIENT : SQLITE_STATIC);
}

int SQLite3Stmt::bind(int idx) {
    return sqlite3_bind_null(stmt_, idx);
}

int SQLite3Stmt::bind(const char *name, int32_t value) {
    int idx = sqlite3_bind_parameter_index(stmt_, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char *name, uint32_t value) {
    int idx = sqlite3_bind_parameter_index(stmt_, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char *name, int64_t value) {
    int idx = sqlite3_bind_parameter_index(stmt_, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char *name, uint64_t value) {
    int idx = sqlite3_bind_parameter_index(stmt_, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char *name, double value) {
    int idx = sqlite3_bind_parameter_index(stmt_, name);
    return bind(idx, value);
}

int SQLite3Stmt::bind(const char *name, const char *value, Type type) {
    int idx = sqlite3_bind_parameter_index(stmt_, name);
    return bind(idx, value, type);
}

int SQLite3Stmt::bind(const char *name, const std::string &value, Type type) {
    int idx = sqlite3_bind_parameter_index(stmt_, name);
    return bind(idx, value, type);
}

int SQLite3Stmt::bind(const char *name, const void *value, int len, Type type) {
    int idx = sqlite3_bind_parameter_index(stmt_, name);
    return bind(idx, value, len, type);
}

int SQLite3Stmt::bind(const char *name) {
    int idx = sqlite3_bind_parameter_index(stmt_, name);
    return bind(idx);
}

int SQLite3Stmt::prepare(const char *stmt) {
    auto rt = finish();
    if (rt != SQLITE_OK) {
        return rt;
    }
    return sqlite3_prepare_v2(db_->getDB(), stmt, strlen(stmt), &stmt_, nullptr);
}

int SQLite3Stmt::finish() {
    auto rc = SQLITE_OK;
    if (stmt_) {
        rc = sqlite3_finalize(stmt_);
        stmt_ = nullptr;
    }
    return rc;
}

ISQLData::ptr SQLite3Stmt::query() {
    return SQLite3Data::ptr(new SQLite3Data(shared_from_this(), 0, ""));
}

int SQLite3Stmt::execute() {
    int rt = step();
    if (rt == SQLITE_DONE) {
        rt = SQLITE_OK;
    }
    return rt;
}

SQLite3Transaction::SQLite3Transaction(const SQLite3::ptr &db, bool auto_commit, Type type)
    : db_(db), type_(type), status_(0), auto_commit_(auto_commit) {

}

SQLite3Transaction::SQLite3Transaction(SQLite3::ptr &&db, bool auto_commit, Type type)
    : db_(std::move(db)), type_(type), status_(0), auto_commit_(auto_commit) {

}

SQLite3Transaction::~SQLite3Transaction() noexcept {
    if (status_ == 1) {
        if (auto_commit_) {
            commit();
        } else {
            rollback();
        }

        if (status_ == 1) {
            FLEXY_LOG_ERROR(g_logger) << db_ << "auto_commit = " << auto_commit_ << " fail";
        }
    }
}

int SQLite3Transaction::execute(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rt = db_->execute(fmt, ap);
    va_end(ap);
    return rt;
}

int SQLite3Transaction::execute(const std::string &sql) {
    return db_->execute(sql);
}

int64_t SQLite3Transaction::getLastInsertId() {
    return db_->getLastInsertId();
}

bool SQLite3Transaction::begin() {
    if (status_ == 0) {
        const char* sql = "BEGIN";
        if (type_ == IMMEDIATE) {
            sql = "BEGIN IMMEDIATE";
        } else if (type_ == EXCLUSIVE) {
            sql = "BEGIN EXCLUSIVE";
        }
        int rt = db_->execute(sql);
        if (rt == SQLITE_OK) {
            status_ = 1;
        }
        return rt == SQLITE_OK;
    }
    return false;
}

bool SQLite3Transaction::commit() {
    if (status_ == 1) {
        int rc = db_->execute("COMMIT");
        if (rc == SQLITE_OK) {
            status_ = 2;
        }
        return rc == SQLITE_OK;
    }
    return false;
}

bool SQLite3Transaction::rollback() {
    if (status_ == 1) {
        int rc = db_->execute("ROLLBACK");
        if (rc == SQLITE_OK) {
            status_ = 3;
        }
        return rc == SQLITE_OK;
    }
    return false;
}

SQLite3Manager::~SQLite3Manager() {
    for (auto& [name, sqlite_deque] : conns_) {
        for (auto ptr : sqlite_deque) {
            delete ptr;
        }
    }
}

SQLite3::ptr SQLite3Manager::get(const std::string& name) {
    unique_lock<decltype(mutex_)> lock(mutex_);
    if (auto it = conns_.find(name); it != conns_.end()) {
        if (it->second.size() - counts_[name] != 0) {
            while (!it->second.front()) {
                it->second.pop_front();
                counts_[name]--;
            }
            SQLite3* rt = it->second.front();
            it->second.pop_front();
            lock.unlock();
            return SQLite3::ptr(rt, std::bind(&SQLite3Manager::freeSQLite3, this, 
                                name, std::placeholders::_1));
        }
    }

    auto& config = g_sqlite3_dbs->getValue();
    std::map<std::string, std::string> args;
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

    auto path = GetParamValue<std::string>(args, "path");
    if (path.empty()) {
        FLEXY_LOG_ERROR(g_logger) << "open db name = " << name << " path is null";
        return nullptr;
    }

    if (path.find(":") == path.npos) {
        path = EnvMgr::GetInstance().getAbsolutePath(path);     // 需初始化env模块
    }

    sqlite3* db;
    if (sqlite3_open_v2(path.c_str(), &db, SQLite3::CREATE | SQLite3::READWRITE, nullptr)) {
        FLEXY_LOG_ERROR(g_logger) << "open db name = " << name << " path = " << path
        << " fail";
        return nullptr;
    }   

    SQLite3* rt = new SQLite3(db);
    auto sql = GetParamValue<std::string>(args, "sql");
    if (!sql.empty()) {
        if (rt->execute(sql)) {
            FLEXY_LOG_ERROR(g_logger) << "execute sql = " << sql << " errno = "
            << rt->getErrno() << " errstr = " << rt->getErrStr();
        }
    }

    rt->lastUsedTime_ = time(0);
    return SQLite3::ptr(rt, std::bind(&SQLite3Manager::freeSQLite3, this, 
                        name, std::placeholders::_1));
}

void SQLite3Manager::registerSQLite3(const std::string& name, const std::map<std::string, std::string>& params) {
    LOCK_GUARD(mutex_);
    dbDefines_[name] = params;
}

void SQLite3Manager::registerSQLite3(const std::string& name, std::map<std::string, std::string>&& params) {
    LOCK_GUARD(mutex_);
    dbDefines_[name] = std::move(params);
}

void SQLite3Manager::checkConnection(uint64_t sec) {
    uint64_t now = time(0);
    std::vector<SQLite3*> conns;
    {
        LOCK_GUARD(mutex_);
        for (auto& [name, sqlite_deque] : conns_) {
            for (auto& ptr : sqlite_deque) {
                if (now - ptr->lastUsedTime_ >= sec) {
                    conns.push_back(ptr);
                    ptr = nullptr;
                    counts_[name]++;
                }
            }
        }
    }

    for (auto ptr : conns) {
        delete ptr;
    }
}


int SQLite3Manager::execute(const std::string& name, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int rt = execute(name, fmt, ap);
    va_end(ap);
    return rt;
}

int SQLite3Manager::execute(const std::string& name, const char* fmt, va_list ap) {
    auto conn = get(name);
    if (!conn) {
        FLEXY_LOG_ERROR(g_logger) << "SQLite3Manager::execute, get(" << name
        << ") fail, format = " << fmt;
        return -1;
    }
    return conn->execute(fmt, ap);
}

int SQLite3Manager::execute(const std::string& name, const std::string& sql) {
    auto conn = get(name);
    if (!conn) {
        FLEXY_LOG_ERROR(g_logger) << "SQLite3Manager::execute, get(" << name
        << ") fail, sql = " << sql;
        return -1;
    }
    return conn->execute(sql);
}

ISQLData::ptr SQLite3Manager::query(const std::string& name, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    auto rt = query(name, fmt, ap);
    va_end(ap);
    return rt;
}

ISQLData::ptr SQLite3Manager::query(const std::string& name, const char* fmt, va_list ap) {
    auto conn = get(name);
    if (!conn) {
        FLEXY_LOG_ERROR(g_logger) << "SQLite3Manager::query, get(" << name
        << ") fail, format = " << fmt;
        return nullptr;
    }
    return conn->query(fmt, ap);
}

ISQLData::ptr SQLite3Manager::query(const std::string& name, const std::string& sql) {
    auto conn = get(name);
    if (!conn) {
        FLEXY_LOG_ERROR(g_logger) << "SQLite3Manager::query, get(" << name
        << ") fail, sql = " << sql;
        return nullptr;
    }
    return conn->query(sql);
}


SQLite3Transaction::ptr SQLite3Manager::openTransaction(const std::string& name, bool auto_commit) {
    auto conn = get(name);
    if (!conn) {
        FLEXY_LOG_ERROR(g_logger) << "SQLite3Manager::openTransaction, get(" << name
        << ") fail";
        return nullptr;
    }
    SQLite3Transaction::ptr trans(new SQLite3Transaction(conn, auto_commit));
    return trans;
}

void SQLite3Manager::freeSQLite3(const std::string& name, SQLite3* m) {
    if (m->db_) {
        LOCK_GUARD(mutex_);
        if (conns_[name].size() - counts_[name] < maxConn_) {
            conns_[name].push_back(m);
            return;
        }
    }
    delete m;
}

}