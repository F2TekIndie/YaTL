include(../../common.pri)
TEMPLATE = lib
CONFIG += staticlib
QT = core network sql
TARGET = yatlcore
DESTDIR = $$YATL_BUILD/lib
HEADERS = taskstore.h taskmodel.h desktopipc.h
SOURCES = taskstore.cpp taskmodel.cpp desktopipc.cpp
