#ifndef PLOTTER_H
#define PLOTTER_H
#include <QMap>
#include <QPixmap>
#include <QVector>
#include <QWidget>
class QToolButton;

class PlotSettings;

/*
 * The Plotter widget displays one or more curves specified as vectors of coordinates.
 * The user can draw a rubber band on the image, and the Plotter will zoom in on the area
 * enclosed by the rubber band. The user draws the rubber band by clicking a point on the graph,
 * dragging the mouse to another position with the left mouse button held down, and releasing the
 * mouse button.
 */

class Plotter : public QWidget
{
Q_OBJECT
public:
    Plotter(QWidget *parent = 0);
    void setPlotSettings(const PlotSettings &settings);
    void setCurveData(int id, const QVector<QPointF> &data);
    void clearCurve(int id);
    QSize minimumSizeHint() const;
    QSize sizeHint() const;

public slots:
    void zoomIn();
    void zoomOut();

protected:
    void paintEvent(QPaintEvent *event);
    void resizeEvent(QResizeEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void keyPressEvent(QKeyEvent *event);
    void wheelEvent(QWheelEvent *event);

private:
    void updateRubberBandRegion();
    void refreshPixmap();
    void drawGrid(QPainter *painter);
    void drawCurves(QPainter *painter);

    //used to provide some spacing around the graph
    enum { Margin = 50 };

    QToolButton *zoomInButton;
    QToolButton *zoomOutButton;

    //store a curve's points as a QVector<QPointF>
    QMap<int, QVector<QPointF> > curveMap;
    QVector<PlotSettings> zoomStack;
    int curZoom;

    //
    bool rubberBandIsShown;
    QRect rubberBandRect;

    /*
     * Holds a copy of the whole widget's rendering
     * identical to what is shown on screen
     * The plot is always drawn onto this off-screen pixmap first;
     * then the pixmap is copied onto the widget.
     */
    QPixmap pixmap;

};

/*
 * The PlotSettings class specifies the range of the x and y axes(坐标轴)
 * and the number of ticks for these axes.
 * 用于设置坐标轴的大小和精度。
 */
class PlotSettings
{
public:
    PlotSettings();

    void scroll(int dx, int dy);
    void adjust();

    double spanX() const
    {
        return maxX - minX;
    }
    double spanY() const
    {
        return maxY - minY;
    }

    double minX;
    double maxX;
    int numXTicks;

    double minY;
    double maxY;
    int numYTicks;

private:
    static void adjustAxis(double &min, double &max, int &numTicks);
};

#endif // PLOTTER_H
