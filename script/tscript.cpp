/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2014  Tom-Andre Barstad
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

#include "tscript.h"

#include "constants.h"

#include <QFile>
#include <iostream>
#include <QHashIterator>
#include <QVector>

TScript::TScript(QObject *parent, QWidget *dialogParent, QString fname) :
    QObject(parent),
    dlgParent(dialogParent),
    ifn(&sockets, &fnindex, &dialogs, &files),
    filename(fname)
{
  connect(&sockets, SIGNAL(runEvent(e_iircevent,QStringList)),
          this, SLOT(runEvent(e_iircevent,QStringList)));
}

void TScript::delWhitespace(QString *text)
{
  int i = 0;

  for (; i <= text->length()-1; i++) {
    if (text->at(i) == 0x20) // Space
      continue;
    if (text->at(i) == 0x0B) // TAB
      continue;
    if (text->at(i) == 0x0D) // Carriage return
      continue;
    if (text->at(i) == '\n')
      continue;

    break; // break at first occurence of _anything else_ than whitespaces.
  }

  // use 'i' variable to determine how much to skip
  *text = text->mid(i);
}

QString TScript::setRelativePath(QString folder, QString file)
{
  /*

  includeFile -> file
  parent -> folder

*/

  QString dir = folder;
  QStringList n = folder.split('/'); // parent script filename (maybe with path)
  QStringList fnn = file.split('/'); // include file
  bool scriptFullpath = false;

  // If fnn is more than 1 it might be a full path.
  if (fnn.length() > 1) {
    // If the include file have folders defined, run full path if that's the case.
#ifdef Q_WS_X11
    if (file.at(0) == '/') {
      // include is a full path.
      dir.clear();
      scriptFullpath = true;
    }
#else
    if (file.mid(1,2) == ":/") {
      // include is a full path.
      dir.clear();
      scriptFullpath = true;
    }
#endif
  }

  // if n got multiple items, it is a path and last item is filename.
  // fnn being 1 means it is in same folder as parent script
  if ((n.length() > 1) && (scriptFullpath == false)) {
    QString last = n.last(); // filename of parent script
    dir = folder.mid(0, folder.length()-last.length()); // directory to parent script
  }

  return dir + file;
}

e_scriptresult TScript::loadScript2(QString includeFile, QString parent)
{
    QString fn = filename;
    if (includeFile.length() == 0) {
        std::cout << "Running loadScript() on '" << filename.toStdString().c_str() << "'" << std::endl;
        scriptstr.clear();
        errorKeyword.clear();
        curLine = 1;
        include.clear();
        command.clear();
        tevent.clear();

        QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
        while (i.hasNext()) {
            i.next();
            delete i.value();
        }
        dialogs.clear();

    }
    else {
      fn = setRelativePath(parent, includeFile);
      std::cout << "Including '" << fn.toStdString().c_str() << "'" << std::endl;
    }



    std::cout << "attempt: " << fn.toStdString().c_str() << std::endl;

    if (QFile::exists(fn) == false)
        return se_FileNotExist;


    QFile f(fn);
    bool fo = f.open(QIODevice::ReadOnly | QIODevice::Text);
    if (fo == false)
        return se_FileCannotOpen;

    QString ba = f.readAll();
    if (ba.length() < 1)
        return se_FileEmpty;

    // Remove comments, keep newlines:
    QString tmp;
    bool isCommented = false;

    for (int i = 0; i <= ba.length()-1; i++) {
        QChar c = ba.at(i);

        if (c == '\n') { // newline
            delWhitespace(&tmp);
            if (tmp.length() > 0)
                scriptstr.push_back(tmp + '\n');
            isCommented = false;
            tmp.clear();
            continue;
        }

        if (isCommented) // part of a comment, ignore.
            continue;

        if (c == ';') { // comment, skip anything after.
            isCommented = true;
            continue;
        }

        tmp.append(c);
    }

    if (tmp.length() > 0) // insert anything missing from very last line.
        scriptstr.push_back(tmp + '\n');

    // This was an include, the first instance of loadScript() will take care of retreiving the rest.
    if (includeFile.length() > 0)
      return se_None;


    // Search script for functions and parse the meta.
    QString keyword;
    bool getKeyword = true; // Always begin with getting keyword.
    bool comment = false;

    enum { // Expecting
      ex_Unknown = 0, // If we use this we must pose an error too.
      ex_Block,
      ex_Brace,
      ex_Statement,
      ex_Ignore, // ignore until next block (still handles nesting levels)
      ex_Include = 5,
      ex_Command,
      ex_Event,
      ex_Timer,
      ex_ScriptName,
      ex_FunctionName = 10,
      ex_MenuName,
      ex_DialogName,
      ex_DialogTitle,
      ex_DialogGeometry,
      ex_DialogIcon = 15,
      ex_DialogLabel,
      ex_DialogButton,
      ex_DialogEditBox,
      ex_DialogTextBox,
      ex_DialogListBox = 20
    };

    enum {
      st_None = 0, // used when waiting for block name keyword
      st_Meta,
      st_Function,
      st_Menu,
      st_Dialog
    };

    int state = st_None;
    int ex = ex_Block; // We expect to find a script block to begin with. either function, script or menu.
    int n = 0; // Nesting level. 0 is where script blocks are at (script, function, menu, etc.)

    QString temp[2];
    TCustomScriptDialog *dialog = NULL;

    for (int i = 0; i <= scriptstr.length()-1; i++) {
        QChar cc = scriptstr.at(i);

        errorKeyword = keyword + cc;

        if (cc == '\n')
          curLine++;

        if (ex != ex_Ignore) {
          if (cc == '\\') { // Escape, skip completely and add to keyword
            if (i == scriptstr.length()-1)
              return se_EscapeOnEndLine;
            if (scriptstr.at(i+1) == '\n')
              return se_EscapeOnEndLine;

            keyword += scriptstr.at(i+1);
            i++;
            continue;
          }

          /* Comment handling */
          /* Put ; anywhere but must end with newline. */
          if (cc == '\n')
            comment = false;
          if (comment)
            continue;
          if (cc == ';') {
            comment = true;
            continue;
          }
          /* **************** */
        }



        if (cc == '{') {
          if ((ex != ex_Brace) && (ex != ex_Ignore) && (n == 0)) {
            return se_UnexpectedToken;
          }
          n++;
        }
        if (cc == '}') {
          if (n == 0)
            return se_UnexpectedToken;

          n--;

          if (state == st_Dialog) {
            dialogs.insert( dialog->getName(), dialog );
            connect(dialog, SIGNAL(runEvent(e_tircevent,QStringList)),
                    this, SLOT(runEvent(e_tircevent,QStringList)));
            state = st_None;
          }

          if (n == 0)
            ex = ex_Block;
        }

        if (ex == ex_Brace) {
          if ((cc != ' ') && (cc != '\t') && (cc != '\n') && (cc != '{') && (cc != '}'))
            return se_UnexpectedToken;

          if ((cc == ' ') || (cc == '\t') || (cc == '\n'))
            continue;

          ex = ex_Statement;
          continue;
        }

        if (cc == '}')
            continue;

        if (ex == ex_FunctionName) {
          // Stop this one at (
          // If we encounter \n first, pose error.

          if (cc == '\n')
            return se_UnexpectedNewline;
          if (i == scriptstr.length()-1)
            return se_UnexpectedFinish;

          if (cc == '(') {
            fnindex.insert(keyword.toUpper(), i);
            ex = ex_Ignore;
            continue;
          }

          if (cc == ' ') {
            QChar cc2 = scriptstr.at(i+1);
            if (cc2 == '(')
              continue;
            else
              return se_UnexpectedToken;
          }
        }

        // Newline, receiving keyword but keyword got is none. ignore.
        if ((cc == '\n') && (getKeyword == true) && (keyword.length() == 0))
          continue;

        // Newline, space receiving keyword and got a keyword. parse.
        if (((cc == '\n') || (cc == ' ')) && (getKeyword == true) && (keyword.length() >= 0)) {
          QString keywup = keyword.toUpper();

          if (n == 0) { // Here we always expect script blocks.
            if (ex == ex_Block) {

              if (keywup == "SCRIPT") {
                ex = ex_ScriptName;
                state = st_Meta;
                keyword.clear();
                continue;
              }

              else if (keywup == "META") {
                ex = ex_Brace;
                state = st_Meta;
                keyword.clear();
                continue;
              }

              else if (keywup == "FUNCTION") {
                ex = ex_FunctionName;
                state = st_Function;
                keyword.clear();
                continue;
              }
              /*
              else if (keywup == "MENU") {
                ex = ex_MenuName;
                continue;
              }
              */
              else if (keywup == "DIALOG") {
                ex = ex_DialogName;
                state = st_Dialog;
                keyword.clear();
                continue;
              }

              else {
                return se_InvalidBlockType;
              }

            }

            if (ex == ex_ScriptName) {
              name = keyword;
              ex = ex_Brace;
              keyword.clear();
              continue;
            }

            if (ex == ex_DialogName) {
              dialog =  new TCustomScriptDialog(keyword, dlgParent, this);
              ex = ex_Brace;
              keyword.clear();
              continue;
            }

          }

          if (n > 0) {

            if (state == st_Meta) {

              if (ex == ex_Statement) {
                keyword.clear(); // this is safe here.
                if (keywup == "INCLUDE") {
                  ex = ex_Include;
                  continue;
                }
                else if (keywup == "COMMAND") {
                  ex = ex_Command;
                  continue;
                }
                else if (keywup == "EVENT") {
                  ex = ex_Event;
                  continue;
                }
                else if (keywup == "TIMER") {
                  ex = ex_Timer;
                  continue;
                }
                else {
                  return se_InvalidMetaCommand;
                }
              }

              if (ex == ex_Include) {
                if (cc == ' ') {
                  keyword += cc;
                  continue;
                }
                include.push_back(keyword);
                e_scriptresult r = loadScript2(keyword, fn);
                keyword.clear();
                ex = ex_Statement;

                if (r != se_None)
                  return r;

                continue;
              }

              if ((ex == ex_Command) || (ex == ex_Event) || (ex == ex_Timer)) {

                if (temp[0].length() == 0)
                  temp[0] = keyword; // command name
                else
                  temp[1] = keyword; // function
                keyword.clear();

                if (((temp[0].length() == 0) || (temp[1].length() == 0)) && (cc == '\n'))
                  return se_UnexpectedNewline;

                if (cc == '\n') {
                  // this one will run if both temp are filled.

                  if (ex == ex_Command)
                    command.insert(temp[0].toUpper(), temp[1]);

                  if (ex == ex_Event) {
                    e_iircevent evt = getEvent(temp[0]);

                    if (evt == te_noevent)
                      return se_InvalidEvent;

                    tevent.insertMulti(evt, temp[1]);
                  }


                  if (ex == ex_Timer) {
                    if (timers.contains(temp[0].toUpper()) == true)
                      goto loadScript__ex_TimerAdd_Cleanup;


                    TTimer *tmr = new TTimer(temp[1].toUpper(), this);
                    connect(tmr, SIGNAL(timeout(QString)),
                            this, SLOT(timerTimeout(QString)));

                    timers.insert(temp[0].toUpper(), tmr);
                  }

                  loadScript__ex_TimerAdd_Cleanup:

                  ex = ex_Statement;
                  temp[0].clear();
                  temp[1].clear();

                  keyword.clear();
                  continue;
                }

                keyword.clear();
                continue;
              }

            }

            if (state == st_Dialog) {

              if (ex == ex_Statement) {
                keyword.clear();

                if (keywup == "TITLE")
                  ex = ex_DialogTitle;

                else if (keywup == "GEOMETRY")
                  ex = ex_DialogGeometry;

                else if (keywup == "LABEL")
                  ex = ex_DialogLabel;

                else if (keywup == "BUTTON")
                    ex = ex_DialogButton;

                else if (keywup == "EDITBOX")
                    ex = ex_DialogEditBox;

                else if (keywup == "TEXTBOX")
                    ex = ex_DialogTextBox;

                else if (keywup == "LISTBOX")
                    ex = ex_DialogListBox;

                else
                    return se_UnexpectedToken;

                continue;
              }

              if (ex == ex_DialogTitle) {
                for (; i <= scriptstr.length()-1; i++) {
                  QChar c = scriptstr.at(i);
                  if (c == '\n')
                    break;
                  keyword += c;
                }
                dialog->setTitle(keyword);
                keyword.clear();
                ex = ex_Statement;
                continue;
              }

              if (ex == ex_DialogGeometry) {
                int param = 2; // 1X 2Y 3W 4H
                int X, Y, W, H;
                X = keyword.toInt();
                keyword.clear();
                for (i++; i <= scriptstr.length()-1; i++) {
                  QChar c = scriptstr.at(i);

                  if ((c == ' ') || (c == '\n')) {
                      if (param == 2)
                          Y = keyword.toInt();
                      if (param == 3)
                          W = keyword.toInt();
                      if (param == 4)
                          H = keyword.toInt();
                      keyword.clear();
                      param++;
                  }

                  if (c == '\n')
                    break;


                  if (param == 5)
                      break;

                  keyword += c;
                }

                if (param != 5)
                    return se_InvalidParamCount;

                dialog->setGeometry(X, Y, W, H);
                keyword.clear();
                ex = ex_Statement;
                continue;
              }

              if (ex >= ex_DialogLabel) {
                int param = 2;
                int X, Y, W, H = 0;
                QString oname = keyword;
                QString arg;
                keyword.clear();
                for (i++; i <= scriptstr.length()-1; i++) {
                  QChar c = scriptstr.at(i);

                  if (param < 6) {
                      if ((c == ' ') || (c == '\n')) {
                          if (param == 2)
                              X = keyword.toInt();
                          if (param == 3)
                              Y = keyword.toInt();
                          if (param == 4)
                              W = keyword.toInt();
                          if (param == 5)
                              H = keyword.toInt();
                          keyword.clear();
                          param++;
                      }

                  }
                  if (c == '\n')
                    break;

                  if (param >= 6)
                      arg += c;
                  else
                      keyword += c;
                }

                if (param != 6)
                    return se_InvalidParamCount;

                arg = arg.mid(1);


                if (ex == ex_DialogLabel)
                    dialog->addLabel(oname, X, Y, W, H, arg);

                if (ex == ex_DialogButton)
                    dialog->addButton(oname, X, Y, W, H, arg);

                if (ex == ex_DialogEditBox)
                    dialog->addEditbox(oname, X, Y, W, H);

                if (ex == ex_DialogTextBox)
                    dialog->addTextbox(oname, X, Y, W, H);

                if (ex == ex_DialogListBox)
                    dialog->addListbox(oname, X, Y, W, H);


                keyword.clear();
                ex = ex_Statement;
                continue;
              }

            }

          }

          keyword.clear();

          continue;
        }

        keyword += cc;
    }

    if (n > 0)
      return se_UnexpectedFinish;

    std::cout << "Script " << name.toStdString().c_str() << " (re)loaded with:" << std::endl;
    std::cout << "- " << fnindex.count() << " functions." << std::endl;
    std::cout << "- " << tevent.count() << " events." << std::endl;
    std::cout << "- " << include.count() << " includes." << std::endl;
    std::cout << "- " << command.count() << " commands." << std::endl;
    std::cout << "- " << timers.count() << " timers." << std::endl;
    std::cout << "---" << std::endl;

    return se_None;
}

QByteArray TScript::extractBinVars(QString &text, QHash<QString,QByteArray> *binVar)
{
  // Scan through the text and put data into 'temp'.
  // When encountering '%' sign, do not add that to temp, but figure the complete variable name (when we encounter a space),
  // and then insert variable data to temp.
  bool getVarName = false;
  QByteArray tmp;
  QString var; // Var name
  for (int i = 0; i <= text.length()-1; i++) {
    QChar c = text.at(i);

    if (getVarName == false) {
      if (c == '%') {
        getVarName = true;
        var.clear();
        var += c;
        continue;
      }

      tmp += c;
    }

    if (getVarName == true) {
      if ((c == ' ') || (i == text.length()-1)) {
        getVarName = false;

        if ((i == text.length()-1) && (c != ' '))
          var += c;

        bool ex = binVar->contains(var);
        if (ex)
          tmp += binVar->value(var);

        if (c == ' ')
          tmp += ' ';
        continue;
      }

      var += c;
    }

  }

  return tmp;
}

QString TScript::extractVars(QString &text, QStringList *varName, QStringList *varData, QHash<QString,QByteArray> *binVar)
{
    enum {
        st_add = 0, // Add anything until c is % - escaping allowed like \%
        st_var,
        st_fnignore
    };

    int st = st_add;
    QString result;
    QString var;
    int nl = 0;
    for (int i = 0; i <= text.length()-1; i++) {
        QChar c = text.at(i);

        if ((c == '\\') && (st != st_fnignore)) {
            if (i == text.length()-1)
                break;
            i++;
            c = text.at(i);
            result += c;
            continue;
        }

        if (st == st_add) {
            if (c == '%') {
                st = st_var;
                var = "%";
                continue;
            }
            if (c == '$')
                st = st_fnignore;

            result += c;
            continue;
        }

        if (st == st_fnignore) {
            if (((c == ' ') || (c == '$')) && (nl == 0)) {
                st = st_add;
                if (c == ' ')
                    result += ' ';
                continue;
            }


            result += c;

            if (c == '(')
                nl++;
            if (c == ')')
                nl--;

            if (nl == 0)
                st = st_add;

            continue;
        }

        if (st == st_var) {
            if ((c == ' ') || (c == '%') || (i == text.length()-1)) {

                if ((c != ' ') && (c != '%') && (i == text.length()-1))
                    var += c;

                if (binVar->contains(var))
                    result += var;
                else {
                    int idx = varName->indexOf(var);
                    if (idx > -1)
                        result += varData->at(idx);

                    if (c == ' ')
                        result += ' ';
                }
                st = st_add;
                continue;
            }

            var += c;
        }
    }

    return result;
}


QString TScript::extractFunction(QString &text, QStringList *varName, QStringList *varData, QHash<QString,QByteArray> *binVar)
{
    enum {
        st_Add = 0,
        st_Name,
        st_ParamWS,
        st_Param
    };


    QString result;
    int st = st_Add;
    QString para;
    QStringList params;
    QString fname;
    for (int i = 0; i <= text.length()-1; i++) {
        QChar c = text.at(i);

        if (st == st_Add) {
            if (c == '$') {
                st = st_Name;
                fname.clear();
                continue;
            }
            result += c;
        }

        if (st == st_Name) {
            if (c == ' ') {
                QString r;
                params.clear();
                runf(fname, params, r);
                result += r;
                st = st_Add;
                continue;
            }

            if (i == text.length()-1) {
                QString r;
                fname += c;
                runf(fname, params, r);
                result += r;
                st = st_Add;
                params.clear();
                continue;
            }

            if (c == '(') {
                params.clear();
                para.clear();
                st = st_ParamWS;
                continue;
            }

            fname += c;
        }

        if (st == st_ParamWS) {
            if (i == text.length()-1)
                return ""; // unexpected end

            if ((c == ' ') || (c == '\t'))
                continue;
            if (c == ')') {
                QString r;
                runf(fname, params, r);
                result += r;
                st = st_Add;
                params.clear();
                continue;
            }

            st = st_Param;
        }

        if (st == st_Param) {
            bool add = true;
            if (c == '\\') {
                if (i == text.length()-1)
                    break;
                para += text.at(++i);
                continue;
            }

            if (c == '$') {
                int nl = 0;
                para += c;
                for (i++; i <= text.length()-1; i++) {
                    c = text.at(i);
                    QChar c1;
                    if (i == 0)
                        c1 = c;
                    else
                        c1 = text.at(i-1);

                    if (nl == 0) {
                        if (c == ' ') {
                            para += ' ';
                            break;
                        }
                        if (c == ')')
                            break;
                        if (c == ',')
                            break;
                        if (i == text.length()-1) {
                            para += c;
                            break;
                        }
                    }

                    para += c;
                    add = false;

                    if ((c == '(') && (c1 != '\\'))
                        nl++;
                    if ((c == ')') && (c1 != '\\'))
                        nl--;

                }
            }

            if (c == ',') {
                para = extractVars(para, varName, varData, binVar);
                para = extractFunction(para, varName, varData, binVar);
                params.push_back(para);
                para.clear();
                st = st_ParamWS;
                continue;
            }

            if (c == ')') {
                if (para.length() > 0) {
                    para = extractVars(para, varName, varData, binVar);
                    para = extractFunction(para, varName, varData, binVar);
                    params.push_back(para);
                }

                QString r;
                runf(fname, params, r);
                result += r;
                st = st_Add;
                params.clear();
                continue;
            }

            if (add)
                para += c;
        }
    }

    return result;
}


bool TScript::solveBool(QString &data, QStringList *varName, QStringList *varData, QHash<QString,QByteArray> *binVar)
{
  /// This function solves the individual bool expressions like  test == 1234

  QString Q1 = "";
  QString Q2 = "";
  bool onQ2 = false; // Sets to true when we get an operator.

  enum {
      NOOPERATOR = 0,
      NOT_EQ,
      IS_EQ,
      LESSTHAN,
      GREATERTHAN,
      LESSTHANEQUAL,
      GREATERTHANEQUAL
  };

  short op = NOOPERATOR;

  for (int i = 0; i <= data.length()-1; i++) {
    QChar c = data.at(i);

    if (c == '%') {
      int vl = data.indexOf(' ', i);
      if (vl == -1)
          vl = data.length()-1;

      QString var = data.mid(i, vl);

      var = extractVars(var, varName, varData, binVar);
      if (onQ2)
        Q2 += var;
      else
        Q1 += var;

      i = vl;
      continue;
    }

    if ((c == '=') && (data.at(i+1) == '=')) {
      onQ2 = true;
      op = IS_EQ;
      i += 2;
      continue;
    }
    if ((c == '!') && (data.at(i+1) == '=')) {
      onQ2 = true;
      op = NOT_EQ;
      i += 2;
      continue;
    }
    if ((c == '>') && (data.at(i+1) == '=')) {
      onQ2 = true;
      op = GREATERTHANEQUAL;
      i += 2;
      continue;
    }
    if ((c == '<') && (data.at(i+1) == '=')) {
      onQ2 = true;
      op = LESSTHANEQUAL;
      i += 2;
      continue;
    }
    if (c == '>') {
      onQ2 = true;
      op = GREATERTHAN;
      i += 1;
      continue;
    }
    if (c == '<') {
      onQ2 = true;
      op = LESSTHAN;
      i += 1;
      continue;
    }

    if (onQ2)
      Q2 += c;
    else
      Q1 += c;

  }

  if (Q1 == "\\n")
    Q1 = "\n";
  if (Q2 == "\\n")
    Q2 = "\n";

  Q1 = extractFunction(Q1, varName, varData, binVar);
  Q2 = extractFunction(Q2, varName, varData, binVar);

  QString Q1u = Q1.toUpper();
  QString Q2u = Q2.toUpper();

  //Q1.remove(Q1.length()-1); // there will always be an extra whitechar @ end...

  switch (op) {
    case NOT_EQ:
      return (Q1u != Q2u);
    case IS_EQ:
      return (Q1u == Q2u);
    case LESSTHAN:
      return (Q1.toDouble() < Q2.toDouble());
    case GREATERTHAN:
      return (Q1.toDouble() > Q2.toDouble());
    case LESSTHANEQUAL:
      return (Q1.toDouble() <= Q2.toDouble());
    case GREATERTHANEQUAL:
      return (Q1.toDouble() >= Q2.toDouble());
    default:
      return false;
  }

  // Basically, we shouldn't reach here, but for safety, we make it return false.
  return false;
}

bool TScript::solveLogic(QString &data, QStringList *varName, QStringList *varData, QHash<QString, QByteArray> *binVar)
{
  /// This one solves a sentence with bool expressions, like: ((test == 1234) && (lol != 4))
  /// For solving of  test == 1234  see solveBool();

  data = extractFunction(data, varName, varData, binVar);

  int tl = data.length();

  if (tl == 0)
    return false;

  int nst = 0; // Paranthesis nesting level
  QString exp = ""; // Expression to run with mathexp.h
  QString lt = ""; // Logic testing, ex.: "testvar != 1234"
  bool addLt = false;

  for (int i = 0; i <= tl-1; i++) {
    QChar c = data.at(i); // Current character
    QChar c1 = 0x00; // Next character

    if (i < tl-1) // Only get next character if we aren't on the end.
      c1 = data.at(i+1);

    if (c == '(') {
      nst++;
      exp += c;
      lt.clear();
      continue;
    }
    if ((c == ')') && (addLt == false)) {
      nst--;
      exp += c;
      if (nst == 0) {
        if (i < tl-1) {
          // apparently we're on the end of parsing but looking on the numbers, something wrong probably happened.
          /// echo("Script error: Mismatching paranthesis count");
          exp.clear();
          break;
        }
        break; // Done, yay, etc
      }
      continue;
    }
    if ((c == '|') && (c1 == '|')) {
      // OR
      exp += '+';
      i++; // we're checking 2 characters, so increment one time now and it'll increment one by end of this loop.
      continue;
    }
    if ((c == '&') && (c1 == '&')) {
      // AND
      exp += '*';
      i++; // we're checking 2 characters, so increment one time now and it'll increment one by end of this loop.
      continue;
    }

    // When we reach here, none of the above statements was true;
    // so we treat it as Logic Testing; add data to 'lt' until we reach paranthesis end - )

    addLt = true;

    if ((c == ')') && (addLt == true)) {
      int l = solveBool(lt, varName, varData, binVar);
      exp += QString::number(l) + ")";
      lt.clear();
      addLt = false;
      continue;
    }
    lt += c; // Add expression to lt
  }

  if (exp.length() > 0)
    if (ifn.calc(exp).toInt() > 0)
      return true;

  return false;
}

bool TScript::runEvent(e_iircevent evt, QStringList param)
{
  QHashIterator<e_iircevent,QString> i(tevent);
  bool found = false;


  while (i.hasNext()) {
    i.next();

    e_iircevent ievt = i.key();
    QString fnct = i.value();

    if (ievt == evt) {
      // Do not break here, there might be multiple of same events.
      found = true;
      QString r;

      e_scriptresult ok = runf(fnct, param, r);
      if (ok != se_RunfDone)
        emit error("Unable to run function " + fnct + " (Script " + name + " ) - Parameter count?");
    }
  }

  return found;
}

e_scriptresult TScript::runf(QString function, QStringList param, QString &result, bool ignoreParamCount)
{
#ifdef TIRC_DEBUG_SCRIPT
  std::cout << "runf(" << function.toStdString().c_str() << "):" << std::endl;
#endif

  /* Here we attempt to run a function. It does not have to exist.
   * This is where we initiate the main script interpreter.
   * If the function does not exist it'll produce a script error.
  */

  if (function.length() == 0)
    return se_FunctionEmpty; // Avoid crashes if function name is none.

  if (function == "$")
    return se_InvalidFunction;

  if (function.at(0) == '$')
    function = function.mid(1); // Prevent any dollar signs (function identifier) at beginning

  // See if it's an internal function, if it is, run it, get true result and return true (sucess). Ignore parsing if the function is internal.
  // This means, nobody can override the internal function names. A list of these should be given in TIRC documentation.
  if (ifn.runFunction(function, param, result) == true)
    return se_RunfDone;

  int pos = fnindex.value(function.toUpper(), -1); // Get internal line number where function resides, for faster executing on large scripts.

  // Retreive parameters list from script, and insert our params from here to there...
  QString par;

  if (pos < 0) {
    error("Error in attempting to call an undefined function " + function);
    return se_InvalidFunction;
  }

  bool getParams = false;
  for (int i = pos; i <= scriptstr.length()-1; i++) {
    QChar c = scriptstr.at(i);

    if (getParams == true) {
      if (c == ')')
        break;
      if (c == '\n')
        return se_UnexpectedNewline;
      if (c == '(')
        return se_UnexpectedToken;
      if ((c == ' ') || (c == '\t'))
        continue;

      par += c;
    }

    if (getParams == false) {
      if (c == '(')
        getParams = true;
      if (c == '\n')
        return se_UnexpectedNewline;
    }
  }

  QStringList scpar = par.split(","); // parameter names

  if (scpar.count() == 1)
    if (scpar.at(0).length() == 0)
      scpar.clear();

  if (scpar.count() == 0)
      param.clear(); // no need for this if we cannot bind the data to ANY parameters (they're non-existant)

  if (scpar.count() > 0) {
    if (ignoreParamCount == false) {
      // Make sure that parameter count is _equal_
      if (param.count() != scpar.count())
        return se_InvalidParamCount;
      /// @note From here, parameter names are found in 'scpar', while the data is in 'param'.
    }
    if (ignoreParamCount == true) {
      // Fill out parameters until we got no more data; and the rest will be put at last variable.

      // Got more variables to fill than actual tokenized data
      if (scpar.count() > param.count())
        for (int i = param.count(); i <= scpar.count()-1; i++) { param.push_back(""); }

      // Got more tokenized data than variable parameters. The overflow of input data goes to last variable in list.
      if (scpar.count() < param.count()) {
      /**

        example to run:
        prototype: test(%somevar, %temp, %i, %x, %text)
        params: test lol 5 7 some text here

             varname     |      data
        ----------------------------------
        somevar          | test
        temp             | lol
        i                | 5
        x                | 7
        text             | some text here


        scpar = param/var names
        param = param/var data
     */

        QString endparam = param.last();
        int i = scpar.count();
        while (scpar.count() != param.count()) {
          endparam += ' ';
          endparam += param.at(i);
          param.removeAt(i);
        }
        // Set the last parameter with all tokens that exceed.
        // Any parameters after this is garbage and *should* just be ignored.
        param.replace(scpar.count()-1, endparam);
      }
    }
  }

  QHash<QString,QByteArray> binVar;
  e_scriptresult res = _runf_private2(pos, function, &scpar, &param, &binVar, result);

  return res;
}

e_scriptresult TScript::_runf_private2(int pos, QString function, QStringList *varName, QStringList *varData, QHash<QString, QByteArray> *binVar, QString &result)
{
#ifdef TIRC_DEBUG_SCRIPT
  std::cout << "TScript::_runf_private2(" << pos << ", \"" << function.toStdString().c_str() << "\", ...);" << std::endl;
#endif

  /* Here we run functions we _know_ exist. Do not run this one directly.
   * This one should go from runf(); function unless you know what you're doing!
   *
   * Following comes a 2000 line script interpreter:
  */


  // Typically, "keyword" would be the first word in every code it executes.
  QString keyword;
  bool getKeyword = true; // Always begin with getting keyword.
  bool comment = false;

  enum {
    ex_Param = 0,
    ex_BraceOpen,
    ex_BraceClose,
    ex_Literal
  };

  enum {
    st_Param = 0,
    st_Literal,
    st_IgnoreNest
  };

  int ex = ex_BraceOpen; // Begin with expecting parameters.
  int state = ex_Param;

  int nl = 0; // Nesting level.
  int nl_ignore = 0; // If an IF statement went false, set this to the nesting level we will return executing.

  curLine = 1; // Should be the current line, but will be wrong with included files.
  for (int i = 0; i <= pos-1; i++)
    if (scriptstr.at(i) == '\n')
      curLine++;


  // "while" level and positions:
  QVector<int> wnl; // while nest level.
  QVector<int> wbp; // contains byte positions of where every while that surrounds the outer nests.

  // We loop through the script byte by byte.
  for (int i = pos; i <= scriptstr.length()-1; i++) {
      QChar cc = scriptstr.at(i);

      if (cc == '\n')
        curLine++;

      if (cc == '\\') { // Escape, skip completely and add to keyword
        if (i == scriptstr.length()-1)
          return se_EscapeOnEndLine;

        i++;

        if (scriptstr.at(i) == '\n')
          return se_EscapeOnEndLine;

        keyword += scriptstr.at(i);
        continue;
      }

      /* Comment handling */
      /* Put ; anywhere but must end with newline. */
      /* You may escape comments with \; */
      if (cc == '\n')
        comment = false;
      if (comment)
        continue;
      if (cc == ';') {
        comment = true;
        continue;
      }
      /* **************** */

      if (state == st_Param) {
        // Skip the parameter definition, it's already been sorted.
        if (cc == '{') {
          keyword.clear();
          state = st_Literal;
          ex = ex_Literal;
          nl++;
          continue;
        }

        continue;
      }

      if (state == st_IgnoreNest) {
        if (cc == '{')
          nl++;
        if (cc == '}')
          nl--;

        if ((cc == '}') && (nl == nl_ignore)) {
          state = st_Literal;
          ex = ex_Literal;
          keyword.clear();
        }
        continue;
      }

      if (state == st_Literal) {

        if (ex == ex_BraceOpen) {
          if ((cc == ' ') || (cc == '\t') || (cc == '\n'))
            continue;

          else if (cc == '{') {
            ex = ex_Literal;
            nl++;
            keyword.clear();
            continue;
          }

          else
            return se_UnexpectedToken;
        }

      }

      // Newline, receiving keyword but keyword got is none. ignore.
      if (((cc == '\n') || (cc == ' ')) && (getKeyword == true) && (keyword.length() == 0))
        continue;

      // Newline, space receiving keyword and got a keyword. parse.
      if (((cc == '\n') || (cc == ' ')) && (getKeyword == true) && (keyword.length() >= 0)) {

        if (keyword == "}") {
          nl--;

          if (wnl.size() > 0) {
            // We're possibly exiting a while loop. check.
            int l = wnl.back();

            if (nl == l) { // re-check for loop validity.
              i = wbp.back();
              wnl.pop_back();
              wbp.pop_back();
              keyword.clear();
              ex = ex_Literal;
              state = st_Literal;
              continue;
            }

          }

          if (nl == 0)
            return se_RunfDone;
          keyword.clear();
          continue;
        }

        /// Run function
        if (keyword.at(0) == '$') {
          for (; i <= scriptstr.length()-1; i++) {
            if (scriptstr.at(i) == '\n')
              break;
            keyword += scriptstr.at(i);
          }
          extractFunction(keyword, varName, varData, binVar);
          keyword.clear();
          continue;
        }



        /** ***************************************** **/
        /** Parser keywords IF, WHILE, VAR, CON, etc. **/
        /** ***************************************** **/

        /* Notes on adding new keywords:
         *
         * Always clear the keyword after parsing
         * Always run 'continue' when finished (or else it'll be parsed as a /command)
         */

        QString keywup = keyword.toUpper();


        /** ############## **/
        /** RETURN KEYWORD **/
        /** ############## **/

        if (keywup == "RETURN") {
          QString rs; // Whatever's being returned. Not binary safe yet... oops.

          bool wait = true; // Skip whitespace between RETURN ... start
          for (; i <= scriptstr.length()-1; i++) {
            QChar c = scriptstr.at(i);
            if (wait == true) {
              if ((c == ' ') || (c == '\t'))
                continue;
              wait = false;
            }
            if (wait == false) {
              if (c == '\n')
                break;
              rs += c;
            }
          }

          rs = extractFunction(rs, varName, varData, binVar);
          rs = extractVars(rs, varName, varData, binVar);

          result.clear();
          result.append(rs);

          if (rs == "0")
            emit stopURLDisplay();

          return se_RunfDone;
        }


        /** ################## **/
        /** CONTROL STATEMENTS **/
        /** ################## **/

        if (keywup == "IF") {
          /// @note This one can do some cleaning up or rewrite... Remember to fix "while" too.

          int pn = 0; // Paranthesis nest
          bool waitPB = true; // Wait for paranthesis at beginning
          QString logic = "(";
          for (; i <= scriptstr.length()-1; i++) {
            cc = scriptstr.at(i);
            if (waitPB) {
              if ((cc == ' ') || (cc == '\t'))
                continue;
              else if (cc == '(') {
                pn++;
                waitPB = false;
                continue;
              }
              else
                return se_UnexpectedToken;
            }

            logic += cc;

            if (cc == '(')
              pn++;
            if (cc == ')')
              pn--;
            if (pn == 0)
              break;

          }

          if (pn != 0)
            return se_UnexpectedToken;

          bool ok = solveLogic(logic, varName, varData, binVar);
          lastIFresult = ok;

          if (ok == false) {
            state = st_IgnoreNest;
            ex = ex_Literal;
            nl_ignore = nl;
          }
          else {
            state = st_Literal;
            ex = ex_BraceOpen;
          }

          keyword.clear();
          continue;
        }

        if (keywup == "ELSE") {
          if (lastIFresult == false) {
            state = st_Literal;
            ex = ex_BraceOpen;
          }
          if (lastIFresult == true) {
            state = st_IgnoreNest;
            ex = ex_Literal;
            nl_ignore = nl;
          }

          keyword.clear();
          continue;
        }

        if (keywup == "WHILE") {
          wbp.push_back(i-6);
          wnl.push_back(nl);

          int pn = 0; // Paranthesis nest
          bool waitPB = true; // Wait for paranthesis at beginning
          QString logic = "(";
          for (; i <= scriptstr.length()-1; i++) {
            cc = scriptstr.at(i);
            if (waitPB) {
              if ((cc == ' ') || (cc == '\t'))
                continue;
              else if (cc == '(') {
                pn++;
                waitPB = false;
                continue;
              }
              else
                return se_UnexpectedToken;
            }

            logic += cc;

            if (cc == '(')
              pn++;
            if (cc == ')')
              pn--;
            if (pn == 0)
              break;

          }

          if (pn != 0)
            return se_UnexpectedToken;

          bool ok = solveLogic(logic, varName, varData, binVar);

          if (ok == false) {
            wbp.pop_back();
            wnl.pop_back();
            state = st_IgnoreNest;
            ex = ex_Literal;
            nl_ignore = nl;
          }
          else {
            state = st_Literal;
            ex = ex_BraceOpen;
          }

          keyword.clear();
          continue;
        }

        if (keywup == "BREAK") {
          if (wnl.size() == 0)
            return se_BreakNoWhile;

          // Break current loop
          nl_ignore = wnl.back();
          wbp.pop_back();
          wnl.pop_back();
          state = st_IgnoreNest;
          ex = ex_Literal;

          keyword.clear();
          continue;
        }

        if (keywup == "CONTINUE") {
          if (wnl.size() == 0)
            return se_ContinueNoWhile;
          i = wbp.back();
          nl = wnl.back();
          wnl.pop_back();
          wbp.pop_back();
          ex = ex_Literal;
          state = st_Literal;

          keyword.clear();
          continue;
        }


        /** ################## **/
        /** VARIABLE SPECIFICS **/
        /** ################## **/

        if (keywup == "VAR") {
            // var %variable data

            enum {
                st_VarPrefix = 0,
                st_VarName,
                st_Data
            };

            int st = st_VarPrefix;
            QString vname = "%";
            QString data;

            for (; i <= scriptstr.length()-1; i++) {
                QChar c = scriptstr.at(i);

                if (st == st_VarPrefix) {
                    if ((c == ' ') || (c == '\t'))
                        continue;
                    else if (c == '%')
                        st = st_VarName;
                    else
                        return se_UnexpectedToken;

                    continue;
                }

                if (st == st_VarName) {
                    if (c == '\n')
                        return se_UnexpectedNewline;
                    else if (c == ' ')
                        st = st_Data;
                    else
                        vname += c;

                    continue;
                }

                if (st == st_Data) {
                    if (c == '\n')
                        break; // Got vname and data ready.

                    data += c;

                    continue;
                }

            }

            if (st != st_Data) // In case something very weird happens, this prevents TIRC to go bananas.
              return  se_UnexpectedNewline;

            data = extractFunction(data, varName, varData, binVar);
            data = extractVars(data, varName, varData, binVar);

            int pos = varName->indexOf(vname);

            if (pos > -1) {
                varName->removeAt(pos);
                varData->removeAt(pos);
            }

            varName->push_back(vname);
            varData->push_back(data);

            keyword.clear();
            continue;
        }

        if ((keywup == "INC") || (keywup == "DEC")) {
            // inc %var [val]
            // dev %var [val]

            enum {
                st_VarPrefix = 0,
                st_VarName,
                st_Value
            };

            int st = st_VarPrefix;
            QString vname = "%";
            QString value;

            for (; i <= scriptstr.length()-1; i++) {
                QChar c = scriptstr.at(i);

                if (st == st_VarPrefix) {
                    if ((c == ' ') || (c == '\t'))
                        continue;
                    else if (c == '%')
                        st = st_VarName;
                    else
                        return se_UnexpectedToken;

                    continue;
                }

                if (st == st_VarName) {
                    if (c == '\n')
                        break; // This makes us increment/decrement by one
                    else if (c == ' ')
                        st = st_Value;
                    else
                        vname += c;

                    continue;
                }

                if (st == st_Value) {
                    if (c == '\n')
                        break; // Got vname and value ready.

                    value += c;

                    continue;
                }

            }

            if (st == st_VarPrefix) // In case something very weird happens, this prevents TIRC to go bananas.
              return  se_UnexpectedNewline;

            double val = 1.0f;
            if (value.length() > 0) {
                value = extractFunction(value, varName, varData, binVar);
                value = extractVars(value, varName, varData, binVar);

                bool ok = false;
                val = value.toDouble(&ok);
                if (!ok)
                    val = 1.0f;
            }

            // Increment or decrement value is in 'val'

            double varval = 0.0f; // default value of the variable (if it doesn't exist)
            int pos = varName->indexOf(vname);
            if (pos > -1)
              varval = varData->at(pos).toDouble();


            if (keywup == "INC")
                varval += val;
            if (keywup == "DEC")
                varval -= val;

            // save to variable
            if (pos > -1) {
              // Variable exist
              varData->replace(pos, QString::number(varval));
            }
            else {
              // Variable does not, create new.
              varName->push_back(vname);
              varData->push_back(QString::number(varval));
            }

            keyword.clear();
            continue;
        }

        if (keywup == "VARTABLE") {
            // vartable - DEBUGGING ONLY.
          emit warning("Printing vartable result in standard output stream.");
          std::cout << "Following values MUST be identical: varNameCount: " << varName->count() << "  varDataCount: " << varData->count() << std::endl;
          for (int i = 0; i <= varName->count()-1; i++) {
            std::cout << "'" << varName->at(i).toStdString().c_str() << "' :: '" << varData->at(i).toStdString().c_str() << "'" << std::endl;
          }
          if (varData->count() > varName->count()) {
            for (int i = varName->count(); i <= varData->count()-1; i++) {
              std::cout << "'----' :: '" << varData->at(i).toStdString().c_str() << "'" << std::endl;
            }
          }

          keyword.clear();
          continue;
        }


        /** ########## **/
        /** CONTAINERS **/
        /** ########## **/

        if (keywup == "WCON") {
          // wcon id data

          enum {
            st_Wait = 0,
            st_GetID,
            st_GetData
          };

          QString id;
          QString data;

          int st = st_Wait;
          for (; i <= scriptstr.length()-1; i++) {
            QChar c = scriptstr.at(i);

            if (st == st_Wait) {
              if (c == ' ')
                continue;
              else
                st = st_GetID;
            }

            if (st == st_GetID) {
              if (c == '\n')
                return se_UnexpectedNewline;
              if (c == ' ') {
                st = st_GetData;
                continue;
              }
              id += c;
              continue;
            }

            if (st == st_GetData) {
              if (c == '\n')
                break;

              data += c;
            }
          }

          QString cname = extractFunction(id, varName, varData, binVar);
          cname = extractVars(cname, varName, varData, binVar);

          QString d = extractFunction(data, varName, varData, binVar);
          d = extractVars(d, varName, varData, binVar);

          writeCon(cname, d);

          keyword.clear();
          continue;
        }

        if (keywup == "DCON") {
          // dcon id

          QString id;

          for (; i <= scriptstr.length()-1; i++) {
            QChar c = scriptstr.at(i);

            if (c == '\n')
              break;

            if (c == ' ')
              continue;

            id += c;
          }

          QString cname = extractFunction(id, varName, varData, binVar);
          cname = extractVars(cname, varName, varData, binVar);

          delCon(cname);

          keyword.clear();
          continue;
        }

        if (keywup == "SCON") {
          // scon %namevar idx pattern
          enum {
            st_Wait,
            st_VarPrefix = 0,
            st_VarName,
            st_Index,
            st_Pattern
          };

          QString var;
          QString idx;
          QString pattern = "*";
          int st = st_VarPrefix;
          for (; i <= scriptstr.length()-1; i++) {
            QChar c = scriptstr.at(i);

            if (st == st_Wait) {
              if (c == ' ')
                continue;
              else
                st = st_VarPrefix;
            }

            if (st == st_VarPrefix) {
              if (c == '\n')
                return se_UnexpectedNewline;
              if ((c == ' ') || (c == '\t'))
                continue;
              if (c == '%')
                st = st_VarName;
            }

            if (st == st_VarName) {
              if (c == '\n')
                return se_UnexpectedNewline;
              if (c == ' ') {
                st = st_Index;
                continue;
              }

              var += c;
            }

            if (st == st_Index) {
              if (c == '\n')
                return se_UnexpectedNewline;
              if (c == ' ') {
                st = st_Pattern;
                continue;
              }

              idx += c;
            }

            if (st == st_Pattern) {
              if (c == '\n')
                break;
              if (c == ' ') // Spaces not allowed in the pattern.
                return se_UnexpectedToken;

              pattern += c;
            }
          }

          idx = extractFunction(idx, varName, varData, binVar);
          idx = extractVars(idx, varName, varData, binVar);
          pattern = extractFunction(pattern, varName, varData, binVar);
          pattern = extractVars(pattern, varName, varData, binVar);

          int index = idx.toInt();
          if (index < 0)
            return se_NegativeNotAllowed;

          int count = 1;
          QString data;
          QHashIterator<QString,QString> i(container);
          while (i.hasNext()) {
            i.next();

            QString iname = i.key();
            QRegExp rx( pattern );
            rx.setPatternSyntax(QRegExp::WildcardUnix);
            int rxidx = iname.indexOf(rx);

            if ((index == 0) && (rxidx > -1)) {
              count++;
              continue;
            }

            if ((index > 0) && (rxidx > -1) && (count == index)) {
              data = iname;
              break;
            }
            if ((index > 0) && (rxidx > -1) && (count < index)) {
              count++;
              continue;
            }

          } /// while (i.hasNext())


          if (index == 0)
            data = QString::number( count-1 );

          int varpos = varName->indexOf(var);

          if (varpos > -1)
            varData->replace(varpos, data); // var exists, just update with new val.

          else {
            // Var does not exist, create new.
            varName->push_back(var);
            varData->push_back(data);
          }

          keyword.clear();
          continue;
        }

        if (keywup == "CON") {
          // con id %var

          enum {
            st_Wait = 0,
            st_Id,
            st_VarPrefix,
            st_VarName
          };

          QString id;
          QString vname;

          int st = st_Wait;
          for (; i <= scriptstr.count()-1; i++) {
            QChar c = scriptstr.at(i);

            if (st == st_Wait) {
              if (c == ' ')
                continue;
              else
                st = st_Id;
            }

            if (st == st_Id) {
              if (c == '\n')
                return se_UnexpectedNewline;
              if (c == ' ') {
                st = st_VarPrefix;
                continue;
              }
              id += c;
              continue;
            }

            if (st == st_VarPrefix) {
              if (c == '\n')
                return se_UnexpectedNewline;
              if (c == ' ')
                continue;
              else if (c == '%')
                st = st_VarName;
              else
                return se_UnexpectedToken;
            }

            if (st == st_VarName) {
              if (c == '\n')
                break;
              if (c == ' ')
                return se_UnexpectedToken;
              vname += c;
            }

          }


          id = extractFunction(id, varName, varData, binVar);
          id = extractVars(id, varName, varData, binVar);

          QString d = container.value(id);
          int pos = varName->indexOf(vname);
          if (pos > -1) {
            // var exists, just update with new val.
            varData->replace(pos, d);
          }
          else {
            // Var does not exist, create new.
            varName->push_back(vname);
            varData->push_back(d);
          }

          keyword.clear();
          continue;
        }


        /** ###### **/
        /** TIMERS **/
        /** ###### **/

        if (keywup == "TIMER") {
            // timer id msec

          enum {
            st_Wait = 0,
            st_Id,
            st_Msec
          };

          QString id;
          QString msec;

          int st = st_Wait;
          for (; i <= scriptstr.count()-1; i++) {
            QChar c = scriptstr.at(i);

            if (st == st_Wait) {
              if (c == ' ')
                continue;
              else
                st = st_Id;
            }

            if (st == st_Id) {
              if (c == '\n')
                return se_UnexpectedToken;
              if (c == ' ') {
                st = st_Msec;
                continue;
              }
              id += c;
            }

            if (st == st_Msec) {
              if (c == '\n')
                break;
              if (c == ' ')
                return se_UnexpectedToken;

              msec += c;
              continue;
            }

          }

          id = extractFunction(id, varName, varData, binVar);
          id = extractVars(id, varName, varData, binVar);

          msec = extractFunction(msec, varName, varData, binVar);
          msec = extractVars(msec, varName, varData, binVar);

          int ms = msec.toInt();
          TTimer *tmr = timers.value(id.toUpper(), 0);
          if (tmr == 0)
            return se_InvalidTimer;

          tmr->runTimer(ms);

          keyword.clear();
          continue;
        }

        if (keywup == "STIMER") {
          // stimer id
          QString id;
          for (; i <= scriptstr.count()-1; i++) {
            QChar c = scriptstr.at(i);

            if (c == ' ')
              continue;

            if (c == '\n')
              break;

            id += c;
          }

          id = extractFunction(id, varName, varData, binVar);
          id = extractVars(id, varName, varData, binVar);

          TTimer *tmr = timers.value(id.toUpper(), 0);
          if (tmr == 0)
            return se_InvalidTimer;

          tmr->stopTimer();

          keyword.clear();
          continue;
        }


        /** ####### **/
        /** SOCKETS **/
        /** ####### **/

        if (keywup == "SOCK") {
          // sock switch arguments

          enum {
            st_Wait = 0, // Wait for switch -
            st_Switches,
            st_Sockname,
            st_Args
          };

          int st = st_Wait;
          QString sw;
          QString sockname;
          QString args;
          for (; i <= scriptstr.length()-1; i++) {
            QChar c = scriptstr.at(i);

            if (st == st_Wait) {
              if (c == ' ')
                continue;
              else if (c == '-')
                st = st_Switches;
              else
                return se_UnexpectedToken;

              continue;
            }

            if (st == st_Switches) {
              if (c == '\n')
                return se_UnexpectedToken;
              else if (c == ' ')
                st = st_Sockname;
              else
                sw += c;

              continue;
            }

            if (st == st_Sockname) {
              if (c == '\n')
                break; // Allow newline here, not all sock commands require args.
              else if (c == ' ')
                st = st_Args;
              else
                sockname += c;

              continue;
            }

            if (st == st_Args) {
              if (c == '\n')
                break;

              args += c;
              continue;
            }

          }

          sockname = extractFunction(sockname, varName, varData, binVar);
          sockname = extractVars(sockname, varName, varData, binVar);

          QString argsEx = extractFunction(args, varName, varData, binVar);
          argsEx = extractVars(argsEx, varName, varData, binVar);

          bool accept = false;  // a
          bool close = false;   // c
          bool decline = false; // d
          bool listen = false;  // l
          bool open = false;    // o
          bool read = false;    // r
          bool write = false;   // w
          bool newline = false; // n
          bool binary = false;  // b
          bool switchfail = false;

          for (int i = 0; i <=sw.length()-1; i++) {
            QChar s = sw.at(i);

            switch (s.toLatin1()) {
            case 'a': // Accept
              accept = true;
              break;
            case 'b': // Binary variable
              binary = true;
              break;
            case 'c': // Close
              close = true;
              break;
            case 'd': // Decline
              decline = true;
              break;
            case 'l': // Listen
              listen = true;
              break;
            case 'o': // Open
              open = true;
              break;
            case 'r': // Read
              read = true;
              break;
            case 'w': // Write
              write = true;
              break;
            case 'n': // Newline
              newline = true;
              break;
            case '-':
              break;
            default:
              switchfail = true;
            }
          }

          if (switchfail == true)
            return se_InvalidSwitches;

          int swsum = accept + close + decline + listen + open + write; // These CANNOT be combined.
          if (swsum > 1) // Some of them are combined, stop !
            return se_InvalidSwitches;

          if (listen) {
            if (argsEx.length() == 0)
              return se_InvalidParamCount;
            sockets.socklisten(sockname, argsEx.toInt());

            keyword.clear();
            continue;
          }

          if (open) {
            if (argsEx.length() == 0)
              return se_InvalidParamCount;

            QStringList argl = argsEx.split(" ");
            if (argl.length() < 2)
              return se_InvalidParamCount;

            sockets.sockopen(sockname, argl.at(0), argl.at(1).toInt());
            keyword.clear();
            continue;
          }

          if (close) {
            // Close a socket
            sockets.sockclose(sockname);
            keyword.clear();
            continue;
          }

          if (write) {
            // Write to socket
            QByteArray data;

            if (args.length() == 0) {
              if (newline) {
                data.append('\n');
                sockets.sockwrite(sockname, &data);
              }
              keyword.clear();
              continue;
            }

            args = extractFunction(args, varName, varData, binVar);

            data.append( extractVars(args, varName, varData, binVar) );

            if (binary) {
              QString d(data);
              data = extractBinVars(d, binVar);
            }

            if (newline)
              data.append('\n');

            sockets.sockwrite(sockname, &data);
            keyword.clear();
            continue;
          }

          if (read) {
            // Read a socket.

            if (args.length() == 0)
              return se_InvalidParamCount;

            QString vname = args;
            QByteArray data;

            if (newline)
              data.append(sockets.sockreadLn(sockname));

            else
              data = sockets.sockread(sockname);


            if (vname.at(0) != '%')
              return se_MissingVariable; // variables must begin wtih a percent %

            if (! binary) {
              /// String variables
              int pos = varName->indexOf(vname);
              if (pos > -1) {
                // var exists, just update with new val.
                varData->replace(pos, data);
              }
              else {
                // Var does not exist, create new.
                varName->push_back(vname);
                varData->push_back(data);
              }

              if (binVar->contains(vname))
                  binVar->remove(vname); // Remove binary variable.
            }

            if (binary) {
              // QHash will replace existant values.
              binVar->insert(vname, data);

              int pos = varName->indexOf(vname); // Find exsisting variable
              if (pos > -1) { // Delete text variable
                varName->removeAt(pos);
                varData->removeAt(pos);
              }
            }

            keyword.clear();
            continue;
          }

          if (accept) {
            if (args.length() == 0)
              return se_InvalidParamCount;

            QString new_sockname = argsEx;

            bool ok = false;
            if (sockets.hasName(args) == false)
              ok = sockets.sockAcceptNext(sockname, new_sockname);

            keyword.clear();
            continue;
          }

          if (decline) {
            bool ok = sockets.sockDeclineNext(sockname);

            keyword.clear();
            continue;
          }

          if (newline) {
            // Reaching here means it wasn't combined correctly
            emit warning("sock: Invalid use of newline switch");

            keyword.clear();
            continue;
          }

          if (binary) {
            // Reaching here means it wasn't combined correctly
            emit warning("sock: Invalid use of binary switch");

            keyword.clear();
            continue;
          }

          keyword.clear();
          continue;
        }


        /** ####### **/
        /** TOOLBAR **/
        /** ####### **/

        if (keywup == "TOOLBAR") {
            /* toolbar -aidf name args

              toolbar -a name tooltip
              toolbar -i name path/to/icon
              toolbar -d name
              toolbar -f name function
*/
            enum {
                st_Wait, // Wait for switch
                st_Switch,
                st_Name,
                st_Arg
            };

            bool add = false;
            bool icon = false;
            bool del = false;
            bool fnct = false;
            QString toolname;
            QString arg;

            int st = st_Wait;
            for (; i <= scriptstr.length()-1; i++) {
                QChar c = scriptstr.at(i);

                if (st == st_Wait) {
                    if (c == '\n')
                        return se_UnexpectedNewline;
                    else if (c == ' ')
                        continue;
                    else if (c == '-')
                        st = st_Switch;
                    else
                        return se_UnexpectedToken;

                    continue;
                }

                if (st == st_Switch) {
                    if (c == '\n')
                        return se_UnexpectedNewline;

                    if (c == ' ') {
                      st = st_Name;
                      continue;
                    }

                    if (c == '-')
                        continue;
                    else if (c == 'a')
                        add = true;
                    else if (c == 'i')
                        icon = true;
                    else if (c == 'd')
                        del = true;
                    else if (c == 'f')
                        fnct = true;
                    else
                        return se_InvalidSwitches;

                    continue;
                }

                if (st == st_Name) {
                    if (c == '\n')
                        break;
                    if (c == ' ') {
                        st = st_Arg;
                        continue;
                    }
                    toolname += c;
                    continue;
                }

                if (st == st_Arg) {
                    if (c == '\n')
                        break;

                    arg += c;
                    continue;
                }

            }

            int sum = add+del+icon+fnct;
            if (sum != 1)
                return se_InvalidSwitches;

            if (add) {
              if (arg.length() == 0)
                return se_InvalidParamCount;
              emit toolbarAdd(toolname, name, arg);
            }

            if (del)
              emit toolbarDel(toolname);

            if (icon) // empty icon path means remove icon.
              // "filename" is script filename. Setting icon relative path to it, unless icon path is complete.
              emit toolbarSetIcon(toolname, setRelativePath(filename, arg));


            if (fnct) {
              if (arg.length() == 0)
                return se_InvalidParamCount;
              emit toolbarSetFunction(toolname, name, arg); // "name" is script name.
            }

            keyword.clear();
            continue;
        }


        /** ############## **/
        /** DIALOG CONTROL **/
        /** ############## **/

        if (keywup == "DLG") {

            enum {
                st_Wait,
                st_Switch,
                st_Arg
            };

            // dlg -shclide name [obj] [args]

            bool show = false;
            bool hide = false;
            bool close = false;
            bool label = false;
            bool additem = false;
            bool clear = false;
            bool delitem = false;

            QString arg;
            int st = st_Wait;
            for (; i <= scriptstr.length()-1; i++) {
                QChar c = scriptstr.at(i);

                if (st == st_Wait) {
                    if (c == ' ')
                        continue;

                    else if (c == '-')
                        st = st_Switch;

                    else
                        return se_UnexpectedToken;
                }

                if (st == st_Switch) {
                    if (c == ' ') {
                        st = st_Arg;
                        continue;
                    }

                    if (c == '-')
                        continue;
                    else if (c == 's')
                        show = true;
                    else if (c == 'h')
                        hide = true;
                    else if (c == 'c')
                        close = true;
                    else if (c == 'l')
                        label = true;
                    else if (c == 'i')
                        additem = true;
                    else if (c == 'd')
                        delitem = true;
                    else if (c == 'e')
                        clear = true;
                    else
                        return se_InvalidSwitches;
                }

                if (st == st_Arg) {
                    if (c == '\n')
                        break;
                    arg += c;
                }

            }

            int sum = show+hide+close+label+additem+delitem+clear;
            if (sum > 1)
                return se_InvalidSwitches;

            arg = extractFunction(arg, varName, varData, binVar);
            arg = extractVars(arg, varName, varData, binVar);

            bool ok = false;
            if (show)
                ok = customDialogShow(arg);

            if (hide)
                ok = customDialogHide(arg);

            if (close)
                ok = customDialogClose(arg);

            if (label) {
                QStringList par = arg.split(' '); // dialog object text ...
                if (par.count() < 3)
                  return se_UnexpectedNewline;
                QString text = arg.mid(par.at(0).length() + par.at(1).length() + 2);
                ok = customDialogSetLabel(par.at(0), par.at(1), text);
            }

            if (additem) {
                QStringList par = arg.split(' '); // dialog object text ...
                if (par.count() < 3)
                  return se_UnexpectedNewline;
                QString text = arg.mid(par.at(0).length() + par.at(1).length() + 2);
                ok = customDialogAddItem(par.at(0), par.at(1), text);
            }

            if (delitem) {
                QStringList par = arg.split(' '); // dialog object index ...
                if (par.count() < 3)
                  return se_UnexpectedNewline;
                ok = customDialogDelItem(par.at(0), par.at(1), par.at(2));
            }

            if (clear) {
              QStringList par = arg.split(' '); // dialog object
              if (par.count() < 2)
                return se_UnexpectedNewline;
              ok = customDialogClear(par.at(0), par.at(1));
            }

            if (! ok)
                return se_UnexpectedToken;

            keyword.clear();
            continue;
        }


        /** ############ **/
        /** FILE CONTROL **/
        /** ############ **/

        if (keywup == "FREAD") {
          // fread %fd length %var
          // Read all data in buffer to %var

          enum {
            st_ignore,
            st_fd,
            st_length,
            st_var
          };

          int st = st_ignore;
          int stnext = st_fd;
          int fd = 0;
          qint64 len;
          QString fds;
          QString length;
          QString var;

          for (; i <= scriptstr.length()-1; i++) {
            QChar c = scriptstr.at(i);
            if (st == st_ignore) {
              if (c == '\n')
                return se_UnexpectedNewline;
              if (c == ' ')
                continue;
              st = stnext;
            }

            if (st == st_fd) {
              if (c == ' ') {
                st = st_ignore;
                stnext = st_length;
                fds = extractFunction(fds, varName, varData, binVar);
                fds = extractVars(fds, varName, varData, binVar);
                continue;
              }
              if (c == '\n')
                return se_UnexpectedNewline;

              fds += c;
              continue;
            }

            if (st == st_length) {
              if (c == ' ') {
                st = st_ignore;
                stnext = st_var;
                length = extractFunction(length, varName, varData, binVar);
                length = extractVars(length, varName, varData, binVar);
                continue;
              }
              if (c == '\n')
                return se_UnexpectedNewline;

              length += c;
              continue;
            }

            if (st == st_var) {
              if (c == ' ')
                continue;
              if (c == '\n')
                break;

              var += c;
              continue;
            }
          }

          fd = fds.toInt();
          len = length.toLongLong();

          if (! files.contains(fd))
            return se_InvalidFileDescriptor;

          t_sfile ts = files.value(fd);
          QFile *f = ts.file;

          if (len == -1)
            len = f->size();


          QByteArray data;
          if ((len == 0) && (ts.binary == false))
            data = f->readLine();
          else
            data = f->read(len);

          if (ts.binary == true)
            binVar->insert(var, data);
          else {
            varName->push_back(var);
            varData->push_back(data);
          }


          keyword.clear();
          continue;
        }

        if (keywup == "FWRITE") {
          // fwrite %fd %data

          enum {
            st_ignore,
            st_fd,
            st_data
          };
          int st = st_ignore;
          int stnext = st_fd;

          QString fds, datas;
          QByteArray data;
          for (; i <= scriptstr.length()-1; i++) {
            QChar c = scriptstr.at(i);

            if (st == st_ignore) {
              if (c == ' ')
                continue;
              if (c == '\n')
                return se_UnexpectedNewline;
              st = stnext;
            }

            if (st == st_fd) {
              if (c == ' ') {
                st = st_ignore;
                stnext = st_data;
                fds = extractFunction(fds, varName, varData, binVar);
                fds = extractVars(fds, varName, varData, binVar);
                continue;
              }
              if (c == '\n')
                return se_UnexpectedNewline;

              fds += c;
              continue;
            }

            if (st == st_data) {
              if (c == '\n') {
                break;
              }

              datas += c;
              continue;
            }

          }

          datas = extractFunction(datas, varName, varData, binVar);
          datas = extractVars(datas, varName, varData, binVar);



          data = extractBinVars(datas, binVar);
          int fd = fds.toInt();
          if (! files.contains(fd))
            return se_InvalidFileDescriptor;

          t_sfile ts = files.value(fd);
          QFile *f = ts.file;


          int r = f->write(data);
          f->flush();

          keyword.clear();
          continue;
        }

        if (keywup == "FSEEK") {
          enum {
            st_ignore,
            st_fd,
            st_pos
          };

          int st = st_ignore;
          int stnext = st_fd;
          int fd = 0;
          qint64 pos;
          QString poss;
          QString fds;

          for (; i <= scriptstr.length()-1; i++) {
            QChar c = scriptstr.at(i);
            if (st == st_ignore) {
              if (c == '\n')
                return se_UnexpectedNewline;
              if (c == ' ')
                continue;
              st = stnext;
            }

            if (st == st_fd) {
              if (c == '\n')
                return se_UnexpectedNewline;
              if (c == ' ') {
                st = st_ignore;
                stnext = st_pos;
                fds = extractFunction(fds, varName, varData, binVar);
                fds = extractVars(fds, varName, varData, binVar);
                continue;
              }
              fds += c;
              continue;
            }

            if (st == st_pos) {
              if (c == '\n')
                break;
              if (c == ' ')
                continue;

              poss += c;
              continue;
            }
          }

          poss = extractFunction(poss, varName, varData, binVar);
          poss = extractVars(poss, varName, varData, binVar);

          pos = poss.toLongLong();
          fd = fds.toInt();
          if (! files.contains(fd))
            return se_InvalidFileDescriptor;

          t_sfile ts = files.value(fd);
          QFile *f = ts.file;
          bool ok = f->seek(pos);
          if (! ok)
            return se_FSeekFailed;

          keyword.clear();
          continue;
        }

        if (keywup == "FCLOSE") {
          QString fds;
          int fd;
          for (; i <= scriptstr.length()-1; i++) {
            QChar c = scriptstr.at(i);
            if (c == '\n')
              break;
            if (c == ' ')
              continue;

            fds += c;
          }

          fds = extractFunction(fds, varName, varData, binVar);
          fds = extractVars(fds, varName, varData, binVar);
          fd = fds.toInt();

          if (! files.contains(fd))
            return se_InvalidFileDescriptor;

          t_sfile ts = files.value(fd);
          ts.file->close();
          delete ts.file;
          files.remove(fd);

          keyword.clear();
          continue;
        }

        /** ############################################################################## **/
        /** ############################################################################## **/
        /** ############################################################################## **/


        /// If we reach here, treat it as /command.
        for (; i <= scriptstr.length()-1; i++) {
          if (scriptstr.at(i) == '\n')
            break;
          keyword += scriptstr.at(i);
        }

        QString c = extractFunction(keyword, varName, varData, binVar);
        c = extractVars(c, varName, varData, binVar);
        execCmd(c);

        keyword.clear();
        continue;
      }



      keyword += cc;
  }

  return se_RunfDone; // If there was no errors we should reach here.
}

bool TScript::runCommand(QString cmd)
{
  if (cmd.length() == 0)
    return false;

  if (cmd.left(1) == "/") // Remove a possible / at beginning
    cmd = cmd.mid(1);

  QString c = cmd.split(' ').at(0); // contains the command

  if (command.contains(c.toUpper()) == false) // checks if 'c' is an actual registered command
    return false;

  QStringList token = cmd.split(' ');

  QStringList param;
  if (token.length() > 1) // if there's parameters we should add them.
    param = cmd.mid(c.length()+1).split(' ');

  QString fn = command.value(c.toUpper());

  // It's safe to ignore param count when running from a command.
  QString r;

#ifdef TIRC_DEBUG_SCRIPT
  std::cout << "runCommand(" << cmd.toStdString().c_str() << ")" << std::endl;
#endif

  e_scriptresult res = runf(fn, param, r, true);
  errorHandler(res);

  bool ok = true;
  if (res != se_RunfDone)
    ok = false;

  return ok;
}

bool TScript::hasCommand(QString cmd)
{
  QString c = cmd.split(' ').at(0);
  if (c.left(1) == "/")
    c = c.mid(1);

#ifdef TIRC_DEBUG_SCRIPT
  std::cout << "hasCommand(" << cmd.toStdString().c_str() << ")" << std::endl;
#endif

  return command.contains(c);
}

void TScript::writeCon(QString id, QString data)
{
  // QHash replaces existing id's for us.
  container.insert(id, data);
}

void TScript::delCon(QString id)
{
  container.remove(id);
}

QString TScript::readCon(QString id)
{
  return container.value(id, QString(""));
}

void TScript::timerTimeout(QString fn)
{
  QStringList par;
  QString r;
  runf(fn, par, r);

}

e_iircevent TScript::getEvent(QString event)
{
  QString evt = event.toUpper();

  if (evt == "START")
    return te_start;

  else if (evt == "LOAD")
    return te_load;

  else if (evt == "UNLOAD")
    return te_unload;

  else if (evt == "EXIT")
    return te_exit;

  else if (evt == "CONNECT")
    return te_connect;

  else if (evt == "DISCONNECT")
    return te_disconnect;

  else if (evt == "JOIN")
    return te_join;

  else if (evt == "PART")
    return te_part;

  else if (evt == "QUIT")
    return te_quit;

  else if (evt == "MSG")
    return te_msg;

  else if (evt == "SOCKOPEN")
    return te_sockopen;

  else if (evt == "SOCKREAD")
    return te_sockread;

  else if (evt == "SOCKCLOSE")
    return te_sockclose;

  else if (evt == "SOCKERROR")
    return te_sockerror;

  else if (evt == "SOCKLISTEN")
    return te_socklisten;

  else if (evt == "MOUSEMOVE")
    return te_mousemove;

  else if (evt == "MOUSELEFTDOWN")
    return te_mouseleftdown;

  else if (evt == "MOUSELEFTUP")
    return te_mouseleftup;

  else if (evt == "MOUSEMIDDLEDOWN")
    return te_mousemiddledown;

  else if (evt == "MOUSEMIDDLEUP")
    return te_mousemiddleup;

  else if (evt == "MOUSEMIDDLEROLL")
    return te_mousemiddleroll;

  else if (evt == "MOUSERIGHTDOWN")
    return te_mouserightdown;

  else if (evt == "MOUSERIGHTUP")
    return te_mouserightup;

  else if (evt == "URLCLICK")
    return te_urlclick;

  else if (evt == "DBUTTONCLICK")
      return te_dbuttonclick;

  else if (evt == "DLISTBOXSELECT")
      return te_dlistboxselect;
  else
    return te_noevent;
}

void TScript::errorHandler(e_scriptresult res)
{

  switch (res) {

    case se_None:
      break;

    case se_RunfDone:
      break;

    case se_InvalidParamCount:
      emit error("Invalid parameter count around '"+errorKeyword+"' at line " + QString::number(curLine));
      break;

    case se_InvalidSwitches:
      emit error("Invalid switches around '"+errorKeyword+"' at line " + QString::number(curLine));
      break;

    case se_UnexpectedToken:
      emit error("Unexpected token around '"+errorKeyword+"' at line " + QString::number(curLine));
      break;

    case se_EscapeOnEndLine:
      emit error("Escape character on line break at line " + QString::number(curLine));
      break;

    case se_InvalidFunction:
      emit warning("Invalid function at line " + QString::number(curLine));
      break;

    case se_NegativeNotAllowed:
      emit error("Negative value not allowed around '"+errorKeyword+"' at line " + QString::number(curLine));
      break;

    case se_InvalidTimer:
      emit error("Invalid timer on line " + QString::number(curLine));
      break;

    case se_BreakNoWhile:
      emit error("Trying to 'break' outside while loop, line " + QString::number(curLine));
      break;

    case se_ContinueNoWhile:
      emit error("Trying to 'continue' outside while loop, line " + QString::number(curLine));
      break;

    default:
      emit warning("Function ended abnormal (code " + QString::number(res) + ") around '"+errorKeyword+"' at line " + QString::number(curLine));
      break;

  }

}

bool TScript::customDialogShow(QString oname)
{
    QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
    while (i.hasNext()) {
        i.next();
        TCustomScriptDialog *dlg = i.value();
        if (dlg->getName().toUpper() == oname.toUpper()) {
            dlg->showDlg();
            return true;
        }
    }

    return false;
}

bool TScript::customDialogHide(QString oname)
{
    QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
    while (i.hasNext()) {
        i.next();
        TCustomScriptDialog *dlg = i.value();
        if (dlg->getName().toUpper() == oname.toUpper()) {
            dlg->hideDlg();
            return true;
        }
    }

    return false;
}

bool TScript::customDialogClose(QString oname)
{
    QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
    while (i.hasNext()) {
        i.next();
        TCustomScriptDialog *dlg = i.value();
        if (dlg->getName().toUpper() == oname.toUpper()) {
            dlg->closeDlg();
            return true;
        }
    }

    return false;
}

bool TScript::customDialogSetLabel(QString dlg, QString oname, QString text)
{
  QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
  while (i.hasNext()) {
      i.next();
      TCustomScriptDialog *d = i.value();
      if (d->getName().toUpper() == dlg.toUpper()) {
          return d->setLabel(oname, text);
      }
  }

  return false;
}

bool TScript::customDialogAddItem(QString dlg, QString oname, QString text)
{
  QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
  while (i.hasNext()) {
      i.next();
      TCustomScriptDialog *d = i.value();
      if (d->getName().toUpper() == dlg.toUpper()) {
          return d->addItem(oname, text);
      }
  }

  return false;
}

bool TScript::customDialogDelItem(QString dlg, QString oname, QString index)
{
  QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
  while (i.hasNext()) {
      i.next();
      TCustomScriptDialog *d = i.value();
      if (d->getName().toUpper() == dlg.toUpper()) {
          bool ok = false;
          int idx = index.toInt(&ok);
          if (! ok)
            return false;

          return d->delItem(oname, idx);
      }
  }

  return false;
}

bool TScript::customDialogClear(QString dlg, QString oname)
{
  QHashIterator<QString,TCustomScriptDialog*> i(dialogs);
  while (i.hasNext()) {
      i.next();
      TCustomScriptDialog *d = i.value();
      if (d->getName().toUpper() == dlg.toUpper()) {
          return d->clear(oname);
      }
  }

  return false;
}
