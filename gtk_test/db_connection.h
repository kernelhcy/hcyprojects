/* 
 * File:   db_connection.h
 * Author: hcy
 *
 */

#ifndef _DB_CONNECTION_H
#define	_DB_CONNECTION_H

#include <mysql++.h>
#include <iostream>
using namespace std;
class db_connection {

public:
    db_connection(bool flag=true);
    db_connection(const db_connection& orig);
    virtual ~db_connection();
    bool connect();
    void test();
private:
    //the information used to connect to the database
    const char *db;
    const char *server;
    const char *user;
    const char *pass;
    mysqlpp::Connection conn;
    bool is_connect;
};

#endif	/* _DB_CONNECTION_H */

