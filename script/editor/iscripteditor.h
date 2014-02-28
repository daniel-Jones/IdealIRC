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

#ifndef ISCRIPTEDITOR_H
#define ISCRIPTEDITOR_H

#include <QDialog>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QHash>

#include "config.h"
#include "editorwidget.h"
#include "tscripteditorhighlighter.h"

namespace Ui {
class IScriptEditor;
}

typedef struct T_FILE {
    bool modified;
    QStandardItem *item;
    QByteArray *text;
} file_t;

class IScriptEditor : public QDialog
{
    Q_OBJECT

public:
    explicit IScriptEditor(QWidget *parent, QString script, config *cfg);
    ~IScriptEditor();

private:
    Ui::IScriptEditor *ui;
    QString scriptFile;
    EditorWidget editor;
    TScriptEditorHighlighter *highlight;
    config *conf;
    QString current;
    QItemSelectionModel *selection;
    QStandardItemModel treeModel;
    QStandardItem *firstItem;
    QHash<QString,file_t> files;
    bool ignoreNextTextChange;
    bool ignoreNextRowChange;

    void saveFile(QString filename); // Save given file
    void saveAll(); // Save all files
    void store(QString file); // Store to internal memory
    void load(QString file, bool select = false); // Load from internal memory
    void setupTreeView(); // Use this to (re)load the entire tree-view.
    void setupTreeView(QStandardItem *parent); // used to fill in children
    QString setRelativePath(QString folder, QString file);
    void setBold(QStandardItem *item);
    void unsetBold(QStandardItem *item);


private slots:
    void currentRowChanged(const QModelIndex &current, const QModelIndex &previous);
    void textChanged();
    void on_btnSave_clicked();
    void on_btnSaveAll_clicked();
};


#endif // ISCRIPTEDITOR_H
