#include "sortdialog.h"
#include <QtGui>

SortDialog::SortDialog(QWidget * parent)
	: QDialog(parent)
{
	label = new QLabel(tr("Find & What:"));

	sortButton = new QPushButton(tr("&Sort"));
	sortButton -> setDefault(true);
		
	closeButton = new QPushButton(tr("Close"));
	
	connect(sortButton, SIGNAL(clicked()), this, SLOT(sort()));
	
	connect(closeButton, SIGNAL(clicked()), this, SLOT(close()));
	
	QVBoxLayout *rightLayout = new QVBoxLayout;
	rightLayout -> addWidget(sortButton);
	rightLayout -> addWidget(closeButton);
	rightLayout -> addStretch();

	setLayout(rightLayout);
	setWindowTitle(tr("Sort"));
	setFixedHeight(sizeHint().height());
}

void SortDialog::sort()
{
	std::cout << "sort\n";
}
void SortDialog::setSpreadsheet(Spreadsheet *spreadSheet)
{
	this -> spreadSheet = spreadSheet;
}
