#include "ss_mdiarea.h"
#include <QMdiSubWindow>

QMdiSubWindow* SS_MdiArea::newWindow(QWidget *widget, Qt::WindowFlags flags)
{
    QMdiSubWindow *subWin = addSubWindow(widget, flags);
    subWin -> setAttribute(Qt::WA_DeleteOnClose);

    tabBar -> addTab("test");
    tabBar -> updateGeometry();
    tabBar -> update();

}
