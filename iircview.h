#ifndef IIRCVIEW_H
#define IIRCVIEW_H

#include <QWidget>
#include <QPaintEvent>
#include <QVector>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTimer>
#include <QScrollBar>
#include <QPoint>
#include <QLine>
#include <QClipboard>
#include <QFocusEvent>
#include <QResizeEvent>

#include "config.h"
#include "constants.h"

typedef struct T_TEXT {
    int type; // what type? (what main color)
    quint64 ts; // timestamp of when the text was written
    QString sender; // who or what sent the text
    QString text; // actual text
    bool reset; // Used when printing texts, reset ctrl codes, links, etc.
} t_text;

typedef struct PRINTLINE_T {
    int type; // what type? (what main color)
    quint64 ts; // timestamp of when the text was written
    QString sender; // who or what sent the text
    QVector<QString> lines;
} t_printLine;

typedef struct ANCHOR_T {
    QPoint P1;
    QPoint P2;
    QString url;
} t_Anchor;

class IIRCView : public QWidget
{
    Q_OBJECT
public:
    explicit IIRCView(config *cfg, QWidget *parent = 0);
    ~IIRCView();
    void addLine(QString sender, QString text, int type = PT_NORMAL);
    int getSplitterPos() { return splitterPos; }
    void changeFont(QString fontName, int pxSize);
    void clear();
    void redraw();

private:
    QColor getColorFromCode(int num);
    QColor invertColor(QColor c);
    QColor getColorFromType(int type); // see constants.h for PT_*
    config *conf;
    QVector<t_text> lines;
    quint64 lastUpdate;
    int fontSize;
    int splitterPos;
    bool resizingSplitter;
    QColor bgColor;
    QColor sColor;
    QFontMetrics *fm;
    QTimer cooldown; // When update is ran many times in a row, we want a cooldown to prevent the display to lag.
    QScrollBar scrollbar;
    QVector<t_printLine> visibleLines;
    QImage *backgroundImage; // Original image
    QImage *pBackgroundImage; // Processed bg image- use this!

    bool mouseDown;
    bool draggingText;
    QLine textCpyVect;
    QString textToCopy;

    int getLineCount(QVector<t_printLine>* lstPtr);

    QString getLink(int x, int y);
    QVector<t_Anchor> anchors;
    void setAnchorUrl(QVector<t_Anchor>* lstPtr, QString url);
    void resizeBackground(QSize size);

protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void wheelEvent(QWheelEvent *e);
    void focusInEvent(QFocusEvent *e);
    void contextMenuEvent(QContextMenuEvent *e);
    void mouseDoubleClickEvent(QMouseEvent *e);
    void resizeEvent(QResizeEvent *e);

signals:
    void joinChannel(QString channel);
    void gotFocus();
    void menuRequested(QPoint point);
    void mouseDblClick();
};

#endif // IIRCVIEW_H
