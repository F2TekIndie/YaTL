#pragma once

#include <QSqlDatabase>
#include <QString>
#include <QVector>

struct Task {
    QString id;
    QString title;
    QString createdAt;
    QString completedAt;
    QString projectId;
    QString note;
    bool completed() const { return !completedAt.isEmpty(); }
};

struct Project {
    QString id;
    QString name;
};

class TaskStore {
public:
    static constexpr int SchemaVersion = 3;
    explicit TaskStore(const QString &path);
    ~TaskStore();
    TaskStore(const TaskStore &) = delete;
    TaskStore &operator=(const TaskStore &) = delete;

    static QString defaultPath();
    Project addProject(const QString &name);
    QVector<Project> projects() const;
    Task add(const QString &title, const QString &projectId = {});
    bool edit(const QString &id, const QString &title, const QString &note, const QString &projectId);
    bool complete(const QString &id);
    bool reopen(const QString &id);
    QVector<Task> tasks(const QString &filter = QStringLiteral("open"),
                        const QString &projectId = QStringLiteral("*")) const;
    int dataVersion() const;

private:
    QSqlDatabase db_;
    void migrate();
    void validateProject(const QString &id) const;
    bool setCompleted(const QString &id, bool completed);
};
