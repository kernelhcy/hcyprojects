/*
 * CreateMidcode.h
 *
 *  Created on: 2009-8-5
 *      Author: hcy
 */

#ifndef CREATEMIDCODE_H_
#define CREATEMIDCODE_H_

#include "headers.h"
#include "LexicalAnalysis.h"
#include <sstream>
/*
 * the entry of the variable table
 * type : 	int 1
 * 			bool 2
 */
#define INT_T 1
#define BOOL_T 2

typedef struct _variable_table_entry
{

		std::string name;
		//the type of this variable
		// int or bool
		int type;

		//the default of this type
		std::string default_value;

		/*
		 * the scope of the variable
		 * the outest scope is 0
		 */
		int scope;
		//是否是常量
		bool is_const;

} variable_table_entry;

/*
 * four tuple definition
 *
 * if the operand is "", this operand is not used.
 */
typedef struct _four_tuple
{
		std::string op; //operator
		std::string first; //the first operand
		std::string sencond; //the second operand
		std::string result; //the result
		/*
		 * the number of the next four tuple
		 * used in jump sentence
		 * in other sentence , it is -1.no sense.
		 */
		int next_sen;
} four_tuple;

class CreateMidcode
{
	public:

		/*
		 * the boundary of the id of the variable and constant
		 * id >= VAR_CONS , constant
		 * id < VAR_CONS , variable
		 */
		const static int VAR_CONS = 100000000;
		/*
		 * the boundary of the id of the variable and constant
		 * id >= CONS_TMP , temporary variable
		 * VAR_CONS <= id < CONS_TMP , variable
		 */
		const static int CONS_TMP = 200000000;
		/*
		 * get the only instance of this class
		 */
		static CreateMidcode* get_instance(void);

		/*
		 * create the middle codes according the 'sen'
		 * and the left part of the generation formula 'part'
		 * scope : the scope of the variable at the current position
		 */
		int create(char left_part, int right_part_id, int scope);
		~CreateMidcode(void);
		/*
		 * print the information in the variable table
		 */
		void print_variable_table(void);

		/*
		 * store the variable and the constant
		 */
		void store_variables_ids(int id);

		/*
		 * print the four tuples
		 */
		void print_tuples(void);
		/*
		 * fill the type information into the variable table
		 * variable : the name of the variable
		 * type		: the type of the variable
		 * scope    : the scope of the variable
		 * is_const : whether this variable is constant or not
		 */
		void fill(const std::string& variable, int type, int scope, bool is_const);
		/*
		 * R -> ""
		 * return
		 * 		0 : success
		 * 		1 : failed
		 */
		int R(int right_part_id);

		/*
		 * U -> ""
		 * return
		 * 		0 : success
		 * 		1 : failed
		 */
		int U(int right_part_id);
	private:

		//true list
		std::stack<int> tc;
		//false list
		std::stack<int> fc;
		//quad
		std::stack<int> m_quad;
		std::stack<int> n_nextlist;
		std::stack<int> s_nextlist;
		//下一条将要生成的语句的序号
		int next_quad;

		//used to show the error position
		LexicalAnalysis * la;
		//the counter of the temporary variable.
		int temp_var_cnt;
		/*
		 * variable table
		 * store the information of all variables in the program
		 */
		std::map<std::string, variable_table_entry*> variable_table;

		//label table
		std::vector<std::string>* label_table;
		//constant table
		std::vector<std::string>* constant_table;

		//store the four tuples
		/*
		 * 尽量使用指针。
		 * 直接存储four_tuple可能会出现内存错误！！
		 */
		std::vector<four_tuple *> tuples;

		//the scope of the variable
		int scope;

		//the type of the variable
		//used in definition sentence
		int var_type;

		/*
		 * the id stack of the variables and constants
		 * used to transfer the ids of the variables and constants to
		 * the class CreateMidcode to create the middle code.
		 *
		 * if id < VAR_CONS, this is variable,
		 * 	or this is a constant.
		 *
		 * if this is variable,
		 * id is the position of this variable in the variable table.
		 * if this is a constant id,
		 * id - VAR_CONS is the position of this constant in the constant table.
		 */

		std::stack<int> var_cons_id_stack;
		//the only instance
		static CreateMidcode* _instance;
		/*
		 * Can not create more instances.
		 */
		CreateMidcode(void);

		/*
		 * M-> int i N | bool i N
		 * return the type
		 */
		int M(int right_part_id);

		/*
		 * N -> ,iN | ""
		 * return the type
		 */
		int N(int right_part_id);

		/*
		 * I -> &GI | |GI | !GI | ""
		 * return
		 * 		0 : success
		 * 		1 : failed
		 */
		int I(int right_part_id);

		/*
		 * O -> +TO | -TO | ""
		 * return
		 * 		0 : success
		 * 		1 : failed
		 */
		int O(int right_part_id);

		/*
		 * H -> *PH | ""
		 * return
		 * 		0 : success
		 * 		1 : failed
		 */
		int H(int right_part_id);
		/*
		 * Q -> /FQ | ""
		 * return
		 * 		0 : success
		 * 		1 : failed
		 */
		int Q(int right_part_id);

		/*
		 * B -> i=E;
		 * return
		 * 		0 : success
		 * 		1 : failed
		 */
		int B(int right_part_id);

		/*
		 * L -> M;
		 * return
		 * 		0 : success
		 * 		1 : failed
		 */
		int L(int right_part_id);

		/*
		 * F -> (E) | i | t | s | c
		 * return
		 * 		0 : success
		 * 		1 : failed
		 */
		int F(int right_part_id);

		/*
		 * C -> if(i){A}K | while(I){A}
		 * return
		 * 		0 : success
		 * 		1 : failed
		 */
		int C(int right_part_id);

		/*
		 * K -> else{A} | ""
		 * return
		 * 		0 : success
		 * 		1 : failed
		 */
		int K(int right_part_id);

		/*
		 * A -> BA | CA | LA | ""
		 * return
		 * 		0 : success
		 * 		1 : failed
		 */
		int A(int right_part_id);

		/*
		 * get the variable name from the label table
		 */
		//int get_name_id_of_label_table(const std::string & sen, int start_pos);

		/*
		 * create a temporaty variable
		 */
		std::string new_temp(void);
		/*
		 * create a four tuple
		 */
		void create_four_tuple(const std::string &op, const std::string &first,
				const std::string &second, const std::string &result);

		/*
		 * get the operand
		 */
		std::string get_operand(void);

		/*
		 * merge the list p1 and list p2
		 * return the id of the new list
		 */
		int merge(int p1, int p2);

		/*
		 * change all the forth field of  all the tuples of
		 * 	the list list_id to ft_id
		 */
		int backpatch(int list_id, int ft_id);

		/*
		 * get the newest tc,fc or m.quad
		 * append new tc,fc or m.quad
		 */
		int get_tc();
		void append_tc(int id);
		int get_fc();
		void append_fc(int id);
		int get_m_quad();
		void append_m_quad(int m);
		int get_n_nextlist();
		void append_n_nextlist(int m);
		int get_s_nextlist();
		void append_s_nextlist(int m);

		/*
		 * test whether the type of the variable named name is type or not
		 * return : yes 0
		 *          no  1
		 */
		int test_var(const std::string name, int type);

};

#endif /* CREATEMIDCODE_H_ */
