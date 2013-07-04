#ifndef GRAPH_H
#define GRAPH_H

#include <QBrush>
#include <QPen>
#include <QRect>
#include <QWidget>

#include <vector>

class graph : public QWidget
{
    Q_OBJECT
public:
    explicit graph(QWidget *parent = 0);

    QSize minimumSizeHint() const;
    QSize sizeHint() const;

public slots:
    void setPen(const QPen &pen);
    void setBrush(const QBrush &brush);
    void setAntialiased(bool antialiased);

    void setPlotRange(const QRect& rect);
    void setPlotRange();

    void drawAxes();
    void drawData();

protected:
    void paintEvent(QPaintEvent *event);

private:
    QPen pen;
    QBrush brush;
    bool antialiased;

    QRect plotrange;

    std::vector<float>* data;
};

#endif // GRAPH_H
