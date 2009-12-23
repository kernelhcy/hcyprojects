#ifndef MAINWINDOW_H
#define MAIhNWINDOW_H
#include <QMainWindow>
#include <QtGui>
#include "heads.h"
#include "finddialog.h"
#include "gotocelldialog.h"
#include "sortdialog.h"
#include "spreadsheet.h"

class QAction;
class QLabel;
class FindDialog;
class Spreadsheet;
class GoToCellDialog;

class MainWindow : public QMainWindow
{
	Q_OBJECT
	public:
		MainWindow();
	protected:
		void closeEvent(QCloseEvent *event);
	private slots:
		void newFile();
		void open();
		bool save();
		bool saveAs();
		void find();
		void goToCell();
		void sort();
		void about();
		void openRecentFile();
		void updateStatusBar();
		void spreadsheetModified();
	private:
		void createActions();
		void createMenus();
		void createContextMenu();
		void createToolBars();
		void createStatusBar();
		void readSettings();
		void writeSettings();
		bool okToContinue();
		bool loadFile(const QString &fileName);
		bool saveFile(const QString &fileName);
		void setCurrentFile(const QString &fileName);
		void updateRecentFileActions();
		QString strippedName(const QString &fullFileName);
		Spreadsheet *spreadsheet;
		FindDialog *findDialog;

		QLabel *locationLabel;
		QLabel *formulaLabel;
		QStringList recentFiles;
		QString curFile;
		enum { MaxRecentFiles = 5 };
		QAction *recentFileActions[MaxRecentFiles];
		QAction *separatorAction;
		QMenu *fileMenu;
		QMenu *editMenu;
		QMenu *toolsMenu;
		QMenu *optionsMenu;
		QMenu *helpMenu;
		QMenu *selectSubMenu;

		QToolBar *fileToolBar;
		QToolBar *editToolBar;
        QToolBar *actionToolBar;

		QAction *newAction;
		QAction *openAction;
		QAction *aboutQtAction;
		
		QAction *selectAllAction;
		QAction *showGridAction;
		QAction *saveAction;
		QAction *saveAsAction;
		QAction *cutAction;
		QAction *copyAction;
		QAction *pasteAction;
		QAction *deleteAction;
		QAction *findAction;
		QAction *goToCellAction;
		QAction *exitAction;
		QAction *aboutAction;
		QAction *selectRowAction;
		QAction *selectColumnAction;
		QAction *recalculateAction;
		QAction *sortAction;
		QAction *autoRecalcAction;
};

#endif
