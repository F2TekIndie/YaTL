#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QStringList>
#include <QVector>

struct Tag {
    QString id;
    QString name;
    QString color = "#59675c";
};

struct Task {
    QString id;
    QString title;
    QString createdAt;
    QString completedAt;
    QString projectId;
    QString note;
    QString listId;
    qint64 sortOrder = 0;
    QString scheduledDate;
    QString dueDate;
    int priority = 0;
    QString projectName;
    QString listName;
    bool archived = false;
    QVector<Tag> tags;
    QString recurrence = "none";
    QString recurrenceSourceId;
    QString deletedAt;
    QString deletedWithProjectId;
    bool completed() const { return !completedAt.isEmpty(); }
};

struct Project {
    QString id;
    QString name;
    QString color = "#376548";
    bool archived = false;
    qint64 sortOrder = 0;
    QString deletedAt;
};

struct TaskList {
    QString id;
    QString projectId;
    QString name;
    qint64 sortOrder = 0;
};

struct TaskNotification {
    QString taskId;
    QString title;
    QString kind;
    QString date;
};

struct Settings {
    QString defaultProjectId;
    bool notificationsEnabled = true;
    int notificationDaysBefore = 0;
    bool dmsShowNextTask = true;
    bool dmsUseDefaultProject = false;
};

class TaskStore {
public:
    static constexpr int SchemaVersion = 11;
    explicit TaskStore(const QString &path);
    ~TaskStore();
    TaskStore(const TaskStore &) = delete;
    TaskStore &operator=(const TaskStore &) = delete;

    static QString defaultPath();
    Project addProject(const QString &name);
    bool editProject(const QString &id, const QString &name, const QString &color);
    bool archiveProject(const QString &id, bool archived);
    bool deleteProject(const QString &id);
    bool restoreProject(const QString &id);
    bool purgeProject(const QString &id);
    bool moveProject(const QString &id, const QString &direction);
    QVector<Project> projects(bool includeArchived = false) const;
    TaskList addList(const QString &projectId, const QString &name);
    bool renameList(const QString &id, const QString &name);
    bool moveList(const QString &id, const QString &direction);
    QVector<TaskList> lists(const QString &projectId) const;
    Tag addTag(const QString &name, const QString &color);
    bool editTag(const QString &id, const QString &name, const QString &color);
    QVector<Tag> tags() const;
    bool moveTask(const QString &id, const QString &direction, const QString &listFilter = "*");
    Task add(const QString &title, const QString &projectId = {}, const QString &listId = {},
             const QString &scheduledDate = {}, const QString &dueDate = {}, int priority = 0,
             const QStringList &tagIds = {}, const QString &recurrence = "none");
    bool edit(const QString &id, const QString &title, const QString &note, const QString &projectId,
              const QString &listId = {}, const QString &scheduledDate = {},
              const QString &dueDate = {}, int priority = 0, const QStringList &tagIds = {},
              const QString &recurrence = "none");
    Task task(const QString &id) const;
    bool complete(const QString &id);
    bool reopen(const QString &id);
    bool archiveTask(const QString &id, bool archived);
    bool deleteTask(const QString &id);
    bool restoreTask(const QString &id);
    bool purgeTask(const QString &id);
    QVector<Task> tasks(const QString &filter = QStringLiteral("open"),
                        const QString &projectId = QStringLiteral("*"),
                        const QString &listId = QStringLiteral("*"),
                        const QString &scope = QStringLiteral("project"),
                        const QString &search = {}, const QString &tagId = {}) const;
    int dataVersion() const;
    QVector<TaskNotification> pendingNotifications(const QString &throughDate) const;
    bool markNotificationSent(const TaskNotification &notification);
    Settings settings() const;
    void saveSettings(const Settings &settings);

private:
    QSqlDatabase db_;
    void migrate();
    void validateProject(const QString &id, bool writable = false) const;
    void validateList(const QString &id, const QString &projectId) const;
    void validatePlanning(const QString &scheduledDate, const QString &dueDate, int priority) const;
    void validateRecurrence(const QString &recurrence, const QString &scheduledDate,
                            const QString &dueDate) const;
    QStringList validateTagIds(const QStringList &tagIds) const;
    QVector<Tag> taskTags(const QString &taskId) const;
    void replaceTaskTags(const QString &taskId, const QStringList &tagIds);
    qint64 nextOrder() const;
    bool setCompleted(const QString &id, bool completed);
};
