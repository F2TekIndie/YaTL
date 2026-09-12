include(../../common.pri)
TEMPLATE = lib
CONFIG += staticlib
QT = core sql
TARGET = yatlcore
DESTDIR = $$YATL_BUILD/lib
HEADERS = taskstore.h taskmodel.h
SOURCES = taskstore.cpp taskmodel.cpp
