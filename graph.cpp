#include <QtGui>

#include "graph.h"

graph::graph(QWidget *parent) :
    QWidget(parent)
{
    antialiased = false;
    //transformed = false;
    //pixmap.load(":/images/qt-logo.png");

    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
}

QSize graph::minimumSizeHint() const
{
    return QSize(100, 100);
}

QSize graph::sizeHint() const
{
    return QSize(400, 200);
}

void graph::setPen(const QPen &pen)
{
    this->pen = pen;
    update();
}

void graph::setBrush(const QBrush &brush)
{
    this->brush = brush;
    update();
}

void graph::setAntialiased(bool antialiased)
{
    this->antialiased = antialiased;
    update();
}


void graph::paintEvent(QPaintEvent * /* event */)
{
    QRect rect(10, 20, 80, 60);

    QPainterPath path;
    path.moveTo(20, 80);
    path.lineTo(20, 30);
    path.cubicTo(80, 0, 50, 50, 80, 80);


    QPainter painter(this);
    painter.setPen(pen);
    painter.setBrush(brush);
    if (antialiased)
        painter.setRenderHint(QPainter::Antialiasing, true);

    for (int x = 0; x < width(); x += 100) {
        for (int y = 0; y < height(); y += 100) {
            painter.save();
            painter.translate(x, y);

            painter.drawLine(rect.bottomLeft(), rect.topRight());

            painter.restore();
        }
    }

    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setPen(palette().dark().color());
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(QRect(0, 0, width() - 1, height() - 1));
}


void setPlotRange(const QRect& rect);
void setPlotRange();

void drawAxes();
void drawData();

