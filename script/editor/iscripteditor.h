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

/*! \class IScriptEditor
 *  \brief The script editor dialog.
 */

#ifndef ISCRIPTEDITOR_H
#define ISCRIPTEDITOR_H

#include <QDialog>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QItemSelectionModel>
#include <QHash>
#include <QCloseEvent>
#include <QPoint>
#include <QHBoxLayout>

#include "script/tscript.h"
#include "script/editor/iscripteditoree.h"
#include "config.h"
#include "editorwidget.h"
#include "tscripteditorhighlighter.h"
#include "iscripteditorsettings.h"

namespace Ui {
class IScriptEditor;
}

typedef struct T_FILE {
    bool modified;
    QStandardItem *item;
    EditorWidget *editor;
    TScriptEditorHighlighter *highlight;
} file_t;

class IScriptEditor : public QDialog
{
    Q_OBJECT

public:
    explicit IScriptEditor(QWidget *parent, TScript *s, config *cfg);
    ~IScriptEditor();

private:
    Ui::IScriptEditor *ui; //!< Qt Creator generated GUI class.
    TScript *script; //!< Pointer to the script we're editing. However, we don't edit directly onto this.
    QString scriptFile; //!< /path/to/script
    QString scriptName; //!< Name of the script we edit.
    config *conf; //!< Pointer to config class (iirc.ini)
    QString current; //!< Current file the editor's on
    EditorWidget *currentEditor; //!< Current editor widget we're on
    QItemSelectionModel *selection; //!< Keeps track of the selection in the included files treeview.
    QStandardItemModel treeModel; //!< Model of included files.
    QStandardItem *firstItem; //!< Top item, the main script
    QHash<QString,file_t> files; //!< Files we're editing.\n Key: /path/to/script\n Value: File
    bool ignoreNextTextChange; //!< Used when loading script to editor, so it doesn't get considered as file change.
    bool ignoreNextRowChange;
    IScriptEditorSettings settings;
    IScriptEditorEE explorer;
    bool firstShow;
    QHBoxLayout *frameLayout;

    void saveFile(QString filename, bool reload = true); // Save given file
    void saveAll(); // Save all files
    void load(QString file, bool select = false, bool loadAfterSave = false); // Load from internal memory
    void setupTreeView(); // Use this to (re)load the entire tree-view.
    void setupTreeView(QStandardItem *parent); // used to fill in children
    QString setRelativePath(QString folder, QString file);
    void setBold(QStandardItem *item); // changes made -indicator (bold text in tree view)
    void unsetBold(QStandardItem *item);

    file_t getFileStruct(QString filename);

private slots:
    void currentRowChanged(const QModelIndex &current, const QModelIndex &previous);
    void textChanged();
    void settingsSaved();
    void on_btnSave_clicked();
    void on_btnSaveAll_clicked();
    void on_btnSettings_clicked();
    void on_btnEE_clicked();
    void findFunction(QString name);

protected:
    void showEvent(QShowEvent *);
    void closeEvent(QCloseEvent *e);
};


#endif // ISCRIPTEDITOR_H
