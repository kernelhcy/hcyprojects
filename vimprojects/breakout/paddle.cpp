#include "paddle.h"


Paddle::Paddle()
{
  	image.load("pngs/paddle.png");

  	rect = image.rect();
  	resetState();
}

Paddle::~Paddle()
{
 	//printf("paddle deleted\n");
}

void Paddle::moveLeft(int left)
{
  	if (rect.left() >= min_x)
	{
    	rect.moveTo(left, rect.top());
	}
}

void Paddle::moveRight(int right)
{
  	if (rect.right() <= max_x)
	{
    	rect.moveTo(right, rect.top());
	}
}

void Paddle::moveTop(int top)
{
  	if (rect.top() >= min_y)
	{
    	rect.moveTo(rect.left(), top);
	}
}

void Paddle::moveBottom(int bottom)
{
  	if (rect.bottom() <= max_y)
	{
    	rect.moveTo(rect.left(), bottom);
	}
}

void Paddle::resetState()
{
  	rect.moveTo(x, y);
}

QRect Paddle::getRect()
{
  	return rect;
}

QImage & Paddle::getImage()
{
  	return image;
}

void Paddle::setX(int xx)
{
	x = xx;
}
void Paddle::setY(int yy)
{
	y = yy;
}
int Paddle::getX()
{
	return x;
}
int Paddle::getY()
{
	return y;
}

void Paddle::setMaxX(int xx)
{
	max_x = xx;
}
void Paddle::setMaxY(int yy)
{
	max_y = yy;
}
int Paddle::getMaxX()
{
	return max_x;
}
int Paddle::getMaxY()
{
	return max_y;
}



void Paddle::setMinX(int xx)
{
	min_x = xx;
}
void Paddle::setMinY(int yy)
{
	min_y = yy;
}
int Paddle::getMinX()
{
	return min_x;
}
int Paddle::getMinY()
{
	return min_y;
}
