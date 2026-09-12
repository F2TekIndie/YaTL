#include "taskmodel.h"
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QTextStream>
#include <exception>

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("yatl");
    app.setApplicationVersion("0.2.0");
    app.setOrganizationName("YaTL");
    app.setDesktopFileName("org.yatl.YaTL");
    QCommandLineParser parser;
    parser.setApplicationDescription("YaTL — local projects and tasks");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"database", "Use a specific SQLite database (for testing).", "path", TaskStore::defaultPath()});
    parser.process(app);
    try {
        TaskStore store(parser.value("database"));
        TaskModel model(store);
        QQuickStyle::setStyle("Fusion");
        QQmlApplicationEngine engine;
        engine.rootContext()->setContextProperty("taskModel", &model);
        engine.load(QUrl("qrc:/qml/Main.qml"));
        if (engine.rootObjects().isEmpty()) return 1;
        return app.exec();
    } catch (const std::exception &e) {
        QTextStream(stderr) << "YaTL: " << e.what() << '\n';
        return 1;
    }
}
