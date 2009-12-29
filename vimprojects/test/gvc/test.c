#include <stdlib.h>
#include <stdio.h>

#include <gvc.h>

int main(int argc, char **argv)
{
	GVC_t *gvc = NULL;
	graph_t *g = NULL;
	FILE *fp = NULL;

	gvc = gvContext();

	if (argc > 1)
	{
		fp = fopen(argv[1], "r");
	}
	else 
	{
		fp = stdin;
	}

	g = agread(fp);
	gvLayout(gvc, g, "dot");
	//gvRenderFilename(gvc, g, "png", "out.png");
	//gvRenderFilename(gvc, g, "plain", "out");
	//gvRenderFilename(gvc, g, "plain-ext", "out");
	gvRenderFilename(gvc, g, "GXL", "out");
	gvFreeLayout(gvc, g);
	agclose(g);

	return (gvFreeContext(gvc));

}
