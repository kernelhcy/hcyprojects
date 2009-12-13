#include "html.h"


static int html_append(html_entry *html, const char *s)
{
	if (NULL == html || NULL == s)
	{
		return 0;
	}

	buffer_append(html -> entry, s);
	return 1;
}

html_entry* html_init()
{
	html_entry *e = NULL;
	e = (html_entry *)malloc(sizeof(&e));

	if (NULL == e)
	{
		log_err("Create html entry error. %s %d", __FILE__, __LINE__);
		return NULL;
	}

	e -> entry = buffer_init();
	html_append(e, "<html>\n");
	return e;
}

int html_close(html_entry *html)
{
	return html_append(html, "</html>\n");
}

int begin_head(html_entry *html)
{
	return html_append(html, "<head>\n");
}
int end_head(html_entry *html)
{
	return html_append(html, "</head>\n");
}

int i_title(html_entry *html, const char *title)
{
	
	if (NULL == html || NULL == title)
	{
		return 0;
	}
	
	html_append(html, "<title>");
	html_append(html, title);
	html_append(html, "</title>\n");

	return 1;
}

int begin_body(html_entry *html)
{
	return html_append(html, "<body>\n");
}
int end_body(html_entry *html)
{
	return html_append(html, "</body>\n");
}

int begin_p(html_entry *html)
{
	return html_append(html, "<p>\n");
}
int end_p(html_entry *html)
{
	return html_append(html, "</p>\n");
}

int i_br(html_entry *html)
{
	return html_append(html, "<br/>\n");
}
int i_a(html_entry *html, const char *text, const char *url)
{
	if (NULL == html)
	{
		return 0;
	}

	html_append(html, "<a href='");
	html_append(html, url == NULL ? " " : url);
	html_append(html, "'>");
	html_append(html, text == NULL ? " " : text );
	return html_append(html, "</a>");
}

int i_font(html_entry *html, const char *text, const char *font, int size, const char *color)
{
	if (NULL == html)
	{
		return 0;
	}

	html_append(html, "<font font='");
	html_append(html, font == NULL ? "宋体" : font);
	html_append(html, "' color='");
	html_append(html, color == NULL ? "black" : color);
	html_append(html, "' size='");
	buffer_append_int(html -> entry, size);
	html_append(html, "'>");
	html_append(html, text);
	return html_append(html, "</font>\n");
}


