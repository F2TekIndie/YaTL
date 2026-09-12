#include "taskstore.h"

#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>
#include <QVariant>
#include <stdexcept>

namespace {
[[noreturn]] void fail(const QString &message) {
    throw std::runtime_error(message.toStdString());
}
void run(QSqlQuery &query) {
    if (!query.exec()) fail(query.lastError().text());
}
void exec(QSqlDatabase db, const QString &sql) {
    QSqlQuery query(db);
    if (!query.exec(sql)) fail(query.lastError().text());
}
QString now() { return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs); }
class Transaction {
public:
    explicit Transaction(QSqlDatabase db) : db_(db) { exec(db_, "BEGIN IMMEDIATE"); }
    ~Transaction() { if (!committed_) db_.rollback(); }
    void commit() {
        if (!db_.commit()) fail(db_.lastError().text());
        committed_ = true;
    }
private:
    QSqlDatabase db_;
    bool committed_ = false;
};
QString titleText(const QString &title) {
    const auto cleaned = title.trimmed();
    if (cleaned.isEmpty()) fail("Enter a task title.");
    if (cleaned.size() > 500) fail("Task titles must be 500 characters or fewer.");
    return cleaned;
}
qint64 positiveId(const QString &id) {
    bool valid = false;
    const auto number = id.toLongLong(&valid);
    if (!valid || number <= 0) fail("ID must be a positive integer.");
    return number;
}
void event(QSqlDatabase db, const QString &id, const QString &type, const QString &time,
           const QString &previous = {}, const QString &next = {}) {
    QSqlQuery query(db);
    query.prepare("INSERT INTO task_events(task_id, type, timestamp, previous_value, new_value) VALUES (?, ?, ?, ?, ?)");
    query.addBindValue(id);
    query.addBindValue(type);
    query.addBindValue(time);
    query.addBindValue(previous);
    query.addBindValue(next);
    run(query);
}
}

QString TaskStore::defaultPath() {
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/yatl/yatl.sqlite3");
}

TaskStore::TaskStore(const QString &path) {
    if (path.isEmpty()) fail("Database path must not be empty.");
    if (path != ":memory:" && !QDir().mkpath(QFileInfo(path).absolutePath()))
        fail("Cannot create database directory.");
    db_ = QSqlDatabase::addDatabase("QSQLITE", QUuid::createUuid().toString());
    db_.setDatabaseName(path);
    db_.setConnectOptions("QSQLITE_BUSY_TIMEOUT=5000");
    try {
        if (!db_.open()) fail(db_.lastError().text());
        exec(db_, "PRAGMA foreign_keys=ON");
        migrate();
    } catch (...) {
        const auto name = db_.connectionName();
        db_.close();
        db_ = QSqlDatabase();
        QSqlDatabase::removeDatabase(name);
        throw;
    }
}

TaskStore::~TaskStore() {
    const auto name = db_.connectionName();
    db_.close();
    db_ = QSqlDatabase();
    QSqlDatabase::removeDatabase(name);
}

void TaskStore::migrate() {
    Transaction transaction(db_);
    QSqlQuery version(db_);
    if (!version.exec("PRAGMA user_version") || !version.next()) fail("Cannot read schema version.");
    const int current = version.value(0).toInt();
    version.finish();
    if (current > SchemaVersion) fail("Database was created by a newer YaTL. Upgrade YaTL to open it.");
    if (current < 1) {
        exec(db_, "CREATE TABLE tasks (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "title TEXT NOT NULL CHECK(length(trim(title)) BETWEEN 1 AND 500), "
                  "created_at TEXT NOT NULL, completed_at TEXT)");
        exec(db_, "PRAGMA user_version=1");
    }
    if (current < 2) {
        exec(db_, "CREATE TABLE task_events (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "task_id INTEGER NOT NULL REFERENCES tasks(id), "
                  "type TEXT NOT NULL CHECK(type IN ('created', 'completed')), timestamp TEXT NOT NULL)");
        exec(db_, "INSERT INTO task_events(task_id, type, timestamp) SELECT id, 'created', created_at FROM tasks");
        exec(db_, "INSERT INTO task_events(task_id, type, timestamp) "
                  "SELECT id, 'completed', completed_at FROM tasks WHERE completed_at IS NOT NULL");
        exec(db_, "CREATE INDEX tasks_completion ON tasks(completed_at, id)");
        exec(db_, "CREATE INDEX events_task ON task_events(task_id, id)");
        exec(db_, "PRAGMA user_version=2");
    }
    if (current < 3) {
        exec(db_, "CREATE TABLE projects (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "name TEXT NOT NULL COLLATE NOCASE UNIQUE CHECK(length(trim(name)) BETWEEN 1 AND 120))");
        exec(db_, "ALTER TABLE tasks ADD COLUMN project_id INTEGER REFERENCES projects(id)");
        exec(db_, "ALTER TABLE tasks ADD COLUMN note TEXT NOT NULL DEFAULT ''");
        exec(db_, "CREATE INDEX tasks_project ON tasks(project_id, completed_at, id)");
        // Rebuild the constrained event table, preserving existing event IDs and history.
        exec(db_, "ALTER TABLE task_events RENAME TO task_events_v2");
        exec(db_, "CREATE TABLE task_events (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "task_id INTEGER NOT NULL REFERENCES tasks(id), "
                  "type TEXT NOT NULL CHECK(type IN ('created', 'completed', 'edited', 'reopened')), "
                  "timestamp TEXT NOT NULL, previous_value TEXT, new_value TEXT)");
        exec(db_, "INSERT INTO task_events(id, task_id, type, timestamp) "
                  "SELECT id, task_id, type, timestamp FROM task_events_v2");
        exec(db_, "DROP TABLE task_events_v2");
        exec(db_, "CREATE INDEX events_task ON task_events(task_id, id)");
        exec(db_, "PRAGMA user_version=3");
    }
    transaction.commit();
}

void TaskStore::validateProject(const QString &id) const {
    if (id.isEmpty()) return; // NULL project means Inbox.
    QSqlQuery query(db_);
    query.prepare("SELECT id FROM projects WHERE id=?");
    query.addBindValue(positiveId(id));
    run(query);
    if (!query.next()) fail("Project not found.");
}

Project TaskStore::addProject(const QString &name) {
    const auto cleaned = name.trimmed();
    if (cleaned.isEmpty() || cleaned.size() > 120) fail("Project names must contain 1 to 120 characters.");
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT id FROM projects WHERE name=?");
    query.addBindValue(cleaned);
    run(query);
    if (query.next()) fail("A project with this name already exists.");
    query.finish();
    query.prepare("INSERT INTO projects(name) VALUES (?)");
    query.addBindValue(cleaned);
    run(query);
    Project project{query.lastInsertId().toString(), cleaned};
    transaction.commit();
    return project;
}

QVector<Project> TaskStore::projects() const {
    QSqlQuery query(db_);
    if (!query.exec("SELECT id, name FROM projects ORDER BY name COLLATE NOCASE, id")) fail(query.lastError().text());
    QVector<Project> result;
    while (query.next()) result.append({query.value(0).toString(), query.value(1).toString()});
    return result;
}

Task TaskStore::add(const QString &title, const QString &projectId) {
    const QString cleaned = titleText(title);
    Transaction transaction(db_);
    QSqlQuery query(db_);
    const QString timestamp = now();
    validateProject(projectId);
    query.prepare("INSERT INTO tasks(title, created_at, project_id) VALUES (?, ?, ?)");
    query.addBindValue(cleaned);
    query.addBindValue(timestamp);
    query.addBindValue(projectId.isEmpty() ? QVariant() : QVariant(positiveId(projectId)));
    run(query);
    Task task{query.lastInsertId().toString(), cleaned, timestamp, {},
              projectId.isEmpty() ? QString() : QString::number(positiveId(projectId)), {}};
    event(db_, task.id, "created", timestamp);
    transaction.commit();
    return task;
}

bool TaskStore::edit(const QString &id, const QString &title, const QString &note, const QString &projectId) {
    const auto number = positiveId(id);
    const auto cleaned = titleText(title);
    if (note.size() > 100000) fail("Task notes must be 100,000 characters or fewer.");
    Transaction transaction(db_);
    validateProject(projectId);
    QSqlQuery query(db_);
    query.prepare("SELECT title, note, project_id FROM tasks WHERE id=?");
    query.addBindValue(number);
    run(query);
    if (!query.next()) fail("Task not found.");
    const QString canonicalProject = projectId.isEmpty() ? QString() : QString::number(positiveId(projectId));
    const QJsonObject previous{{"title", query.value(0).toString()}, {"note", query.value(1).toString()},
                               {"project_id", query.value(2).toString()}};
    const QJsonObject next{{"title", cleaned}, {"note", note}, {"project_id", canonicalProject}};
    query.finish();
    if (previous == next) { transaction.commit(); return false; }
    query.prepare("UPDATE tasks SET title=?, note=?, project_id=? WHERE id=?");
    query.addBindValue(cleaned);
    query.addBindValue(note.isNull() ? QStringLiteral("") : note);
    query.addBindValue(canonicalProject.isEmpty() ? QVariant() : QVariant(canonicalProject));
    query.addBindValue(number);
    run(query);
    event(db_, id, "edited", now(), QString::fromUtf8(QJsonDocument(previous).toJson(QJsonDocument::Compact)),
          QString::fromUtf8(QJsonDocument(next).toJson(QJsonDocument::Compact)));
    transaction.commit();
    return true;
}

bool TaskStore::complete(const QString &id) { return setCompleted(id, true); }
bool TaskStore::reopen(const QString &id) { return setCompleted(id, false); }

bool TaskStore::setCompleted(const QString &id, bool completed) {
    const auto number = positiveId(id);
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT completed_at FROM tasks WHERE id = ?");
    query.addBindValue(number);
    run(query);
    if (!query.next()) fail("Task not found.");
    const bool wasCompleted = !query.value(0).isNull();
    query.finish();
    if (wasCompleted == completed) { transaction.commit(); return false; }
    const QString timestamp = now();
    query.prepare("UPDATE tasks SET completed_at = ? WHERE id = ?");
    query.addBindValue(completed ? QVariant(timestamp) : QVariant());
    query.addBindValue(number);
    run(query);
    event(db_, id, completed ? "completed" : "reopened", timestamp,
          wasCompleted ? "completed" : "open", completed ? "completed" : "open");
    transaction.commit();
    return true;
}

QVector<Task> TaskStore::tasks(const QString &filter, const QString &projectId) const {
    QString sql = "SELECT id, title, created_at, completed_at, project_id, note FROM tasks WHERE 1=1";
    if (filter == "open") sql += " AND completed_at IS NULL";
    else if (filter == "completed") sql += " AND completed_at IS NOT NULL";
    else if (filter != "all") fail("Filter must be open, completed, or all.");
    if (projectId.isEmpty()) sql += " AND project_id IS NULL";
    else if (projectId != "*") { validateProject(projectId); sql += " AND project_id=?"; }
    sql += " ORDER BY id DESC";
    QSqlQuery query(db_);
    query.prepare(sql);
    if (!projectId.isEmpty() && projectId != "*") query.addBindValue(positiveId(projectId));
    run(query);
    QVector<Task> result;
    while (query.next()) result.append({query.value(0).toString(), query.value(1).toString(),
                                       query.value(2).toString(), query.value(3).toString(),
                                       query.value(4).toString(), query.value(5).toString()});
    return result;
}

int TaskStore::dataVersion() const {
    QSqlQuery query(db_);
    if (!query.exec("PRAGMA data_version") || !query.next()) fail("Cannot check database changes.");
    return query.value(0).toInt();
}
