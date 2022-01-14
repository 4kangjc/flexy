#include <iostream>
#include <flexy/db/mysql.h>
#include <flexy/flexy.h>

static auto s_mysql_confif = flexy::Config::Lookup("mysql", std::map<std::string, std::string>(), "mysql config");

void run() {
    do {
        flexy::Config::LoadFromConDir<false>("conf/");

        auto mysql = std::make_shared<flexy::MySQL>(s_mysql_confif->getValue());
        if (!mysql->connect()) {
            std::cout << "connect fail" << std::endl;
            return;
        }
        
        int rt = mysql->execute("update user set update_time = %s where id = 1", "'2012-01-31 14:34:17'");
        FLEXY_ASSERT(rt == 0);
        rt = mysql->execute("insert into user (update_time) values ('%s')", flexy::TimeToStr().c_str());
        FLEXY_ASSERT(rt == 0);
        // auto stmt = flexy::MySQLStmt::Create(mysql, "update user set update_time = ? where id = 1");
        // stmt->bindString(1, "2021-12-29 10:10:10");
        // int rt = stmt->execute();
        // std::cout << "rt = " << rt << std::endl;

        auto&& res = mysql->query("select * from user");
        std::cout << "---------------------------------------------\n";
        int row = res->getDataCount(), col = res->getColumnCount();
        for (int j = 0; j < col; ++j) {
            std::cout << res->getColumnName(j) << '\t';
        }
        std::cout << std::endl;
        for (int i = 0; i < row; ++i) {
            res->next();
            for (int j = 0; j < col; ++j) {
                std::cout << res->getString(j) << '\t';
            }
            std::cout << std::endl;
        }
        std::cout << "---------------------------------------------\n";
    } while (false);
    std::cout << "over" << std::endl;
}

int main(int argc, char** argv) {
    flexy::IOManager iom;
    // iom.addRecTimer(1000, run);
    go run;
    return 0;
} 