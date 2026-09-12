include(../../common.pri)
TEMPLATE = app
CONFIG += testcase no_testcase_installs
QT = core gui qml quick quickcontrols2 qmltest sql
TARGET = tst_ui
SOURCES = main.cpp
LIBS += -L$$YATL_BUILD/lib -lyatlcore
PRE_TARGETDEPS += $$YATL_BUILD/lib/libyatlcore.a
DEFINES += QUICK_TEST_SOURCE_DIR=\"\\\"$$PWD\\\"\"
