/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2013  Tom-Andre Barstad
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

#include "tircview.h"
#include <iostream>
#include <QScrollBar>
#include <QTextCursor>
#include <QPainter>
#include <QVectorIterator>
#include <QApplication>

TIRCView::TIRCView(config *cfg, QWidget *parent) :
  QTextBrowser(parent),
  conf(cfg)
{
  clip = QApplication::clipboard();

  QFont font(conf->fontName);
  font.setPixelSize(conf->fontSize);
  setFont(font);

  connect(this, SIGNAL(selectionChanged()),
          this, SLOT(textSelected()));

  rebuild();
}

TIRCView::~TIRCView()
{
}

void TIRCView::textSelected()
{
  QString copytext = textCursor().selectedText();
  if (copytext.length() == 0)
      return; // No text to copy. stop.

  clip->setText( copytext );
}

void TIRCView::addLine(QString line, int ptype, bool rebuilding)
{
  /// "bool rebuilding" prevents this function to add data to the text vector.
  /// Used to rebuild the QTextBrowser with new css style.
  /// Usually you want to keep this one to the default (false) value.

  QTextCursor c = textCursor();
  c.movePosition(QTextCursor::End);
  setTextCursor(c);

  line = QString::fromUtf8(line.toStdString().c_str());

  QString buildLine;
  QString mainClass = "PT_NORMAL";

  switch (ptype) {

    case PT_NORMAL:
      buildLine.append("<span class=\"PT_NORMAL\">");
      mainClass = "PT_NORMAL";
      break;

    case PT_LOCALINFO:
      buildLine.append("<span class=\"PT_LOCALINFO\">");
      mainClass = "PT_LOCALINFO";
      break;

    case PT_SERVINFO:
      buildLine.append("<span class=\"PT_SERVINFO\">");
      mainClass = "PT_SERVINFO";
      break;

    case PT_NOTICE:
      buildLine.append("<span class=\"PT_NOTICE\">");
      mainClass = "PT_NOTICE";
      break;

    case PT_ACTION:
      buildLine.append("<span class=\"PT_ACTION\">");
      mainClass = "PT_ACTION";
      break;

    case PT_CTCP:
      buildLine.append("<span class=\"PT_CTCP\">");
      mainClass = "PT_CTCP";
      break;

    case PT_OWNTEXT:
      buildLine.append("<span class=\"PT_OWNTEXT\">");
      mainClass = "PT_OWNTEXT";
      break;

    default:
      buildLine.append("<span class=\"PT_NORMAL\">");
      mainClass = "PT_NORMAL";
      break;

  }

  bool bold = false;
  bool underline = false;
  //bool buildColor = false;
  bool rebuild = false; // Rebuild a new <span>

  bool buildLinkChan = false;
  bool buildLinkURL = false;
  QString link;
  QString linkText;

  QString colorClasses;
  QString fgCol;
  QString bgCol;

  for (int i = 0; i <= line.length()-1; i++) {
    // Find control codes, etc.
    QChar cc = line.at(i);
    QChar prevcc = line.at(i);
    if (i > 0)
      prevcc = line.at(i-1);

    if (rebuild) {
      // 'c' contains control codes:
      QString c = colorClasses;
      if (bold)
        c.append(" ctbold");
      if (underline)
        c.append(" ctunderline");

      if (c.length() == 0)
        buildLine.append("</span><span class=\"" + mainClass + "\">");
      else
        buildLine.append("</span><span class=\"" + mainClass + " " + c + "\">");

      rebuild = false; // done.
    }

    if (cc == 0x02) { // Bold
      bold = ! bold;
      rebuild = true;
      continue;
    }


    if (cc == 0x03) { // Colors
      if (i == line.length()-1)
        break; // At end, just break. nothing more to do. ever.

//      cc = line.at(i);

      bool bgParse = false;
      QString fgColNum, bgColNum;
      for (i++; i <= line.length()-1; i++) {
        // This loop also skips the color codes.
        // Begin the loop with skipping the ctrl-char.

        cc = line.at(i);

        if (bgParse)
          goto ParseBackground;
        else
          goto ParseForeground;

        ParseForeground:
        switch (cc.toLatin1()) {
          case '0':
            fgColNum += cc;
            continue;
          case '1':
            fgColNum += cc;
            continue;
          case '2':
            fgColNum += cc;
            continue;
          case '3':
            fgColNum += cc;
            continue;
          case '4':
            fgColNum += cc;
            continue;
          case '5':
            fgColNum += cc;
            continue;
          case '6':
            fgColNum += cc;
            continue;
          case '7':
            fgColNum += cc;
            continue;
          case '8':
            fgColNum += cc;
            continue;
          case '9':
            fgColNum += cc;
            continue;

          case ',':
            bgParse = true;
            continue;

          default:
            goto ParseEnd;
        }

        ParseBackground:
        switch (cc.toLatin1()) {
          case '0':
            bgColNum += cc;
            continue;
          case '1':
            bgColNum += cc;
            continue;
          case '2':
            bgColNum += cc;
            continue;
          case '3':
            bgColNum += cc;
            continue;
          case '4':
            bgColNum += cc;
            continue;
          case '5':
            bgColNum += cc;
            continue;
          case '6':
            bgColNum += cc;
            continue;
          case '7':
            bgColNum += cc;
            continue;
          case '8':
            bgColNum += cc;
            continue;
          case '9':
            bgColNum += cc;
            continue;

          default:
            goto ParseEnd;
        }

        ParseEnd:
        // Reaching here means we got no more numbers to parse.
        i--;
        break;
      }
      // At this point of code, fgCol and bgCol contains numbers of what colors to use.

      if (fgColNum.length() > 0)
        fgCol = "cf" + getCustomCSSColor(fgColNum);
      if (bgColNum.length() > 0)
        bgCol = "cb" + getCustomCSSColor(bgColNum);

      if (fgCol == "cf")
          fgCol.clear();
      if (bgCol == "cb")
          bgCol.clear();

      if ((fgCol.length() == 0) && (bgCol.length() == 0))
        buildLine.append("</span>");

      colorClasses = fgCol + " " + bgCol + " ";

      rebuild = true;
      continue; // i++ will be run at  beginning at for() loop; skipping the ctrl-code for colors will be done here.
    }

    if (cc == 0x1F) { // Underline
      underline = ! underline;
      rebuild = true;
      continue;
    }

    // Assume we add the character directly
    QString add = cc;

    if ((buildLinkURL) || (buildLinkChan))
        goto pastSwitch; /// We do not want to prepare new anchors when building a link.

    /// Detect URL anchor
    if (line.mid(i, 4) == "http") {
      link = linkText = "";
      buildLinkURL = true;
      goto pastSwitch;
    }

    if (line.mid(i, 4) == "www.") {
      link = "http://";
      linkText = "";
      buildLinkURL = true;
      goto pastSwitch;
    }

    /// Detect channel anchor
    if (cc == '#') {

      if ((i > 0) && (prevcc != ' ')) // Only allow channel names as own word.
        goto pastSwitch;

      add = "#";
      buildLinkChan = true;
      link = "channel:";
      linkText = "";

      goto pastSwitch; // This goes right after the following switch satement.
    }

    switch (cc.toLatin1()) {
      case '>':
        add = "&gt;";
        break;

      case '<':
        add = "&lt;";
        break;

      case ' ':
        if (prevcc == ' ')
          add = "&nbsp;";
        break;

      case '&':
        add = "&amp;";
        break;

      default:
        break;
    }

    //if ((num < 0x20) || (num > 0x7E))
    //    add = "&#" + QString::number(num) + ";";

    pastSwitch:


    if (buildLinkChan) {

      bool stopper = false;
      switch (cc.toLatin1()) {
        case ' ':
          stopper = true;
        case ':':
          stopper = true;
        case '.':
          stopper = true;
        case ';':
          stopper = true;
        case ',':
          stopper = true;
        case '\'':
          stopper = true;
        case '"':
          stopper = true;

      }

      if (i == line.length()-1) {
        stopper = true;
        link += cc;
        linkText += cc;
        i++;
      }

      if (stopper == true) {
        buildLinkChan = false;
        buildLine.append("<a href=\"" + link + "\">"+linkText+"</a>");
        i--;
      }
      else {
        link += cc;
        linkText += cc;
      }

      continue;
    }

    if (buildLinkURL) {

      if (i == line.length()-1) {
        link += cc;
        linkText += cc;
      }

      if ((cc == ' ') || (i == line.length()-1)) {
        buildLinkURL = false;
        buildLine.append("<a href=\"" + link + "\">"+linkText+"</a>");

        if (i != line.length()-1)
          i--;
      }
      else {
        link += cc;
        linkText += cc;
      }

      continue;
    }

    else
      buildLine.append(add);

  }

  if ((buildLinkURL) && (buildLinkChan))
    buildLine.append("<a href=\"" + link + "\">"+linkText+"</a>");


  if ((bold) || (underline) || (colorClasses.length() > 0))
    buildLine.append("</span>");

  if (ptype == PT_OWNTEXT)
      buildLine.append("</span>\n");
  else
      buildLine.append("</span><br>\n");

  colorClasses.clear();

  if (ptype == PT_OWNTEXT) {
      int a = textHTML.length()-5;
      if (textHTML.mid(a) == "<br>\n")
          textHTML = textHTML.mid(0,a);
      textHTML += "<div class=\"PT_OWNTEXT_BACKGROUND\">" + buildLine + "</div>";
  }
  else
      textHTML += buildLine;


  if (! rebuilding) {
    line_t l;
    l.type = ptype;
    l.text = line.toUtf8();
    text.append(l);
  }

  setHtml(textHTML);

  c = textCursor();
  c.movePosition(QTextCursor::End);
  setTextCursor(c);

  setOpenLinks(false);
}

void TIRCView::rebuild()
{
  QFont font(conf->fontName);
  font.setPixelSize(conf->fontSize);
  setFont(font);

  textHTML = "<body>";
  reloadCSS();

  setHtml(textHTML);

  QVectorIterator<line_t> i(text);
  while (i.hasNext()) {
    line_t line = i.next();
    addLine(line.text, line.type, true);
  }
}

void TIRCView::reloadCSS()
{
  QFile *f = new QFile(":/css/TIRCView.css");
  f->open(QIODevice::ReadOnly);
  QString css = f->readAll();
  f->close();

  QString ptnormal = getColorFromCode(conf->colDefault).name();
  QString ptlocalinfo = getColorFromCode(conf->colLocalInfo).name();
  QString ptservinfo = getColorFromCode(conf->colServerInfo).name();
  QString ptnotice = getColorFromCode(conf->colNotice).name();
  QString ptaction = getColorFromCode(conf->colAction).name();
  QString ptctcp = getColorFromCode(conf->colCTCP).name();
  QString ptowntextbg = "none";
  QString ptowntext = getColorFromCode(conf->colOwntext).name();

  if (conf->colOwntextBg > -1)
      ptowntextbg = getColorFromCode(conf->colOwntextBg).name();

  QString bgColor = getColorFromCode(conf->colBackground).name();
  QString bgImagePath = conf->bgImagePath;

  QString linkColor = getColorFromCode(conf->colLinks).name();
  QString linkUnderline;

  if (conf->linkUnderline == false)
      linkUnderline = "text-decoration: none;";

  css.replace("%PT_NORMAL",              ptnormal);
  css.replace("%PT_LOCALINFO",           ptlocalinfo);
  css.replace("%PT_SERVINFO",            ptservinfo);
  css.replace("%PT_NOTICE",              ptnotice);
  css.replace("%PT_ACTION",              ptaction);
  css.replace("%PT_CTCP",                ptctcp);
  css.replace("%PT_OWNTEXT_BACKGROUND",  ptowntextbg);
  css.replace("%PT_OWNTEXT",             ptowntext);

  css.replace("%bgColor",                bgColor);
  css.replace("%bgImage",                bgImagePath);

  css.replace("%linkColor",              linkColor);
  css.replace("%linkUnderline",          linkUnderline);


  document()->setDefaultStyleSheet(css);
  setHtml(textHTML);

  QTextCursor c = textCursor();
  c.movePosition(QTextCursor::End);
  setTextCursor(c);
}

void TIRCView::clear()
{
    text.clear();
    textHTML.clear();

    QTextBrowser::clear();
}

void TIRCView::focusInEvent(QFocusEvent *e)
{
  emit gotFocus();
  e->ignore();
}

void TIRCView::contextMenuEvent(QContextMenuEvent *e)
{
  emit menuRequested(e->globalPos());
}

QColor TIRCView::getColorFromCode(int num)
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

QString TIRCView::getCustomCSSColor(QString numeric)
{
    if (numeric == "0")
      return "White";

    if (numeric == "1")
      return "Black";

     if (numeric == "2")
      return "Blue";

    if (numeric == "3")
      return "Green";

    if (numeric == "4")
      return "BrightRed";

    if (numeric == "5")
      return "Red";

    if (numeric == "6")
      return "Magenta";

    if (numeric == "7")
      return "Brown";

    if (numeric == "8")
      return "Yellow";

    if (numeric == "9")
      return "BrightGreen";

    if (numeric == "10")
      return "Cyan";

    if (numeric == "11")
      return "BrightCyan";

    if (numeric == "12")
      return "BrightBlue";

    if (numeric == "13")
      return "BrightMagenta";

    if (numeric == "14")
      return "DarkGray";

    if (numeric == "15")
      return "LightGray";

    return "";
}
