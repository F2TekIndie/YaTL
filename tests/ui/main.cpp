#include "taskmodel.h"
#include <QQmlContext>
#include <QQmlEngine>
#include <QTemporaryDir>
#include <QtQuickTest/quicktest.h>
#include <memory>

class Setup : public QObject {
    Q_OBJECT
public slots:
    void qmlEngineAvailable(QQmlEngine *engine) {
        store_ = std::make_unique<TaskStore>(directory_.filePath("ui.sqlite3"));
        model_ = std::make_unique<TaskModel>(*store_);
        engine->rootContext()->setContextProperty("taskModel", model_.get());
    }
    void cleanupTestCase() {
        model_.reset();
        store_.reset();
    }
private:
    QTemporaryDir directory_;
    std::unique_ptr<TaskStore> store_;
    std::unique_ptr<TaskModel> model_;
};
QUICK_TEST_MAIN_WITH_SETUP(yatl_ui, Setup)
#include "main.moc"
