#    IdealIRC - Internet Relay Chat client
#    Copyright (C) 2013  Tom-Andre Barstad
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License along
#    with this program; if not, write to the Free Software Foundation, Inc.,
#    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

QT       += core gui network widgets

QMAKE_CXXFLAGS += -std=c++11

TARGET = idealirc
TEMPLATE = app

RC_FILE = idealirc.rc

#DEFINES += QT_NO_CAST_FROM_ASCII

SOURCES += main.cpp\
        idealirc.cpp \
    iwin.cpp \
    tircview.cpp \
    config.cpp \
    qinifile.cpp \
    tpicturewindow.cpp \
    qmylineedit.cpp \
    qmylistwidget.cpp \
    iconnection.cpp \
    iconfig.cpp \
    iconfig/iconfiggeneral.cpp \
    servermgr.cpp \
    icommand.cpp \
    ichanconfig.cpp \
    bantablemodel.cpp \
    unsupportedmodel.cpp \
    iabout.cpp \
    iconfig/iconfigcustomize.cpp \
    iconfig/colorpickerscene.cpp

HEADERS  += idealirc.h \
    iwin.h \
    constants.h \
    tircview.h \
    config.h \
    qinifile.h \
    tpicturewindow.h \
    qmylistwidget.h \
    qmylineedit.h \
    iconnection.h \
    iconfig.h \
    iconfig/iconfiggeneral.h \
    servermgr.h \
    numerics.h \
    icommand.h \
    ichanconfig.h \
    bantablemodel.h \
    unsupportedmodel.h \
    iabout.h \
    iconfig/iconfigcustomize.h \
    iconfig/colorpickerscene.h

FORMS    += idealirc.ui \
    iwin.ui \
    iconfig.ui \
    iconfig/iconfiggeneral.ui \
    ichanconfig.ui \
    iabout.ui \
    iconfig/iconfigcustomize.ui

RESOURCES += \
    resources.qrc

OTHER_FILES += \
    TIRCView.css \
    CHANGELOG.txt \
    LICENSE.txt \
    idealirc.rc \
    README
