/* 
 * File:   db_connection.cpp
 * Author: hcy
 * 
 */

#include "db_connection.h"
#include <mysql++.h>

db_connection::db_connection(bool flag) {
    db = "test";
    server = "localhost";
    user = "hcy";
    pass = "hcyilyzj";
    is_connect=false;
    
}

db_connection::db_connection(const db_connection& orig) { }

db_connection::~db_connection() { }

bool db_connection::connect(){
    if(is_connect){
        return is_connect;
    }else{
        return is_connect=conn.connect(db, server, user, pass);
    }
    
}

void db_connection::test() {
    //connect to the database
    if (is_connect) {

        // Retrieve a subset of the sample stock table set up by resetdb
        // and display it.

        mysqlpp::Query query = conn.query();
        query << "select * from Student";

        mysqlpp::Result res = query.store();

        if (res) {
            mysqlpp::Row row;
            mysqlpp::Result::iterator i;

            for (i = res.begin(); i != res.end(); i++) {

                row = *i;
                cout << row.at(0) << "  " << row.at(1) << "  " << row.at(2) << endl;
            }
        } else {
            cerr << "Failed to get item list: " << query.error() << endl;

        }

    } else {
        cerr << "No connection!!" << endl;

    }
}
