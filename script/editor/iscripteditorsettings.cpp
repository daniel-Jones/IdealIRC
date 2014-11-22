#include "iscripteditorsettings.h"
#include "ui_iscripteditorsettings.h"
#include <QPainter>
#include <QImage>
#include <qdebug.h>

ColorPicker::ColorPicker(QWidget *parent) :
    QDialog(parent),
    firstShow(true)
{
    colormap.load(":/other/gfx/colorpicker.png");
}

void ColorPicker::showEvent(QShowEvent *)
{
    if (! firstShow)
        return;
    firstShow = false;

    setWindowTitle(tr("Pick a color"));
    setGeometry(200, 200, 310, 330);
    setMaximumSize(310, 330);
    setMinimumSize(310, 330);
}

void ColorPicker::paintEvent(QPaintEvent *)
{
    QPainter paint(this);
    paint.drawPixmap(0, 0, colormap);
}

void ColorPicker::mouseReleaseEvent(QMouseEvent *e)
{
    emit colorPicked(colormap.toImage().pixel(e->x(), e->y()));
}

IScriptEditorSettings::IScriptEditorSettings(config *cfg, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::IScriptEditorSettings),
    conf(cfg),
    cpick(this)
{
    ui->setupUi(this);

    connect(&cpick, SIGNAL(colorPicked(QColor)),
            this, SLOT(colorPicked(QColor)));

    editorLineHighlight = conf->editorLineHighlight;
    editorFunctionDef = conf->editorFunctionDef;
    editorMetaKeyword = conf->editorMetaKeyword;
    editorKeyword = conf->editorKeyword;
    editorWindow = conf->editorWindow;
    editorVariable = conf->editorVariable;
    editorComment = conf->editorComment;
    editorFontName = conf->editorFontName;
    editorFontSize = conf->editorFontSize;

    ui->fontname->setCurrentFont( conf->editorFontName );
    ui->fontsize->setValue( conf->editorFontSize );

    setButtonColor(conf->editorLineHighlight);
    selectedColor = ui->LineHighlight->objectName();

    connect(&colorRadioSignals, SIGNAL(mapped(QString)),
            this, SLOT(colorRadioButton_clicked(QString)));

    connect(ui->LineHighlight, SIGNAL(clicked()),
            &colorRadioSignals, SLOT(map()));
    connect(ui->FunctionDef, SIGNAL(clicked()),
            &colorRadioSignals, SLOT(map()));
    connect(ui->MetaKeyword, SIGNAL(clicked()),
            &colorRadioSignals, SLOT(map()));
    connect(ui->Keyword, SIGNAL(clicked()),
            &colorRadioSignals, SLOT(map()));
    connect(ui->Window, SIGNAL(clicked()),
            &colorRadioSignals, SLOT(map()));
    connect(ui->Variable, SIGNAL(clicked()),
            &colorRadioSignals, SLOT(map()));
    connect(ui->Comment, SIGNAL(clicked()),
            &colorRadioSignals, SLOT(map()));

    colorRadioSignals.setMapping(ui->LineHighlight, ui->LineHighlight->objectName());
    colorRadioSignals.setMapping(ui->FunctionDef, ui->FunctionDef->objectName());
    colorRadioSignals.setMapping(ui->MetaKeyword, ui->MetaKeyword->objectName());
    colorRadioSignals.setMapping(ui->Keyword, ui->Keyword->objectName());
    colorRadioSignals.setMapping(ui->Window, ui->Window->objectName());
    colorRadioSignals.setMapping(ui->Variable, ui->Variable->objectName());
    colorRadioSignals.setMapping(ui->Comment, ui->Comment->objectName());
}

IScriptEditorSettings::~IScriptEditorSettings()
{
    delete ui;
}

QColor IScriptEditorSettings::invertColor(QColor c)
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

void IScriptEditorSettings::setButtonColor(QColor color)
{
    ui->colorbtn->setStyleSheet(QString("color: %1; background-color: %2;")
                                .arg(invertColor(color).name())
                                .arg(color.name()));
}

void IScriptEditorSettings::on_saveButton_clicked()
{
    conf->fontName = ui->fontname->currentFont().toString();
    conf->fontSize = ui->fontsize->value();

    conf->editorLineHighlight = editorLineHighlight;
    conf->editorFunctionDef = editorFunctionDef;
    conf->editorMetaKeyword = editorMetaKeyword;
    conf->editorKeyword = editorKeyword;
    conf->editorWindow = editorWindow;
    conf->editorVariable = editorVariable;
    conf->editorComment = editorComment;

    conf->save();

    emit settingsSaved();
}

void IScriptEditorSettings::colorRadioButton_clicked(QString objName)
{
    if (objName == "LineHighlight") {
        setButtonColor(editorLineHighlight);
        selectedColor = ui->LineHighlight->objectName();
    }
    if (objName == "FunctionDef") {
        setButtonColor(editorFunctionDef);
        selectedColor = ui->FunctionDef->objectName();
    }
    if (objName == "MetaKeyword") {
        setButtonColor(editorMetaKeyword);
        selectedColor = ui->MetaKeyword->objectName();
    }
    if (objName == "Keyword") {
        setButtonColor(editorKeyword);
        selectedColor = ui->Keyword->objectName();
    }
    if (objName == "Window") {
        setButtonColor(editorWindow);
        selectedColor = ui->Window->objectName();
    }
    if (objName == "Variable") {
        setButtonColor(editorVariable);
        selectedColor = ui->Variable->objectName();
    }
    if (objName == "Comment") {
        setButtonColor(editorComment);
        selectedColor = ui->Comment->objectName();
    }
}

void IScriptEditorSettings::on_colorbtn_clicked()
{
    cpick.show();

    QRect geo = geometry();
    geo.setTopLeft(geo.topRight());
    cpick.setGeometry(geo);
}

void IScriptEditorSettings::colorPicked(QColor color)
{
    if (selectedColor == "LineHighlight") {
        editorLineHighlight = color;
        setButtonColor(editorLineHighlight);
    }
    if (selectedColor == "FunctionDef") {
        editorFunctionDef = color;
        setButtonColor(editorFunctionDef);
    }
    if (selectedColor == "MetaKeyword") {
        editorMetaKeyword = color;
        setButtonColor(editorMetaKeyword);
    }
    if (selectedColor == "Keyword") {
        editorKeyword = color;
        setButtonColor(editorKeyword);
    }
    if (selectedColor == "Window") {
        editorWindow = color;
        setButtonColor(editorWindow);
    }
    if (selectedColor == "Variable") {
        editorVariable = color;
        setButtonColor(editorVariable);
    }
    if (selectedColor == "Comment") {
        editorComment = color;
        setButtonColor(editorComment);
    }
}
