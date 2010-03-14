#include "mainwindow.h"
#include <iostream>
#include <QtGui>

MainWindow::MainWindow()
    :documents()
{
    tabWidget = new QTabWidget(this);
    tabWidget -> setTabsClosable(true);
    tabWidget -> setUsesScrollButtons(true);
    tabWidget -> setDocumentMode(true);
    connect(tabWidget, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
    connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(currentTabChanged(int)));
    setCentralWidget(tabWidget);

	createActions();
	createMenus();
    createContextMenu(0);
	createToolBars();
	createStatusBar();
	readSettings();
    findDialog = 0;
    setWindowIcon(QIcon(":/images/pics/ss.png"));
	setCurrentFile("");

    //test
    setWindowModified(true);
    setCurrentFile(tr("data.sp"));
    setCurrentFile(tr("money.sp"));
    setCurrentFile(tr("students.sp"));
    setCurrentFile(tr("teachers.sp"));
    setCurrentFile(tr("total.sp"));
}

MainWindow::~MainWindow()
{
    return;
}
void MainWindow::createActions()
{
    newAction = new QAction(tr("&New"), this );
    newAction->setIcon(QIcon(":/images/pics/new.png"));
	newAction->setShortcut(tr("Ctrl+N"));
    newAction->setStatusTip(tr("Create a new spreadsheet."));
	connect(newAction, SIGNAL(triggered()), this , SLOT(newFile()));
	// 其他相关 action
	for ( int i = 0; i < MaxRecentFiles; ++i)
	{
        recentFileActions[i] = new QAction(this);
        recentFileActions[i]->setVisible(false);
		connect(recentFileActions[i], SIGNAL(triggered()), this , SLOT(openRecentFile()));
	}
	
    selectAllAction = new QAction(tr("&Select All"), this );
	selectAllAction->setShortcut(tr("Ctrl+A"));
    selectAllAction->setStatusTip(tr("Select all the data" "spreadsheet"));
    //connect(selectAllAction, SIGNAL(triggered()), spreadsheet, SLOT(selectAll())) ;
	
    aboutQtAction = new QAction(tr("About &Qt"), this );
    aboutQtAction -> setIcon(QIcon(":/images/pics/qt.png"));
    aboutQtAction->setStatusTip(tr("Show the Qt library's About box"));
	connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	
    saveAction = new QAction(tr("&Save"), this );
    saveAction -> setShortcut(tr("Ctrl+S"));
    saveAction -> setIcon(QIcon(":/images/pics/save.png"));
	connect(saveAction, SIGNAL(triggered()), this, SLOT(save())) ;
	
    saveAsAction = new QAction(tr("Save &As"), this );
	connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs())) ;
	
    openAction = new QAction(tr("&Open"), this);
    openAction -> setShortcut(tr("Ctrl+O"));
    openAction -> setIcon(QIcon(":/images/pics/open.png"));
	connect(openAction, SIGNAL(triggered()), this, SLOT(open()));
	
    cutAction = new QAction(tr("C&ut"), this );
    cutAction -> setShortcut(tr("Ctrl+X"));
    cutAction -> setIcon(QIcon(":/images/pics/cut.png"));
    connect(cutAction, SIGNAL(triggered()), this, SLOT(cut())) ;
	
    copyAction = new QAction(tr("&Copy"), this );
    copyAction -> setShortcut(tr("Ctrl+X"));
    copyAction -> setIcon(QIcon(":/images/pics/copy.png"));
    connect(copyAction, SIGNAL(triggered()), this, SLOT(copy())) ;
	
    pasteAction = new QAction(tr("&Past"), this );
    pasteAction -> setShortcut(tr("Ctrl+P"));
    pasteAction -> setIcon(QIcon(":/images/pics/paste.png"));
    connect(pasteAction, SIGNAL(triggered()), this, SLOT(paste())) ;
	
    deleteAction = new QAction(tr("&Delete"), this );
    deleteAction -> setShortcut(tr("Ctrl+D"));
    deleteAction -> setIcon(QIcon(":/images/pics/delete.png"));
    connect(deleteAction, SIGNAL(triggered()), this, SLOT(del())) ;
	
    findAction = new QAction(tr("&Find"), this );
    findAction -> setShortcut(tr("Ctrl+F"));
    findAction -> setIcon(QIcon(":/images/pics/search.png"));
	connect(findAction, SIGNAL(triggered()), this, SLOT(find())) ;
	
    goToCellAction = new QAction(tr("Go &to Cell"), this );
    goToCellAction -> setShortcut(tr("Ctrl+G"));
    goToCellAction -> setIcon(QIcon(":/images/pics/gotocell.png"));
    connect(goToCellAction, SIGNAL(triggered()), this, SLOT(goToCell()));

    exitAction = new QAction(tr("&Quit"), this );
    exitAction -> setShortcut(tr("Ctrl+Q"));
    exitAction -> setIcon(QIcon(":/images/pics/quit.png"));
	connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
	
    aboutAction = new QAction(tr("&About Spreadsheet"), this );
    aboutAction -> setIcon(QIcon(":/images/pics/about.png"));
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
	
    selectRowAction = new QAction(tr("Select &row"), this );
    selectColumnAction = new QAction(tr("Select &column"), this );

    recalculateAction = new QAction(tr("&ReCalculate"), this);
    recalculateAction -> setShortcut(tr("Ctrl+R"));
    recalculateAction -> setIcon(QIcon(":/images/pics/cal.png"));

    sortAction = new QAction(tr("&Sort"), this);
    sortAction -> setShortcut(tr("Ctrl+T"));
    sortAction -> setIcon(QIcon(":/images/pics/sort.png"));
	connect(sortAction, SIGNAL(triggered()), this, SLOT(sort()));
	
    showGridAction = new QAction(tr("Show &Grid"), this );
    showGridAction->setCheckable( true );
    //showGridAction->setChecked(spreadsheet->showGrid());
    showGridAction->setStatusTip(tr("Show or hide the grid"));
    //connect(showGridAction, SIGNAL(toggled( bool )),spreadsheet, SLOT(setShowGrid( bool )));

    autoRecalcAction = new QAction(tr("&Auto RecalcAction"), this);
    autoRecalcAction -> setCheckable(true);
    autoRecalcAction -> setChecked(true);
}


void MainWindow::createMenus()
{
    fileMenu = menuBar() -> addMenu(tr("&File"));
	fileMenu -> addAction(newAction);
	fileMenu -> addAction(openAction);
	fileMenu -> addAction(saveAction);
	fileMenu -> addAction(saveAsAction);
	
	separatorAction = fileMenu -> addSeparator();
	
	for ( int i = 0; i < MaxRecentFiles; ++i)
	{
		fileMenu -> addAction(recentFileActions[i]);
	}
	
	fileMenu -> addSeparator();
	
	fileMenu -> addAction(exitAction);
    editMenu = menuBar() -> addMenu(tr("&Edit"));
	editMenu -> addAction(cutAction);
	editMenu -> addAction(copyAction);
	editMenu -> addAction(pasteAction);
	editMenu -> addAction(deleteAction);
    editMenu -> addSeparator();
	
    selectSubMenu = editMenu -> addMenu(tr("&Select"));
	selectSubMenu -> addAction(selectRowAction);
	selectSubMenu -> addAction(selectColumnAction);
	selectSubMenu -> addAction(selectAllAction);
	
	editMenu -> addSeparator();
	
	editMenu -> addAction(findAction);
	editMenu -> addAction(goToCellAction);
	
    toolsMenu = menuBar() -> addMenu(tr("&Tools"));
	toolsMenu -> addAction(recalculateAction);
	toolsMenu -> addAction(sortAction);
	
    optionsMenu = menuBar() -> addMenu(tr("&Options"));
	optionsMenu -> addAction(showGridAction);
	optionsMenu -> addAction(autoRecalcAction);
	menuBar() -> addSeparator();
	
    helpMenu = menuBar() -> addMenu(tr("A&bout"));
	helpMenu -> addAction(aboutAction);
	helpMenu -> addAction(aboutQtAction);

}

void MainWindow::createContextMenu(QWidget *w)
{
    if (!w)
    {
        return;
    }

    w -> addAction(cutAction);
    w -> addAction(copyAction);
    w -> addAction(pasteAction);
    w -> addAction(separatorAction);
    w -> addAction(deleteAction);
    w -> setContextMenuPolicy(Qt::ActionsContextMenu);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("&File"));
	fileToolBar -> addAction(newAction);
	fileToolBar -> addAction(openAction);	
	fileToolBar -> addAction(saveAction);
	
    editToolBar = addToolBar(tr("&Edit"));
	editToolBar -> addAction(cutAction);
	editToolBar -> addAction(copyAction);
	editToolBar -> addAction(pasteAction);
	editToolBar -> addSeparator();
    editToolBar -> addAction(deleteAction);

    actionToolBar = addToolBar(tr("&Acion"));
    actionToolBar -> addAction(findAction);
    actionToolBar -> addAction(goToCellAction);
    actionToolBar -> addSeparator();
    actionToolBar -> addAction(sortAction);
    actionToolBar -> addAction(recalculateAction);
    actionToolBar -> addSeparator();
    actionToolBar -> addAction(exitAction);
    actionToolBar -> addAction(aboutAction);
}

void MainWindow::createStatusBar()
{
	locationLabel = new QLabel(" W999 ");
	locationLabel -> setAlignment(Qt::AlignHCenter);
	locationLabel -> setMinimumSize(locationLabel -> sizeHint());
	formulaLabel = new QLabel;
	formulaLabel -> setIndent(3);
    formulaLabel -> setMaximumSize(formulaLabel -> sizeHint());
	statusBar() -> addWidget(locationLabel);
	statusBar() -> addWidget(formulaLabel, 1);
    Spreadsheet *ss = static_cast<Spreadsheet *> (tabWidget ->currentWidget());
    connect(ss, SIGNAL(currentCellChanged( int , int , int , int ))
                                  , this , SLOT(updateStatusBar()));
    connect(ss, SIGNAL(modified()), this , SLOT(spreadsheetModified()));

    //show the tabbar in the status bar
//    QWidget *hb = new QWidget(statusBar());
//    hb->setObjectName("taskbar");
//    QHBoxLayout *hbLayout = new QHBoxLayout(hb);
//    hbLayout->setMargin(0);
//    hbLayout->setObjectName("tasklayout");
//    statusBar()->addWidget(hb);
//    //create a tabbar in the hbox
//    hbLayout->addWidget(mdiArea->getTabBar());

	updateStatusBar();
}


void MainWindow::updateStatusBar()
{
    //std::cout << "update status bar\n";
    Spreadsheet *ss = static_cast<Spreadsheet *> (tabWidget ->currentWidget());
    locationLabel->setText(ss->currentLocation());
    formulaLabel->setText(ss->currentFormula());
}


void MainWindow::spreadsheetModified()
{
    setWindowModified(true);
	updateStatusBar();
}

void MainWindow::newFile()
{
	if (okToContinue()) 
	{
        //we just create a spreatsheet file.
        Spreadsheet *ss = new Spreadsheet(this);
        int index = tabWidget -> addTab(ss, tr("untitled"));
        documents[index] = ss;
        ss -> clear();
        createContextMenu(ss);
	}
    //std::cout << "create a new window\n";

}

bool MainWindow::okToContinue()
{
	if (isWindowModified()) 
	{
		int r = QMessageBox::warning( this , tr("Spreadsheet")
                        , tr("The file has been modified。""Do you want to save it？"),
						QMessageBox::Yes | QMessageBox::Default,
						QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
		if (r == QMessageBox::Yes) 
		{
			return save();
		} 
		else if (r == QMessageBox::Cancel) 
		{
			return false ;
		}
	}
	return true ;
}

void MainWindow::open()
{
	if (okToContinue()) 
	{
		QString fileName = QFileDialog::getOpenFileName( this ,
                    tr("Open"), ".",
                    tr("Spreadsheet files (*.sp),""All files (*.*)"));
		if (!fileName.isEmpty())
		{
			loadFile(fileName);
		}
	}
}


bool MainWindow::loadFile(const QString &fileName)
{
    //std::cout << "load file\n";
    Spreadsheet *ss = new Spreadsheet();
    ss -> setFileName(fileName);
    createContextMenu(ss);
    int index = tabWidget -> addTab(ss, fileName);
    //document[index] = ss;
    if (!ss -> load())
    {
        statusBar() -> showMessage(tr("Loading canceled"), 2000);
        return false ;
    }
    setCurrentFile(fileName);
    statusBar() -> showMessage(tr("File loaded"), 2000);
	return true ;
}

bool MainWindow::save()
{
	if (curFile.isEmpty()) 
	{
		return saveAs();
	} 
	else 
	{
		return saveFile(curFile);
	}
}

bool MainWindow::saveFile(const QString &fileName)
{
    //std::cout << "save file\n";
    //just deal with spreadsheet
    Spreadsheet *ss = static_cast<Spreadsheet *> (tabWidget ->currentWidget());
    ss -> setFileName(fileName);

    if (!ss -> save())
    {
        statusBar() -> showMessage(tr("Saving canceled"), 2000);
        return false ;
    }
    setCurrentFile(fileName);
	statusBar()->showMessage(tr("File saved"), 2000);
	return true ;
}

bool MainWindow::saveAs()
{
	QString fileName = QFileDialog::getSaveFileName( this ,
								tr("Save Spreadsheet"), ".",
								tr("Spreadsheet files (*.sp)"));
								
	if (fileName.isEmpty())
	{
		return false ;
	}
	return saveFile(fileName);
}

void MainWindow::closeEvent(QCloseEvent * event )
{
	if (okToContinue()) 
	{
		writeSettings();
		event ->accept();
	} 
	else 
	{
		event ->ignore();
	}
}


void MainWindow::setCurrentFile(const QString &fileName)
{
	curFile = fileName;
    setWindowModified(false);
	QString shownName = "Untitled";
	if (!curFile.isEmpty()) 
	{
		shownName = strippedName(curFile);
		recentFiles.removeAll(curFile);
		recentFiles.prepend(curFile);
		updateRecentFileActions();
	}
	setWindowTitle(tr("%1[*] - %2").arg(shownName).arg(tr("Spreadsheet")));
}

QString MainWindow::strippedName( const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

void MainWindow::updateRecentFileActions()
{
	QMutableStringListIterator i(recentFiles);
	while (i.hasNext()) 
	{
		if (!QFile::exists(i.next()))
		i.remove();
	}
	for ( int j = 0; j < MaxRecentFiles; ++j) 
	{
		if (j < recentFiles.count()) 
		{
			QString text = tr("&%1 %2").arg(j + 1).arg(strippedName(recentFiles[j]));
			recentFileActions[j] -> setText(text);
			recentFileActions[j] -> setData(recentFiles[j]);
			recentFileActions[j] -> setVisible( true );
		} 
		else 
		{
			recentFileActions[j] -> setVisible( false );
		}
	}
	separatorAction -> setVisible(!recentFiles.isEmpty());
}

void MainWindow::openRecentFile()
{
	if (okToContinue()) 
	{
		QAction *action = qobject_cast<QAction *>(sender());
		if (action)
		{
			loadFile(action->data().toString());
		}
	}
}

void MainWindow::find()
{
    if (!findDialog)
    {
		findDialog = new FindDialog( this );
	}
    Spreadsheet *ss = static_cast<Spreadsheet *> (tabWidget ->currentWidget());
    connect(findDialog, SIGNAL (findNext( const QString &, Qt::CaseSensitivity)),
            ss, SLOT (findNext( const QString &, Qt::CaseSensitivity)));
    connect(findDialog, SIGNAL(findPrevious( const QString &,Qt::CaseSensitivity)),
            ss, SLOT(findPrevious( const QString &,Qt::CaseSensitivity)));
	findDialog -> show();
	findDialog -> activateWindow();
}

void MainWindow::goToCell()
{
    GoToCellDialog dialog(this);
    if (dialog.exec()) {
        QString str = dialog.lineEdit->text().toUpper();
        Spreadsheet *ss = static_cast<Spreadsheet *> (tabWidget ->currentWidget());
        ss -> setCurrentCell(str.mid(1).toInt() - 1, str[0].unicode() - 'A');
    }
}

void MainWindow::sort()
{
    SortDialog dialog(this);
    Spreadsheet *ss = static_cast<Spreadsheet *> (tabWidget ->currentWidget());
    QTableWidgetSelectionRange range = ss -> selectedRange();
    dialog.setColumnRange('A' + range.leftColumn(), 'A' + range.rightColumn());
    if (dialog.exec())
    {
        SpreadsheetCompare compare;
        compare.keys[0] = dialog.primaryColumnCombo->currentIndex();
        compare.keys[1] = dialog.secondaryColumnCombo->currentIndex() - 1;
        compare.keys[2] = dialog.tertiaryColumnCombo->currentIndex() - 1;
        compare.ascending[0] = (dialog.primaryOrderCombo->currentIndex() == 0);
        compare.ascending[1] = (dialog.secondaryOrderCombo->currentIndex() == 0);
        compare.ascending[2] = (dialog.tertiaryOrderCombo->currentIndex() == 0);
        ss->sort(compare);
    }
}


void MainWindow::about()
{
	QMessageBox::about(this, tr("About Spreadsheet"),
		tr("<h2>Spreadsheet 1.1</h2>"
		"<p>Copyright &copy; 2006 Software Inc."
		"<p>Spreadsheet is a small application that "
		"demonstrates QAction, QMainWindow, QMenuBar, "
		"QStatusBar, QTableWidget, QToolBar, and many other "
		"Qt classes."));
}


void MainWindow::writeSettings()
{
	QSettings settings("Software Inc.", "Spreadsheet");
	settings.setValue("geometry", geometry());
	settings.setValue("recentFiles", recentFiles);
	settings.setValue("showGrid", showGridAction->isChecked());
	settings.setValue("autoRecalc", autoRecalcAction->isChecked());
}

void MainWindow::readSettings()
{
	QSettings settings("Software Inc.", "Spreadsheet");
	QRect rect = settings.value("geometry", QRect(200, 200, 400, 400)).toRect();
	move(rect.topLeft());
	resize(rect.size());
	recentFiles = settings.value("recentFiles").toStringList();
	updateRecentFileActions();
	bool showGrid = settings.value("showGrid", true).toBool();
	showGridAction->setChecked(showGrid);
	bool autoRecalc = settings.value("autoRecalc", true).toBool();
	autoRecalcAction->setChecked(autoRecalc);
}

void MainWindow::closeTab(int index)
{
    if (index > tabWidget -> count())
    {
        return;
    }

    QWidget *widget = tabWidget -> widget(index);

    if (widget -> isWindowModified())
    {
        int r = QMessageBox::warning( this , tr("Spreadsheet")
                        , tr("The file has been modified。""Do you want to save it？"),
                        QMessageBox::Yes | QMessageBox::Default,
                        QMessageBox::No, QMessageBox::Cancel | QMessageBox::Escape);
        if (r == QMessageBox::Yes)
        {
            save();
        }
        else if (r == QMessageBox::Cancel)
        {
            return ;
        }
    }

    tabWidget -> removeTab(index);
}

void MainWindow::currentTabChanged(int index)
{
    QString title = tabWidget -> tabText(index);
    setWindowTitle(tr("%1[*] - %2").arg(title).arg(tr("Spreadsheet")));
}

int MainWindow::newTab(QWidget *w, const QString &title)
{
    if (w == 0)
    {
        return 0;
    }
    if (w -> isWindowModified())
    {
        setWindowModified(true);
    }

    setWindowTitle(tr("%1[*] - %2").arg(title).arg(tr("Spreadsheet")));
    int index = tabWidget -> addTab(w, title);
    tabWidget -> setTabText(index, title);
    return index;
}

/************
 * 封装文档的slot
 *
 ************
 */
void MainWindow::cut()
{
    Spreadsheet *ss = static_cast<Spreadsheet *> (tabWidget ->currentWidget());
    ss -> cut();
}
void MainWindow::copy()
{
    Spreadsheet *ss = static_cast<Spreadsheet *> (tabWidget ->currentWidget());
    ss -> copy();
}
void MainWindow::del()
{
    Spreadsheet *ss = static_cast<Spreadsheet *> (tabWidget ->currentWidget());
    ss -> del();
}
void MainWindow::paste()
{
    Spreadsheet *ss = static_cast<Spreadsheet *> (tabWidget ->currentWidget());
    ss -> paste();
}

