#ifndef _SETTINGDIALOG_H
#define _SETTINGDIALOG_H

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QButtonGroup>
#include <QRadioButton>
#include "breakout.h"
#include "config.h"

class Breakout;
class Config;

class SettingDialog : public QDialog
{
	Q_OBJECT
	public:
		SettingDialog(Breakout *bo, QWidget *parent = 0);
		~SettingDialog();
	private slots:
		void defaultval();
		void ok();
		void cancel();

	private:
		Breakout *bo;
		QSpinBox *line_spinbox;
		QSpinBox *col_spinbox;
		QSlider *speed;
		QRadioButton *rand;
		QRadioButton *nor;
		/* 配置信息 */
    	Config *config;
};

#endif
