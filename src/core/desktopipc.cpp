#include "desktopipc.h"

#include <QCryptographicHash>
#include <QFileInfo>
#include <QLocalSocket>

QString DesktopIpc::serverName(const QString &databasePath, bool quickCapture) {
    const QString canonical = databasePath == ":memory:"
        ? databasePath
        : QFileInfo(databasePath).absoluteFilePath();
    const auto digest = QCryptographicHash::hash(canonical.toUtf8(), QCryptographicHash::Sha256)
                            .toHex().left(16);
    return QStringLiteral("yatl-%1-%2")
        .arg(quickCapture ? QStringLiteral("capture") : QStringLiteral("main"),
             QString::fromLatin1(digest));
}

QByteArray DesktopIpc::request(const QString &serverName, const QByteArray &command, int timeoutMs) {
    QLocalSocket socket;
    socket.connectToServer(serverName, QIODevice::ReadWrite);
    if (!socket.waitForConnected(timeoutMs)) return {};
    socket.write(command + '\n');
    if (!socket.waitForBytesWritten(timeoutMs)) return {};
    if (!socket.waitForReadyRead(timeoutMs)) return {};
    return socket.readAll().trimmed();
}
