#include "mainwindow.h"

MainWindow::MainWindow()
{
	spreadsheet = new Spreadsheet;
    setCentralWidget(spreadsheet);
	createActions();
	createMenus();
	createContextMenu();
	createToolBars();
	createStatusBar();
	readSettings();
	findDialog = 0;
	//setWindowIcon(QIcon(":/images/icon.png"));
	setCurrentFile("");
	
}


void MainWindow::createActions()
{
	newAction = new QAction(tr("&新建"), this );
	//newAction->setIcon(QIcon(":/images/new.png"));
	newAction->setShortcut(tr("Ctrl+N"));
    newAction->setStatusTip(tr("创建一个新的表格文件"));
	connect(newAction, SIGNAL(triggered()), this , SLOT(newFile()));
	// 其他相关 action
	for ( int i = 0; i < MaxRecentFiles; ++i)
	{
		recentFileActions[i] = new QAction( this );
		recentFileActions[i]->setVisible( false );
		connect(recentFileActions[i], SIGNAL(triggered()), this , SLOT(openRecentFile()));
	}
	
    selectAllAction = new QAction(tr("&全选"), this );
	selectAllAction->setShortcut(tr("Ctrl+A"));
    selectAllAction->setStatusTip(tr("选择全部数据" "spreadsheet"));
	//connect(selectAllAction, SIGNAL(triggered()), spreadsheet, SLOT(selectAll())) ;
	
    showGridAction = new QAction(tr("&显示网格"), this );
	showGridAction->setCheckable( true );
	//showGridAction->setChecked(spreadsheet->showGrid());
    showGridAction->setStatusTip(tr("显示或隐藏网格"));
	//connect(showGridAction, SIGNAL(toggled( bool )),spreadsheet, SLOT(setShowGrid( bool )));
	
    aboutQtAction = new QAction(tr("关于&Qt"), this );
    aboutQtAction->setStatusTip(tr("显示Qt库的关于框"));
	connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
	
    saveAction = new QAction(tr("&保存"), this );
	connect(saveAction, SIGNAL(triggered()), this, SLOT(save())) ;
	
    saveAsAction = new QAction(tr("另存为"), this );
	connect(saveAsAction, SIGNAL(triggered()), this, SLOT(saveAs())) ;
	
    openAction = new QAction(tr("&打开"), this);
	connect(openAction, SIGNAL(triggered()), this, SLOT(open()));
	
    cutAction = new QAction(tr("剪切"), this );
	//connect(cutAction, SIGNAL(triggered()), this, SLOT(cut())) ;
	
    copyAction = new QAction(tr("&复制"), this );
	//connect(copyAction, SIGNAL(triggered()), this, SLOT(copy())) ;
	
    pasteAction = new QAction(tr("&粘贴"), this );
	//connect(pasteAction, SIGNAL(triggered()), this, SLOT(paste())) ;
	
    deleteAction = new QAction(tr("删除"), this );
	//connect(deleteAction, SIGNAL(triggered()), this, SLOT(del())) ;
	
    findAction = new QAction(tr("&查找"), this );
	connect(findAction, SIGNAL(triggered()), this, SLOT(find())) ;
	
    goToCellAction = new QAction(tr("&跳至格(cell)"), this );
    connect(goToCellAction, SIGNAL(triggered()), this, SLOT(goToCell()));

    exitAction = new QAction(tr("&退出"), this );
	connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
	
    aboutAction = new QAction(tr("&关于Spreadsheet"), this );
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
	
    selectRowAction = new QAction(tr("选择行"), this );
    selectColumnAction = new QAction(tr("选择列"), this );
    recalculateAction = new QAction(tr("重新计算"), this);
    sortAction = new QAction(tr("排序"), this);
	connect(sortAction, SIGNAL(triggered()), this, SLOT(sort()));
	
	autoRecalcAction = new QAction(tr("Auto RecalcAction"), this);
}


void MainWindow::createMenus()
{
    fileMenu = menuBar() -> addMenu(tr("&文件"));
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
    editMenu = menuBar() -> addMenu(tr("&编辑"));
	editMenu -> addAction(cutAction);
	editMenu -> addAction(copyAction);
	editMenu -> addAction(pasteAction);
	editMenu -> addAction(deleteAction);
	
    selectSubMenu = editMenu -> addMenu(tr("&选择"));
	selectSubMenu -> addAction(selectRowAction);
	selectSubMenu -> addAction(selectColumnAction);
	selectSubMenu -> addAction(selectAllAction);
	
	editMenu -> addSeparator();
	
	editMenu -> addAction(findAction);
	editMenu -> addAction(goToCellAction);
	
    toolsMenu = menuBar() -> addMenu(tr("&工具"));
	toolsMenu -> addAction(recalculateAction);
	toolsMenu -> addAction(sortAction);
	
    optionsMenu = menuBar() -> addMenu(tr("&选项"));
	optionsMenu -> addAction(showGridAction);
	optionsMenu -> addAction(autoRecalcAction);
	menuBar() -> addSeparator();
	
    helpMenu = menuBar() -> addMenu(tr("&帮助"));
	helpMenu -> addAction(aboutAction);
	helpMenu -> addAction(aboutQtAction);
}

void MainWindow::createContextMenu()
{
	std::cout << "create context menu\n";
	//spreadsheet -> addAction(cutAction);
	//spreadsheet -> addAction(copyAction);
	//spreadsheet -> addAction(pasteAction);
	//spreadsheet -> setContextMenuPolicy(Qt::ActionsContextMenu);
}

void MainWindow::createToolBars()
{
    fileToolBar = addToolBar(tr("&文件"));
	fileToolBar -> addAction(newAction);
	fileToolBar -> addAction(openAction);	
	fileToolBar -> addAction(saveAction);
	
    editToolBar = addToolBar(tr("&编辑"));
	editToolBar -> addAction(cutAction);
	editToolBar -> addAction(copyAction);
	editToolBar -> addAction(pasteAction);
	
	editToolBar -> addSeparator();
	
	editToolBar -> addAction(findAction);
	editToolBar -> addAction(goToCellAction);
}

void MainWindow::createStatusBar()
{
	locationLabel = new QLabel(" W999 ");
	locationLabel -> setAlignment(Qt::AlignHCenter);
	locationLabel -> setMinimumSize(locationLabel -> sizeHint());
	formulaLabel = new QLabel;
	formulaLabel -> setIndent(3);
	statusBar() -> addWidget(locationLabel);
	statusBar() -> addWidget(formulaLabel, 1);
	//connect(spreadsheet, SIGNAL(currentCellChanged( int , int , int , int )), this , SLOT(updateStatusBar()));
	//connect(spreadsheet, SIGNAL(modified()), this , SLOT(spreadsheetModified()));
	updateStatusBar();
}


void MainWindow::updateStatusBar()
{
	std::cout << "update status bar\n";
	//locationLabel->setText(spreadsheet->currentLocation());
	//formulaLabel->setText(spreadsheet->currentFormula());
}


void MainWindow::spreadsheetModified()
{
	setWindowModified( true );
	updateStatusBar();
}

void MainWindow::newFile()
{
	if (okToContinue()) 
	{
		//spreadsheet -> clear();
		setCurrentFile("");
	}
	std::cout << "create a new window\n";

}

bool MainWindow::okToContinue()
{
	if (isWindowModified()) 
	{
		int r = QMessageBox::warning( this , tr("Spreadsheet")
                        , tr("文件已经被修改。""你想保存修改么？"),
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
                    tr("打开"), ".",
                    tr("Spreadsheet文件(*.sp)"));
		if (!fileName.isEmpty())
		{
			loadFile(fileName);
		}
	}
}


bool MainWindow::loadFile( const QString &fileName)
{
	std::cout << "load file\n";
	//if (!spreadsheet -> readFile(fileName)) 
	//{
		//statusBar() -> showMessage(tr("Loading canceled"), 2000);
		//return false ;
	//}
	//setCurrentFile(fileName);
	//statusBar() -> showMessage(tr("File loaded"), 2000);
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

bool MainWindow::saveFile( const QString &fileName)
{
	std::cout << "save file\n";
	
	//if (!spreadsheet->writeFile(fileName)) 
	//{
	//	statusBar()->showMessage(tr("Saving canceled"), 2000);
	//	return false ;
	//}
	//setCurrentFile(fileName);
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


void MainWindow::setCurrentFile( const QString &fileName)
{
	curFile = fileName;
	setWindowModified( false );
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
	if (!findDialog) {
		findDialog = new FindDialog( this );
        //connect(findDialog, SIGNAL (findNext( const QString &, Qt::CaseSensitivity)),
        //        spreadsheet, SLOT (findNext( const QString &, Qt::CaseSensitivity)));
        //connect(findDialog, SIGNAL(findPrevious( const QString &,Qt::CaseSensitivity)),
        //        spreadsheet, SLOT(findPrevious( const QString &,Qt::CaseSensitivity)));
	}
	findDialog -> show();
	findDialog -> activateWindow();
}

void MainWindow::goToCell()
{
    GoToCellDialog *goToCellDialog = new GoToCellDialog(this);



}

void MainWindow::sort()
{
	SortDialog *dialog = new SortDialog(this);
	dialog -> setSpreadsheet(spreadsheet);
	dialog -> exec();
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



