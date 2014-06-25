#include <QPainter>
#include <QDateTime>
#include <QDebug>
#include <QVectorIterator>
#include <QFile>
#include <QFontMetrics>
#include <QDateTime>
#include <QVectorIterator>
#include <QDesktopServices>

#include "iircview.h"
#include "constants.h"

IIRCView::IIRCView(config *cfg, QWidget *parent) :
    QWidget(parent),
    conf(cfg),
    lastUpdate(0),
    splitterPos(150),
    resizingSplitter(false),
    scrollbar(this),
    draggingText(false)
{
    fm = new QFontMetrics(font());
    fontSize = fm->height();

    setMouseTracking(true);
    scrollbar.setMaximum(0);
    connect(&scrollbar, SIGNAL(valueChanged(int)),
            this, SLOT(update()));

    scrollbar.setInvertedAppearance(true);
    scrollbar.setInvertedControls(false);

    cooldown.setSingleShot(true);
}
void IIRCView::changeFont(QString fontName, int pxSize)
{
    QFont f(fontName);
    f.setPixelSize(pxSize);
    setFont(f);
    delete fm;
    fm = new QFontMetrics(font());
    fontSize = fm->height();
    update();
}

QColor IIRCView::getColorFromCode(int num)
{
      switch(num) {
        case 0:
            return C_WHITE;

        case 1:
              return C_BLACK;

        case 2:
            return C_BLUE;

        case 3:
            return C_GREEN;

        case 4:
            return C_BRIGHTRED;

        case 5:
            return C_RED;

        case 6:
            return C_MAGENTA;

        case 7:
            return C_BROWN;

        case 8:
            return C_YELLOW;

        case 9:
            return C_BRIGHTGREEN;

        case 10:
            return C_CYAN;

        case 11:
            return C_BRIGHTCYAN;

        case 12:
            return C_BRIGHTBLUE;

        case 13:
            return C_BRIGHTMAGENTA;

        case 14:
            return C_DARKGRAY;

        case 15:
            return C_LIGHTGRAY;

        default:
            return C_BLACK;
      }
}

QColor IIRCView::invertColor(QColor c)
{
    // Bitwise XOR to invert colors.
    int R = c.red() ^ 255;
    int G = c.green() ^ 255;
    int B = c.blue() ^ 255;

    QColor rs;
    rs.setRed(R);
    rs.setGreen(G);
    rs.setBlue(B);
    return rs;
}

QColor IIRCView::getColorFromType(int type)
{
    switch (type) {
        case PT_NORMAL:
            return conf->colDefault;

        case PT_LOCALINFO:
            return conf->colLocalInfo;

        case PT_SERVINFO:
            return conf->colServerInfo;

        case PT_NOTICE:
            return conf->colNotice;

        case PT_ACTION:
            return conf->colAction;

        case PT_CTCP:
            return conf->colCTCP;

        case PT_OWNTEXT:
            return conf->colOwntext;

        case PT_HIGHLIGHT:
            return conf->colHighlight;

        default:
            return conf->colDefault;
    }
}

void IIRCView::addLine(QString sender, QString text, int type)
{
    t_text t;
    t.type = type;
    t.ts = QDateTime::currentMSecsSinceEpoch();
    t.sender = sender;
    t.text = text;
    t.reset = true; // set to false when re-making the textwriter.

    lines.append(t);

    if (lines.count() > 1000) // should be configurable
        lines.pop_front();

    scrollbar.setMaximum(lines.count());

    update();
}

QString IIRCView::getLink(int x, int y)
{
    QVectorIterator<t_Anchor> ia(anchors);
    while (ia.hasNext()) {
        t_Anchor anchor = ia.next();

        if ((x >= anchor.P1.x()) && (x <= anchor.P2.x()) &&
            (y >= anchor.P1.y()) && (y <= anchor.P2.y())) {
            return anchor.url;
        }

    }
    return QString();
}

void IIRCView::setAnchorUrl(QVector<t_Anchor> *lstPtr, QString url)
{
    for (int i = 0; i <= lstPtr->count()-1; ++i) {
        t_Anchor a = lstPtr->at(i);
        a.url = url;
        lstPtr->replace(i, a);
    }
}

void IIRCView::paintEvent(QPaintEvent *)
{
    // Scrollbar positioning
    scrollbar.setGeometry(width()-scrollbar.width(), 0, scrollbar.width(), height());

    // Cooldown checking
    if (cooldown.isActive())
        return; // we're cooling down...

    quint64 current = QDateTime::currentMSecsSinceEpoch();
    if (current-lastUpdate < 33)
        cooldown.singleShot(33, this, SLOT(update()));

    lastUpdate = current;


    QPainter painter(this);

    // Background
    painter.fillRect(0, 0, width(), height(), conf->colBackground);

    //painter.setPen( Qt::red );
    //painter.drawLine(textCpyVect);

    // Splitter
    painter.setPen( invertColor(conf->colBackground) );
    painter.drawLine(splitterPos, -20, splitterPos, height());

    int maxWidth = width()-splitterPos-scrollbar.width()-45;

    anchors.clear();
    // Text items
    // Add items to a vector
    QVector<t_printLine> print;
    int Y = height() - fontSize;
    for (int i = lines.count()-scrollbar.value()-1; (i >= 0 && Y > -fontSize-1); --i) {

        Y -= fontSize;
        t_text t = lines[i];

        // Check if splitter needs resize.
        QString ts = QDateTime::fromMSecsSinceEpoch(t.ts).toString("[hh:mm]");
        if (fm->width(t.sender) > splitterPos-fm->width(ts)) {
            splitterPos = fm->width(t.sender) + fm->width(ts) + 10;
            update();
        }

        // Text line is not wider than widget width.
        if (fm->width(t.text) < maxWidth) {
            t.reset = true;
            t_printLine pl;
            pl.type = t.type;
            pl.ts = t.ts;
            pl.sender = t.sender;
            pl.lines << t.text;
            print << pl;
            continue;
        }

        // Reaching here means the line is too wide for the display, chop it.
        QString addLine;
        QString line = t.text;
        t.text.clear();
        QVector<QString> list;
        int lastSpace = -1;
        int wrapOffset = 0;

        for (int ix = 0; ix <= line.length()-1; ++ix) {
            QChar c = line[ix];
            addLine += c;

            if (c == ' ')
                lastSpace = ix;

            // No spaces were encountered, split line right here.
            if ((fm->width(addLine) >= maxWidth) && (lastSpace == -1)) {
                list << addLine;
                addLine.clear();
            }

            // Split line at previous space
            if ((fm->width(addLine) >= maxWidth) && (lastSpace > -1)) {
                list << line.mid(wrapOffset, (lastSpace-wrapOffset));

                addLine.clear();
                ix = lastSpace;
                lastSpace = -1;
                wrapOffset = ix;
            }
        }

        // Check if we got some remaining text to add...
        if (! addLine.isEmpty())
            list << addLine;

        if (list.count() >= 2) {
            QString s = list.last();
            list.pop_back();

            list << s;
        }

        t_printLine pl;
        pl.type = t.type;
        pl.ts = t.ts;
        pl.sender = t.sender;
        pl.lines << list;

        print << pl;
    } // for (int i = lines.count()-scrollbar.value()-1; (i >= 0 && Y > -fontSize-1); --i)

    // store our array of lines to print.
    visibleLines = print;


    // Draw items from vector
    Y = height() + (fontSize/2); // start at 1/2 font-height from bottom
    bool bold = false;
    bool underline = false;
    bool color = false; // background color on/off
    QColor bgColor;

    QVectorIterator<t_printLine> i(print);
    while (i.hasNext()) {
        t_printLine pl = i.next();

        Y -= fontSize * pl.lines.count();

        if (Y < -fontSize)
            break; // No need to draw outside. finished

        // Set defaults.
        bold = false;
        underline = false;
        color = false;
        QFont font = painter.font();
        font.setBold(false);
        font.setUnderline(false);
        painter.setPen( getColorFromType(pl.type) );
        painter.setFont(font);
        QColor textcolor = getColorFromType(pl.type);


       // painter.setFont(this->font()); // remove any bolds if any...
     //   painter.setPen( getColorFromType(pl.type) );

        // Draw timestamp
        QString ts = QDateTime::fromMSecsSinceEpoch(pl.ts).toString("[hh:mm]");
        painter.drawText(QPoint(0, Y), ts);

        // Draw sender
        int sendLenPx = fm->width(pl.sender)+5; // Senders name length in px
        painter.drawText(QPoint(splitterPos-sendLenPx-5, Y), pl.sender);


        // Restore any customized bold or color
        if (bold) {
            QFont f = painter.font();
            f.setBold(true);
        }
        painter.setPen(textcolor);


        // Loop through text list and add colors and stuff.

        t_Anchor anchor;
        int printY = Y;
        QVectorIterator<QString> si(pl.lines);
        QString url;
        QVector<t_Anchor> addUrl;
        bool readURL = false;
        bool paintLink = false;
        int X, fw;
        int ln = 0; // current line number (bottom up)
        while (si.hasNext()) {
            QString line = si.next();

            if (draggingText) {

            }

            X = splitterPos+5;

            for (int ic = 0; ic <= line.length()-1; ++ic) {
                QChar c = line[ic];
                fw = fm->width(c);
                ++ln;

                if (c == CTRL_BOLD) {
                    QFont font = painter.font();
                    bold = !bold;
                    font.setBold(bold);
                    painter.setFont(font);
                    continue;
                }
                if (c == CTRL_UNDERLINE) {
                    underline = !underline;
                    continue;
                }
                if (c == CTRL_COLOR) {
                    if (ic == line.length()-1)
                        continue;

                    // get colors (or reset)
                    c = line[ic+1];
                    if ((! c.isDigit()) && (c != ',')) { // reset
                        color = false;
                        painter.setPen( getColorFromType(pl.type) ); // TODO: config color
                        continue;
                    }

                    QString s;
                    bool bg = false;
                    for (++ic; ic <= line.length()-1; ++ic) {
                        c = line[ic];
                        if ((c == ',') && (bg)) {
                            // we're already on background, this is "invalid", break out.
                            --ic;
                            break;
                        }

                        if (c == ',') {
                            // Get background
                            bg = true;
                            if (s.isEmpty()) // no fg was defined, reset.
                                painter.setPen( getColorFromType(pl.type) ); // TODO: config color
                            else
                                painter.setPen( getColorFromCode(s.toInt()) );
                            s.clear();

                            continue;
                        }

                        if (! c.isDigit()) { // Coloring is done
                            if (bg) {
                                // bg color
                                if (s.isEmpty()) {
                                    // no background is actually set, break out.
                                    --ic;
                                    break;
                                }
                                bgColor = getColorFromCode(s.toInt());
                                color = true;
                                s.clear();
                            }
                            else {
                                painter.setPen( getColorFromCode(s.toInt()) );
                                s.clear();
                            }

                            break;
                        }

                        s += c;

                    } // (++ic; ic <= line.length()-1; ++ic)

                } // (c == CTRL_COLOR)

                if (c == CTRL_RESET) {
                    bold = false;
                    underline = false;
                    color = false;
                    QFont font = painter.font();
                    font.setBold(false);
                    font.setUnderline(false);
                    painter.setPen(getColorFromType(pl.type)); // TODO use config
                    painter.setFont(font);
                }            

                if (color)
                    painter.fillRect(X, printY-fontSize, fw+1, fontSize+2, bgColor);
                painter.drawText(QPoint(X, printY), c);

                if (underline)
                    painter.drawLine(X, printY+2, X+fw, printY+2);

                if ((ic == 0) && (!readURL))
                    url.clear();

                if (c == ' ') {
                    if (readURL) {
                        // Create url
                        qDebug() << "found url:" << url;
                        readURL = false;
                        paintLink = false;
                        painter.setPen( getColorFromType(pl.type) );

                        anchor.P2 = QPoint(X, printY);
                        addUrl << anchor;
                        qDebug() << "set 1";
                        setAnchorUrl(&addUrl, url);
                        anchors << addUrl;
                    }

                    url.clear();
                }
                if (c != ' ')
                    url += c;

                if ((ic == line.length()-1) && (readURL)) {
                    // End of line, add anchor coords and expect to begin on next line
                    // with same url...
                    anchor.P2 = QPoint(X+fw, printY);
                    addUrl << anchor;

                    if (! si.hasNext()) {
                        // this is also last line to send out if this printline.
                        qDebug() << "set 2";
                        readURL = false;
                        paintLink = false;

                        anchor.P2 = QPoint(X+fw, printY);

                        addUrl << anchor;
                        setAnchorUrl(&addUrl, url);
                        anchors << addUrl;

                        painter.setPen( getColorFromType(pl.type) );
                    }
                }

                if ((ic == 0) && (readURL)) // URL continues... prepare it.
                    anchor.P1 = QPoint(X, printY-fontSize+3);

                X += fw;

                if (((url.toUpper() == "HTTP://") || (url.toUpper() == "HTTPS://")) && (! readURL)) { // TODO: needs regex for HTTP(S)/FTP/IRC
                    readURL = true;
                    paintLink = true;
                    addUrl.clear();
                    // find X to beginning of 'url'
                    int ulen = fm->width(url);
                    X -= ulen;
                    // (!!) Y could be affected if there is a line shift inside 'url'

                    // Erase with background color
                    // (!!) Background images is not possible before we add some way to refresh background image...
                    painter.fillRect(X, printY-fm->height()+4, ulen, fm->height(), conf->colBackground);

                    // Set pen color to links
                    painter.setPen(conf->colLinks);

                    // Store xy for anchor point bounding box
                    anchor.P1 = QPoint(X, printY-fm->height()+4);
                    painter.drawText(X, printY, url);

                    // Set back X, new letter adds and we get proper X set back on next round
                    X += ulen;
                }



            } // for (int ic = 0; ic <= line.length()-1; ++ic)

            printY += fontSize;

        } // while (si.hasNext())


    } // while (i.hasNext())



}

void IIRCView::mouseMoveEvent(QMouseEvent *e)
{
    int x = e->pos().x();

    Qt::CursorShape newCursor = Qt::ArrowCursor;
    Qt::CursorShape currentCursor = cursor().shape();

    if (! getLink(e->pos().x(), e->pos().y()).isEmpty())
        newCursor = Qt::PointingHandCursor;

    if ((x >= (splitterPos-3)) && (x <= (splitterPos+3)))
        newCursor = Qt::SplitHCursor;

    if (currentCursor != newCursor)
        setCursor(newCursor);

    if (draggingText) {
        textCpyVect.setP2( QPoint(e->pos().x(), e->pos().y()) );
        return;
    }

    if (resizingSplitter) {
        if ((x <= 50) || (x >= width()-50))
            return;


        splitterPos = x;
        update();
        return;
    }

}

void IIRCView::mousePressEvent(QMouseEvent *e)
{
    int x = e->pos().x();
    if ((x >= (splitterPos-3)) && (x <= (splitterPos+3)))
        resizingSplitter = true;

    if (x > splitterPos+5) {
        // probable text copy
        textCpyVect.setP1( QPoint(e->pos().x(), e->pos().y()) );
        draggingText = true;
    }
}

void IIRCView::mouseReleaseEvent(QMouseEvent *e)
{
    if (draggingText) {
        // copy text if any.
        textCpyVect.setP2( QPoint(e->pos().x(), e->pos().y()) );
        qDebug() << "Copy:" << textCpyVect;
        update();

    }
    draggingText = false;
    resizingSplitter = false;

    // check for anchor clicking, avoid opening URLs if we're copying text.
    /*if (draggingText == false) {

    }
    */

    QString link = getLink(e->pos().x(), e->pos().y());
    if (! link.isEmpty())
        QDesktopServices::openUrl( QUrl(link) );

}

void IIRCView::wheelEvent(QWheelEvent *e)
{
    if (e->delta() > 0)
        scrollbar.setValue( scrollbar.value()+1 );
    else
        scrollbar.setValue( scrollbar.value()-1 );
}

