#include <QPainter>
#include <QDateTime>
#include <QDebug>
#include <QVectorIterator>
#include <QFile>
#include <QFontMetrics>
#include <QDateTime>
#include <QVectorIterator>
#include <QDesktopServices>
#include <QApplication>

#include "iircview.h"
#include "constants.h"
#include "math.h"

/*!
 * \param cfg Pointer to config class (iirc.ini)
 * \param parent Parent of the text view (usually IWin)
 */
IIRCView::IIRCView(config *cfg, QWidget *parent) :
    QWidget(parent),
    conf(cfg),
    lastUpdate(0),
    splitterPos(150),
    resizingSplitter(false),
    scrollbar(this),
    backgroundImage(nullptr),
    pBackgroundImage(nullptr),
    mouseDown(false),
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

    if (conf->bgImageEnabled) {
        backgroundImage = new QImage(conf->bgImagePath);
        pBackgroundImage = new QImage(conf->bgImagePath);
    }
}

IIRCView::~IIRCView()
{
    if (backgroundImage)
        delete backgroundImage;

    if (pBackgroundImage)
        delete pBackgroundImage;
}

void IIRCView::resizeEvent(QResizeEvent *e)
{
}

/*!
 * \param size New size
 *
 * Makes a copy of backgroundImage to pBackgroundImage.\n
 * Then, pBackgroundImage resizes according to the config and new size.
 */
void IIRCView::resizeBackground(QSize size)
{
    bgImgPos.setX(0);
    bgImgPos.setY(0);

    // Clean up old version of scaled background
    if (pBackgroundImage)
        delete pBackgroundImage;

    // Make room for new copy of background
    pBackgroundImage = new QImage();

    switch (conf->bgImageScale) {
        case Bg_Scale:
            // Scale, but ignore the proportions
            *pBackgroundImage = backgroundImage->scaled(size, Qt::IgnoreAspectRatio,
                                                                  Qt::SmoothTransformation);
            // Center image...
            if (width() > pBackgroundImage->width())
                bgImgPos.setX( (width() - pBackgroundImage->width()) / 2 );
            if (height() > pBackgroundImage->height())
                bgImgPos.setY( (height() - pBackgroundImage->height()) / 2 );
            break;

        case Bg_ScaleAndCut:
            // Scale, keep proportions, but cut image where it overflows
            *pBackgroundImage = backgroundImage->scaled(size, Qt::KeepAspectRatioByExpanding,
                                                                  Qt::SmoothTransformation);
            break;

        case Bg_ScaleKeepProportions:
            // Scale and keep proportions
            *pBackgroundImage = backgroundImage->scaled(size, Qt::KeepAspectRatio,
                                                                  Qt::SmoothTransformation);
            // Center image...
            if (width() > pBackgroundImage->width())
                bgImgPos.setX( (width() - pBackgroundImage->width()) / 2 );
            if (height() > pBackgroundImage->height())
                bgImgPos.setY( (height() - pBackgroundImage->height()) / 2 );
            break;

        case Bg_Center:
            // Just make a copy of original bg img
            *pBackgroundImage = *backgroundImage;
            // Center image...
            if (width() > pBackgroundImage->width())
                bgImgPos.setX( (width() - pBackgroundImage->width()) / 2 );
            if (height() > pBackgroundImage->height())
                bgImgPos.setY( (height() - pBackgroundImage->height()) / 2 );
            break;

        case Bg_Tiled:
            break; // See below.

        default:
            *pBackgroundImage = backgroundImage->scaled(size, Qt::KeepAspectRatio,
                                                                  Qt::SmoothTransformation);
    }

    if (conf->bgImageScale == Bg_Tiled) {
        // Cannot be inside the switch because of QPainter...
        pBackgroundImage = new QImage(this->size(), QImage::Format_ARGB32);
        QPainter painter(pBackgroundImage);
        QBrush brush(*backgroundImage);
        painter.fillRect(geometry(), brush);
    }
}

/*!
 * \param fontName Name of new font
 * \param pxSize Size of font in pixles
 *
 * Sets a new font on the widget.
 */
void IIRCView::changeFont(QString fontName, int pxSize)
{
    QFont f(fontName);
    f.setPixelSize(pxSize);
    setFont(f);
    delete fm;
    fm = new QFontMetrics(f);
    fontSize = fm->height();
    update();
}

/*!
 * Clears all text in the widget.
 */
void IIRCView::clear()
{
    lines.clear();
    update();
}

/*!
 * Re-draw the widget, updating the background image first.
 */
void IIRCView::redraw()
{
    if (backgroundImage)
        delete backgroundImage; // We have a background image active, delete it...

    if (conf->bgImageEnabled)
        backgroundImage = new QImage(conf->bgImagePath); // Config have bg image, overwrite the nullptr...
    else
        backgroundImage = nullptr; // The configuration says we don't have an image (anymore), set it/keep it to nulltpr.

    // Re-draw everything!
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

/*!
 * \param c Color to invert
 *
 * Inverts a color using XOR.
 * \return Inverted color
 */
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

/*!
 * \param sender Sender of message (text in left margin)
 * \param text Text to add
 * \param type Type of text (see constants.h for PT_*)
 *
 * Adds text to the widget.
 */
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

    if (draggingText) { // Preserve selection
        QPoint p = textCpyVect.p1();
        p.setY( p.y() - fontSize );
        textCpyVect.setP1(p);
    }

    update();
}

/*!
 * \param lstPtr Pointer to list
 *
 * Counts all lines within the list.\n
 * Every t_printLine can have multiple lines, so this function counts these aswell.
 * \return Number of lines
 */
int IIRCView::getLineCount(QVector<t_printLine> *lstPtr)
{
    int total = 0;
    QVectorIterator<t_printLine> i(*lstPtr);
    while (i.hasNext()) {
        t_printLine pl = i.next();
        total += pl.lines.count();
    }

    return total;
}

/*!
 * \param x X coordinate
 * \param y Y coordinate
 *
 * Tests the coordinates to see if there's a link there.
 * \return URL, or empty on failure
 */
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

/*!
 * \param lstPtr List of anchors
 * \param url URL to set
 *
 * Sets URL on the specified anchors.\n
 * A list is required, in case a long URL splits over multiple lines, we need multiple anchors aswell.
 */
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

    if (backgroundImage != nullptr) {
        resizeBackground(size()); // construct a new 'pBackgroundImage'
        painter.setOpacity( conf->bgImageOpacity/100.f );
        painter.drawImage(bgImgPos, *pBackgroundImage); // Draw it!
        painter.setOpacity(1);
    }

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
        QString ts = QDateTime::fromMSecsSinceEpoch(t.ts).toString( conf->timestamp );
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
            print.push_front( pl );
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

        print.push_front( pl );
    } // for (int i = lines.count()-scrollbar.value()-1; (i >= 0 && Y > -fontSize-1); --i)

    // store our array of lines to print.
    visibleLines = print;


    // Draw items from vector
    //Y = height() + (fontSize/2); // start at 1/2 font-height from bottom
    Y = height() - (fontSize * getLineCount(&visibleLines));

    bool bold = false;
    bool underline = false;
    bool color = false; // background color on/off
    QColor bgColor;
    bool textCopyBg = false;

    QVectorIterator<t_printLine> i(visibleLines);
    while (i.hasNext()) {
        t_printLine pl = i.next();

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

        // Draw timestamp
        QString ts;
        if (conf->showTimestmap) {
            ts = QDateTime::fromMSecsSinceEpoch(pl.ts).toString( conf->timestamp );
            painter.drawText(QPoint(0, Y), ts);
        }

        // Draw sender
        int sendLenPx = fm->width(pl.sender)+5; // Senders name length in px
        int x1 = splitterPos-sendLenPx-5;
        int x2 = splitterPos-5;
        int y1 = Y - fontSize;

        if (((pl.type == PT_NORMAL) || (pl.type == PT_HIGHLIGHT) || (pl.type == PT_OWNTEXT)) && (! pl.sender.isEmpty())) {
            t_Anchor anchor;
            anchor.P1 = QPoint(x1, y1);
            anchor.P2 = QPoint(x2, Y);
            anchor.url = pl.sender;
            anchors << anchor;
        }

        painter.drawText(QPoint(x1, Y), pl.sender);


        // Restore any customized bold or color
        if (bold) {
            QFont f = painter.font();
            f.setBold(true);
        }
        painter.setPen(textcolor);


        // Loop through text list and add colors and stuff.

        t_Anchor anchor;
       // int printY = Y;
        QVectorIterator<QString> si(pl.lines);
        QString url;
        QVector<t_Anchor> addUrl;
        bool readURL = false;
        bool paintLink = false;
        int X, fw;
        int ln = 0; // current line number (bottom up)
        while (si.hasNext()) {
            QString line = si.next();

            X = splitterPos+9;

            // fetch all instances of matching URLs
            QList<int> urlpos;

            QRegExp ex("(?:https?|ftp|irc)://|(?:#)");
            for (int ic = 0; ; ++ic) {
                ic = line.indexOf(ex, ic);
                if (ic == -1)
                    break;
                urlpos << ic;
            }

            for (int ic = 0; ic <= line.length()-1; ++ic) {
                QChar c = line[ic];
                fw = fm->width(c);
                ++ln;

                // we're now entering an URL.
                if (urlpos.contains(ic)) {
                    readURL = true;
                    paintLink = true;
                    anchor.P1 = QPoint(X, Y-fm->height()+4);
                    painter.setPen(conf->colLinks);
                }

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
                        painter.setPen( getColorFromType(pl.type) );
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
                                painter.setPen( getColorFromType(pl.type) );
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
                    painter.setPen(getColorFromType(pl.type));
                    painter.setFont(font);
                    continue;
                }            

                if (draggingText) {
                    QPoint tl(X, Y-fontSize);
                    QPoint br(X+fw, Y);

                    QPoint p1 = textCpyVect.p1();
                    if ((p1.x() >= tl.x()) && (p1.x() < br.x()) &&
                        (p1.y() >= tl.y()) && (p1.y() < br.y())) {
                        textCopyBg = true;
                    }

                    QPoint p2 = textCpyVect.p2();
                    if ((p2.x() >= tl.x()) && (p2.x() < br.x()) &&
                        (p2.y() >= tl.y()) && (p2.y() < br.y())) {
                        textCopyBg = false;
                        painter.fillRect(X+1, Y-fontSize+2, fw+1, fontSize+1, invertColor(conf->colBackground));
                    }
                }
                if (textCopyBg)
                    painter.fillRect(X+1, Y-fontSize+2, fw+1, fontSize+1, invertColor(conf->colBackground));
                else if (color)
                    painter.fillRect(X+1, Y-fontSize+2, fw+1, fontSize+1, bgColor);


                painter.drawText(QPoint(X, Y), c);

                if (underline)
                    painter.drawLine(X, Y+2, X+fw, Y+2);

                if ((ic == 0) && (!readURL))
                    url.clear();

                if (c == ' ') {
                    if (readURL) {
                        // Create url
                        readURL = false;
                        paintLink = false;
                        painter.setPen( getColorFromType(pl.type) );

                        anchor.P2 = QPoint(X, Y);
                        addUrl << anchor;
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
                    anchor.P2 = QPoint(X+fw, Y);
                    addUrl << anchor;

                    if (! si.hasNext()) {
                        // this is also last line to send out if this printline.
                        readURL = false;
                        paintLink = false;

                        anchor.P2 = QPoint(X+fw, Y);

                        addUrl << anchor;
                        setAnchorUrl(&addUrl, url);
                        anchors << addUrl;

                        painter.setPen( getColorFromType(pl.type) );
                    }
                }

                if ((ic == 0) && (readURL)) // URL continues... prepare it.
                    anchor.P1 = QPoint(X, Y-fontSize+3);

                X += fw;

            } // for (int ic = 0; ic <= line.length()-1; ++ic)

            Y += fontSize;

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

    draggingText = mouseDown;

    if (draggingText) {
        textCpyVect.setP2( QPoint(e->pos().x(), e->pos().y()) );
        update();
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
    if (e->button() != Qt::LeftButton)
        return;

    int x = e->pos().x();
    if ((x >= (splitterPos-3)) && (x <= (splitterPos+3)))
        resizingSplitter = true;

    if (x > splitterPos+5) {
        // probable text copy
        textCpyVect.setP1( QPoint(e->pos().x(), e->pos().y()) );
        mouseDown = true;
    }
}

void IIRCView::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
        return;

    if (draggingText) {
        // copy text if any.
        textCpyVect.setP2( QPoint(e->pos().x(), e->pos().y()) );

        //int Y = height() + (fontSize/2); // start at 1/2 font-height from bottom
        int Y = height() - (fontSize * getLineCount(&visibleLines));
        QVectorIterator<t_printLine> i(visibleLines);
        QString text;
        bool done = false;
        bool getSender = false;

        while (i.hasNext()) {
            t_printLine pl = i.next();


            QVectorIterator<QString> is(pl.lines);
            bool atStart;
            while (is.hasNext()) {
                atStart = ! is.hasPrevious();

                QString line = is.next();
                int X = splitterPos+7;

                for (int ic = 0; ic <= line.count()-1; ++ic) {
                    QChar c = line[ic];
                    QPoint tl(X, Y-fontSize);
                    QPoint br(X+fm->width(c), Y);
                    QPoint p2 = textCpyVect.p2();

                    X += fm->width(c);

                    if ((p2.x() >= tl.x()) && (p2.x() < br.x()) &&
                        (p2.y() >= tl.y()) && (p2.y() < br.y())) {
                        text += c;
                        done = true;
                        break;
                    }

                    if (! text.isEmpty()) {
                        if ((ic == 0) && (atStart)) {
                            getSender = true;
                            QString sender = pl.sender;
                            if ((! sender.isEmpty()) && (sender != "***"))
                                sender = QString("<%1>").arg(sender);
                            text += QString("%1 %2 ")
                                    .arg( QDateTime::fromMSecsSinceEpoch(pl.ts).toString("[hh:mm]") )
                                    .arg(sender);
                        }
                        text += c;
                        continue;
                    }

                    QPoint p1 = textCpyVect.p1();
                    if ((p1.x() >= tl.x()) && (p1.x() < br.x()) &&
                        (p1.y() >= tl.y()) && (p1.y() < br.y())) {

                        if ((ic == 0) && (atStart)) {
                            getSender = true;
                            QString sender = pl.sender;
                            if ((! sender.isEmpty()) && (sender != "***"))
                                sender = QString("<%1>").arg(sender);
                            text += QString("%1 %2 ")
                                    .arg( QDateTime::fromMSecsSinceEpoch(pl.ts).toString("[hh:mm]") )
                                    .arg(sender);
                        }

                        text += c;
                    }
                }

                if (done)
                    break;

                Y += fontSize;

            }

            if (done)
                break;

            if (! text.isEmpty())
                text.append('\n');

        }
        if (! text.isEmpty())
           QApplication::clipboard()->setText(text);
    }

    if (! draggingText) {
        QString link = getLink(e->pos().x(), e->pos().y());
        if (! link.isEmpty()) {
            // TODO  this is 'hardcoded' channel anchors.
            // Instead, find channel types presented by 005 numeric.
            // This'll do anyway for now.
            if (link[1] == '#')
                link.remove(0, 1);
            if (link[0] == '#') { // Channel clicked
                emit joinChannelMenuRequest(e->pos(), link);
            }
            else if (e->pos().x() < splitterPos) {
                // (Most likely) a nickname inside splitter clicked.
                emit nickMenuRequest(e->pos(), link);
            }
            else
                QDesktopServices::openUrl( QUrl(link) );
        }
    }

    mouseDown = false;
    resizingSplitter = false;
    update();
}

void IIRCView::wheelEvent(QWheelEvent *e)
{
    if (e->delta() > 0)
        scrollbar.setValue( scrollbar.value()+1 );
    else
        scrollbar.setValue( scrollbar.value()-1 );

    draggingText = false;
}

void IIRCView::focusInEvent(QFocusEvent *e)
{
    emit gotFocus();
    e->accept();
}

void IIRCView::contextMenuEvent(QContextMenuEvent *e)
{
    emit menuRequested(e->globalPos());
}

void IIRCView::mouseDoubleClickEvent(QMouseEvent *e)
{
    emit mouseDblClick();
}
