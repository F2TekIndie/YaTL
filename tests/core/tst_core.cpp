#include "taskmodel.h"
#include <QSqlQuery>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>
#include <stdexcept>

class CoreTest : public QObject {
    Q_OBJECT
private:
    static void seedV2(const QString &path) {
        sql(path, "CREATE TABLE tasks (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT NOT NULL, created_at TEXT NOT NULL, completed_at TEXT)");
        sql(path, "CREATE TABLE task_events (id INTEGER PRIMARY KEY AUTOINCREMENT, task_id INTEGER NOT NULL REFERENCES tasks(id), type TEXT NOT NULL CHECK(type IN ('created','completed')), timestamp TEXT NOT NULL)");
        sql(path, "CREATE INDEX events_task ON task_events(task_id,id)");
        sql(path, "INSERT INTO tasks VALUES (8,'Old task','2026-09-01','2026-09-02')");
        sql(path, "INSERT INTO task_events VALUES (10,8,'created','2026-09-01'), (11,8,'completed','2026-09-02')");
        sql(path, "PRAGMA user_version=2");
    }
    static QVariant sql(const QString &path, const QString &statement) {
        const QString name = QUuid::createUuid().toString();
        QVariant result;
        {
            auto db = QSqlDatabase::addDatabase("QSQLITE", name);
            db.setDatabaseName(path);
            if (!db.open()) qFatal("Cannot open fixture database");
            QSqlQuery query(db);
            if (!query.exec(statement)) qFatal("Fixture SQL failed: %s", qPrintable(statement));
            if (query.next()) result = query.value(0);
        }
        QSqlDatabase::removeDatabase(name);
        return result;
    }
private slots:
    void captureCompleteRestart() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const auto path = dir.filePath("nested/tasks.sqlite3");
        QString id;
        {
            TaskStore store(path);
            QCOMPARE(store.tasks().size(), 0);
            auto task = store.add("  Ship the first loop  ");
            id = task.id;
            QCOMPARE(task.title, "Ship the first loop");
            QVERIFY(!task.completed());
        }
        {
            TaskStore store(path);
            QCOMPARE(store.tasks().first().id, id);
            QVERIFY(store.complete(id));
            QVERIFY(!store.complete(id));
            QVERIFY(store.tasks().isEmpty());
        }
        {
            TaskStore store(path);
            auto tasks = store.tasks("completed");
            QCOMPARE(tasks.size(), 1);
            QCOMPARE(tasks.first().id, id);
            QVERIFY(tasks.first().completed());
            QCOMPARE(store.tasks("all").size(), 1);
        }
        QCOMPARE(sql(path, "PRAGMA user_version").toInt(), TaskStore::SchemaVersion);
        QCOMPARE(sql(path, "SELECT count(*) FROM task_events").toInt(), 2);
    }
    void validationAndLiteralTitles() {
        TaskStore store(":memory:");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add(" \n\t "));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add(QString(501, 'x')));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.complete("0"));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.complete("garbage"));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.complete("99"));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.tasks("wrong"));
        QVERIFY(store.tasks("all").isEmpty());
        const QString title = "Don't lose <b>literal</b> text; DROP TABLE tasks; 🐈";
        QCOMPARE(store.add(title).title, title);
        store.add(QString(500, 'x'));
        QCOMPARE(store.tasks().size(), 2);
        QCOMPARE(store.tasks().at(1).title, title);
    }
    void upgradesNonEmptyV1() {
        QTemporaryDir dir;
        const auto path = dir.filePath("v1.sqlite3");
        sql(path, "CREATE TABLE tasks (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT NOT NULL, created_at TEXT NOT NULL, completed_at TEXT)");
        sql(path, "INSERT INTO tasks VALUES (7, 'Existing open task', '2026-09-01T10:00:00Z', NULL), (9, 'Existing completed task', '2026-09-01T10:00:00Z', '2026-09-02T10:00:00Z')");
        sql(path, "PRAGMA user_version=1");
        {
            TaskStore store(path);
            QCOMPARE(store.tasks().first().id, "7");
            QCOMPARE(store.tasks("completed").first().id, "9");
            QVERIFY(store.add("New task").id.toInt() > 9);
        }
        QCOMPARE(sql(path, "PRAGMA user_version").toInt(), TaskStore::SchemaVersion);
        QCOMPARE(sql(path, "SELECT count(*) FROM task_events").toInt(), 4);
    }
    void rejectsFutureSchemaWithoutChangingData() {
        QTemporaryDir dir;
        const auto path = dir.filePath("future.sqlite3");
        { TaskStore store(path); store.add("Keep me"); }
        sql(path, "PRAGMA user_version=999");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, TaskStore{path});
        QCOMPARE(sql(path, "PRAGMA user_version").toInt(), 999);
        QCOMPARE(sql(path, "SELECT title FROM tasks").toString(), "Keep me");
    }
    void migrationFailureRollsBack() {
        QTemporaryDir dir;
        const auto path = dir.filePath("broken-v1.sqlite3");
        sql(path, "CREATE TABLE tasks (id INTEGER PRIMARY KEY, title TEXT, created_at TEXT, completed_at TEXT)");
        sql(path, "INSERT INTO tasks VALUES (1, 'Keep me', '2026-09-01', NULL)");
        sql(path, "CREATE INDEX tasks_completion ON tasks(id)");
        sql(path, "PRAGMA user_version=1");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, TaskStore{path});
        QCOMPARE(sql(path, "PRAGMA user_version").toInt(), 1);
        QCOMPARE(sql(path, "SELECT title FROM tasks").toString(), "Keep me");
        QCOMPARE(sql(path, "SELECT count(*) FROM sqlite_master WHERE name='task_events'").toInt(), 0);
    }
    void failedEventRollsBackTaskWrite() {
        QTemporaryDir dir;
        const auto path = dir.filePath("atomic.sqlite3");
        TaskStore store(path);
        const auto id = store.add("Keep open").id;
        sql(path, "CREATE TRIGGER reject_event BEFORE INSERT ON task_events BEGIN SELECT RAISE(ABORT, 'injected failure'); END");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add("Must roll back"));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.complete(id));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.edit(id, "Must not change", "Note", {}));
        QCOMPARE(store.tasks().size(), 1);
        QCOMPARE(store.tasks().first().id, id);
        QCOMPARE(store.tasks().first().title, "Keep open");
        QCOMPARE(sql(path, "SELECT count(*) FROM task_events").toInt(), 1);
    }
    void projectEditReopenRestartLoop() {
        QTemporaryDir dir;
        const auto path = dir.filePath("projects.sqlite3");
        QString taskId, projectId;
        {
            TaskStore store(path);
            projectId = store.addProject("  Release  ").id;
            taskId = store.add("Draft", projectId).id;
            store.add("Inbox only");
            QCOMPARE(store.tasks("open", {}).size(), 1);
            QCOMPARE(store.tasks("open", projectId).first().id, taskId);
            QVERIFY(store.edit(taskId, "  Publish  ", "Line one\n<b>literal</b>", projectId));
            QVERIFY(!store.edit(taskId, "Publish", "Line one\n<b>literal</b>", projectId));
            QVERIFY(store.complete(taskId));
            QVERIFY(store.reopen(taskId));
            QVERIFY(!store.reopen(taskId));
        }
        {
            TaskStore store(path);
            QCOMPARE(store.projects().first().name, "Release");
            const auto task = store.tasks("open", projectId).first();
            QCOMPARE(task.title, "Publish");
            QCOMPARE(task.note, "Line one\n<b>literal</b>");
            QVERIFY(store.edit(taskId, task.title, task.note, {}));
            QVERIFY(store.tasks("all", projectId).isEmpty());
            QCOMPARE(store.tasks("open", {}).size(), 2);
        }
        QCOMPARE(sql(path, "SELECT count(*) FROM task_events WHERE task_id=" + taskId).toInt(), 5);
        const auto before = QJsonDocument::fromJson(sql(path, "SELECT previous_value FROM task_events WHERE type='edited' ORDER BY id LIMIT 1").toByteArray()).object();
        QCOMPARE(before.value("title").toString(), "Draft");
        QCOMPARE(before.value("project_id").toString(), projectId);
    }
    void projectAndEditValidation() {
        TaskStore store(":memory:");
        const auto project = store.addProject("Alpha");
        const auto task = store.add("Original");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.addProject(" alpha "));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.addProject(" "));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.addProject(QString(121, 'x')));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add("Task", "999"));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.edit(task.id, " ", "", project.id));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.edit(task.id, "Changed", "", "999"));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.edit(task.id, "Changed", QString(100001, 'x'), {}));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.edit("999", "Changed", "", {}));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.reopen("999"));
        QCOMPARE(store.tasks().first().title, "Original");
        QCOMPARE(store.projects().size(), 1);
    }
    void upgradesV2AndPreservesEventIds() {
        QTemporaryDir dir;
        const auto path = dir.filePath("v2.sqlite3");
        seedV2(path);
        {
            TaskStore store(path);
            const auto old = store.tasks("completed", {}).first();
            QCOMPARE(old.id, "8");
            QVERIFY(old.note.isEmpty());
            QVERIFY(old.projectId.isEmpty());
            QVERIFY(store.reopen(old.id));
        }
        QCOMPARE(sql(path, "SELECT type FROM task_events WHERE id=11").toString(), "completed");
        QCOMPARE(sql(path, "SELECT id FROM task_events WHERE type='reopened'").toInt(), 12);
        QCOMPARE(sql(path, "PRAGMA user_version").toInt(), TaskStore::SchemaVersion);
    }
    void v3FailureRollsBackColumnsAndHistory() {
        QTemporaryDir dir;
        const auto path = dir.filePath("broken-v2.sqlite3");
        seedV2(path);
        sql(path, "CREATE INDEX tasks_project ON tasks(id)");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, TaskStore{path});
        QCOMPARE(sql(path, "PRAGMA user_version").toInt(), 2);
        QCOMPARE(sql(path, "SELECT count(*) FROM pragma_table_info('tasks') WHERE name='project_id'").toInt(), 0);
        QCOMPARE(sql(path, "SELECT count(*) FROM task_events").toInt(), 2);
        QCOMPARE(sql(path, "SELECT count(*) FROM sqlite_master WHERE name='projects'").toInt(), 0);
    }
    void modelSeesOtherConnectionsAndRecovers() {
        QTemporaryDir dir;
        const auto path = dir.filePath("shared.sqlite3");
        TaskStore store(path);
        TaskModel model(store);
        QVERIFY(!model.add("  "));
        QVERIFY(!model.error().isEmpty());
        QVERIFY(model.add("From UI"));
        QVERIFY(model.error().isEmpty());
        TaskStore external(path);
        const auto id = external.add("From CLI").id;
        QTRY_COMPARE_WITH_TIMEOUT(model.count(), 2, 3000);
        QCOMPARE(model.data(model.index(0), TaskModel::TitleRole).toString(), "From CLI");
        external.complete(id);
        QTRY_COMPARE_WITH_TIMEOUT(model.count(), 1, 3000);
        model.setFilter("completed");
        QCOMPARE(model.count(), 1);
        QCOMPARE(model.data(model.index(0), TaskModel::IdRole).toString(), id);
        const auto projectId = external.addProject("External project").id;
        QTRY_COMPARE_WITH_TIMEOUT(model.projects().size(), 2, 3000);
        model.setProjectId(projectId);
        QCOMPARE(model.count(), 0);
        external.edit(id, "Moved from CLI", "External note", projectId);
        QTRY_COMPARE_WITH_TIMEOUT(model.count(), 1, 3000);
        QCOMPARE(model.data(model.index(0), TaskModel::NoteRole).toString(), "External note");
        model.setProjectId("missing");
        QCOMPARE(model.projectId(), projectId);
        QVERIFY(!model.error().isEmpty());
    }
};
QTEST_GUILESS_MAIN(CoreTest)
#include "tst_core.moc"
