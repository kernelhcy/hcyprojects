/* 
 * File:   main.cpp
 * Author: hcy
 *
 */
#include <string.h>
#include "fs.h"
#include "bitset.h"
/*
 * 
 */
int main(int argc, char** argv)
{
	
	int show_details = 0;

	printf("\nShow details,run: %s details.\n",argv[0]);
	if(argc > 1)
	{
		if(strcmp("details",argv[1]) == 0)
		{
			show_details = 1;	
		}
	}

	bitset *s = bitset_init(200);
	bitset_set_bit(s, 0);
	bitset_set_bit(s, 1);
	bitset_set_bit(s, 2);
	bitset_set_bit(s, 3);
	bitset_set_bit(s, 4);
	bitset_set_bit(s, 5);
	bitset_set_bit(s, 6);
	bitset_set_bit(s, 7);
	bitset_set_bit(s, 8);
	bitset_set_bit(s, 9);
	bitset_set_bit(s, 10);
	bitset_print(s);
	bitset_clear_bit(s, 1);
	bitset_clear_bit(s, 4);
	bitset_clear_bit(s, 7);
	bitset_print(s);
	
	size_t pos = bitset_get_first_unused_bit_pos(s);
	printf("pos : %d\n", pos);
	bitset_set_bit(s, pos);
	pos = bitset_get_first_unused_bit_pos(s);
	printf("pos : %d\n", pos);
	
	bitset_free(s);
	run(1);

    return (EXIT_SUCCESS);
}


