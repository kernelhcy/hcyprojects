#ifndef SS_MDIAREA_H
#define SS_MDIAREA_H
#include <QMdiArea>
#include <QWidget>
#include <QTabBar>

class SS_TabMdiArea : public QMdiArea
{
    Q_OBJECT
public:
    SS_TabMdiArea(QWidget *parent):QMdiArea(parent)
    {
        tabBar = new QTabBar;
        tabBar ->setShape(QTabBar::RoundedSouth);
        //setViewMode(QMdiArea::TabbedView);
        //setDocumentMode(true);
        //show the tabbar in the status bar

    }

    ~SS_TabMdiArea()
    {
        delete tabBar;
    }

    QMdiSubWindow* newWindow(QWidget *widget, Qt::WindowFlags flags=0);

    QTabBar *getTabBar()
    {
        return tabBar;
    }
private:
    QTabBar *tabBar;
};

#endif // SS_MDIAREA_H
