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

	fill("true", BOOL_T, 999999, false);
	fill("false", BOOL_T, 999999, false);

}

CreateMidcode::~CreateMidcode(void)
{

}

int CreateMidcode::create(char left_part, int right_part_id, int scope)
{
	if (right_part_id == 27 && left_part != 'K')//right part is ""
	{
		return 1;
	}
	//std::cout << left_part << "  " << right_part_id << std::endl;
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
		case 'U':
			U(right_part_id);
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
		case 'A':
			A(right_part_id);
			value = 1;
			break;
		case 'C':
			C(right_part_id);
			value = 1;
			break;
		case 'K':
			K(right_part_id);
			value = 1;
			break;
		default:
			break;
	}

	return value;
}

int CreateMidcode::A(int right_part_id)
{
	if (right_part_id == 1 || right_part_id == 2)
	{
		append_s_nextlist(next_quad);
	}
	return 0;
}

int CreateMidcode::U(int right_part_id)
{
	append_n_nextlist(next_quad);
	create_four_tuple("j", "-", "-", "-1");
	return 0;
}

int CreateMidcode::C(int right_part_id)
{
	int s_n_l;
	int m1;
	char tmp[10];
	switch (right_part_id)
	{
		case 14://if(I){A}K
			//推迟判断是否有else语句。
			break;
		case 15://while(E){A}
			backpatch(get_tc(), get_m_quad());
			m1 = get_m_quad();
			sprintf(tmp, "%d", m1);
			create_four_tuple("j", "-", "-", tmp);
			backpatch(get_s_nextlist(), m1);
			s_n_l = get_fc();
			backpatch(s_n_l, next_quad);
			append_s_nextlist(s_n_l);
			break;
		default:
			la -> error("if while error!!");
			return -1;
	}

	return 0;
}
int CreateMidcode::K(int right_part_id)
{
	int s;
	switch (right_part_id)
	{
		case 16://if(I){A} else {A}
			backpatch(get_fc(), get_m_quad());
			backpatch(get_tc(), get_m_quad());
			s = merge(get_s_nextlist(), get_n_nextlist());
			get_s_nextlist();
			append_s_nextlist(s);
			break;
		case 27://if(I){A}
			backpatch(get_tc(), get_m_quad());
			s = merge(get_s_nextlist(), get_fc());
			append_s_nextlist(s);
			break;
		default:
			la -> error("if (else) error!!");
			break;
	}
	return 0;
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
		fill(name, INT_T, scope, false);
		return INT_T;
	}
	else if (right_part_id == 18)//bool
	{
		fill(name, BOOL_T, scope, false);
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
	bool is_const;

	if (id <= VAR_CONS)//a variable
	{
		name = label_table -> at(id);
		is_const = false;
	}
	else//a constant. this can not access any more!
	{
		name = constant_table -> at(id - VAR_CONS);
		is_const = true;
	}

	fill(name, var_type, scope, is_const);
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
			tc_id = get_tc();
			backpatch(get_tc(), m_tmp);
			fc_id = merge(get_fc(), get_fc());
			break;
		case 26://or
			m_tmp = get_m_quad();
			fc_id = get_fc();
			backpatch(get_fc(), m_tmp);
			tc_id = merge(get_tc(), get_tc());
			break;
		case 20://not
			tc_id = get_fc();
			fc_id = get_tc();
			break;
		default:
			la -> error("No such logic symbol!!");
			return -1;
			break;
	}

	append_tc(tc_id);
	append_fc(fc_id);

	return 0;
}

int CreateMidcode::R(int right_part_id)
{
	append_m_quad(next_quad);
	//std::cout << "R : " << next_quad << std::endl;
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
			var_cons_id_stack.pop();
		}
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
		var_cons_id_stack.pop();
		if (test_var(sen) != INT_T)
		{
			la -> error("Need a integer!");
		}

		//first operand
		first = get_operand();
		var_cons_id_stack.pop();
		if (test_var(first) != INT_T)
		{
			la -> error("Need a integer!");
		}

		//将常量转换成值，而不是id
		first
				= (variable_table[first] -> is_const ? la -> const_table[atoi(first.c_str())]
						: first);
		sen = (variable_table[sen] -> is_const ? la -> const_table[atoi(sen.c_str())] : sen);

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
	var_cons_id_stack.pop();
	if (test_var(sen) != INT_T)
	{
		la -> error("Need a integer!");
	}

	//first operand
	first = get_operand();
	var_cons_id_stack.pop();
	if (test_var(first) != INT_T)
	{
		la -> error("Need a integer!");
	}

	first = (variable_table[first] -> is_const ? la -> const_table[atoi(first.c_str())] : first);
	sen = (variable_table[sen] -> is_const ? la -> const_table[atoi(sen.c_str())] : sen);
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
	var_cons_id_stack.pop();
	if (test_var(sen) != INT_T)
	{
		la -> error("Need a integer!");
	}

	//first operand
	first = get_operand();
	var_cons_id_stack.pop();
	if (test_var(first) != INT_T)
	{
		la -> error("Need a integer!");
	}

	first = (variable_table[first] -> is_const ? la -> const_table[atoi(first.c_str())] : first);
	sen = (variable_table[sen] -> is_const ? la -> const_table[atoi(sen.c_str())] : sen);
	//result
	result = new_temp();

	create_four_tuple(op, first, sen, result);

	return 0;
}
int CreateMidcode::B(int right_part_id)
{

	std::string tmp, op, first, sen, result;
	op = "=";

	int tmp_id = var_cons_id_stack.top();
	var_cons_id_stack.pop();

	//first operand
	first = get_operand();
	var_cons_id_stack.pop();

	first = (variable_table[first] -> is_const ? la -> const_table[atoi(first.c_str())] : first);
	//no use sencond operand
	sen = "-";
	//result
	if (var_cons_id_stack.size() <= 0)
	{
		result = first;
		var_cons_id_stack.push(tmp_id);
		first = get_operand();
	}
	else
	{
		result = get_operand();
		var_cons_id_stack.pop();
		var_cons_id_stack.push(tmp_id);
	}

	if (test_var(first) != test_var(result))
	{
		la->error("Assignment : WRONG type!");
	}

	if(first == "true")
	{
		first = "1";
	}
	else if (first == "false")
	{
		first = "0";
	}

	create_four_tuple(op, first, sen, result);

	return 0;
}
int CreateMidcode::L(int right_part_id)
{

	return 0;
}

void CreateMidcode::fill(const std::string& variable, int type, int scope, bool is_const)
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
	tmp -> is_const = is_const;
	variable_table[variable] = tmp;
}

void CreateMidcode::store_variables_ids(int id)
{
	//std::cout << "store_variables_ids: " << id << std::endl;
	//std::cout << "store_variables_ids: size " << var_cons_id_stack.size() << std::endl;
	var_cons_id_stack.push(id);

}

void CreateMidcode::print_variable_table(void)
{
	std::map<std::string, variable_table_entry*>::iterator pos;
	variable_table_entry * vte;
	for (pos = variable_table.begin(); pos != variable_table.end(); ++pos)
	{
		vte = pos -> second;
		std::cout << pos -> first << " : " << vte -> type << "  ";
	}
	std::cout << std::endl;
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

	fill(tmp, INT_T, -1, false);

	return tmp;
}

void CreateMidcode::create_four_tuple(const std::string &op, const std::string &first,
		const std::string &second, const std::string &result)
{
	four_tuple * tuple = new four_tuple;
	tuple -> op = op;
	tuple -> first = first;
	tuple -> sencond = second;
	tuple -> result = result;

	tuple -> next_sen = -1;

	++next_quad;

	tuples.push_back(tuple);

}

void CreateMidcode::print_tuples(void)
{
	for (unsigned int num = 0; num < tuples.size(); ++num)
	{

		std::cout << num << " : ( ";
		std::cout << tuples[num] -> op << " , \t";
		std::cout << tuples[num] -> first << " , \t";
		std::cout << tuples[num] -> sencond << " , \t";
		if (tuples[num] -> op == "j" || tuples[num] -> op == "jnz")
		{
			std::cout << tuples[num] -> next_sen;
		}
		else
		{
			std::cout << tuples[num] -> result;
		}
		std::cout << " )  \t\n";

	}
	std::cout << std::endl;
}

std::string CreateMidcode::get_operand(void)
{
	if (var_cons_id_stack.size() <= 0)
	{
		std::cerr << "Get_operand: var_cons_id_stack 's size <=0!!\n";
	}

	std::string tmp("");

	//	std::cout << "get_operand: " << var_cons_id_stack.top() << std::endl;
	//	std::stack<int> ttt(var_cons_id_stack);
	//	int sss = ttt.size();
	//	for (int i = 0; i < sss; ++i)
	//	{
	//		std::cout << ttt.top() << "  ";
	//		ttt.pop();
	//	}
	//	std::cout << std::endl;

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
		ss << var_cons_id_stack.top() - VAR_CONS;
		tmp += ss.str();
	}
	else
	{
		std::stringstream ss;
		ss << var_cons_id_stack.top() - CONS_TMP;
		tmp = "T";
		tmp += ss.str();
	}
	//	std::cout << "get operand: " << tmp << std::endl;

	if (var_cons_id_stack.top() == TRUE_ID)
	{
		tmp = "true";
	}
	else if (var_cons_id_stack.top() == FALSE_ID)
	{
		tmp = "false";
	}

	return tmp;

}

int CreateMidcode::merge(int p1, int p2)
{
	unsigned int index = p2;
	if (index >= tuples.size())
	{
		return p2;
	}
	while (tuples[index] -> next_sen != -1)
	{
		index = tuples[index] -> next_sen;
	}

	tuples[index] -> next_sen = p1;

	return p2;
}

int CreateMidcode::backpatch(int list_id, int ft_id)
{
	int next = tuples[list_id] -> next_sen;
	tuples[list_id] -> next_sen = ft_id;
	int tmp;

	while (next >= 0)
	{
		tmp = tuples[next] -> next_sen;
		tuples[next] -> next_sen = ft_id;
		next = tmp;
	}

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
int CreateMidcode::get_n_nextlist()
{
	int tmp = n_nextlist.top();
	n_nextlist.pop();
	return tmp;

}
void CreateMidcode::append_n_nextlist(int m)
{
	n_nextlist.push(m);
}
int CreateMidcode::get_s_nextlist()
{
	int tmp = s_nextlist.top();
	s_nextlist.pop();
	return tmp;

}
void CreateMidcode::append_s_nextlist(int m)
{

	s_nextlist.push(m);
}
int CreateMidcode::test_var(const std::string name)
{
	variable_table_entry *vte = NULL;
	vte = variable_table[name];
	if (vte == NULL)
	{
		la -> error("Need a variable." + name);
		return -1;
	}
	return vte -> type;
}
