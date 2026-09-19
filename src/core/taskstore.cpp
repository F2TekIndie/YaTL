#include "taskstore.h"

#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QHash>
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
QString tagNameText(const QString &name) {
    const auto cleaned = name.trimmed();
    if (cleaned.isEmpty() || cleaned.size() > 60)
        fail("Tag names must contain 1 to 60 characters.");
    return cleaned;
}
QString tagColorText(const QString &color) {
    if (!QRegularExpression("^#[0-9a-fA-F]{6}$").match(color).hasMatch())
        fail("Use a tag color in #RRGGBB format.");
    return color.toLower();
}
QString canonicalDate(const QString &value, const QString &field) {
    if (value.isEmpty()) return {};
    const auto parsed = QDate::fromString(value, Qt::ISODate);
    if (!parsed.isValid() || parsed.toString(Qt::ISODate) != value)
        fail(field + " must use YYYY-MM-DD or be empty.");
    return value;
}
QString nextRecurringDate(const QString &value, const QString &recurrence) {
    if (value.isEmpty()) return {};
    QDate date = QDate::fromString(value, Qt::ISODate);
    if (recurrence == "daily") date = date.addDays(1);
    else if (recurrence == "weekly") date = date.addDays(7);
    else if (recurrence == "monthly") date = date.addMonths(1);
    else if (recurrence == "weekdays") {
        do { date = date.addDays(1); } while (date.dayOfWeek() > 5);
    }
    return date.toString(Qt::ISODate);
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
    if (current < 4) {
        exec(db_, "ALTER TABLE projects ADD COLUMN color TEXT NOT NULL DEFAULT '#376548'");
        exec(db_, "ALTER TABLE projects ADD COLUMN archived INTEGER NOT NULL DEFAULT 0 CHECK(archived IN (0,1))");
        exec(db_, "CREATE TABLE task_lists (id INTEGER PRIMARY KEY AUTOINCREMENT, project_id INTEGER NOT NULL REFERENCES projects(id), "
                  "name TEXT NOT NULL COLLATE NOCASE CHECK(length(trim(name)) BETWEEN 1 AND 120), UNIQUE(project_id,name))");
        exec(db_, "ALTER TABLE tasks ADD COLUMN list_id INTEGER REFERENCES task_lists(id)");
        exec(db_, "ALTER TABLE tasks ADD COLUMN sort_order INTEGER NOT NULL DEFAULT 0");
        exec(db_, "UPDATE tasks SET sort_order=-id");
        exec(db_, "CREATE INDEX tasks_order ON tasks(project_id, completed_at, sort_order, id)");
        for (const auto &operation : {QString("INSERT"), QString("UPDATE OF project_id,list_id")}) {
            const auto trigger = operation == "INSERT" ? "tasks_list_insert" : "tasks_list_update";
            exec(db_, QString("CREATE TRIGGER %1 BEFORE %2 ON tasks WHEN NEW.list_id IS NOT NULL AND NOT EXISTS "
                  "(SELECT 1 FROM task_lists WHERE id=NEW.list_id AND project_id=NEW.project_id) "
                  "BEGIN SELECT RAISE(ABORT,'Task list must belong to the task project'); END").arg(trigger, operation));
        }
        exec(db_, "PRAGMA user_version=4");
    }
    if (current < 5) {
        exec(db_, "ALTER TABLE tasks ADD COLUMN scheduled_date TEXT");
        exec(db_, "ALTER TABLE tasks ADD COLUMN due_date TEXT");
        exec(db_, "ALTER TABLE tasks ADD COLUMN priority INTEGER NOT NULL DEFAULT 0 CHECK(priority BETWEEN 0 AND 3)");
        exec(db_, "CREATE INDEX tasks_scheduled ON tasks(completed_at, scheduled_date, priority)");
        exec(db_, "CREATE INDEX tasks_due ON tasks(completed_at, due_date, priority)");
        exec(db_, "PRAGMA user_version=5");
    }
    if (current < 6) {
        exec(db_, "ALTER TABLE projects ADD COLUMN sort_order INTEGER NOT NULL DEFAULT 0");
        exec(db_, "WITH ordered AS (SELECT id, ROW_NUMBER() OVER (ORDER BY name COLLATE NOCASE,id)-1 position FROM projects) "
                  "UPDATE projects SET sort_order=(SELECT position FROM ordered WHERE ordered.id=projects.id)");
        exec(db_, "ALTER TABLE task_lists ADD COLUMN sort_order INTEGER NOT NULL DEFAULT 0");
        exec(db_, "WITH ordered AS (SELECT id, ROW_NUMBER() OVER (PARTITION BY project_id ORDER BY id)-1 position FROM task_lists) "
                  "UPDATE task_lists SET sort_order=(SELECT position FROM ordered WHERE ordered.id=task_lists.id)");
        exec(db_, "ALTER TABLE tasks ADD COLUMN archived INTEGER NOT NULL DEFAULT 0 CHECK(archived IN (0,1))");
        exec(db_, "CREATE INDEX projects_order ON projects(archived,sort_order,id)");
        exec(db_, "CREATE INDEX lists_order ON task_lists(project_id,sort_order,id)");
        exec(db_, "CREATE INDEX tasks_archive ON tasks(archived,project_id,completed_at,sort_order)");
        exec(db_, "ALTER TABLE task_events RENAME TO task_events_v5");
        exec(db_, "CREATE TABLE task_events (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "task_id INTEGER NOT NULL REFERENCES tasks(id), "
                  "type TEXT NOT NULL CHECK(type IN ('created', 'completed', 'edited', 'reopened', 'archived', 'restored')), "
                  "timestamp TEXT NOT NULL, previous_value TEXT, new_value TEXT)");
        exec(db_, "INSERT INTO task_events(id,task_id,type,timestamp,previous_value,new_value) "
                  "SELECT id,task_id,type,timestamp,previous_value,new_value FROM task_events_v5");
        exec(db_, "DROP TABLE task_events_v5");
        exec(db_, "CREATE INDEX events_task ON task_events(task_id,id)");
        exec(db_, "PRAGMA user_version=6");
    }
    if (current < 7) {
        exec(db_, "CREATE TABLE tags (id INTEGER PRIMARY KEY AUTOINCREMENT, "
                  "name TEXT NOT NULL COLLATE NOCASE UNIQUE CHECK(length(trim(name)) BETWEEN 1 AND 60), "
                  "color TEXT NOT NULL)");
        exec(db_, "CREATE TABLE task_tags (task_id INTEGER NOT NULL REFERENCES tasks(id) ON DELETE CASCADE, "
                  "tag_id INTEGER NOT NULL REFERENCES tags(id) ON DELETE CASCADE, "
                  "PRIMARY KEY(task_id,tag_id))");
        exec(db_, "CREATE INDEX task_tags_tag ON task_tags(tag_id,task_id)");
        exec(db_, "PRAGMA user_version=7");
    }
    if (current < 8) {
        exec(db_, "ALTER TABLE tasks ADD COLUMN recurrence TEXT NOT NULL DEFAULT 'none' "
                  "CHECK(recurrence IN ('none','daily','weekdays','weekly','monthly'))");
        exec(db_, "ALTER TABLE tasks ADD COLUMN recurrence_source_id INTEGER REFERENCES tasks(id)");
        exec(db_, "CREATE UNIQUE INDEX tasks_recurrence_source ON tasks(recurrence_source_id) "
                  "WHERE recurrence_source_id IS NOT NULL");
        exec(db_, "CREATE TABLE task_notifications (task_id INTEGER NOT NULL REFERENCES tasks(id) ON DELETE CASCADE, "
                  "kind TEXT NOT NULL CHECK(kind IN ('scheduled','due')), date TEXT NOT NULL, "
                  "notified_at TEXT NOT NULL, PRIMARY KEY(task_id,kind,date))");
        exec(db_, "CREATE INDEX notifications_date ON task_notifications(date,task_id)");
        exec(db_, "PRAGMA user_version=8");
    }
    if (current < 9) {
        exec(db_, "CREATE TABLE settings (key TEXT PRIMARY KEY, value TEXT NOT NULL, schema_version INTEGER NOT NULL)");
        exec(db_, "INSERT INTO settings(key,value,schema_version) VALUES "
                  "('default_project_id','',1),('notifications_enabled','true',1),"
                  "('notification_days_before','0',1),('dms_show_next_task','true',1),"
                  "('dms_use_default_project','false',1)");
        exec(db_, "PRAGMA user_version=9");
    }
    if (current < 10) {
        exec(db_, "ALTER TABLE projects ADD COLUMN deleted_at TEXT");
        exec(db_, "ALTER TABLE tasks ADD COLUMN deleted_at TEXT");
        exec(db_, "CREATE INDEX projects_deleted ON projects(deleted_at,archived,sort_order,id)");
        exec(db_, "CREATE INDEX tasks_deleted ON tasks(deleted_at,archived,project_id,completed_at,sort_order,id)");
        exec(db_, "PRAGMA user_version=10");
    }
    if (current < 11) {
        // Keep project-cascaded soft deletion distinct from tasks that the user
        // deleted individually, so restoring a project cannot resurrect them.
        exec(db_, "ALTER TABLE tasks ADD COLUMN deleted_with_project_id INTEGER REFERENCES projects(id)");
        exec(db_, "CREATE INDEX tasks_deleted_project ON tasks(deleted_with_project_id,id)");
        exec(db_, "PRAGMA user_version=11");
    }
    transaction.commit();
}

void TaskStore::validateProject(const QString &id, bool writable) const {
    if (id.isEmpty()) return; // NULL project means Inbox.
    QSqlQuery query(db_);
    query.prepare("SELECT archived,deleted_at FROM projects WHERE id=?");
    query.addBindValue(positiveId(id));
    run(query);
    if (!query.next()) fail("Project not found.");
    if (!query.value(1).isNull()) fail("Project was deleted.");
    if (writable && query.value(0).toBool()) fail("Restore the archived project before changing its tasks or lists.");
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
    query.prepare("INSERT INTO projects(name,sort_order) VALUES (?,(SELECT COALESCE(MAX(sort_order),-1)+1 FROM projects))");
    query.addBindValue(cleaned);
    run(query);
    const QString projectId = query.lastInsertId().toString();
    query.finish();
    query.prepare("SELECT id,name,color,archived,sort_order FROM projects WHERE id=?");
    query.addBindValue(projectId);
    run(query);
    if (!query.next()) fail("Cannot read the new project.");
    Project project{query.value(0).toString(), query.value(1).toString(), query.value(2).toString(),
                    query.value(3).toBool(), query.value(4).toLongLong(), {}};
    transaction.commit();
    return project;
}

QVector<Project> TaskStore::projects(bool includeArchived) const {
    QSqlQuery query(db_);
    if (!query.exec(QString("SELECT id,name,color,archived,sort_order,deleted_at FROM projects WHERE deleted_at IS NULL ")
                    + (includeArchived ? "" : "AND archived=0 ") + "ORDER BY sort_order,id"))
        fail(query.lastError().text());
    QVector<Project> result;
    while (query.next()) result.append({query.value(0).toString(), query.value(1).toString(),
                                       query.value(2).toString(), query.value(3).toBool(),
                                       query.value(4).toLongLong(), query.value(5).toString()});
    return result;
}

void TaskStore::validatePlanning(const QString &scheduledDate, const QString &dueDate, int priority) const {
    canonicalDate(scheduledDate, "Scheduled date");
    canonicalDate(dueDate, "Due date");
    if (priority < 0 || priority > 3) fail("Priority must be between 0 and 3.");
    if (!scheduledDate.isEmpty() && !dueDate.isEmpty() && scheduledDate > dueDate)
        fail("Scheduled date cannot be later than due date.");
}

void TaskStore::validateRecurrence(const QString &recurrence, const QString &scheduledDate,
                                   const QString &dueDate) const {
    if (recurrence != "none" && recurrence != "daily" && recurrence != "weekdays"
        && recurrence != "weekly" && recurrence != "monthly")
        fail("Recurrence must be none, daily, weekdays, weekly, or monthly.");
    if (recurrence != "none" && scheduledDate.isEmpty() && dueDate.isEmpty())
        fail("A recurring task needs a scheduled or due date.");
}

Task TaskStore::add(const QString &title, const QString &projectId, const QString &listId,
                    const QString &scheduledDate, const QString &dueDate, int priority,
                    const QStringList &tagIds, const QString &recurrence) {
    const QString cleaned = titleText(title);
    validatePlanning(scheduledDate, dueDate, priority);
    validateRecurrence(recurrence, scheduledDate, dueDate);
    Transaction transaction(db_);
    const auto canonicalTags = validateTagIds(tagIds);
    QSqlQuery query(db_);
    const QString timestamp = now();
    validateProject(projectId, true);
    validateList(listId, projectId);
    const auto order = nextOrder();
    query.prepare("INSERT INTO tasks(title, created_at, project_id, list_id, sort_order, scheduled_date, due_date, priority, recurrence) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    query.addBindValue(cleaned);
    query.addBindValue(timestamp);
    query.addBindValue(projectId.isEmpty() ? QVariant() : QVariant(positiveId(projectId)));
    query.addBindValue(listId.isEmpty() ? QVariant() : QVariant(positiveId(listId)));
    query.addBindValue(order);
    query.addBindValue(scheduledDate.isEmpty() ? QVariant() : QVariant(scheduledDate));
    query.addBindValue(dueDate.isEmpty() ? QVariant() : QVariant(dueDate));
    query.addBindValue(priority);
    query.addBindValue(recurrence);
    run(query);
    const QString taskId = query.lastInsertId().toString();
    replaceTaskTags(taskId, canonicalTags);
    event(db_, taskId, "created", timestamp);
    transaction.commit();
    return task(taskId);
}

bool TaskStore::edit(const QString &id, const QString &title, const QString &note,
                     const QString &projectId, const QString &listId,
                     const QString &scheduledDate, const QString &dueDate, int priority,
                     const QStringList &tagIds, const QString &recurrence) {
    const auto number = positiveId(id);
    const auto cleaned = titleText(title);
    if (note.size() > 100000) fail("Task notes must be 100,000 characters or fewer.");
    validatePlanning(scheduledDate, dueDate, priority);
    validateRecurrence(recurrence, scheduledDate, dueDate);
    Transaction transaction(db_);
    auto canonicalTags = validateTagIds(tagIds);
    canonicalTags.sort();
    validateProject(projectId, true);
    validateList(listId, projectId);
    QSqlQuery query(db_);
    query.prepare("SELECT title,note,project_id,list_id,scheduled_date,due_date,priority,archived,recurrence,deleted_at FROM tasks WHERE id=?");
    query.addBindValue(number);
    run(query);
    if (!query.next()) fail("Task not found.");
    if (query.value(7).toBool()) fail("Restore the archived task before changing it.");
    if (!query.value(9).isNull()) fail("Task was deleted.");
    QStringList previousTags;
    for (const auto &tag : taskTags(id)) previousTags.append(tag.id);
    previousTags.sort();
    validateProject(query.value(2).toString(), true);
    const QString canonicalList = listId.isEmpty() ? QString() : QString::number(positiveId(listId));
    const QString canonicalProject = projectId.isEmpty() ? QString() : QString::number(positiveId(projectId));
    const QJsonObject previous{{"title", query.value(0).toString()}, {"note", query.value(1).toString()},
                               {"project_id", query.value(2).toString()}, {"list_id", query.value(3).toString()},
                               {"scheduled_date", query.value(4).toString()}, {"due_date", query.value(5).toString()},
                               {"priority", query.value(6).toInt()},
                               {"recurrence", query.value(8).toString()},
                               {"tag_ids", QJsonArray::fromStringList(previousTags)}};
    const QJsonObject next{{"title", cleaned}, {"note", note}, {"project_id", canonicalProject},
                           {"list_id", canonicalList}, {"scheduled_date", scheduledDate},
                           {"due_date", dueDate}, {"priority", priority},
                           {"recurrence", recurrence},
                           {"tag_ids", QJsonArray::fromStringList(canonicalTags)}};
    query.finish();
    if (previous == next) { transaction.commit(); return false; }
    query.prepare("UPDATE tasks SET title=?, note=?, project_id=?, list_id=?, scheduled_date=?, due_date=?, priority=?, recurrence=? WHERE id=?");
    query.addBindValue(cleaned);
    query.addBindValue(note.isNull() ? QStringLiteral("") : note);
    query.addBindValue(canonicalProject.isEmpty() ? QVariant() : QVariant(canonicalProject));
    query.addBindValue(canonicalList.isEmpty() ? QVariant() : QVariant(canonicalList));
    query.addBindValue(scheduledDate.isEmpty() ? QVariant() : QVariant(scheduledDate));
    query.addBindValue(dueDate.isEmpty() ? QVariant() : QVariant(dueDate));
    query.addBindValue(priority);
    query.addBindValue(recurrence);
    query.addBindValue(number);
    run(query);
    replaceTaskTags(id, canonicalTags);
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
    query.prepare("SELECT completed_at,project_id,archived,title,note,list_id,scheduled_date,due_date,priority,recurrence,deleted_at "
                  "FROM tasks WHERE id=?");
    query.addBindValue(number);
    run(query);
    if (!query.next()) fail("Task not found.");
    if (query.value(2).toBool()) fail("Restore the archived task before changing it.");
    if (!query.value(10).isNull()) fail("Task was deleted.");
    validateProject(query.value(1).toString(), true);
    const bool wasCompleted = !query.value(0).isNull();
    const QString projectId = query.value(1).toString();
    const QString title = query.value(3).toString();
    const QString note = query.value(4).toString();
    const QString listId = query.value(5).toString();
    const QString scheduledDate = query.value(6).toString();
    const QString dueDate = query.value(7).toString();
    const int priority = query.value(8).toInt();
    const QString recurrence = query.value(9).toString();
    query.finish();
    if (wasCompleted == completed) { transaction.commit(); return false; }
    const QString timestamp = now();
    query.prepare("UPDATE tasks SET completed_at = ? WHERE id = ?");
    query.addBindValue(completed ? QVariant(timestamp) : QVariant());
    query.addBindValue(number);
    run(query);
    event(db_, id, completed ? "completed" : "reopened", timestamp,
          wasCompleted ? "completed" : "open", completed ? "completed" : "open");
    if (completed && recurrence != "none") {
        query.finish();
        query.prepare("SELECT id FROM tasks WHERE recurrence_source_id=?");
        query.addBindValue(number);
        run(query);
        const bool alreadyGenerated = query.next();
        query.finish();
        if (!alreadyGenerated) {
            query.prepare("INSERT INTO tasks(title,created_at,project_id,note,list_id,sort_order,scheduled_date,due_date,priority,recurrence,recurrence_source_id) "
                          "VALUES (?,?,?,?,?,(SELECT COALESCE(MAX(sort_order),-1)+1 FROM tasks),?,?,?,?,?)");
            query.addBindValue(title);
            query.addBindValue(timestamp);
            query.addBindValue(projectId.isEmpty() ? QVariant() : QVariant(projectId));
            query.addBindValue(note);
            query.addBindValue(listId.isEmpty() ? QVariant() : QVariant(listId));
            query.addBindValue(nextRecurringDate(scheduledDate, recurrence));
            query.addBindValue(nextRecurringDate(dueDate, recurrence));
            query.addBindValue(priority);
            query.addBindValue(recurrence);
            query.addBindValue(number);
            run(query);
            const QString nextId = query.lastInsertId().toString();
            QStringList tags;
            for (const auto &tag : taskTags(id)) tags.append(tag.id);
            replaceTaskTags(nextId, tags);
            event(db_, nextId, "created", timestamp, {}, id);
        }
    }
    transaction.commit();
    return true;
}

bool TaskStore::archiveTask(const QString &id, bool archived) {
    const auto number = positiveId(id);
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT project_id,archived,deleted_at FROM tasks WHERE id=?");
    query.addBindValue(number);
    run(query);
    if (!query.next()) fail("Task not found.");
    if (!query.value(2).isNull()) fail("Task was deleted.");
    validateProject(query.value(0).toString(), true);
    const bool wasArchived = query.value(1).toBool();
    query.finish();
    if (wasArchived == archived) { transaction.commit(); return false; }
    query.prepare("UPDATE tasks SET archived=? WHERE id=?");
    query.addBindValue(archived);
    query.addBindValue(number);
    run(query);
    event(db_, id, archived ? "archived" : "restored", now(),
          wasArchived ? "archived" : "active", archived ? "archived" : "active");
    transaction.commit();
    return true;
}

bool TaskStore::deleteTask(const QString &id) {
    const auto number = positiveId(id);
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT deleted_at FROM tasks WHERE id=?");
    query.addBindValue(number);
    run(query);
    if (!query.next()) fail("Task not found.");
    if (!query.value(0).isNull()) { transaction.commit(); return false; }
    query.finish();
    query.prepare("UPDATE tasks SET deleted_at=?,deleted_with_project_id=NULL WHERE id=? AND deleted_at IS NULL");
    query.addBindValue(now());
    query.addBindValue(number);
    run(query);
    transaction.commit();
    return true;
}

bool TaskStore::restoreTask(const QString &id) {
    const auto number = positiveId(id);
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT project_id,deleted_at FROM tasks WHERE id=?");
    query.addBindValue(number);
    run(query);
    if (!query.next()) fail("Task not found.");
    const QString projectId = query.value(0).toString();
    if (query.value(1).isNull()) { transaction.commit(); return false; }
    if (!projectId.isEmpty()) validateProject(projectId);
    query.finish();
    query.prepare("UPDATE tasks SET deleted_at=NULL,deleted_with_project_id=NULL WHERE id=?");
    query.addBindValue(number);
    run(query);
    transaction.commit();
    return true;
}

bool TaskStore::purgeTask(const QString &id) {
    const auto number = positiveId(id);
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT id FROM tasks WHERE id=? AND deleted_at IS NOT NULL");
    query.addBindValue(number);
    run(query);
    if (!query.next()) fail("Deleted task not found.");
    query.finish();
    // A generated recurrence occurrence remains useful after its deleted
    // predecessor is purged; detach the lineage before removing the source.
    query.prepare("UPDATE tasks SET recurrence_source_id=NULL WHERE recurrence_source_id=?");
    query.addBindValue(number);
    run(query);
    query.finish();
    for (const auto &table : {QStringLiteral("task_events"), QStringLiteral("task_notifications"), QStringLiteral("task_tags")}) {
        query.prepare(QString("DELETE FROM %1 WHERE task_id=?").arg(table));
        query.addBindValue(number);
        run(query);
        query.finish();
    }
    query.prepare("DELETE FROM tasks WHERE id=?");
    query.addBindValue(number);
    run(query);
    transaction.commit();
    return true;
}

QVector<Task> TaskStore::tasks(const QString &filter, const QString &projectId,
                               const QString &listId, const QString &scope,
                               const QString &search, const QString &tagId) const {
    if (scope != "project" && scope != "today" && scope != "upcoming" && scope != "search")
        fail("View must be project, today, upcoming, or search.");
    QString sql = "SELECT t.id,t.title,t.created_at,t.completed_at,t.project_id,t.note,t.list_id,t.sort_order,"
                  "t.scheduled_date,t.due_date,t.priority,COALESCE(p.name,''),COALESCE(l.name,''),t.archived,t.recurrence,t.recurrence_source_id "
                  "FROM tasks t LEFT JOIN projects p ON p.id=t.project_id "
                  "LEFT JOIN task_lists l ON l.id=t.list_id WHERE t.deleted_at IS NULL";
    QVector<QVariant> bindings;
    if (filter == "open") sql += " AND t.completed_at IS NULL AND t.archived=0";
    else if (filter == "completed") sql += " AND t.completed_at IS NOT NULL AND t.archived=0";
    else if (filter == "archived") sql += " AND t.archived=1";
    else if (filter != "all") fail("Filter must be open, completed, archived, or all.");
    if (!tagId.isEmpty() && tagId != "*") {
        const auto canonicalTag = validateTagIds({tagId}).first();
        sql += " AND EXISTS (SELECT 1 FROM task_tags tf WHERE tf.task_id=t.id AND tf.tag_id=?)";
        bindings.append(positiveId(canonicalTag));
    }
    if (scope == "project") {
        if (projectId.isEmpty()) sql += " AND t.project_id IS NULL";
        else if (projectId != "*") {
            validateProject(projectId);
            sql += " AND t.project_id=?";
            bindings.append(positiveId(projectId));
        }
        if (projectId == "*") sql += " AND (t.project_id IS NULL OR (p.archived=0 AND p.deleted_at IS NULL))";
        if (listId.isEmpty()) sql += " AND t.list_id IS NULL";
        else if (listId != "*") {
            validateList(listId, projectId);
            sql += " AND t.list_id=?";
            bindings.append(positiveId(listId));
        }
    } else {
        sql += " AND (t.project_id IS NULL OR (p.archived=0 AND p.deleted_at IS NULL))";
        const QString today = QDate::currentDate().toString(Qt::ISODate);
        if (scope == "today") {
            sql += " AND ((t.due_date IS NOT NULL AND t.due_date<=?) OR "
                   "(t.scheduled_date IS NOT NULL AND t.scheduled_date<=?))";
            bindings.append(today);
            bindings.append(today);
        } else if (scope == "upcoming") {
            const QString tomorrow = QDate::currentDate().addDays(1).toString(Qt::ISODate);
            const QString end = QDate::currentDate().addDays(28).toString(Qt::ISODate);
            sql += " AND ((t.due_date BETWEEN ? AND ?) OR (t.scheduled_date BETWEEN ? AND ?))";
            bindings << tomorrow << end << tomorrow << end;
        } else {
            if (search.trimmed().isEmpty()) return {};
            QString pattern = search.trimmed().toLower();
            pattern.replace("\\", "\\\\");
            pattern.replace("%", "\\%");
            pattern.replace("_", "\\_");
            pattern = "%" + pattern + "%";
            sql += " AND (LOWER(t.title) LIKE ? ESCAPE '\\' OR LOWER(t.note) LIKE ? ESCAPE '\\' OR "
                   "LOWER(COALESCE(p.name,'')) LIKE ? ESCAPE '\\' OR LOWER(COALESCE(l.name,'')) LIKE ? ESCAPE '\\' OR "
                   "EXISTS (SELECT 1 FROM task_tags ts JOIN tags gs ON gs.id=ts.tag_id "
                   "WHERE ts.task_id=t.id AND LOWER(gs.name) LIKE ? ESCAPE '\\'))";
            bindings << pattern << pattern << pattern << pattern << pattern;
        }
    }
    if (scope == "project") sql += " ORDER BY t.sort_order,t.id DESC";
    else if (scope == "search")
        sql += " ORDER BY t.archived,(t.completed_at IS NOT NULL),t.priority DESC,t.sort_order,t.id DESC";
    else
        sql += " ORDER BY t.priority DESC,COALESCE(t.due_date,t.scheduled_date),t.sort_order,t.id DESC";
    QSqlQuery query(db_);
    query.prepare(sql);
    for (const auto &binding : bindings) query.addBindValue(binding);
    run(query);
    QVector<Task> result;
    while (query.next()) {
        Task task{query.value(0).toString(), query.value(1).toString(),
                  query.value(2).toString(), query.value(3).toString(),
                  query.value(4).toString(), query.value(5).toString(),
                  query.value(6).toString(), query.value(7).toLongLong(),
                  query.value(8).toString(), query.value(9).toString(),
                  query.value(10).toInt(), query.value(11).toString(),
                  query.value(12).toString(), query.value(13).toBool(), {},
                  query.value(14).toString(), query.value(15).toString(), {}, {}};
        result.append(std::move(task));
    }
    if (!result.isEmpty()) {
        QStringList ids;
        for (const auto &task : result) ids.append(task.id);
        QSqlQuery tagsQuery(db_);
        tagsQuery.prepare("SELECT tt.task_id,t.id,t.name,t.color FROM task_tags tt JOIN tags t ON t.id=tt.tag_id WHERE tt.task_id IN (" +
                          QStringList(ids.size(), "?").join(',') + ") ORDER BY t.name COLLATE NOCASE,t.id");
        for (const auto &id : ids) tagsQuery.addBindValue(id);
        run(tagsQuery);
        QHash<QString,QVector<Tag>> tagsByTask;
        while (tagsQuery.next()) tagsByTask[tagsQuery.value(0).toString()].append(
            {tagsQuery.value(1).toString(), tagsQuery.value(2).toString(), tagsQuery.value(3).toString()});
        for (auto &task : result) task.tags = tagsByTask.value(task.id);
    }
    return result;
}

Task TaskStore::task(const QString &id) const {
    QSqlQuery query(db_);
    query.prepare("SELECT t.id,t.title,t.created_at,t.completed_at,t.project_id,t.note,t.list_id,t.sort_order,"
                  "t.scheduled_date,t.due_date,t.priority,COALESCE(p.name,''),COALESCE(l.name,''),t.archived,t.recurrence,t.recurrence_source_id,"
                  "t.deleted_at,t.deleted_with_project_id "
                  "FROM tasks t LEFT JOIN projects p ON p.id=t.project_id "
                  "LEFT JOIN task_lists l ON l.id=t.list_id WHERE t.id=?");
    query.addBindValue(positiveId(id));
    run(query);
    if (!query.next()) fail("Task not found.");
    Task result{query.value(0).toString(), query.value(1).toString(), query.value(2).toString(),
                query.value(3).toString(), query.value(4).toString(), query.value(5).toString(),
                query.value(6).toString(), query.value(7).toLongLong(), query.value(8).toString(),
                query.value(9).toString(), query.value(10).toInt(), query.value(11).toString(),
                query.value(12).toString(), query.value(13).toBool(), {},
                query.value(14).toString(), query.value(15).toString(),
                query.value(16).toString(), query.value(17).toString()};
    result.tags = taskTags(result.id);
    return result;
}

int TaskStore::dataVersion() const {
    QSqlQuery query(db_);
    if (!query.exec("PRAGMA data_version") || !query.next()) fail("Cannot check database changes.");
    return query.value(0).toInt();
}

QVector<TaskNotification> TaskStore::pendingNotifications(const QString &throughDate) const {
    canonicalDate(throughDate, "Notification date");
    if (throughDate.isEmpty()) fail("Notification date must not be empty.");
    QSqlQuery query(db_);
    query.prepare("SELECT t.id,t.title,t.scheduled_date,t.due_date FROM tasks t "
                  "LEFT JOIN projects p ON p.id=t.project_id WHERE t.deleted_at IS NULL AND t.completed_at IS NULL AND t.archived=0 "
                  "AND (t.project_id IS NULL OR (p.archived=0 AND p.deleted_at IS NULL)) AND "
                  "((t.scheduled_date IS NOT NULL AND t.scheduled_date<=?) OR (t.due_date IS NOT NULL AND t.due_date<=?)) "
                  "ORDER BY t.priority DESC,COALESCE(t.due_date,t.scheduled_date),t.id");
    query.addBindValue(throughDate);
    query.addBindValue(throughDate);
    run(query);
    QVector<TaskNotification> result;
    while (query.next()) {
        const QString id = query.value(0).toString();
        const QString title = query.value(1).toString();
        const QString scheduled = query.value(2).toString();
        const QString due = query.value(3).toString();
        const auto append = [&](const QString &kind, const QString &date) {
            if (date.isEmpty() || date > throughDate) return;
            QSqlQuery sent(db_);
            sent.prepare("SELECT 1 FROM task_notifications WHERE task_id=? AND kind=? AND date=?");
            sent.addBindValue(id); sent.addBindValue(kind); sent.addBindValue(date); run(sent);
            if (!sent.next()) result.append({id, title, kind, date});
        };
        if (!due.isEmpty()) append("due", due);
        else append("scheduled", scheduled);
    }
    return result;
}

bool TaskStore::markNotificationSent(const TaskNotification &notification) {
    canonicalDate(notification.date, "Notification date");
    if (notification.kind != "scheduled" && notification.kind != "due")
        fail("Notification kind must be scheduled or due.");
    QSqlQuery query(db_);
    query.prepare("INSERT OR IGNORE INTO task_notifications(task_id,kind,date,notified_at) VALUES (?,?,?,?)");
    query.addBindValue(positiveId(notification.taskId));
    query.addBindValue(notification.kind);
    query.addBindValue(notification.date);
    query.addBindValue(now());
    run(query);
    return query.numRowsAffected() > 0;
}

Settings TaskStore::settings() const {
    Settings result;
    QSqlQuery query(db_);
    query.prepare("SELECT key,value FROM settings");
    run(query);
    while (query.next()) {
        const QString key = query.value(0).toString();
        const QString value = query.value(1).toString();
        if (key == "default_project_id") result.defaultProjectId = value;
        else if (key == "notifications_enabled") result.notificationsEnabled = value == "true";
        else if (key == "notification_days_before") result.notificationDaysBefore = value.toInt();
        else if (key == "dms_show_next_task") result.dmsShowNextTask = value == "true";
        else if (key == "dms_use_default_project") result.dmsUseDefaultProject = value == "true";
    }
    if (!result.defaultProjectId.isEmpty()) {
        query.finish();
        query.prepare("SELECT 1 FROM projects WHERE id=? AND archived=0 AND deleted_at IS NULL");
        query.addBindValue(positiveId(result.defaultProjectId));
        run(query);
        if (!query.next()) result.defaultProjectId.clear();
    }
    return result;
}

void TaskStore::saveSettings(const Settings &settings) {
    if (settings.notificationDaysBefore < 0 || settings.notificationDaysBefore > 30)
        fail("Notification advance must be between 0 and 30 days.");
    validateProject(settings.defaultProjectId, true);
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("INSERT INTO settings(key,value,schema_version) VALUES (?,?,1) "
                  "ON CONFLICT(key) DO UPDATE SET value=excluded.value,schema_version=excluded.schema_version");
    const QVector<QPair<QString,QString>> values{
        {"default_project_id", settings.defaultProjectId},
        {"notifications_enabled", settings.notificationsEnabled ? "true" : "false"},
        {"notification_days_before", QString::number(settings.notificationDaysBefore)},
        {"dms_show_next_task", settings.dmsShowNextTask ? "true" : "false"},
        {"dms_use_default_project", settings.dmsUseDefaultProject ? "true" : "false"}};
    for (const auto &value : values) {
        query.bindValue(0, value.first);
        query.bindValue(1, value.second);
        run(query);
        query.finish();
    }
    transaction.commit();
}

bool TaskStore::editProject(const QString &id, const QString &name, const QString &color) {
    const auto cleaned = name.trimmed();
    if (cleaned.isEmpty() || cleaned.size() > 120) fail("Project names must contain 1 to 120 characters.");
    if (!QRegularExpression("^#[0-9a-fA-F]{6}$").match(color).hasMatch()) fail("Use a project color in #RRGGBB format.");
    Transaction transaction(db_);
    validateProject(id, true);
    QSqlQuery query(db_);
    query.prepare("SELECT id FROM projects WHERE name=? AND id<>?");
    query.addBindValue(cleaned);
    query.addBindValue(positiveId(id));
    run(query);
    if (query.next()) fail("A project with this name already exists.");
    query.finish();
    query.prepare("UPDATE projects SET name=?,color=? WHERE id=? AND (name COLLATE BINARY <> ? OR color<>?)");
    query.addBindValue(cleaned);
    query.addBindValue(color.toLower());
    query.addBindValue(positiveId(id));
    query.addBindValue(cleaned);
    query.addBindValue(color.toLower());
    run(query);
    const bool changed = query.numRowsAffected() > 0;
    transaction.commit();
    return changed;
}

bool TaskStore::archiveProject(const QString &id, bool archived) {
    Transaction transaction(db_);
    validateProject(id);
    QSqlQuery query(db_);
    query.prepare("UPDATE projects SET archived=? WHERE id=? AND archived<>?");
    query.addBindValue(archived);
    query.addBindValue(positiveId(id));
    query.addBindValue(archived);
    run(query);
    const bool changed = query.numRowsAffected() > 0;
    transaction.commit();
    return changed;
}

bool TaskStore::deleteProject(const QString &id) {
    const auto number = positiveId(id);
    Transaction transaction(db_);
    validateProject(id);
    const QString deletedAt = now();
    QSqlQuery query(db_);
    query.prepare("UPDATE projects SET deleted_at=? WHERE id=? AND deleted_at IS NULL");
    query.addBindValue(deletedAt);
    query.addBindValue(number);
    run(query);
    if (query.numRowsAffected() == 0) { transaction.commit(); return false; }
    query.prepare("UPDATE tasks SET deleted_at=?,deleted_with_project_id=? WHERE project_id=? AND deleted_at IS NULL");
    query.addBindValue(deletedAt);
    query.addBindValue(number);
    query.addBindValue(number);
    run(query);
    transaction.commit();
    return true;
}

bool TaskStore::restoreProject(const QString &id) {
    const auto number = positiveId(id);
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT deleted_at FROM projects WHERE id=?");
    query.addBindValue(number);
    run(query);
    if (!query.next()) fail("Project not found.");
    if (query.value(0).isNull()) { transaction.commit(); return false; }
    query.finish();
    query.prepare("UPDATE projects SET deleted_at=NULL WHERE id=?");
    query.addBindValue(number);
    run(query);
    query.prepare("UPDATE tasks SET deleted_at=NULL,deleted_with_project_id=NULL WHERE deleted_with_project_id=?");
    query.addBindValue(number);
    run(query);
    transaction.commit();
    return true;
}

bool TaskStore::purgeProject(const QString &id) {
    const auto number = positiveId(id);
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT id FROM projects WHERE id=? AND deleted_at IS NOT NULL");
    query.addBindValue(number);
    run(query);
    if (!query.next()) fail("Deleted project not found.");
    query.finish();
    // Successors may have been moved to another project. Preserve them while
    // detaching references to occurrences that are about to be purged.
    query.prepare("UPDATE tasks SET recurrence_source_id=NULL WHERE recurrence_source_id IN "
                  "(SELECT id FROM tasks WHERE project_id=?)");
    query.addBindValue(number); run(query);
    query.prepare("DELETE FROM task_events WHERE task_id IN (SELECT id FROM tasks WHERE project_id=?)");
    query.addBindValue(number); run(query);
    query.prepare("DELETE FROM task_notifications WHERE task_id IN (SELECT id FROM tasks WHERE project_id=?)");
    query.addBindValue(number); run(query);
    query.prepare("DELETE FROM task_tags WHERE task_id IN (SELECT id FROM tasks WHERE project_id=?)");
    query.addBindValue(number); run(query);
    query.prepare("DELETE FROM tasks WHERE project_id=?");
    query.addBindValue(number); run(query);
    query.prepare("DELETE FROM task_lists WHERE project_id=?");
    query.addBindValue(number); run(query);
    query.prepare("UPDATE settings SET value='' WHERE key='default_project_id' AND value=?");
    query.addBindValue(QString::number(number)); run(query);
    query.prepare("DELETE FROM projects WHERE id=?");
    query.addBindValue(number); run(query);
    transaction.commit();
    return true;
}

bool TaskStore::moveProject(const QString &id, const QString &direction) {
    if (direction != "up" && direction != "down") fail("Direction must be up or down.");
    Transaction transaction(db_);
    validateProject(id, true);
    const auto ordered = projects(false);
    int index = -1;
    for (int i = 0; i < ordered.size(); ++i)
        if (ordered[i].id.toLongLong() == positiveId(id)) index = i;
    if (index < 0) fail("Project not found.");
    const int other = index + (direction == "up" ? -1 : 1);
    if (other < 0 || other >= ordered.size()) { transaction.commit(); return false; }
    QSqlQuery query(db_);
    for (const auto &pair : {qMakePair(index, other), qMakePair(other, index)}) {
        query.prepare("UPDATE projects SET sort_order=? WHERE id=?");
        query.addBindValue(ordered[pair.second].sortOrder);
        query.addBindValue(ordered[pair.first].id);
        run(query);
    }
    transaction.commit();
    return true;
}

void TaskStore::validateList(const QString &id, const QString &projectId) const {
    if (id.isEmpty()) return;
    if (projectId.isEmpty() || projectId == "*") fail("Choose a project for this task list.");
    QSqlQuery query(db_);
    query.prepare("SELECT id FROM task_lists WHERE id=? AND project_id=?");
    query.addBindValue(positiveId(id));
    query.addBindValue(positiveId(projectId));
    run(query);
    if (!query.next()) fail("Task list does not belong to this project.");
}

TaskList TaskStore::addList(const QString &projectId, const QString &name) {
    const auto cleaned = name.trimmed();
    if (cleaned.isEmpty() || cleaned.size() > 120) fail("List names must contain 1 to 120 characters.");
    if (projectId.isEmpty()) fail("Task lists require a project.");
    Transaction transaction(db_);
    validateProject(projectId, true);
    QSqlQuery query(db_);
    query.prepare("INSERT INTO task_lists(project_id,name,sort_order) VALUES (?,?,"
                  "(SELECT COALESCE(MAX(sort_order),-1)+1 FROM task_lists WHERE project_id=?))");
    query.addBindValue(positiveId(projectId));
    query.addBindValue(cleaned);
    query.addBindValue(positiveId(projectId));
    run(query);
    const QString listId = query.lastInsertId().toString();
    query.finish();
    query.prepare("SELECT id,project_id,name,sort_order FROM task_lists WHERE id=?");
    query.addBindValue(listId);
    run(query);
    if (!query.next()) fail("Cannot read the new task list.");
    TaskList list{query.value(0).toString(), query.value(1).toString(), query.value(2).toString(),
                  query.value(3).toLongLong()};
    transaction.commit();
    return list;
}

bool TaskStore::renameList(const QString &id, const QString &name) {
    const auto cleaned = name.trimmed();
    if (cleaned.isEmpty() || cleaned.size() > 120) fail("List names must contain 1 to 120 characters.");
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT project_id FROM task_lists WHERE id=?");
    query.addBindValue(positiveId(id));
    run(query);
    if (!query.next()) fail("Task list not found.");
    validateProject(query.value(0).toString(), true);
    query.finish();
    query.prepare("UPDATE task_lists SET name=? WHERE id=? AND name COLLATE BINARY <> ?");
    query.addBindValue(cleaned);
    query.addBindValue(positiveId(id));
    query.addBindValue(cleaned);
    run(query);
    const bool changed = query.numRowsAffected() > 0;
    transaction.commit();
    return changed;
}

bool TaskStore::moveList(const QString &id, const QString &direction) {
    if (direction != "up" && direction != "down") fail("Direction must be up or down.");
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT project_id FROM task_lists WHERE id=?");
    query.addBindValue(positiveId(id));
    run(query);
    if (!query.next()) fail("Task list not found.");
    const QString projectId = query.value(0).toString();
    query.finish();
    validateProject(projectId, true);
    const auto ordered = lists(projectId);
    int index = -1;
    for (int i = 0; i < ordered.size(); ++i)
        if (ordered[i].id.toLongLong() == positiveId(id)) index = i;
    if (index < 0) fail("Task list not found.");
    const int other = index + (direction == "up" ? -1 : 1);
    if (other < 0 || other >= ordered.size()) { transaction.commit(); return false; }
    for (const auto &pair : {qMakePair(index, other), qMakePair(other, index)}) {
        query.prepare("UPDATE task_lists SET sort_order=? WHERE id=?");
        query.addBindValue(ordered[pair.second].sortOrder);
        query.addBindValue(ordered[pair.first].id);
        run(query);
    }
    transaction.commit();
    return true;
}

QVector<TaskList> TaskStore::lists(const QString &projectId) const {
    if (projectId.isEmpty()) return {};
    validateProject(projectId);
    QSqlQuery query(db_);
    query.prepare("SELECT id,project_id,name,sort_order FROM task_lists WHERE project_id=? ORDER BY sort_order,id");
    query.addBindValue(positiveId(projectId));
    run(query);
    QVector<TaskList> result;
    while (query.next()) result.append({query.value(0).toString(), query.value(1).toString(),
                                       query.value(2).toString(), query.value(3).toLongLong()});
    return result;
}

Tag TaskStore::addTag(const QString &name, const QString &color) {
    const QString cleaned = tagNameText(name);
    const QString canonicalColor = tagColorText(color);
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT id FROM tags WHERE name=?");
    query.addBindValue(cleaned);
    run(query);
    if (query.next()) fail("A tag with this name already exists.");
    query.finish();
    query.prepare("INSERT INTO tags(name,color) VALUES (?,?)");
    query.addBindValue(cleaned);
    query.addBindValue(canonicalColor);
    run(query);
    Tag tag{query.lastInsertId().toString(), cleaned, canonicalColor};
    transaction.commit();
    return tag;
}

bool TaskStore::editTag(const QString &id, const QString &name, const QString &color) {
    const auto number = positiveId(id);
    const QString cleaned = tagNameText(name);
    const QString canonicalColor = tagColorText(color);
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT id FROM tags WHERE id=?");
    query.addBindValue(number);
    run(query);
    if (!query.next()) fail("Tag not found.");
    query.finish();
    query.prepare("SELECT id FROM tags WHERE name=? AND id<>?");
    query.addBindValue(cleaned);
    query.addBindValue(number);
    run(query);
    if (query.next()) fail("A tag with this name already exists.");
    query.finish();
    query.prepare("UPDATE tags SET name=?,color=? WHERE id=? AND (name COLLATE BINARY<>? OR color<>?)");
    query.addBindValue(cleaned);
    query.addBindValue(canonicalColor);
    query.addBindValue(number);
    query.addBindValue(cleaned);
    query.addBindValue(canonicalColor);
    run(query);
    const bool changed = query.numRowsAffected() > 0;
    transaction.commit();
    return changed;
}

QVector<Tag> TaskStore::tags() const {
    QSqlQuery query(db_);
    if (!query.exec("SELECT id,name,color FROM tags ORDER BY name COLLATE NOCASE,id"))
        fail(query.lastError().text());
    QVector<Tag> result;
    while (query.next())
        result.append({query.value(0).toString(), query.value(1).toString(), query.value(2).toString()});
    return result;
}

QStringList TaskStore::validateTagIds(const QStringList &tagIds) const {
    if (tagIds.size() > 50) fail("A task can have at most 50 tags.");
    QStringList result;
    QSet<qint64> seen;
    for (const auto &id : tagIds) {
        const auto number = positiveId(id);
        if (seen.contains(number)) fail("Each tag may be assigned only once.");
        QSqlQuery query(db_);
        query.prepare("SELECT id FROM tags WHERE id=?");
        query.addBindValue(number);
        run(query);
        if (!query.next()) fail("Tag not found.");
        seen.insert(number);
        result.append(QString::number(number));
    }
    return result;
}

QVector<Tag> TaskStore::taskTags(const QString &taskId) const {
    QSqlQuery query(db_);
    query.prepare("SELECT g.id,g.name,g.color FROM tags g JOIN task_tags tt ON tt.tag_id=g.id "
                  "WHERE tt.task_id=? ORDER BY g.name COLLATE NOCASE,g.id");
    query.addBindValue(positiveId(taskId));
    run(query);
    QVector<Tag> result;
    while (query.next())
        result.append({query.value(0).toString(), query.value(1).toString(), query.value(2).toString()});
    return result;
}

void TaskStore::replaceTaskTags(const QString &taskId, const QStringList &tagIds) {
    QSqlQuery query(db_);
    query.prepare("DELETE FROM task_tags WHERE task_id=?");
    query.addBindValue(positiveId(taskId));
    run(query);
    for (const auto &tagId : tagIds) {
        query.prepare("INSERT INTO task_tags(task_id,tag_id) VALUES (?,?)");
        query.addBindValue(positiveId(taskId));
        query.addBindValue(positiveId(tagId));
        run(query);
    }
}

qint64 TaskStore::nextOrder() const {
    QSqlQuery query(db_);
    if (!query.exec("SELECT COALESCE(MIN(sort_order),0)-1 FROM tasks") || !query.next()) fail("Cannot determine task order.");
    return query.value(0).toLongLong();
}

bool TaskStore::moveTask(const QString &id, const QString &direction, const QString &listFilter) {
    if (direction != "up" && direction != "down") fail("Direction must be up or down.");
    Transaction transaction(db_);
    QSqlQuery query(db_);
    query.prepare("SELECT project_id,completed_at,archived,deleted_at FROM tasks WHERE id=?");
    query.addBindValue(positiveId(id));
    run(query);
    if (!query.next()) fail("Task not found.");
    if (query.value(2).toBool()) fail("Restore the archived task before changing it.");
    if (!query.value(3).isNull()) fail("Task was deleted.");
    const auto project = query.value(0).toString();
    const bool completed = !query.value(1).isNull();
    query.finish();
    validateProject(project, true);
    const auto ordered = tasks(completed ? "completed" : "open", project, listFilter);
    int index = -1;
    for (int i = 0; i < ordered.size(); ++i) if (ordered[i].id.toLongLong() == positiveId(id)) index = i;
    if (index < 0) fail("Task is not in the selected list.");
    const int other = index + (direction == "up" ? -1 : 1);
    if (other < 0 || other >= ordered.size()) { transaction.commit(); return false; }
    const QString timestamp = now();
    for (const auto &pair : {qMakePair(index, other), qMakePair(other, index)}) {
        const auto &task = ordered[pair.first];
        const auto position = ordered[pair.second].sortOrder;
        query.prepare("UPDATE tasks SET sort_order=? WHERE id=?");
        query.addBindValue(position);
        query.addBindValue(task.id);
        run(query);
        event(db_, task.id, "edited", timestamp,
              QString::fromUtf8(QJsonDocument(QJsonObject{{"sort_order", task.sortOrder}}).toJson(QJsonDocument::Compact)),
              QString::fromUtf8(QJsonDocument(QJsonObject{{"sort_order", position}}).toJson(QJsonDocument::Compact)));
    }
    transaction.commit();
    return true;
}
