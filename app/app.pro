include(../common.pri)
TEMPLATE = app
QT = core gui network quick quickcontrols2 sql
TARGET = yatl
SOURCES = main.cpp
RESOURCES = qml.qrc
LIBS += -L$$YATL_BUILD/lib -lyatlcore
PRE_TARGETDEPS += $$YATL_BUILD/lib/libyatlcore.a
target.path = /usr/local/bin
desktop.files = ../integrations/niri/org.yatl.YaTL.desktop ../integrations/niri/org.yatl.YaTL.QuickCapture.desktop
desktop.path = /usr/local/share/applications
desktop.CONFIG += nostrip
dms.files = ../integrations/dms/YaTL
dms.path = /usr/local/share/yatl/dms
niri.files = ../integrations/niri/yatl.kdl ../integrations/niri/README.md
niri.path = /usr/local/share/yatl/niri
niri.CONFIG += nostrip
INSTALLS += target desktop dms niri
