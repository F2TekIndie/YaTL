#include "taskmodel.h"
#include "dmsthemeprovider.h"
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
        theme_ = std::make_unique<DmsThemeProvider>();
        qInfo() << "theme" << theme_->surface() << theme_->onSurface() << theme_->primary();
        engine->rootContext()->setContextProperty("taskModel", model_.get());
        engine->rootContext()->setContextProperty("dmsTheme", theme_.get());
    }
    void cleanupTestCase() {
        model_.reset();
        theme_.reset();
        store_.reset();
    }
private:
    QTemporaryDir directory_;
    std::unique_ptr<TaskStore> store_;
    std::unique_ptr<TaskModel> model_;
    std::unique_ptr<DmsThemeProvider> theme_;
};
QUICK_TEST_MAIN_WITH_SETUP(yatl_ui, Setup)
#include "main.moc"
