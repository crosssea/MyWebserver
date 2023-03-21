#ifndef SQLCONNRAII_H
#define SQLCONNRAII_H


#include "sqlconnpool.h"


class SqlConnRAII
{
private:
    MYSQL *sql_;
    SqlConnPool *connpool_;
    
public:
    SqlConnRAII(MYSQL **sql, SqlConnPool *connpool);
    ~SqlConnRAII();
};

SqlConnRAII::SqlConnRAII(MYSQL **sql, SqlConnPool *connpool)
{
    assert(connpool);
    *sql = connpool->GetConn();
    sql_  = *sql;
    connpool_ = connpool;
}

SqlConnRAII::~SqlConnRAII()
{
    if(sql_) {
        connpool_->FreeConn(sql_);
    }
}
#endif