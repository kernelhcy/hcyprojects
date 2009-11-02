#include "ball.h"

Ball::Ball()
{

  	xdir = 1;
  	ydir = -1;

	x = 0;
	y = 0; 

  	image.load("pngs/ball.png");

  	rect = image.rect();
  	resetState();

}

Ball::~Ball()
{
  	//printf("Ball deleted\n");
}


void Ball::autoMove()
{
  	rect.translate(xdir, ydir);

  	if (rect.left() <= min_x) 
  	{
    	xdir = 1;
  	}

  	if (rect.right() >= max_x) 
  	{
    	xdir = -1;
  	}

  	if (rect.top() <= min_y) 
  	{
    	ydir = 1;
  	}
}

void Ball::resetState() 
{
 	rect.moveTo(x, y);
}

void Ball::moveBottom(int bottom)
{
  	rect.moveBottom(bottom);
}

void Ball::moveTop(int top)
{
  	rect.moveTop(top);
}

void Ball::moveLeft(int left)
{
  	rect.moveLeft(left);
}

void Ball::moveRight(int right)
{
  	rect.moveRight(right);
}

void Ball::setXDir(int x)
{
  	xdir = x;
}

void Ball::setYDir(int y)
{
  	ydir = y;
}

int Ball::getXDir()
{
  	return xdir;
}

int Ball::getYDir()
{
  	return ydir;
}

void Ball::setX(int xx)
{
	x = xx;
	//printf("The ball x: %d\n", x);
}
void Ball::setY(int yy)
{
	y = yy;
	//printf("The ball y: %d\n", y);
}
int Ball::getX()
{
	return x;
}
int Ball::getY()
{
	return y;
}

void Ball::setMaxX(int xx)
{
	max_x = xx;
}
void Ball::setMaxY(int yy)
{
	max_y = yy;
}
int Ball::getMaxX()
{
	return max_x;
}
int Ball::getMaxY()
{
	return max_y;
}



void Ball::setMinX(int xx)
{
	min_x = xx;
}
void Ball::setMinY(int yy)
{
	min_y = yy;
}
int Ball::getMinX()
{
	return min_x;
}
int Ball::getMinY()
{
	return min_y;
}


QRect Ball::getRect()
{
  	return rect;
}

QImage & Ball::getImage()
{
  	return image;
}
