#include "breakout.h"


Breakout::Breakout(QMainWindow *parent)
    : QMainWindow(parent)
{

	setWindowTitle("BreakOut");
	setWindowIcon(QIcon("pngs/gnome-starthere.png"));
  	
  	config = Config::getInstance();
  	
  	createMenu();
  	  	
  	createGame();

}

Breakout::~Breakout() 
{
 	deleteAll();
}

void Breakout::deleteAll()
{
	delete core;
 	delete ball;
 	delete paddle;
 	int k = 0;
	for (int i = 0; i < brick_line; i++) 
	{
    	for (int j = 0; j < brick_col; j++) 
		{
      		delete bricks[k];
      		k++; 
    	}
  	}
 	delete[] bricks;
}

void Breakout::deleteGame()
{
	deleteAll();
}

void Breakout::createGame()
{
	gameOver = FALSE;
    gameWon = FALSE;
    gameStarted = FALSE;
    paused = FALSE;
  	
  	interval = 21 - config -> speed;
  	
  	brick_line = config -> line;
	brick_col = config -> col;
  	
  	ball = new Ball();
  	paddle = new Paddle();

	/* 产生砖块 */
	brick_cnt = brick_line * brick_col;
  	
	bricks = new Brick*[brick_cnt];

	gap = 1;
	Brick *tbrick = new Brick(1, 1);;
  	int wb = (tbrick -> getRect()).width() + gap;
  	int hb = (tbrick -> getRect()).height() + gap;
	delete tbrick;
	
	int k = 0;
	for (int i = 0; i < brick_line; i++) 
	{
    	for (int j = 0; j < brick_col; j++) 
		{
      		bricks[k] = new Brick(j * wb + 10, i * hb + 30);
      		k++; 
    	}
  	}

	/* 计算并设置窗口的大小 */
	win_width = wb * brick_col + 20 - gap;
	win_height = hb * brick_line + 350;

	/* 设置窗口的最大和最小值，以便改变窗口大小 */
	setMaximumSize(QSize(999999, 999999));
	setMinimumSize(QSize(1, 1));
	
	resize(win_width, win_height);
	/* 设置窗口大小不可变 */
	setMaximumSize(QSize(win_width, win_height));
	setMinimumSize(QSize(win_width, win_height));

	/*设置paddle和ball的初始位置*/
	ball -> setX(win_width / 2);
	ball -> setY(win_height - 28);
	paddle -> setX(win_width / 2 - 20);
	paddle -> setY(win_height - 20);
   	ball->resetState();
    paddle->resetState();

	/*设置paddle和ball的活动范围*/
	ball -> setMaxX(win_width);
	ball -> setMaxY(win_height);
	ball -> setMinX(0);
	ball -> setMinY(20);
	
	paddle -> setMaxX(win_width - 2);
	paddle -> setMaxY(win_height - 20 + (paddle -> getRect()).height() - 2);
	paddle -> setMinX(1);
	paddle -> setMinY(win_height - 200);
	
	
  	core = new Core(ball, bricks, paddle, brick_cnt);
  	core -> setPaddleFly(TRUE);
  	repaint();
}

void Breakout::createMenu()
{
 	QPixmap newpix("pngs/new.png");
 	QPixmap settingpix("pngs/setting.png");
 	QPixmap rulepix("pngs/rule.png");
 	QPixmap quitpix("pngs/quitter.png");
 	
	QAction *newa = new QAction(newpix, tr("新游戏(&N)"), this);
  	QAction *setting = new QAction(settingpix, tr("设置(&S)"), this);
  	QAction *rule = new QAction(rulepix, tr("规则(&R)"), this);
  	QAction *quit = new QAction(quitpix, tr("退出(&Q)"), this);
  	
  	quit->setShortcut(tr("CTRL+Q"));
  	connect(newa, SIGNAL(triggered()), this, SLOT(newGame()));
	connect(setting, SIGNAL(triggered()), this, SLOT(setting()));
	connect(rule, SIGNAL(triggered()), this, SLOT(rule()));
	connect(quit, SIGNAL(triggered()), qApp, SLOT(quit()));

  	QMenu *game;
  	game = menuBar() -> addMenu(tr("游戏(&G)"));
  	game -> addAction(newa);
  	game -> addAction(setting);
  	game -> addAction(rule);
  	game -> addSeparator();
  	game -> addAction(quit);
  	
  	QMenu *about = menuBar() -> addMenu(tr("关于(&A)"));
  	
  	QPixmap aboutbopix("pngs/aboutbo.png");
  	QPixmap aboutqtpix("pngs/aboutqt.png");
  	
  	QAction *aboutbo = new QAction(aboutbopix, tr("关于 &Breakout"), this);
  	QAction *aboutqt = new QAction(aboutqtpix, tr("关于 &Qt"), this);
  	
  	about -> addAction(aboutbo);
  	about -> addAction(aboutqt);
  	
  	connect(aboutbo, SIGNAL(triggered()), this, SLOT(aboutBreakout()));
  	connect(aboutqt, SIGNAL(triggered()),  qApp, SLOT(aboutQt()));
}

/* private slots */
void Breakout::newGame()
{
	killTimer(timerId);
	createGame();
}
void Breakout::setting()
{
	sd = new SettingDialog(this, this);
}
void Breakout::rule()
{
    //printf("Rule.\n");
    new RuleDialog(this);
}
void Breakout::aboutBreakout()
{
    //printf("About Breakout.\n");
    new AboutDialog(this);
}

void Breakout::paintEvent(QPaintEvent *event)
{
  	QPainter painter(this);

  	if (gameOver) 
	{
		/* 设置字体 */
    	QFont font("Courier", 15, QFont::DemiBold);
    	QFontMetrics fm(font);
    	int textWidth = fm.width("Game Over");

    	painter.setFont(font);
    	int h = height();
    	int w = width();

    	painter.translate(QPoint(w/2, h/2));
    	painter.drawText(-textWidth/2, 0, "Game Over");
  	}
  	else if(gameWon) 
	{
    	QFont font("Courier", 15, QFont::DemiBold);
    	QFontMetrics fm(font);
    	int textWidth = fm.width("Victory");

    	painter.setFont(font);
    	int h = height();
    	int w = width();

    	painter.translate(QPoint(w/2, h/2));
    	painter.drawText(-textWidth/2, 0, "Victory");
  	}
  	else 
	{
		/* 重新绘制球，砖和挡板 */
    	painter.drawImage(ball->getRect(), ball->getImage());
    	painter.drawImage(paddle->getRect(), paddle->getImage());

    	for (int i = 0; i < brick_cnt; i++) 
		{
        	if (!bricks[i]->isDestroyed()) 
          		painter.drawImage(bricks[i]->getRect(), bricks[i]->getImage());
    	}

		/*
		 * 绘制警戒线
		 * ball所能达到的最底线，过了就失败了。。。
		 */
		painter.drawLine(0, win_height, win_width, win_height);
		painter.drawLine(0, win_height - 2, win_width, win_height - 2);
		painter.drawLine(0, win_height - 4, win_width, win_height - 4);
  	}
}

void Breakout::keyPressEvent(QKeyEvent *event)
{
    switch (event->key()) 
	{
    	case Qt::Key_Left:
       		{  
    			int x = paddle->getRect().x();
                for (int i = 1; i <= 10; i++)
				{
                    paddle->moveLeft(x-=2);
				}
			}
			break;

    	case Qt::Key_Right:
        	{
				int x = paddle->getRect().x();
                for (int i = 1; i <= 10; i++)
				{
                    paddle->moveRight(x+=2);
				}
			}
        	break;

    	case Qt::Key_Up:
        	if (core -> getPaddleFly())
			{
				int y = paddle->getRect().y();
    			for (int i = 1; i <= 15; i++)
				{
      				paddle->moveTop(y--);
				}
			}
       	 	break;

		
    	case Qt::Key_Down:
        	if (core -> getPaddleFly())
			{
				int y = paddle->getRect().y();
    			for (int i = 1; i <= 15; i++)
				{
      				paddle->moveBottom(y++);
				}
			}
        	break;

    	case Qt::Key_P:
         	pauseGame();
        	break;

    	case Qt::Key_Space:
        	startGame();
        	break;

    	case Qt::Key_Escape:
        	qApp->exit();
       		break;

    	default:
        	QWidget::keyPressEvent(event);
    }
}

/* 计时器定时触发 */
void Breakout::timerEvent(QTimerEvent *event)
{
	/* 移动球 */
  	ball->autoMove();
	/* 判断碰撞 */
  	int state = core -> checkCollision();
  	if (state == -1)
  	{
  		stopGame();
  	}
  	else if (state == 1)
  	{
  		victory();
  	}
  	
	/* 调用slot，触发重新绘制事件 */
  	repaint();
}




void Breakout::victory()
{
  	killTimer(timerId);
  	gameWon = TRUE;  
  	gameStarted = FALSE;
}

void Breakout::startGame()
{ 
  	if (!gameStarted) 
  	{
		/* 设置ball和paddle的起始位置 */
   		ball->resetState();
    	paddle->resetState();

		/* 设置砖块 */
    	for (int i = 0; i < brick_cnt; i++) 
		{
      		bricks[i]->setDestroyed(FALSE);
    	}

    	gameOver = FALSE; 
    	gameWon = FALSE; 
    	gameStarted = TRUE;
		/* 计时器开始 */
		timerId = startTimer(interval); 
  	}
}

void Breakout::pauseGame()
{
  	if (paused) 
	{
    	timerId = startTimer(interval);
    	paused = FALSE;
  	} 
	else 
	{
    	paused = TRUE;
		/* 停止计时器 */
    	killTimer(timerId); 
   	}
}

void Breakout::stopGame()
{
  	killTimer(timerId);    
  	gameOver = TRUE;      
  	gameStarted = FALSE;
}

