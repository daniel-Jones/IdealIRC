#ifndef ISCRIPTEDITORSETTINGS_H
#define ISCRIPTEDITORSETTINGS_H

#include <QDialog>
#include <QSignalMapper>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QPixmap>
#include <QShowEvent>
#include "config.h"

namespace Ui {
class IScriptEditorSettings;
}

class ColorPicker : public QDialog
{
    Q_OBJECT

public:
    explicit ColorPicker(QWidget *parent = 0);

protected:
    void showEvent(QShowEvent *);
    void paintEvent(QPaintEvent *);
    void mouseReleaseEvent(QMouseEvent *e);

private:
    bool firstShow;
    QPixmap colormap;

signals:
    void colorPicked(QColor color);
};

class IScriptEditorSettings : public QDialog
{
    Q_OBJECT

public:
    explicit IScriptEditorSettings(config *cfg, QWidget *parent = 0);
    ~IScriptEditorSettings();

private slots:
    void on_saveButton_clicked();
    void colorRadioButton_clicked(QString objName);
    void on_colorbtn_clicked();
    void colorPicked(QColor color);

private:
    Ui::IScriptEditorSettings *ui;
    config *conf;
    QColor invertColor(QColor c);
    void setButtonColor(QColor color);

    // Copy of configs
    QColor editorLineHighlight;
    QColor editorFunctionDef;
    QColor editorMetaKeyword;
    QColor editorKeyword;
    QColor editorWindow;
    QColor editorVariable;
    QColor editorComment;
    QString editorFontName;
    int editorFontSize;

    // Group of radio slots directed to correct
    // color to switch and change
    QSignalMapper colorRadioSignals;
    QString selectedColor;

    ColorPicker cpick;

signals:
    void settingsSaved();
};

#endif // ISCRIPTEDITORSETTINGS_H
