include(../../common.pri)
TEMPLATE = lib
CONFIG += staticlib
QT = core gui network sql
TARGET = yatlcore
DESTDIR = $$YATL_BUILD/lib
HEADERS = taskstore.h taskmodel.h desktopipc.h notificationservice.h dmsthemeprovider.h
SOURCES = taskstore.cpp taskmodel.cpp desktopipc.cpp notificationservice.cpp dmsthemeprovider.cpp
