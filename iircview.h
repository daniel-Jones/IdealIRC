#ifndef IIRCVIEW_H
#define IIRCVIEW_H

#include <QWidget>
#include <QPaintEvent>
#include <QVector>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QScrollBar>

#include "config.h"
#include "constants.h"

typedef struct T_TEXT {
    int type; // what type? (what main color)
    quint64 ts; // timestamp of when the text was written
    QString sender; // who or what sent the text
    QString text; // actual text
    bool reset; // Used when printing texts, reset ctrl codes, links, etc.
} text_t;

typedef struct PRINTLINE_T {
    int type; // what type? (what main color)
    quint64 ts; // timestamp of when the text was written
    QString sender; // who or what sent the text
    QVector<QString> lines;
} printLine_t;

class IIRCView : public QWidget
{
    Q_OBJECT
public:
    explicit IIRCView(config *cfg, QWidget *parent = 0);
    void addLine(QString sender, QString text, int type = PT_NORMAL);
    int getSplitterPos() { return splitterPos; }
    void changeFont(QString fontName, int pxSize);

private:
    QColor getColorFromCode(int num);
    QColor invertColor(QColor c);
    QColor getColorFromType(int type); // see constants.h for PT_*
    config *conf;
    QVector<text_t> lines;
    quint64 lastUpdate;
    int fontSize;
    int splitterPos;
    bool resizingSplitter;
    QColor bgColor;
    QColor sColor;
    QFontMetrics *fm;
    QTimer cooldown; // When update is ran many times in a row, we want a cooldown to prevent the display to lag.
    QScrollBar scrollbar;
    QVector<printLine_t> visibleLines;

protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *e);
};

#endif // IIRCVIEW_H
