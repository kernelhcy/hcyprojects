/*
 * CreateMidcode_imp.cpp
 *
 *  Created on: 2009-8-5
 *      Author: hcy
 */
#include "../headers/CreateMidcode.h"

CreateMidcode* CreateMidcode::_instance = NULL;

CreateMidcode* CreateMidcode::get_instance(void)
{
	if (_instance == NULL)
	{
		_instance = new CreateMidcode();
	}

	return _instance;
}

CreateMidcode::CreateMidcode(void)
{
	//std::cout << "CreateMidCode.\n";
	variable_table.clear();
	scope = 0;
	var_type = 0;
	temp_var_cnt = 0;
	next_quad = 0;//语句序号从0开始
	la = LexicalAnalysis::get_instance();
	this -> label_table = la -> get_label_table();
	this -> constant_table = la -> get_const_table();

}

CreateMidcode::~CreateMidcode(void)
{

}

int CreateMidcode::create(char left_part, int right_part_id, int scope)
{
//	std::cout << left_part << "  " << right_part_id << std::endl;
	int value = 0;

	this -> scope = scope;

	switch (left_part)
	{
		case 'M':
			var_type = M(right_part_id);
			value = 1;
			break;
		case 'N':
			var_type = N(right_part_id);
			value = 1;
			break;
		case 'F':
			F(right_part_id);
			value = 1;
			break;
		case 'I':
			I(right_part_id);
			value = 1;
			break;
		case 'O':
			O(right_part_id);
			value = 1;
			break;
		case 'H':
			H(right_part_id);
			value = 1;
			break;
		case 'Q':
			Q(right_part_id);
			value = 1;
			break;
		case 'B':
			B(right_part_id);
			value = 1;
			break;
		case 'L':
			L(right_part_id);
			value = 1;
			break;
		default:
			break;
	}

	return value;
}

int CreateMidcode::M(int right_part_id)
{

	std::string name;
	int id = var_cons_id_stack.top();
	var_cons_id_stack.pop();

	if (id <= VAR_CONS)//a variable
	{
		name = label_table -> at(id);
	}
	else//a constant. this can not access any more!
	{
		name = constant_table -> at(id - VAR_CONS);
	}

	if (right_part_id == 17)//int
	{
		fill(name, INT_T, scope);
		return INT_T;
	}
	else if (right_part_id == 18)//bool
	{
		fill(name, BOOL_T, scope);
		return BOOL_T;
	}
	else
	{
		std::cerr << "M : ERROR.\n";
	}
	return 0;
}

int CreateMidcode::N(int right_part_id)
{
	std::string name;
	int id = var_cons_id_stack.top();
	var_cons_id_stack.pop();

	if (id <= VAR_CONS)//a variable
	{
		name = label_table -> at(id);
	}
	else//a constant. this can not access any more!
	{
		name = constant_table -> at(id - VAR_CONS);
	}

	fill(name, var_type, scope);
	return var_type;
}

int CreateMidcode::I(int right_part_id)
{

	return 0;
}

int CreateMidcode::F(int right_part_id)
{
	if(F_flag == LOGIC)
	{

	}
	return 0;
}

int CreateMidcode::O(int right_part_id)
{

	std::string tmp, op, first, sen, result;

	if (right_part_id == 7 || right_part_id == 8)//+TO | -TO
	{
		//operation
		if (right_part_id == 7)
		{
			op = "+";
		}
		else
		{
			op = "-";
		}

		//second operand
		sen = get_operand();

		//first operand
		first = get_operand();
		//result
		result = new_temp();

		create_four_tuple(op, first, sen, result);

	}
	else
	{
		return 1;
	}

	return 0;
}
int CreateMidcode::H(int right_part_id)
{
	std::string op, first, sen, result;

	op = "*";

	//second operand
	sen = get_operand();

	//first operand
	first = get_operand();

	//result
	result = new_temp();

	create_four_tuple(op, first, sen, result);

	return 0;
}
int CreateMidcode::Q(int right_part_id)
{
	std::string tmp, op, first, sen, result;
	op = "/";

	//second operand
	sen = get_operand();

	//first operand
	first = get_operand();

	//result
	result = new_temp();

	create_four_tuple(op, first, sen, result);

	return 0;
}
int CreateMidcode::B(int right_part_id)
{

	std::string tmp, op, first, sen, result;
	op = "=";

	//first operand
	first = get_operand();

	//no use sencond operand
	sen = "-";
	//result
	result = get_operand();

	create_four_tuple(op, first, sen, result);

	return 0;
}
int CreateMidcode::L(int right_part_id)
{

	return 0;
}

void CreateMidcode::fill(const std::string& variable, int type, int scope)
{
	//test whether the variable has already defined or not.
	if (variable_table[variable] != NULL && variable_table[variable]->scope == scope)
	{
		std::cerr << "ERROR : variable " << variable << "has multiple definition.";
	}

	//store the information of this variable in the variable table
	variable_table_entry * tmp = new variable_table_entry;
	tmp -> name = variable;
	tmp -> type = type;
	tmp -> scope = scope;

	variable_table[variable] = tmp;
}

void CreateMidcode::store_variables_ids(int id)
{
	var_cons_id_stack.push(id);

}

void CreateMidcode::print_variable_table(void)
{
	std::map<std::string, variable_table_entry*>::iterator pos;
	variable_table_entry * vte;
	for (pos = variable_table.begin(); pos != variable_table.end(); ++pos)
	{
		vte = pos -> second;
		std::cout << pos -> first << " : " << vte -> type << std::endl;
	}
}

std::string CreateMidcode::new_temp(void)
{

	//push the temporary variable in the stack
	store_variables_ids(temp_var_cnt + CONS_TMP);

	std::string tmp("T");
	std::stringstream ss;
	ss << temp_var_cnt;
	tmp += ss.str();

	++temp_var_cnt;

	return tmp;
}

void CreateMidcode::create_four_tuple(const std::string &op, const std::string &first,
		const std::string &second, const std::string &result)
{
	four_tuple tuple;
	tuple.op = op;
	tuple.first = first;
	tuple.sencond = second;
	tuple.result = result;

	tuple.next_sen = -1;

	++next_quad;

	tuples.push_back(tuple);

}

void CreateMidcode::print_tuples(void)
{
	for (unsigned int num = 0; num < tuples.size(); ++num)
	{

		std::cout << num << " : ( ";
		std::cout << tuples[num].op << " , ";
		std::cout << tuples[num].first << " , ";
		std::cout << tuples[num].sencond << " , ";
		std::cout << tuples[num].result;
		std::cout << " )\n";
	}
}

std::string CreateMidcode::get_operand(void)
{
	std::string tmp("");
	if (var_cons_id_stack.top() < VAR_CONS)
	{
		std::string vname = label_table -> at(var_cons_id_stack.top());
		variable_table_entry * vte = NULL;
		vte = variable_table[vname];
		if (vte == NULL)
		{
			la -> error("Operand " + vname + " has not declared!!");
			return tmp;
		}
		tmp = vte -> name;
	}
	else if (var_cons_id_stack.top() < CONS_TMP)
	{
		std::stringstream ss;
		ss << constant_table -> at(var_cons_id_stack.top() - VAR_CONS);
		tmp += ss.str();
	}
	else
	{
		std::stringstream ss;
		ss << var_cons_id_stack.top() - CONS_TMP;
		tmp = "T";
		tmp += ss.str();
	}
	var_cons_id_stack.pop();

	return tmp;

}
