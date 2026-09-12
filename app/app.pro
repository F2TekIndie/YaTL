include(../common.pri)
TEMPLATE = app
QT = core gui quick quickcontrols2 sql
TARGET = yatl
SOURCES = main.cpp
RESOURCES = qml.qrc
LIBS += -L$$YATL_BUILD/lib -lyatlcore
PRE_TARGETDEPS += $$YATL_BUILD/lib/libyatlcore.a
target.path = /usr/local/bin
desktop.files = ../integrations/niri/org.yatl.YaTL.desktop
desktop.path = /usr/local/share/applications
INSTALLS += target desktop
