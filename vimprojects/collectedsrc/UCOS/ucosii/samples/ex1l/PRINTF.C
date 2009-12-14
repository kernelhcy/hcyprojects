
#include "printf.h"
#include "windows.h"
#include <stdio.h>

/////////////////////////////////////////////////////////////////////////////////////
//
//
void  put_char ( char ch )
{
	PWINDOWS ptr = GetActiveWindow();

	if (ch == '\n' || ch == '\0')
	{
		if (ptr->cy < (ptr->ey - ptr->sy))
		{
			if (ptr->cy < ptr->ey - 1)
				ptr->cy ++;
		}
		else
		{
			if (ptr->cy < ptr->ey - 1)
				ptr->cy ++;
		}
		ptr->cx = 0;
	}
	else
	{
		PC_DispChar(ptr->cx, ptr->cy, ch, ptr->color);
		ptr->cx ++;
	}
}

void  printshort ( int n, const char* fmt )
{
	char buf[50], c;
	int i = 0;

	sprintf( buf, fmt, n);
	while (i < 50)
	{
		c = buf[i];
		if (c == '\0')
			return;
		put_char(c);
		i ++;
	}

}

void  printfloat ( float n, const char* fmt )
{
	char buf[50], c;
	int i = 0;

	sprintf( buf, fmt, n);
	while (i < 50)
	{
		c = buf[i];
		if (c == '\0')
			return;
		put_char(c);
		i ++;
	}
}

void  print ( const char * fmt, ... )
{
	register char *s, *n, *p, par[8];
	register *adx, c, cn;

	adx = &fmt + 1;			//point to first param
loop:
	while ((c = *fmt ++) != '%' )
	{
		put_char(c);
		if (c == '\0')
			return;
	}
	c = *fmt ++;
	if (c == 'd')
	{
		printshort(*(int*)adx, "%d");
		adx += 1;
	}
	else if (c == 'f')
	{
		printfloat(*(double*)adx, "%f");
		adx += 4;
	}
	else if (c == 's')
	{
		s = *(char**)adx;
		while (c = *s++)
		{
			if (c == '\0')
				break;
			put_char(c);
		}
		adx += 2;
	}
	else if( c >= 48 && c < 58 || c == '.' )	//¸ñÊ½Êä³ö %3.2f %3f %.3f %3.f
	{
		n = fmt-2;
		memcpy(par, n, 8);

		if ((p = strstr(par, "d")) != NULL)
		{
			*(p+1) = '\0';
			printshort(*(int*)adx, par);
			fmt += p - par - 1;
			adx += 1;
		}

		if ((p = strstr(par, "f")) != NULL )
		{
			*(p + 1) = '\0';
			printfloat( *(double*)adx, par );
			fmt += p - par - 1;
			adx += 4;
		}
	}
	else
	{
		put_char('%');
		fmt--;
	}

	goto loop;
}

//////////////////////////////////////////////////////////////////////////////////////
