#ifndef MAINWINDOW_H
#define MAIhNWINDOW_H
#include <QMainWindow>
#include <QAction>
#include <QLabel>
#include <QMdiArea>
#include <QTabWidget>
#include <QMap>

#include "heads.h"
#include "finddialog.h"
#include "gotocelldialog.h"
#include "sortdialog.h"
#include "spreadsheet.h"
#include "ss_tab_mdiarea.h"
#include "document.h"
#include "word.h"

/*
 * 主窗体
 * 有菜单拦，工具栏和状态拦。直接继承自QMainWindow。
 * 通过QTabWidget控制显示的内容。
 *
 */

class MainWindow : public QMainWindow
{
	Q_OBJECT
	public:
		MainWindow();
        ~MainWindow();

	protected:
        //重写关闭事件动作。
        //监控是否修改的文档没有保存，做出相应的提示。
		void closeEvent(QCloseEvent *event);

    private slots:
        //各种动作。
		void newFile();
		void open();
		bool save();
		bool saveAs();
		void find();
		void goToCell();
		void sort();
		void about();
		void openRecentFile();

        //封装文档的slot
        void cut();
        void copy();
        void del();
        void paste();

		void updateStatusBar();
		void spreadsheetModified();

        //关闭tab
        void closeTab(int index);
        //当前tab改变。修改标题和窗口修改状态。
        void currentTabChanged(int index);
        //创建新的tab
        int newTab(QWidget *w = 0, const QString &title = "Untitled");
	private:
        //创建Actions，也就是菜单和工具栏的按钮。
		void createActions();
        //创建菜单
		void createMenus();
        //创建pop菜单，右击显示。
        void createContextMenu(QWidget *w);
		void createToolBars();

        void createStatusBar();

        //读写主窗口的设置
        void readSettings();
		void writeSettings();

        //检测文档是否修改且未保存。
        bool okToContinue();

        //读写文档，参数是文件名
		bool loadFile(const QString &fileName);
		bool saveFile(const QString &fileName);

        void setCurrentFile(const QString &fileName);
		void updateRecentFileActions();

        //删除文件名中的路径名。
		QString strippedName(const QString &fullFileName);

        /*
         *************
         * 私有成员变量
         *************
         */
        //状态栏中的组件，用于显示当前位置和公式。
        QLabel *locationLabel;
		QLabel *formulaLabel;

        //有关最近带开的文件的变量。
        QStringList recentFiles;        //最近打开的文件的列表
        QString curFile;                //当前打开的文件名
        enum { MaxRecentFiles = 5 };    //菜单中显示的最多最近打开文件数
        QAction *recentFileActions[MaxRecentFiles]; //最近打开文件的按钮
        QAction *separatorAction;       //分割符

        //菜单
        QMenu *fileMenu;
		QMenu *editMenu;
		QMenu *toolsMenu;
		QMenu *optionsMenu;
		QMenu *helpMenu;
		QMenu *selectSubMenu;

        //工具栏
		QToolBar *fileToolBar;
		QToolBar *editToolBar;
        QToolBar *actionToolBar;

        //各种菜单和工具栏按钮
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

        //中央组件，用于控制和显示文档。
        QTabWidget *tabWidget;

        //所有文档
        //用一个整型值标记文档，这个值通常是tabWidget的addTab返回的值。
        QMap<int, Document*> documents;

        FindDialog *findDialog;
    };

#endif
