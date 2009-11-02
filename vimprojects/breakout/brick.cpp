#include "brick.h"


Brick::Brick(int x, int y) 
{
  	image.load("pngs/brick.png");
  	destroyed = FALSE;
  	rect = image.rect();
  	rect.translate(x, y);
	//printf("brick w: %d h: %d\n", rect.width(), rect.height());
}

Brick::~Brick() 
{
 //	printf("Brick deleted\n");
}

QRect Brick::getRect()
{
  	return rect;
}

void Brick::setRect(QRect rct)
{
  	rect = rct;
}

QImage & Brick::getImage()
{
  	return image;
}

bool Brick::isDestroyed()
{
  	return destroyed;
}

void Brick::setDestroyed(bool destr)
{
  	destroyed = destr;
}

