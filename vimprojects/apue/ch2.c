#include "ch2.h"

void runit(int argc, char * argv[])
{
	//p_2_1();	
	//p_2_2();
	p_2_3();
	p_2_4();
}

void p_2_1()
{
	
}
void p_2_2()
{

}

/*
 * 程序2-3
 * 为路径名动态分配地址
 */
#ifdef PATH_MAX
static int pathmax = PATH_MAX;
#else
static int pathmax = 0;
#endif

#define SUSV3  200112L

static long posix_version = 0;

/* If PATH_MAX is indeterminate, no guarrantee this is adequate */
#define PATH_MAX_GUESS  1024

/* also return allocated size, if nonnull */
char * path_alloc(int *sizep)
{
	char	*ptr;
	int 	size;
	
	if (posix_version == 0)
	{	
		posix_version = sysconf(_SC_VERSION);
	}
	
	if (pathmax == 0) 
	{
		errno = 0;
		if ((pathmax = pathconf("/", _PC_PATH_MAX)) < 0)
		{
			if (errno == 0)
			{
				pathmax = PATH_MAX_GUESS;
			}
			else
			{
				err_sys("pathconf eror for _PC_PATH_MAX");
			}
		}
		else
		{
			++pathmax;	/* add one since it's relative to root */
		}
	}
	
	if (posix_version < SUSV3)
	{
		size = pathmax + 1;
	}
	else 
	{
		size = pathmax;
	}
	
	if ((ptr = malloc(size)) == NULL)
	{
		err_sys("malloc error for pathname");
	}
	
	if (ptr != NULL)
	{
		*sizep = size;
	}
	
	return (ptr);
}

void p_2_3()
{
	int 	size;
	char 	*p = path_alloc(&size);
	
	printf("The MAX length of the pathname is %d.\n", size);
	return;
}

/*
 * 程序2-4
 * 确定文件描述符数
 */
#ifdef OPEN_MAX
static long openmax = OPEN_MAX;
#else
static long openmax = 0;
#endif

/*
 * If OPEN_MAX id indeterminate, we're not
 * guaranteed that this is adequate.
 */
#define OPEN_MAX_GUESS 256

long open_max(void)
{
	if (openmax == 0) /* first time through */
	{
		errno = 0;
		if ((openmax = sysconf(_SC_OPEN_MAX)) < 0)
		{
			if (errno == 0)
			{
				openmax = OPEN_MAX_GUESS;	/* it's indeterminate. */
			}
			else 
			{
				err_sys("sysconf error for _SC_OPEN_MAX");
			}
		}		
	}
	
	return (openmax);
}
void p_2_4()
{
	long 	openmax = open_max();
	printf("The open max is %d.\n", openmax);
}
