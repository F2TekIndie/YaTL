include(../../common.pri)
TEMPLATE = app
QT = core sql
TARGET = yatlctl
SOURCES = main.cpp
LIBS += -L$$YATL_BUILD/lib -lyatlcore
PRE_TARGETDEPS += $$YATL_BUILD/lib/libyatlcore.a
target.path = /usr/local/bin
INSTALLS += target
