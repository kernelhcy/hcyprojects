#include "settingdialog.h"

SettingDialog::SettingDialog(Breakout *bo, QWidget *parent)
		:bo(bo), QDialog(parent)
{
	setWindowTitle(tr("设置"));
	
	config = Config::getInstance();
	
	QVBoxLayout *vbox = new QVBoxLayout(this);
  	QHBoxLayout *hbox = new QHBoxLayout();
	
	/* 砖块的行数和列数 */
	QLabel *line = new QLabel(tr("行数："));
	QLabel *col = new QLabel(tr("列数："));
	
	line_spinbox = new QSpinBox(this);
  	line_spinbox -> setGeometry(50, 50, 60, 30);
  	line_spinbox -> setMaximum(50);
	line_spinbox -> setMinimum(1);
	line_spinbox -> setValue(config -> line);
	
	col_spinbox = new QSpinBox(this);
  	col_spinbox -> setGeometry(50, 50, 60, 30);
	col_spinbox -> setMaximum(50);
	col_spinbox -> setMinimum(1);
	col_spinbox -> setValue(config -> col);
	
	hbox -> addWidget(line);
	hbox -> addWidget(line_spinbox);
	hbox -> addWidget(col);
	hbox -> addWidget(col_spinbox);
	
	vbox -> addLayout(hbox);
	
	/* 速度 */
	speed = new QSlider(Qt::Horizontal , this);
	
	QLabel *speed_label = new QLabel("", this);
  	speed_label->setGeometry(230, 50, 20, 30);
  	connect(speed, SIGNAL(valueChanged(int)), speed_label, SLOT(setNum(int)));
  	
  	speed -> setGeometry(50, 50, 130, 30);
  	speed -> setMaximum(20);
  	speed -> setSingleStep(10);
  	speed -> setSliderPosition(config -> speed);
  	
  	
  	QHBoxLayout *hbox1 = new QHBoxLayout();
  	hbox1 -> addWidget(new QLabel(tr("设置速度 : "), this));
  	hbox1 -> addWidget(speed_label);
  	vbox -> addLayout(hbox1);
  	vbox -> addWidget(speed);
	
	/* 设置模式 */
	QLabel *mod = new QLabel(tr("模式 ："));
	vbox -> addWidget(mod);
	QButtonGroup *bg = new QButtonGroup(this);
	rand = new QRadioButton(tr("随机模式"), this);
	nor = new QRadioButton(tr("正常模式"), this);
	rand -> setCheckable(TRUE);
	nor -> setCheckable(TRUE);
	
	if (config -> mode == 1)
	{
		rand -> setChecked(TRUE);
	}
	else
	{
		nor -> setChecked(TRUE);
	
	}
	bg -> addButton(nor);
	bg -> addButton(rand);

	QHBoxLayout *hbox3 = new QHBoxLayout();
	hbox3 -> addWidget(nor);
	hbox3 -> addWidget(rand);
	hbox3 -> setSpacing(10);
	vbox -> addLayout(hbox3);

	QPushButton *defaultval = new QPushButton(tr("默认"), this);
	QPushButton *ok = new QPushButton(tr("确定"), this);
	QPushButton *cancel = new QPushButton(tr("取消"), this);
	
	connect(defaultval, SIGNAL(released()), this, SLOT(defaultval()));
	connect(ok, SIGNAL(released()), this, SLOT(ok()));
	connect(cancel, SIGNAL(released()), this, SLOT(cancel()));

	QHBoxLayout *hbox2 = new QHBoxLayout();
  	hbox2 -> addWidget(defaultval);
	hbox2 -> addWidget(ok);
	hbox2 -> addWidget(cancel);
	vbox -> addLayout(hbox2);

	//resize(size());
	
	show();
	
	/* 设置窗口大小不可变 */
	setMaximumSize(size());
	setMinimumSize(size());
	
}
SettingDialog::~SettingDialog()
{
	config -> writeToFile();
}

void SettingDialog::defaultval()
{
	
	line_spinbox -> setValue(5);
	col_spinbox -> setValue(6);
	speed -> setValue(10);
	nor -> setChecked(TRUE);
	//ok();
}
void SettingDialog::ok()
{
	bo -> deleteGame();
	config -> line = line_spinbox -> value();
	config -> col = col_spinbox -> value();
	config -> speed = speed -> value();
	
	if (nor -> isChecked())
	{
		config -> mode = 0;
	}
	else
	{
		config -> mode = 1;
	}
	
	config -> writeToFile();
	
	bo -> newGame();
	
	this -> close();
}
void SettingDialog::cancel()
{
	
	this -> close();
}

