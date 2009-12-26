#ifndef SS_MDIAREA_H
#define SS_MDIAREA_H
#include <QMdiArea>
#include <QWidget>
#include <QTabBar>

class SS_MdiArea : public QMdiArea
{
public:
    SS_MdiArea(QWidget *parent):QMdiArea(parent)
    {
        tabBar = new QTabBar;
        tabBar ->setShape(QTabBar::RoundedSouth);

    }

    ~SS_MdiArea()
    {
        delete tabBar;
    }

    QMdiSubWindow* newWindow(QWidget *widget, Qt::WindowFlags flags=0);

private:
    QTabBar *tabBar;
};

#endif // SS_MDIAREA_H
