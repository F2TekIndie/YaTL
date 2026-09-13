#include "taskmodel.h"
#include "notificationservice.h"
#include "dmsthemeprovider.h"
#include <QFile>
#include <QDir>
#include <QSaveFile>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
#include <QUuid>
#include <QtTest>
#include <algorithm>
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
    static void seedV3(const QString &path) {
        sql(path, "CREATE TABLE projects (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL COLLATE NOCASE UNIQUE)");
        sql(path, "CREATE TABLE tasks (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT NOT NULL, created_at TEXT NOT NULL, completed_at TEXT, project_id INTEGER REFERENCES projects(id), note TEXT NOT NULL DEFAULT '')");
        sql(path, "CREATE TABLE task_events (id INTEGER PRIMARY KEY AUTOINCREMENT, task_id INTEGER NOT NULL REFERENCES tasks(id), type TEXT NOT NULL, timestamp TEXT NOT NULL, previous_value TEXT, new_value TEXT)");
        sql(path, "CREATE INDEX tasks_completion ON tasks(completed_at,id)");
        sql(path, "CREATE INDEX tasks_project ON tasks(project_id,completed_at,id)");
        sql(path, "CREATE INDEX events_task ON task_events(task_id,id)");
        sql(path, "INSERT INTO projects VALUES (4,'Existing project')");
        sql(path, "INSERT INTO tasks VALUES (7,'Older','2026-09-01',NULL,4,''),(9,'Newer','2026-09-02',NULL,4,'Keep note')");
        sql(path, "INSERT INTO task_events VALUES (12,7,'created','2026-09-01',NULL,NULL),(13,9,'created','2026-09-02',NULL,NULL)");
        sql(path, "PRAGMA user_version=3");
    }
    static void seedV4(const QString &path) {
        seedV3(path);
        sql(path, "ALTER TABLE projects ADD COLUMN color TEXT NOT NULL DEFAULT '#376548'");
        sql(path, "ALTER TABLE projects ADD COLUMN archived INTEGER NOT NULL DEFAULT 0");
        sql(path, "CREATE TABLE task_lists (id INTEGER PRIMARY KEY AUTOINCREMENT, project_id INTEGER NOT NULL REFERENCES projects(id), name TEXT NOT NULL COLLATE NOCASE, UNIQUE(project_id,name))");
        sql(path, "ALTER TABLE tasks ADD COLUMN list_id INTEGER REFERENCES task_lists(id)");
        sql(path, "ALTER TABLE tasks ADD COLUMN sort_order INTEGER NOT NULL DEFAULT 0");
        sql(path, "UPDATE tasks SET sort_order=-id");
        sql(path, "CREATE INDEX tasks_order ON tasks(project_id,completed_at,sort_order,id)");
        sql(path, "PRAGMA user_version=4");
    }
    static void seedV5(const QString &path) {
        seedV4(path);
        sql(path, "ALTER TABLE tasks ADD COLUMN scheduled_date TEXT");
        sql(path, "ALTER TABLE tasks ADD COLUMN due_date TEXT");
        sql(path, "ALTER TABLE tasks ADD COLUMN priority INTEGER NOT NULL DEFAULT 0");
        sql(path, "UPDATE tasks SET scheduled_date='2026-09-12',due_date='2026-09-13',priority=3 WHERE id=9");
        sql(path, "INSERT INTO projects(id,name,color,archived) VALUES (6,'Zulu project','#112233',0)");
        sql(path, "INSERT INTO task_lists(id,project_id,name) VALUES (6,4,'First list'),(8,4,'Second list')");
        sql(path, "PRAGMA user_version=5");
    }
    static void seedV6(const QString &path) {
        seedV5(path);
        sql(path, "ALTER TABLE projects ADD COLUMN sort_order INTEGER NOT NULL DEFAULT 0");
        sql(path, "UPDATE projects SET sort_order=CASE id WHEN 4 THEN 0 ELSE 1 END");
        sql(path, "ALTER TABLE task_lists ADD COLUMN sort_order INTEGER NOT NULL DEFAULT 0");
        sql(path, "UPDATE task_lists SET sort_order=CASE id WHEN 6 THEN 0 ELSE 1 END");
        sql(path, "ALTER TABLE tasks ADD COLUMN archived INTEGER NOT NULL DEFAULT 0");
        sql(path, "PRAGMA user_version=6");
    }
    static void seedV7(const QString &path) {
        seedV6(path);
        sql(path, "CREATE TABLE tags (id INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT NOT NULL COLLATE NOCASE UNIQUE,color TEXT NOT NULL)");
        sql(path, "CREATE TABLE task_tags (task_id INTEGER NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,tag_id INTEGER NOT NULL REFERENCES tags(id) ON DELETE CASCADE,PRIMARY KEY(task_id,tag_id))");
        sql(path, "CREATE INDEX task_tags_tag ON task_tags(tag_id,task_id)");
        sql(path, "PRAGMA user_version=7");
    }
    static void seedV8(const QString &path) {
        seedV7(path);
        sql(path, "ALTER TABLE tasks ADD COLUMN recurrence TEXT NOT NULL DEFAULT 'none'");
        sql(path, "ALTER TABLE tasks ADD COLUMN recurrence_source_id INTEGER REFERENCES tasks(id)");
        sql(path, "CREATE UNIQUE INDEX tasks_recurrence_source ON tasks(recurrence_source_id) WHERE recurrence_source_id IS NOT NULL");
        sql(path, "CREATE TABLE task_notifications (task_id INTEGER NOT NULL REFERENCES tasks(id) ON DELETE CASCADE,kind TEXT NOT NULL,date TEXT NOT NULL,notified_at TEXT NOT NULL,PRIMARY KEY(task_id,kind,date))");
        sql(path, "PRAGMA user_version=8");
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
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.archiveTask(id, true));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.archiveTask(id, true));
        QCOMPARE(store.tasks().size(), 1);
        QVERIFY(!store.task(id).archived);
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
    void projectSettingsListsOrderingArchiveRestart() {
        QTemporaryDir dir;
        const auto path = dir.filePath("managed.sqlite3");
        QString projectId, listId, firstId, secondId;
        {
            TaskStore store(path);
            projectId = store.addProject("Release").id;
            QCOMPARE(store.projects().first().color, "#376548");
            QVERIFY(store.editProject(projectId, "Release 1.0", "#A1B2C3"));
            QVERIFY(!store.editProject(projectId, "Release 1.0", "#a1b2c3"));
            QCOMPARE(store.projects().first().color, "#a1b2c3");
            listId = store.addList(projectId, "Review").id;
            QVERIFY(store.renameList(listId, "Ready"));
            firstId = store.add("First", projectId, listId).id;
            secondId = store.add("Second", projectId, listId).id;
            QCOMPARE(store.tasks("open", projectId, listId).first().id, secondId);
            QVERIFY(store.moveTask(secondId, "down", listId));
            auto ordered = store.tasks("open", projectId, listId);
            QCOMPARE(ordered.at(0).id, firstId);
            QCOMPARE(ordered.at(1).id, secondId);
            QVERIFY(!store.moveTask(firstId, "up", listId));
            QVERIFY(store.archiveProject(projectId, true));
            QVERIFY(!store.archiveProject(projectId, true));
            QVERIFY(store.projects().isEmpty());
            QCOMPARE(store.projects(true).first().archived, true);
            QVERIFY(store.tasks().isEmpty());
            QCOMPARE(store.tasks("open", projectId, listId).size(), 2);
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add("Blocked", projectId));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.edit(firstId, "Blocked", "", projectId, listId));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.complete(firstId));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.moveTask(firstId, "down", listId));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.editProject(projectId, "Blocked", "#000000"));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.addList(projectId, "Blocked"));
            QVERIFY(store.archiveProject(projectId, false));
        }
        {
            TaskStore store(path);
            QCOMPARE(store.projects().first().name, "Release 1.0");
            QCOMPARE(store.lists(projectId).first().name, "Ready");
            const auto ordered = store.tasks("open", projectId, listId);
            QCOMPARE(ordered.at(0).id, firstId);
            QCOMPARE(ordered.at(1).id, secondId);
        }
    }
    void listAndProjectManagementValidation() {
        TaskStore store(":memory:");
        const auto alpha = store.addProject("Alpha");
        const auto beta = store.addProject("Beta");
        const auto alphaList = store.addList(alpha.id, "Queue");
        const auto betaList = store.addList(beta.id, "Queue");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.editProject(alpha.id, "Beta", "#112233"));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.editProject(alpha.id, "Alpha", "red"));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.addList({}, "No project"));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.addList(alpha.id, "queue"));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.renameList(alphaList.id, " "));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add("Wrong list", alpha.id, betaList.id));
        const auto task = store.add("Correct", alpha.id, alphaList.id);
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.edit(task.id, "Wrong", "", beta.id, alphaList.id));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.moveTask(task.id, "sideways", alphaList.id));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.moveTask(task.id, "up", betaList.id));
        QCOMPARE(store.tasks("open", alpha.id, {}).size(), 0);
        QCOMPARE(store.tasks("open", alpha.id, "*").size(), 1);
    }
    void upgradesV3WithStableOrderAndRollback() {
        QTemporaryDir dir;
        const auto path = dir.filePath("v3.sqlite3");
        seedV3(path);
        {
            TaskStore store(path);
            QCOMPARE(store.projects().first().color, "#376548");
            QCOMPARE(store.tasks("open", "4").at(0).id, "9");
            QCOMPARE(store.tasks("open", "4").at(1).id, "7");
            QCOMPARE(store.tasks("open", "4").at(0).note, "Keep note");
            QCOMPARE(store.addList("4", "Backlog").id, "1");
        }
        QCOMPARE(sql(path, "PRAGMA user_version").toInt(), TaskStore::SchemaVersion);
        QCOMPARE(sql(path, "SELECT id FROM task_events ORDER BY id DESC LIMIT 1").toInt(), 13);

        const auto broken = dir.filePath("broken-v3.sqlite3");
        seedV3(broken);
        sql(broken, "CREATE INDEX tasks_order ON tasks(id)");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, TaskStore{broken});
        QCOMPARE(sql(broken, "PRAGMA user_version").toInt(), 3);
        QCOMPARE(sql(broken, "SELECT count(*) FROM pragma_table_info('tasks') WHERE name='list_id'").toInt(), 0);
        QCOMPARE(sql(broken, "SELECT count(*) FROM sqlite_master WHERE name='task_lists'").toInt(), 0);
        QCOMPARE(sql(broken, "SELECT count(*) FROM task_events").toInt(), 2);
    }
    void planningTodayUpcomingSearchAndRestart() {
        QTemporaryDir dir;
        const auto path = dir.filePath("planning.sqlite3");
        const auto today = QDate::currentDate();
        QString highId;
        {
            TaskStore store(path);
            const auto project = store.addProject("Release Search");
            const auto list = store.addList(project.id, "Review Queue");
            store.add("Undated", project.id, list.id);
            store.add("Overdue", project.id, list.id, {}, today.addDays(-1).toString(Qt::ISODate), 1);
            store.add("Scheduled today", project.id, list.id, today.toString(Qt::ISODate), {}, 2);
            highId = store.add("High tomorrow", project.id, list.id, {}, today.addDays(1).toString(Qt::ISODate), 3).id;
            store.add("Low tomorrow", project.id, {}, today.addDays(1).toString(Qt::ISODate), {}, 1);
            store.add("Beyond horizon", project.id, {}, {}, today.addDays(29).toString(Qt::ISODate), 3);
            store.add("100% literal_name", {}, {}, {}, {}, 0);
            const auto noteTask = store.add("Opaque", project.id, list.id);
            QVERIFY(store.edit(noteTask.id, noteTask.title, "Needle in notes", project.id, list.id,
                               today.toString(Qt::ISODate), today.addDays(2).toString(Qt::ISODate), 2));

            const auto todayTasks = store.tasks("open", "*", "*", "today");
            QCOMPARE(todayTasks.size(), 3);
            QCOMPARE(todayTasks.at(0).title, "Scheduled today");
            QCOMPARE(todayTasks.at(1).title, "Opaque");
            QCOMPARE(todayTasks.at(2).title, "Overdue");
            const auto upcoming = store.tasks("open", "*", "*", "upcoming");
            QCOMPARE(upcoming.size(), 3);
            QCOMPARE(upcoming.first().id, highId);
            QCOMPARE(store.tasks("all", "*", "*", "search", "needle").first().id, noteTask.id);
            QCOMPARE(store.tasks("all", "*", "*", "search", "release search").size(), 7);
            QCOMPARE(store.tasks("all", "*", "*", "search", "review queue").size(), 5);
            QCOMPARE(store.tasks("all", "*", "*", "search", "%").size(), 1);
            QCOMPARE(store.tasks("all", "*", "*", "search", "_").size(), 1);
            QVERIFY(store.tasks("all", "*", "*", "search", "   ").isEmpty());
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.tasks("open", "*", "*", "invalid"));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add("Bad", {}, {}, "2026-2-01"));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add("Bad", {}, {}, "not-a-date"));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add("Bad", {}, {}, "2026-09-12", "2026-09-11"));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add("Bad", {}, {}, {}, {}, 4));
            QVERIFY(store.complete(highId));
            QCOMPARE(store.tasks("open", "*", "*", "upcoming").size(), 2);
        }
        {
            TaskStore store(path);
            const auto task = store.task(highId);
            QCOMPARE(task.dueDate, today.addDays(1).toString(Qt::ISODate));
            QCOMPARE(task.priority, 3);
            QCOMPARE(task.projectName, "Release Search");
            QCOMPARE(task.listName, "Review Queue");
            QVERIFY(task.completed());
            QVERIFY(store.tasks("all", "*", "*", "search", "high tomorrow").first().completed());
        }
    }
    void planningViewsExcludeArchivedProjects() {
        TaskStore store(":memory:");
        const auto project = store.addProject("Hidden plan");
        store.add("Archived today", project.id, {}, QDate::currentDate().toString(Qt::ISODate), {}, 3);
        QCOMPARE(store.tasks("open", "*", "*", "today").size(), 1);
        QCOMPARE(store.tasks("all", "*", "*", "search", "archived").size(), 1);
        QVERIFY(store.archiveProject(project.id, true));
        QVERIFY(store.tasks("open", "*", "*", "today").isEmpty());
        QVERIFY(store.tasks("all", "*", "*", "search", "archived").isEmpty());
    }
    void upgradesV4PlanningFieldsAndRollsBack() {
        QTemporaryDir dir;
        const auto path = dir.filePath("v4.sqlite3");
        seedV4(path);
        {
            TaskStore store(path);
            const auto existing = store.task("9");
            QVERIFY(existing.scheduledDate.isEmpty());
            QVERIFY(existing.dueDate.isEmpty());
            QCOMPARE(existing.priority, 0);
            QCOMPARE(existing.note, "Keep note");
        }
        QCOMPARE(sql(path, "PRAGMA user_version").toInt(), TaskStore::SchemaVersion);
        QCOMPARE(sql(path, "SELECT id FROM task_events ORDER BY id DESC LIMIT 1").toInt(), 13);

        const auto broken = dir.filePath("broken-v4.sqlite3");
        seedV4(broken);
        sql(broken, "CREATE INDEX tasks_due ON tasks(id)");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, TaskStore{broken});
        QCOMPARE(sql(broken, "PRAGMA user_version").toInt(), 4);
        QCOMPARE(sql(broken, "SELECT count(*) FROM pragma_table_info('tasks') WHERE name='scheduled_date'").toInt(), 0);
        QCOMPARE(sql(broken, "SELECT count(*) FROM task_events").toInt(), 2);
    }
    void taskArchiveRestoreSearchAndRestart() {
        QTemporaryDir dir;
        const auto path = dir.filePath("archive.sqlite3");
        QString completedId;
        QString todayId;
        {
            TaskStore store(path);
            const auto project = store.addProject("Archive project");
            const auto list = store.addList(project.id, "Archive list");
            completedId = store.add("Completed archive target", project.id, list.id).id;
            todayId = store.add("Today archive target", project.id, list.id,
                                QDate::currentDate().toString(Qt::ISODate)).id;
            QVERIFY(store.complete(completedId));
            QVERIFY(store.archiveTask(completedId, true));
            QVERIFY(!store.archiveTask(completedId, true));
            QVERIFY(store.tasks("completed", project.id).isEmpty());
            const auto archived = store.tasks("archived", project.id);
            QCOMPARE(archived.size(), 1);
            QCOMPARE(archived.first().id, completedId);
            QVERIFY(archived.first().archived);
            QVERIFY(archived.first().completed());
            const auto searched = store.tasks("all", "*", "*", "search", "completed archive");
            QCOMPARE(searched.size(), 1);
            QVERIFY(searched.first().archived);
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.complete(completedId));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.reopen(completedId));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error,
                                     store.edit(completedId, "Blocked", "", project.id, list.id));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.moveTask(completedId, "up", list.id));
            QVERIFY(store.archiveTask(todayId, true));
            QVERIFY(store.tasks("open", "*", "*", "today").isEmpty());
        }
        {
            TaskStore store(path);
            QVERIFY(store.task(completedId).archived);
            QVERIFY(store.archiveTask(completedId, false));
            QVERIFY(!store.archiveTask(completedId, false));
            QCOMPARE(store.tasks("completed").first().id, completedId);
            QVERIFY(store.archiveTask(todayId, false));
            QCOMPARE(store.tasks("open", "*", "*", "today").first().id, todayId);
        }
        QCOMPARE(sql(path, "SELECT group_concat(type,',') FROM (SELECT type FROM task_events WHERE task_id="
                           + completedId + " ORDER BY id)").toString(),
                 "created,completed,archived,restored");
    }
    void projectAndListOrderingPersists() {
        QTemporaryDir dir;
        const auto path = dir.filePath("organization-order.sqlite3");
        QString firstProject;
        QString secondProject;
        QString movedProject;
        QString firstList;
        QString secondList;
        QString movedList;
        {
            TaskStore store(path);
            firstProject = store.addProject("First created").id;
            secondProject = store.addProject("Second created").id;
            movedProject = store.addProject("Third created").id;
            QCOMPARE(store.projects().first().id, firstProject);
            QVERIFY(store.moveProject(movedProject, "up"));
            QVERIFY(store.moveProject(movedProject, "up"));
            QVERIFY(!store.moveProject(movedProject, "up"));
            QCOMPARE(store.projects().first().id, movedProject);

            firstList = store.addList(movedProject, "First list").id;
            secondList = store.addList(movedProject, "Second list").id;
            movedList = store.addList(movedProject, "Third list").id;
            QVERIFY(store.moveList(movedList, "up"));
            QVERIFY(store.moveList(movedList, "up"));
            QVERIFY(!store.moveList(movedList, "up"));
            QCOMPARE(store.lists(movedProject).first().id, movedList);
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.moveProject(movedProject, "sideways"));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.moveList(movedList, "sideways"));
            QVERIFY(store.archiveProject(movedProject, true));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.moveProject(movedProject, "down"));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.moveList(movedList, "down"));
            QVERIFY(store.archiveProject(movedProject, false));
        }
        {
            TaskStore store(path);
            QCOMPARE(store.projects().first().id, movedProject);
            QCOMPARE(store.projects().at(1).id, firstProject);
            QCOMPARE(store.projects().last().id, secondProject);
            QCOMPARE(store.lists(movedProject).first().id, movedList);
            QCOMPARE(store.lists(movedProject).at(1).id, firstList);
            QCOMPARE(store.lists(movedProject).last().id, secondList);
        }
    }
    void upgradesV5ArchiveAndOrganizationOrderAndRollsBack() {
        QTemporaryDir dir;
        const auto path = dir.filePath("v5.sqlite3");
        seedV5(path);
        {
            TaskStore store(path);
            const auto task = store.task("9");
            QVERIFY(!task.archived);
            QCOMPARE(task.priority, 3);
            QCOMPARE(store.projects(true).at(0).name, "Existing project");
            QCOMPARE(store.projects(true).at(1).name, "Zulu project");
            QCOMPARE(store.lists("4").at(0).id, "6");
            QCOMPARE(store.lists("4").at(1).id, "8");
        }
        QCOMPARE(sql(path, "PRAGMA user_version").toInt(), TaskStore::SchemaVersion);
        QCOMPARE(sql(path, "SELECT id FROM task_events ORDER BY id DESC LIMIT 1").toInt(), 13);
        QCOMPARE(sql(path, "SELECT count(*) FROM pragma_table_info('tasks') WHERE name='archived'").toInt(), 1);

        const auto broken = dir.filePath("broken-v5.sqlite3");
        seedV5(broken);
        sql(broken, "CREATE INDEX projects_order ON projects(id)");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, TaskStore{broken});
        QCOMPARE(sql(broken, "PRAGMA user_version").toInt(), 5);
        QCOMPARE(sql(broken, "SELECT count(*) FROM pragma_table_info('projects') WHERE name='sort_order'").toInt(), 0);
        QCOMPARE(sql(broken, "SELECT count(*) FROM pragma_table_info('tasks') WHERE name='archived'").toInt(), 0);
        QCOMPARE(sql(broken, "SELECT count(*) FROM task_events").toInt(), 2);
    }
    void tagAssignmentFilterSearchEditAndRestart() {
        QTemporaryDir dir;
        const auto path = dir.filePath("tags.sqlite3");
        QString taskId;
        QString workId;
        QString homeId;
        {
            TaskStore store(path);
            const auto work = store.addTag("  Work  ", "#A1B2C3");
            const auto home = store.addTag("Home", "#445566");
            workId = work.id;
            homeId = home.id;
            QCOMPARE(work.name, "Work");
            QCOMPARE(work.color, "#a1b2c3");
            QCOMPARE(store.tags().size(), 2);
            QCOMPARE(store.tags().first().name, "Home");
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.addTag("work", "#010203"));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.addTag("Bad", "red"));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.addTag(QString(61, 'x'), "#010203"));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.editTag(homeId, "WORK", "#010203"));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add("Unknown", {}, {}, {}, {}, 0, {"999"}));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add("Repeated", {}, {}, {}, {}, 0, {workId, workId}));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error,
                                     store.add("Too many", {}, {}, {}, {}, 0,
                                               QStringList(51, workId)));

            const auto task = store.add("Tagged task", {}, {}, {}, {}, 0, {workId, homeId});
            taskId = task.id;
            QCOMPARE(task.tags.size(), 2);
            QCOMPARE(task.tags.first().name, "Home");
            QCOMPARE(store.tasks("open", {}, "*", "project", {}, workId).first().id, taskId);
            QCOMPARE(store.tasks("open", {}, "*", "project", {}, homeId).first().id, taskId);
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.tasks("open", {}, "*", "project", {}, "999"));
            QCOMPARE(store.tasks("all", "*", "*", "search", "work").first().id, taskId);

            QVERIFY(store.edit(taskId, "Tagged task", "Keep tags in history", {}, {}, {}, {}, 0, {homeId}));
            QCOMPARE(store.task(taskId).tags.size(), 1);
            QCOMPARE(store.task(taskId).tags.first().id, homeId);
            QVERIFY(store.tasks("open", {}, "*", "project", {}, workId).isEmpty());
            QVERIFY(store.editTag(homeId, "Personal", "#778899"));
            QCOMPARE(store.task(taskId).tags.first().name, "Personal");
            QCOMPARE(store.task(taskId).tags.first().color, "#778899");
            QCOMPARE(store.tasks("all", "*", "*", "search", "personal").first().id, taskId);
            QVERIFY(store.tasks("all", "*", "*", "search", "home").isEmpty());
            QVERIFY(!store.editTag(homeId, "Personal", "#778899"));

            const auto eventJson = QJsonDocument::fromJson(sql(path,
                "SELECT new_value FROM task_events WHERE type='edited' ORDER BY id DESC LIMIT 1").toByteArray()).object();
            QCOMPARE(eventJson.value("tag_ids").toArray().size(), 1);
            QCOMPARE(eventJson.value("tag_ids").toArray().first().toString(), homeId);
        }
        {
            TaskStore store(path);
            QCOMPARE(store.task(taskId).tags.first().name, "Personal");
            QCOMPARE(store.tasks("open", {}, "*", "project", {}, homeId).first().id, taskId);
            QVERIFY(store.edit(taskId, "Tagged task", "Keep tags in history", {}, {}, {}, {}, 0, {}));
            QVERIFY(store.task(taskId).tags.isEmpty());
        }
    }
    void upgradesV6TagsAndRollsBack() {
        QTemporaryDir dir;
        const auto path = dir.filePath("v6.sqlite3");
        seedV6(path);
        {
            TaskStore store(path);
            QVERIFY(store.task("9").tags.isEmpty());
            const auto tag = store.addTag("Migrated", "#123456");
            QVERIFY(store.edit("9", "Newer", "Keep note", "4", {}, "2026-09-12", "2026-09-13", 3, {tag.id}));
            QCOMPARE(store.task("9").tags.first().name, "Migrated");
        }
        QCOMPARE(sql(path, "PRAGMA user_version").toInt(), TaskStore::SchemaVersion);
        QCOMPARE(sql(path, "SELECT count(*) FROM tags").toInt(), 1);
        QCOMPARE(sql(path, "SELECT count(*) FROM task_tags").toInt(), 1);
        QCOMPARE(sql(path, "SELECT id FROM task_events ORDER BY id LIMIT 1").toInt(), 12);

        const auto broken = dir.filePath("broken-v6.sqlite3");
        seedV6(broken);
        sql(broken, "CREATE TABLE task_tags(marker INTEGER)");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, TaskStore{broken});
        QCOMPARE(sql(broken, "PRAGMA user_version").toInt(), 6);
        QCOMPARE(sql(broken, "SELECT count(*) FROM sqlite_master WHERE type='table' AND name='tags'").toInt(), 0);
        QCOMPARE(sql(broken, "SELECT count(*) FROM pragma_table_info('task_tags') WHERE name='marker'").toInt(), 1);
        QCOMPARE(sql(broken, "SELECT count(*) FROM tasks").toInt(), 2);
    }
    void recurrenceGeneratesOneCompleteSuccessor() {
        TaskStore store(":memory:");
        const auto project = store.addProject("Routine");
        const auto list = store.addList(project.id, "Cadence");
        const auto tag = store.addTag("Home", "#123456");
        struct Case { QString recurrence; QString date; QString next; };
        const QVector<Case> cases{{"daily","2027-01-05","2027-01-06"},
                                  {"weekdays","2027-01-08","2027-01-11"},
                                  {"weekly","2027-01-05","2027-01-12"},
                                  {"monthly","2027-01-31","2027-02-28"}};
        for (const auto &test : cases) {
            const auto original = store.add(test.recurrence, project.id, list.id,
                                            test.date, test.date, 3, {tag.id}, test.recurrence);
            QVERIFY(store.edit(original.id, original.title, "copied note", project.id, list.id,
                               test.date, test.date, 3, {tag.id}, test.recurrence));
            QVERIFY(store.complete(original.id));
            const auto open = store.tasks("open", project.id);
            const auto found = std::find_if(open.cbegin(), open.cend(), [&](const Task &task) {
                return task.recurrenceSourceId == original.id;
            });
            QVERIFY(found != open.cend());
            const auto successor = *found;
            QCOMPARE(successor.scheduledDate, test.next);
            QCOMPARE(successor.dueDate, test.next);
            QCOMPARE(successor.recurrence, test.recurrence);
            QCOMPARE(successor.note, "copied note");
            QCOMPARE(successor.listId, list.id);
            QCOMPARE(successor.priority, 3);
            QCOMPARE(successor.tags.first().id, tag.id);
            QVERIFY(!store.complete(original.id));
            QVERIFY(store.reopen(original.id));
            QVERIFY(store.complete(original.id));
            const auto after = store.tasks("open", project.id);
            QCOMPARE(std::count_if(after.cbegin(), after.cend(),
                                   [&](const Task &task) { return task.recurrenceSourceId == original.id; }), 1);
        }
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add("No date", {}, {}, {}, {}, 0, {}, "daily"));
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.add("Bad", {}, {}, "2027-01-01", {}, 0, {}, "yearly"));
    }

    void notificationQueueRetriesAndDeduplicates() {
        TaskStore store(":memory:");
        const auto task = store.add("Notify me", {}, {}, "2027-01-02", "2027-01-03");
        QCOMPARE(store.pendingNotifications("2027-01-03").size(), 1);
        auto failed = NotificationService::run(store, "2027-01-03", "/bin/false");
        QCOMPARE(failed.attempted, 1);
        QCOMPARE(failed.failed, 1);
        QCOMPARE(store.pendingNotifications("2027-01-03").size(), 1);
        auto sent = NotificationService::run(store, "2027-01-03", "/bin/true");
        QCOMPARE(sent.sent, 1);
        QCOMPARE(store.pendingNotifications("2027-01-03").size(), 0);
        QCOMPARE(NotificationService::run(store, "2027-01-03", "/bin/true").attempted, 0);
        store.complete(task.id);
        QVERIFY(store.pendingNotifications("2027-12-31").isEmpty());
    }

    void upgradesV7RecurrenceAndNotificationsAndRollsBack() {
        QTemporaryDir dir;
        const auto path = dir.filePath("v7.sqlite3");
        seedV7(path);
        {
            TaskStore store(path);
            QCOMPARE(store.task("9").recurrence, "none");
            QVERIFY(store.task("9").recurrenceSourceId.isEmpty());
            QCOMPARE(store.pendingNotifications("2026-09-13").size(), 1);
        }
        QCOMPARE(sql(path, "PRAGMA user_version").toInt(), TaskStore::SchemaVersion);
        QCOMPARE(sql(path, "SELECT count(*) FROM task_notifications").toInt(), 0);

        const auto broken = dir.filePath("broken-v7.sqlite3");
        seedV7(broken);
        sql(broken, "CREATE TABLE task_notifications(marker INTEGER)");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, TaskStore{broken});
        QCOMPARE(sql(broken, "PRAGMA user_version").toInt(), 7);
        QCOMPARE(sql(broken, "SELECT count(*) FROM pragma_table_info('tasks') WHERE name='recurrence'").toInt(), 0);
        QCOMPARE(sql(broken, "SELECT count(*) FROM pragma_table_info('task_notifications') WHERE name='marker'").toInt(), 1);
    }

    void settingsPersistValidateAndControlNotifications() {
        QTemporaryDir dir;
        const auto path = dir.filePath("settings.sqlite3");
        QString projectId;
        {
            TaskStore store(path);
            const auto defaults = store.settings();
            QVERIFY(defaults.defaultProjectId.isEmpty());
            QVERIFY(defaults.notificationsEnabled);
            QCOMPARE(defaults.notificationDaysBefore, 0);
            QVERIFY(defaults.dmsShowNextTask);
            QVERIFY(!defaults.dmsUseDefaultProject);
            projectId = store.addProject("Default").id;
            store.saveSettings({projectId, false, 5, false, true});
            store.add("Future", projectId, {}, QDate::currentDate().addDays(5).toString(Qt::ISODate));
            QCOMPARE(NotificationService::run(store, {}, "/bin/true").attempted, 0);
            QVERIFY_THROWS_EXCEPTION(std::runtime_error,
                                     store.saveSettings({projectId, true, 31, true, false}));
        }
        {
            TaskStore store(path);
            const auto settings = store.settings();
            QCOMPARE(settings.defaultProjectId, projectId);
            QCOMPARE(settings.notificationDaysBefore, 5);
            QVERIFY(!settings.notificationsEnabled);
            QVERIFY(!settings.dmsShowNextTask);
            QVERIFY(settings.dmsUseDefaultProject);
            auto enabled = settings;
            enabled.notificationsEnabled = true;
            store.saveSettings(enabled);
            QCOMPARE(NotificationService::run(store, {}, "/bin/true").sent, 1);
            QVERIFY(store.archiveProject(projectId, true));
            QVERIFY_THROWS_EXCEPTION(std::runtime_error, store.saveSettings(enabled));
        }
    }

    void upgradesV8SettingsAndRollsBack() {
        QTemporaryDir dir;
        const auto path = dir.filePath("v8.sqlite3");
        seedV8(path);
        {
            TaskStore store(path);
            QVERIFY(store.settings().notificationsEnabled);
            QCOMPARE(store.task("9").title, "Newer");
        }
        QCOMPARE(sql(path, "PRAGMA user_version").toInt(), TaskStore::SchemaVersion);
        QCOMPARE(sql(path, "SELECT count(*) FROM settings").toInt(), 5);

        const auto broken = dir.filePath("broken-v8.sqlite3");
        seedV8(broken);
        sql(broken, "CREATE TABLE settings(marker INTEGER)");
        QVERIFY_THROWS_EXCEPTION(std::runtime_error, TaskStore{broken});
        QCOMPARE(sql(broken, "PRAGMA user_version").toInt(), 8);
        QCOMPARE(sql(broken, "SELECT count(*) FROM pragma_table_info('settings') WHERE name='marker'").toInt(), 1);
    }

    void dmsThemeModesFallbackAndAtomicReplacement() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        qputenv("XDG_CACHE_HOME", dir.path().toUtf8());
        const auto themeDir = dir.filePath("DankMaterialShell");
        QVERIFY(QDir().mkpath(themeDir));
        const auto path = themeDir + "/dms-colors.json";
        {
            QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly));
            file.write("{\"mode\":\"light\",\"light\":{\"primary\":\"#112233\",\"onSurface\":\"#ffffff\"},\"dark\":{\"primary\":\"#aabbcc\"}}");
        }
        DmsThemeProvider provider;
        QCOMPARE(provider.primary(), "#112233");
        QCOMPARE(provider.onSurface(), "#ffffff");
        QSignalSpy changed(&provider, &DmsThemeProvider::themeChanged);
        QSaveFile replacement(path);
        QVERIFY(replacement.open(QIODevice::WriteOnly));
        replacement.write("{\"mode\":\"dark\",\"light\":{\"primary\":\"#112233\"},\"dark\":{\"primary\":\"#aabbcc\"}}");
        QVERIFY(replacement.commit());
        QTRY_VERIFY_WITH_TIMEOUT(changed.count() > 0, 1000);
        QCOMPARE(provider.primary(), "#aabbcc");
        QFile malformed(path); QVERIFY(malformed.open(QIODevice::WriteOnly)); malformed.write("not json"); malformed.close();
        const auto signalCount = changed.count();
        QTRY_VERIFY_WITH_TIMEOUT(changed.count() > signalCount, 1000);
        QVERIFY(provider.primary() != "#aabbcc");
        QFile::remove(path);
        QVERIFY(QDir(themeDir).removeRecursively());
        DmsThemeProvider absent;
        QVERIFY(!absent.background().isEmpty());
        qunsetenv("XDG_CACHE_HOME");
    }
};
QTEST_GUILESS_MAIN(CoreTest)
#include "tst_core.moc"
