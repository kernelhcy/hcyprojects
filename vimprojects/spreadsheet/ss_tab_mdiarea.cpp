#include "ss_tab_mdiarea.h"
#include <QMdiSubWindow>

QMdiSubWindow* SS_TabMdiArea::newWindow(QWidget *widget, Qt::WindowFlags flags)
{
    QMdiSubWindow *subWin = addSubWindow(widget, flags);
    subWin -> setAttribute(Qt::WA_DeleteOnClose);

    subWin ->setMinimumSize(subWin->sizeHint());

    tabBar -> addTab("test");
    tabBar -> updateGeometry();
    tabBar -> update();

    return subWin;
}
