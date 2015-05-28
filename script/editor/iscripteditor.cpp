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

#include <QFile>
#include <QMessageBox>
#include <QDebug>
#include <QHashIterator>
#include <QScrollBar>

#include "iscripteditor.h"
#include "ui_iscripteditor.h"
#include "script/tscriptparent.h"

IScriptEditor::IScriptEditor(QWidget *parent, TScript *s, config *cfg) :
    QDialog(parent),
    ui(new Ui::IScriptEditor),
    script(s),
    scriptFile(s->getPath()),
    scriptName(s->getName()),
    //editor(this, cfg),
    conf(cfg),
    current(s->getPath()),
    currentEditor(nullptr),
    selection(NULL),
    ignoreNextTextChange(false),
    ignoreNextRowChange(false),
    settings(conf, this),
    explorer(this, s),
    firstShow(true)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Window);

    connect(&settings, SIGNAL(settingsSaved()),
            this, SLOT(settingsSaved()));

    connect(&explorer, SIGNAL(findFunction(QString)),
            this, SLOT(findFunction(QString)));

    frameLayout = new QHBoxLayout;
    frameLayout->setMargin(0);
    ui->frame->setLayout(frameLayout);
}

IScriptEditor::~IScriptEditor()
{
    QHashIterator<QString,file_t> i(files);
    while (i.hasNext()) {
        i.next();
        i.value().editor->deleteLater();
        i.value().highlight->deleteLater();
    }

    selection->deleteLater();
    treeModel.deleteLater();
    delete ui;
}

void IScriptEditor::saveFile(QString filename, bool reload)
{
    file_t ft = getFileStruct(filename);
    unsetBold(ft.item);

    if (! ft.modified)
        return;

    QFile file( setRelativePath(scriptFile,filename) );

    if (! file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::question(this, tr("Unable to save file"),
                             tr("Could not open file '%1' for writing.")
                                .arg(filename));
        return;
    }

    QByteArray data;
    data.append(ft.editor->toPlainText());

    file.write(data);
    file.close();

    if (reload) {
        script->getScriptParent()->reloadScript( script->getName() );
        explorer.rebuildMetaModels();
        explorer.rebuildFunctionModel();
    }
}

void IScriptEditor::saveAll()
{
    QHashIterator<QString,file_t> i(files);
    while (i.hasNext()) {
        i.next();
        QString file = i.key();
        file_t ft = getFileStruct(file);

        if (ft.modified == true) {
            saveFile(file, false); // do not reload until we reach the end of this function.
            ft.modified = false;
        }

        files.insert(file, ft); // Update entry
    }

    script->getScriptParent()->reloadScript( script->getName() );
    explorer.rebuildMetaModels();
    explorer.rebuildFunctionModel();
    setupTreeView(); // Reload in case meta-includes are changed.
}

void IScriptEditor::load(QString file, bool select, bool loadAfterSave)
{ // Load from internal memory
    file_t ft = getFileStruct(file);
    QModelIndex index = ft.item->index();

    // hiding the previous editor
    currentEditor->hide();
    frameLayout->removeWidget(currentEditor);

    // setting new current file and editor
    current = file;
    currentEditor = files.value(current).editor;

    // show current editor
    frameLayout->addWidget(currentEditor);
    currentEditor->show();
}

void IScriptEditor::setupTreeView()
{
    ignoreNextTextChange = true;

    disconnect(selection);

    treeModel.clear();
    treeModel.setHorizontalHeaderLabels(QStringList()<<"Included files");

    firstItem = new QStandardItem(scriptFile);
    firstItem->setToolTip(scriptFile);
    setupTreeView(firstItem);

    treeModel.appendRow(firstItem);

    ui->treeView->setModel(&treeModel);
    ui->treeView->expandAll();

    selection = ui->treeView->selectionModel();

    connect(selection, SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentRowChanged(QModelIndex,QModelIndex)));

    load(current, true);
}

void IScriptEditor::setupTreeView(QStandardItem *parent)
{
    QFile file(setRelativePath(scriptFile, parent->text()));

    QByteArray data;

    if (! file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        int q = QMessageBox::question(this, tr("Unable to open file"),
                                      tr("Unable to open '%1'\r\nAttempt to create it?")
                                       .arg(parent->text()),
                                      QMessageBox::Yes | QMessageBox::No);

        if (q == QMessageBox::Yes) {

            if (! file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QMessageBox::question(this, tr("Unable to create file"),
                                      tr("Unable to create file '%1'")
                                        .arg(parent->text()));

                file_t ft = getFileStruct(parent->text());
                ft.modified = false;
                ft.item = parent;
                files.insert(parent->text(), ft);
                return;
            }
            else if (files.contains(parent->text())) {
                QByteArray d;
                d.append( files.value(parent->text()).editor->toPlainText() );
                file.write( d );
                file.close();
            }
        }

        file_t ft = getFileStruct(parent->text());
        ft.modified = false;
        ft.item = parent;

        files.insert(parent->text(), ft);
    }
    else {
        data = file.readAll();
        file.close();

        ignoreNextTextChange = true;

        file_t ft = getFileStruct(parent->text());
        ft.modified = false;
        ft.item = parent;

        if (ft.editor->toPlainText().isEmpty())
            ft.editor->setPlainText( data );

        files.insert(parent->text(), ft);

        if (currentEditor == nullptr) {
            // first time showing the editor, show the main script file.
            currentEditor = ft.editor;
            currentEditor->show();

            QList<int> size;
            size << 150 << 600;
            ui->splitter->setSizes(size);
        }

    }

    enum { st_find=0, st_meta, st_include };
    /* States:
     * * Find: Finding the meta block
     * * Meta: We found meta, now find include keywords until next }.
     * * Include: Reading the include path until newline
     */
    bool comment = false; // If this is set to true, everything is ignored until newline
    int state = st_find;
    QString keyword;
    for (int i = 0; i <= data.length()-1; i++) {
        QChar c(data[i]);

        if (c == ';')
            comment = true;

        if ((comment) && (c == '\n')) {
            comment = false;
            keyword.clear(); // Make sure it's empty for next line.
            continue;
        }

        if (comment)
            continue;

        if (state == st_find) {
            if (((c == ' ') || c == '\n') && (! keyword.isEmpty())) {
                if ((keyword.toUpper() == "SCRIPT") ||
                   (keyword.toUpper() == "META")) {
                    // This should be the first keyword the script contains.

                    // skip until the {
                    for (; i <= data.length()-1; i++) {
                        if (data[i] == '{') {
                            keyword.clear();
                            state = st_meta;
                            break;
                        }
                    }
                }
                else
                    return; // No meta in this script.
            }
            else if ((c == '\n') && (keyword.isEmpty()))
                continue;
            else
                keyword += c;

            continue;
        }

        if (state == st_meta) {
            if (c == ' ') {
                if (keyword.isEmpty())
                    continue;

                if (keyword.toUpper() == "INCLUDE") {
                    // Found include file...
                    state = st_include;
                    keyword.clear();
                    continue;
                }
                else
                    keyword.clear();
            }
            else if (c == '}')
                return;
            else if (c == '\n')
                continue;
            else
                keyword += c;
        }

        if (state == st_include) {
            if (c == '\n') {
                if (keyword.isEmpty()) // empty include set, just ignore this.
                    state = st_meta;
                else {
                    QStandardItem *item = new QStandardItem(keyword);
                    item->setToolTip(keyword);
                    parent->appendRow(item);
                    setupTreeView(item);

                    keyword.clear();
                    state = st_meta;
                }
            }
            else
                keyword += c;
        }
    }
}

QString IScriptEditor::setRelativePath(QString folder, QString file)
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
#ifdef Q_OS_WIN32
    if (file.mid(1,2) == ":/") {
      // include is a full path.
      dir.clear();
      scriptFullpath = true;
    }
#else
      if (file.at(0) == '/') {
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

void IScriptEditor::setBold(QStandardItem *item)
{
    QFont font = item->font();
    font.setBold(true);
    item->setFont(font);
}

void IScriptEditor::unsetBold(QStandardItem *item)
{
    QFont font = item->font();
    font.setBold(false);
    item->setFont(font);
}

void IScriptEditor::currentRowChanged(const QModelIndex &current, const QModelIndex &previous)
{
    if (ignoreNextRowChange) {
        ignoreNextRowChange = false;
        return;
    }

    load( current.data().toString() );
}

void IScriptEditor::textChanged()
{
    if (ignoreNextTextChange) {
        ignoreNextTextChange = false;
        return;
    }

    file_t ft = getFileStruct(current);
    ft.modified = true;
    files.insert(current, ft);

    setBold(ft.item);
}

void IScriptEditor::settingsSaved()
{
    QFont f(conf->editorFontName);
    f.setPixelSize(conf->editorFontSize);

    QHashIterator<QString, file_t> i(files);
    while (i.hasNext()) {
        i.next();
        ignoreNextTextChange = true;
        EditorWidget *editor = i.value().editor;
        TScriptEditorHighlighter *highlight = i.value().highlight;

        editor->setFont(f);

        editor->setStyleSheet( QString("QPlainTextEdit { background-color: %1; color: %2; }")
                               .arg( conf->editorBackground.name() )
                               .arg( conf->editorText.name() )
                              );

        highlight->rehighlight();
    }
}

void IScriptEditor::on_btnSave_clicked()
{
    // save current
    saveFile(current);
    setupTreeView();
}

void IScriptEditor::on_btnSaveAll_clicked()
{
    // save all
    saveAll();
}

void IScriptEditor::showEvent(QShowEvent *)
{
    if (! firstShow)
        return;

    firstShow = false;

    setupTreeView();
}

void IScriptEditor::closeEvent(QCloseEvent *e)
{
    // See if we got any modified files...
    QHashIterator<QString,file_t> i(files);
    bool modified = false;
    while (i.hasNext()) {
        i.next();
        if (i.value().modified)
            modified = true;
        if (modified)
            break;
    }

    if (modified) {
        int b = QMessageBox::question(this, tr("Unsaved changes"),
                                      tr("Do you want to save the changes?"),
                                      QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel);

        if (b == QMessageBox::Yes)
            saveAll();
        if (b == QMessageBox::Cancel)
            e->ignore();
    }
}

void IScriptEditor::on_btnSettings_clicked()
{
    settings.show();
}

file_t IScriptEditor::getFileStruct(QString filename)
{ // This one returns relevant file_t if we have any, or make a new one and return that.
  // If it creates, it does NOT store it to the 'files' hash table!

    if (files.contains(filename))
        return files.value(filename);


    file_t f;
    f.editor = new EditorWidget(ui->frame, conf);
    f.highlight = new TScriptEditorHighlighter(f.editor->document(), conf);

     connect(f.editor, SIGNAL(textChanged()),
             this, SLOT(textChanged()));

     f.editor->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );

     f.editor->hide();

    return f;
}

void IScriptEditor::on_btnEE_clicked()
{
    explorer.show();
}

void IScriptEditor::findFunction(QString name)
{
    QString patt = QString("function %1(").arg(name);

    QHashIterator<QString,file_t> i(files);
    while (i.hasNext()) {
        i.next();
        QString filename = i.key();
        file_t f = i.value();

        if (f.editor->find(patt)) {
            if (filename != current)
                load(filename, true);
            break;
        }
    }
}
