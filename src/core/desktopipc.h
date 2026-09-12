#pragma once

#include <QByteArray>
#include <QString>

namespace DesktopIpc {
QString serverName(const QString &databasePath, bool quickCapture = false);
QByteArray request(const QString &serverName, const QByteArray &command, int timeoutMs = 750);
}
