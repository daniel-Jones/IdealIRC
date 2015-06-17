/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2015  Tom-Andre Barstad
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/*! \class IIRCView
 *  \brief GUI widget used in IWin, for displaying text.
 *
 * Custom not-so-efficient text GUI widget. Capable of parsing clickable links and a primitive text copy
 */

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
    int getSplitterPos() { return splitterPos; } //!< returns the position on X axis where the spliter is.
    void changeFont(QString fontName, int pxSize);
    void clear();
    void redraw();

private:
    QColor getColorFromCode(int num);
    QColor invertColor(QColor c);
    QColor getColorFromType(int type); // see constants.h for PT_*
    config *conf; //!< Pointer to the config class (iirc.ini)
    QVector<t_text> lines; //!< Contains all lines (up to 1000 lines)
    quint64 lastUpdate; //!< Used to not re-run GUI update too frequently. Limits to ca 30 updates per second.
    int fontSize; //!< Font size in pixles.
    int splitterPos; //!< Position on X axis where the splitter is.
    bool resizingSplitter; //!< true if left mouse button is down above the splitter position (+/- 3px)
    QColor bgColor; //!< Background color.
    QColor sColor;
    QFontMetrics *fm; //!< Contains font-specific size information.
    QTimer cooldown;  //!< When update is ran many times in a row, we want a cooldown to prevent the display to lag.
    QScrollBar scrollbar; //!< The scrollbar.
    QVector<t_printLine> visibleLines; //!< Lines parsed in GUI update, which is actually visible / within viewport.
    QImage *backgroundImage;  //!< Original background image, copy of the disk original.
    QImage *pBackgroundImage;  //!< Processed background image. Use this for displaying.
    QPoint bgImgPos; //!< The position of background image. Used for centering it.

    bool mouseDown; //!< true if left mouse button is pushed down.
    bool draggingText; //!< true if we're dragging text, for copying.
    QLine textCpyVect; //!< 2D vector of where mouse press started, and where mouse released, to calculate what text to copy.
    QString textToCopy; //!< Actual text to copy to clipboard.

    int getLineCount(QVector<t_printLine>* lstPtr);

    QString getLink(int x, int y);
    QVector<t_Anchor> anchors; //!< List of bounding boxes, named anchors (think html a href). Clickable links is saved here.
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
    void joinChannelMenuRequest(QPoint point, QString channel);
    void nickMenuRequest(QPoint point, QString nickname);
};

#endif // IIRCVIEW_H
