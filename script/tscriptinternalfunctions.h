#ifndef TSCRIPTINTERNALFUNCTIONS_H
#define TSCRIPTINTERNALFUNCTIONS_H

/*
  Gotta love long class names.
  This one contains all pre-defined $functions such as $calc(), $token(), etc
*/

#include <QObject>
#include <QStringList>
#include <QHash>
#include "tsockfactory.h"
#include "tcustomscriptdialog.h"

#include "exprtk/exprtk.hpp"


typedef struct T_SFILE {
  int fd;
  bool read;
  bool write;
  bool binary;
  QFile *file;
} t_sfile;

class TScriptInternalFunctions : public QObject
{
    Q_OBJECT
  public:
    explicit TScriptInternalFunctions(TSockFactory *sf, QHash<QString,int> *functionIndex, QHash<QString,TCustomScriptDialog*> *dlgs, QHash<int,t_sfile> *fl, QObject *parent = 0);
    bool runFunction(QString function, QStringList param, QString &result);
    QString calc(QString expr); // This one should go public since it also interprets string literals for calculation.

  private: /// @note ALL functions, I mean -ALL-, even those with integer results, MUST return QString.
    exprtk::symbol_table<double> st;
    exprtk::expression<double> ex;
    exprtk::parser<double> parser;
    TSockFactory *sockfactory;
    QHash<QString,int> *fnindex;
    QHash<QString,TCustomScriptDialog*> *dialogs;
    QHash<int,t_sfile> *files;
    int fdc; // file desc. counter
    uint rseed;

    QString sstr(QString text, int start, int stop = -1);
    QString token(QString text, int pos, QChar delim);
    QString rand(int lo, int hi);
};

#endif // TSCRIPTINTERNALFUNCTIONS_H
