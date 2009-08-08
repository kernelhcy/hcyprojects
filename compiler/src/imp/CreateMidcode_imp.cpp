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
	if (right_part_id == 27)//right part is ""
	{
		return 1;
	}
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
		case 'R':
			R(right_part_id);
			value = 1;
			break;
		case 'S':
			S(right_part_id);
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
	int tc_id, fc_id;
	int m_tmp;

	switch (right_part_id)
	{
		case 25://and
			m_tmp = get_m_quad();
			backpatch(get_tc(), m_tmp);
			tc_id = get_tc();
			fc_id = merge(get_fc(), get_fc());
			break;
		case 26://or
			m_tmp = get_m_quad();
			backpatch(get_fc(), m_tmp);
			fc_id = get_fc();
			tc_id = merge(get_tc(), get_tc());
			break;
		case 20://not
			tc_id = get_fc();
			fc_id = get_tc();
			break;
	}

	std::string i1, i2;
	i1 = get_operand();
	i2 = get_operand();

	tc_id = make_list(next_quad);
	fc_id = make_list(next_quad + 1);

	switch (right_part_id)
	{
		case 39://>
			create_four_tuple("j>", i1, i2, "-1");
			break;
		case 40://<
			create_four_tuple("j<", i1, i2, "-1");
			break;
		case 41://==
			create_four_tuple("j==", i1, i2, "-1");
			break;
		case 42://!=
			create_four_tuple("j!=", i1, i2, "-1");
			break;
		default:
			la -> error("No such logic symbol.");
			break;
	}

	create_four_tuple("j", "-", "-", "-1");

	append_tc(tc_id);
	append_fc(fc_id);

	return 0;
}

int CreateMidcode::R(int right_part_id)
{
	append_m_quad(next_quad);
	return 0;
}

int CreateMidcode::S(int right_part_id)
{

	return 0;
}

int CreateMidcode::F(int right_part_id)
{

	if (right_part_id == 13)
	{
		std::string name = get_operand();

		variable_table_entry *vte = variable_table[name];
		if (vte == NULL)
		{
			la -> error("Need boolean variable.");
			return -1;
		}

		if (vte -> type == BOOL_T)
		{
			append_tc(next_quad);
			append_fc(next_quad + 1);
			create_four_tuple("jnz", name, "-", "-1");
			create_four_tuple("j", "-", "-", "-1");
		}
	}
	else if (right_part_id == 23 || right_part_id == 24)
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
		test_var(sen, INT_T);

		//first operand
		first = get_operand();
		test_var(first, INT_T);

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
	test_var(sen, INT_T);

	//first operand
	first = get_operand();
	test_var(first, INT_T);

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
	test_var(sen, INT_T);

	//first operand
	first = get_operand();
	test_var(first, INT_T);

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
	test_var(first, INT_T);

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
		std::cout << tuples[num].op << " , \t";
		std::cout << tuples[num].first << " , \t";
		std::cout << tuples[num].sencond << " , \t";
		std::cout << tuples[num].result;
		std::cout << " )  \t";
		std::cout << tuples[num].next_sen << std::endl;
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

int CreateMidcode::make_list(int ft_id)
{
	return ft_id;
}

int CreateMidcode::merge(int p1, int p2)
{
	int index = p2;
	while (tuples[index].next_sen != -1)
	{
		index = tuples[index].next_sen;
	}

	tuples[index].next_sen = p1;

	return p2;
}

int CreateMidcode::backpatch(int list_id, int ft_id)
{
	return 0;
}

int CreateMidcode::get_tc()
{
	int tmp = tc.top();
	tc.pop();

	return tmp;
}
void CreateMidcode::append_tc(int id)
{
	tc.push(id);
}
int CreateMidcode::get_fc()
{
	int tmp = fc.top();
	fc.pop();

	return tmp;
}
void CreateMidcode::append_fc(int id)
{
	fc.push(id);
}
int CreateMidcode::get_m_quad()
{
	int tmp = m_quad.top();
	m_quad.pop();

	return tmp;
}
void CreateMidcode::append_m_quad(int m)
{
	m_quad.push(m);
}

int CreateMidcode::test_var(const std::string name, int type)
{
	variable_table_entry *vte = NULL;
	vte = variable_table[name];
	if (vte == NULL || vte -> type != type)
	{
		if (type == INT_T)
		{
			la -> error("Need a integer variable.");
		}
		else if (type == BOOL_T)
		{
			la -> error("Need a boolean variable.");
		}
		else
		{
			std::cerr << "CreateMidcode::test_var : Bad TYPE!!\n";
		}
		return 1;
	}
	return 0;
}
