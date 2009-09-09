/*
 * DataTables_imp.cpp
 *
 *  Created on: 2009-9-9
 *      Author: hcy
 */
#include "../headers/DataTables.h"

DataTables* DataTables::_instance = 0;

DataTables::DataTables(void)
{

}

DataTables::~DataTables(void)
{

}

DataTables* DataTables::get_instance()
{
	if (_instance == 0)
	{
		_instance = new DataTables;
	}
	return _instance;
}
