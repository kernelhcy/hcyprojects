#ifndef SORTDIALOG_H
#define SORTDIALOG_H
#include <QDialog>
#include "spreadsheet.h"
#include <iostream>

class QLabel;
class QPushButton;

class SortDialog : public QDialog
{

	Q_OBJECT
	public:
		SortDialog(QWidget *parent = 0);
		void setSpreadsheet(Spreadsheet *spreadSheet);
	private slots:
		void sort();
	private: 
		Spreadsheet *spreadSheet;
		QLabel *label;
		QPushButton *sortButton;
		QPushButton *closeButton;
};

#endif
