#include "desktopipc.h"
#include "taskmodel.h"
#include "dmsthemeprovider.h"
#include "notificationservice.h"
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <qqml.h>
#include <QQuickStyle>
#include <QTextStream>
#include <QTimer>
#include <QWindow>
#include <exception>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("yatl");
    app.setApplicationVersion("0.9.0");
    app.setOrganizationName("YaTL");
    app.setDesktopFileName("org.yatl.YaTL");
    QCommandLineParser parser;
    parser.setApplicationDescription("YaTL — local projects and tasks");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"database", "Use a specific SQLite database (for testing).", "path", TaskStore::defaultPath()});
    parser.addOption({"view", "Open project, today, upcoming, or search view.", "view", "project"});
    parser.addOption({"quick-capture", "Open the compact Inbox capture window."});
    parser.process(app);
    try {
        const QString databasePath = parser.value("database");
        const QString initialView = parser.value("view");
        if (initialView != "project" && initialView != "today" && initialView != "upcoming"
            && initialView != "search")
            throw std::runtime_error("View must be project, today, upcoming, or search.");
        const bool quickCapture = parser.isSet("quick-capture");
        if (quickCapture) app.setDesktopFileName("org.yatl.YaTL.QuickCapture");
        const QString serverName = DesktopIpc::serverName(databasePath, quickCapture);
        const QByteArray initialCommand = quickCapture ? QByteArrayLiteral("focus")
                                                       : QByteArray("view:") + initialView.toUtf8();
        if (!DesktopIpc::request(serverName, initialCommand).isEmpty()) return 0;

        QLocalServer server;
        server.setSocketOptions(QLocalServer::UserAccessOption);
        bool activationAvailable = server.listen(serverName);
        if (!activationAvailable) {
            if (!DesktopIpc::request(serverName, initialCommand, 2000).isEmpty()) return 0;
            QLocalServer::removeServer(serverName);
            activationAvailable = server.listen(serverName);
            if (!activationAvailable)
                qWarning().noquote() << "YaTL: local activation unavailable:" << server.errorString();
        }

        TaskStore store(databasePath);
        TaskModel model(store);
        DmsThemeProvider theme;
        const auto notifySafely = [&store] {
            try {
                NotificationService::runAsync(store, QCoreApplication::instance());
            } catch (const std::exception &) {
                // Notifications are best effort and must never bring down the UI.
            }
        };
        QTimer notificationTimer;
        notificationTimer.setInterval(60000);
        QObject::connect(&notificationTimer, &QTimer::timeout, &app, notifySafely);
        if (!quickCapture) {
            notificationTimer.start();
            QTimer::singleShot(0, &app, notifySafely);
        }
        if (!quickCapture) model.setView(initialView);
        QQuickStyle::setStyle("Fusion");
        qmlRegisterSingletonType<DmsThemeProvider>("YaTL", 1, 0, "DmsTheme",
            [](QQmlEngine *engine, QJSEngine *) -> QObject * { return new DmsThemeProvider(engine); });
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("taskModel", &model);
        engine.rootContext()->setContextProperty("dmsTheme", &theme);
        engine.load(QUrl(quickCapture ? "qrc:/qml/QuickCapture.qml" : "qrc:/qml/Main.qml"));
        if (engine.rootObjects().isEmpty()) return 1;
        auto *window = qobject_cast<QWindow *>(engine.rootObjects().first());
        QObject::connect(&server, &QLocalServer::newConnection, &app, [&] {
            while (server.hasPendingConnections()) {
                auto *socket = server.nextPendingConnection();
                QObject::connect(socket, &QLocalSocket::readyRead, socket, [&, socket] {
                    const QByteArray command = socket->readLine().trimmed();
                    if (!quickCapture && command.startsWith("view:")) {
                        const QString requested = QString::fromUtf8(command.mid(5));
                        if (requested == "project" || requested == "today" || requested == "upcoming"
                            || requested == "search")
                            model.setView(requested);
                    }
                    if (window) {
                        window->show();
                        window->raise();
                        window->requestActivate();
                    }
                    const QJsonObject response{{"ok", true},
                                               {"view", quickCapture ? "capture" : model.view()}};
                    socket->write(QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n');
                    socket->flush();
                    socket->disconnectFromServer();
                });
                QObject::connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            }
        });
        return app.exec();
    } catch (const std::exception &e) {
        QTextStream(stderr) << "YaTL: " << e.what() << '\n';
        return 1;
    }
}
