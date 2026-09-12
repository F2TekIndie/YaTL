#pragma once

#include "taskstore.h"
#include <QAbstractListModel>
#include <QTimer>

class TaskModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(QString projectId READ projectId WRITE setProjectId NOTIFY projectIdChanged)
    Q_PROPERTY(QVariantList projects READ projects NOTIFY projectsChanged)
public:
    enum Role { IdRole = Qt::UserRole + 1, TitleRole, CompletedRole, CompletedAtRole, ProjectRole, NoteRole };
    explicit TaskModel(TaskStore &store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QString filter() const { return filter_; }
    QString error() const { return error_; }
    int count() const { return tasks_.size(); }
    QString projectId() const { return projectId_; }
    QVariantList projects() const { return projects_; }
    void setProjectId(const QString &id);
    void setFilter(const QString &filter);
    Q_INVOKABLE bool add(const QString &title);
    Q_INVOKABLE bool complete(const QString &id);
    Q_INVOKABLE bool reopen(const QString &id);
    Q_INVOKABLE bool addProject(const QString &name);
    Q_INVOKABLE bool edit(const QString &id, const QString &title, const QString &note, const QString &projectId);
    Q_INVOKABLE void refresh();
signals:
    void filterChanged();
    void errorChanged();
    void countChanged();
    void projectIdChanged();
    void projectsChanged();
private:
    void setError(const QString &error);
    TaskStore &store_;
    QVector<Task> tasks_;
    QString filter_ = "open";
    QString error_;
    QString projectId_;
    QVariantList projects_;
    QTimer timer_;
    int dataVersion_ = 0;
};
