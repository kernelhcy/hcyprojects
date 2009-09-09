/*
 * DataTables.h
 *
 *  Created on: 2009-9-9
 *      Author: hcy
 */

#ifndef DATATABLES_H_
#define DATATABLES_H_

#include "headers.h"

class DataTables
{
public:
	static DataTables* get_instance(void);
	//the table of labels
	std::vector<std::string> label_table;

	//the table of constants
	std::vector<std::string> const_table;
private:
	static DataTables* _instance;
	DataTables(void);
	~DataTables(void);

};

#endif /* DATATABLES_H_ */
