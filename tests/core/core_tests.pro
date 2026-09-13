include(../../common.pri)
TEMPLATE = app
CONFIG += testcase no_testcase_installs
QT = core gui sql testlib
TARGET = tst_core
SOURCES = tst_core.cpp
LIBS += -L$$YATL_BUILD/lib -lyatlcore
PRE_TARGETDEPS += $$YATL_BUILD/lib/libyatlcore.a
