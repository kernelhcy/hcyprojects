/* 
 * File:   main.cpp
 * Author: hcy
 * version 1.0
 */

#include <iostream>
#include <mysql++.h>
#include <stdlib.h>

using namespace std;

int main(int argc, char *argv[])
{

    //the information used to connect to the database
    const char db[] = "music";
    const char server[] = "localhost";
    const char user[] = "hcy";
    const char pass[] = "hcyilyzj";


    // create a connection
    mysqlpp::Connection conn(true);

    //connect to the database
    if (conn.connect(db, server, user, pass))
    {

        // Retrieve a subset of the sample stock table set up by resetdb
        // and display it.

        mysqlpp::Query query = conn.query();
        query << "select * from Song";

        mysqlpp::Result res = query.store();

        if (res)
        {
            mysqlpp::Row row;
            mysqlpp::Result::iterator i;

            for (i = res.begin(); i != res.end(); i++)
            {

                row = *i;
                for (int j=0;j<row.size();++j)
                {
                    cout<<row.at(j)<<"\t";
                }
                cout << endl;
            }
        }
        else
        {
            cerr << "Failed to get item list: " << query.error() << endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    else
    {
        cerr << "DB connection failed: " << conn.error() << endl;
        return EXIT_FAILURE;
    }

}
