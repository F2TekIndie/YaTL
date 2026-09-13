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
    Q_PROPERTY(bool showArchived READ showArchived WRITE setShowArchived NOTIFY stateChanged)
    Q_PROPERTY(QVariantMap projectInfo READ projectInfo NOTIFY stateChanged)
    Q_PROPERTY(QString listFilter READ listFilter WRITE setListFilter NOTIFY listFilterChanged)
    Q_PROPERTY(QVariantList taskLists READ taskLists NOTIFY stateChanged)
    Q_PROPERTY(QString view READ view WRITE setView NOTIFY stateChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY stateChanged)
    Q_PROPERTY(QVariantList tags READ tags NOTIFY stateChanged)
    Q_PROPERTY(QString tagFilter READ tagFilter WRITE setTagFilter NOTIFY tagFilterChanged)
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)
public:
    enum Role {
        IdRole = Qt::UserRole + 1, TitleRole, CompletedRole, CompletedAtRole,
        ProjectRole, NoteRole, ListRole, ScheduledRole, DueRole, PriorityRole,
        ProjectNameRole, ListNameRole, ArchivedRole, TagsRole, RecurrenceRole,
        RecurrenceSourceRole
    };
    explicit TaskModel(TaskStore &store, QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;
    QString filter() const { return filter_; }
    QString error() const { return error_; }
    int count() const { return tasks_.size(); }
    QString projectId() const { return projectId_; }
    QVariantList projects() const { return projects_; }
    bool showArchived() const { return showArchived_; }
    QString listFilter() const { return listFilter_; }
    QVariantMap projectInfo() const;
    QVariantList taskLists() const { return taskLists_; }
    QString view() const { return view_; }
    QString searchText() const { return searchText_; }
    QVariantList tags() const { return tags_; }
    QString tagFilter() const { return tagFilter_; }
    QVariantMap settings() const { return settings_; }
    Q_INVOKABLE QVariantList listsFor(const QString &projectId) const;
    Q_INVOKABLE QVariantMap get(int row) const;
    void setShowArchived(bool show);
    void setListFilter(const QString &id);
    void setProjectId(const QString &id);
    void setFilter(const QString &filter);
    void setView(const QString &view);
    void setSearchText(const QString &text);
    void setTagFilter(const QString &id);
    Q_INVOKABLE bool add(const QString &title);
    Q_INVOKABLE bool complete(const QString &id);
    Q_INVOKABLE bool reopen(const QString &id);
    Q_INVOKABLE bool archiveTask(const QString &id, bool archived);
    Q_INVOKABLE bool addProject(const QString &name);
    Q_INVOKABLE bool edit(const QString &id, const QString &title, const QString &note,
                          const QString &projectId, const QString &listId,
                          const QString &scheduledDate, const QString &dueDate, int priority,
                          const QStringList &tagIds, const QString &recurrence);
    Q_INVOKABLE bool addTag(const QString &name, const QString &color);
    Q_INVOKABLE bool editTag(const QString &id, const QString &name, const QString &color);
    Q_INVOKABLE bool editProject(const QString &name, const QString &color);
    Q_INVOKABLE bool archiveProject(bool archived);
    Q_INVOKABLE bool moveProject(const QString &direction);
    Q_INVOKABLE bool addList(const QString &name);
    Q_INVOKABLE bool renameList(const QString &name);
    Q_INVOKABLE bool moveList(const QString &direction);
    Q_INVOKABLE bool moveTask(const QString &id, const QString &direction);
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void clearError() { setError({}); }
    Q_INVOKABLE bool saveSettings(const QString &defaultProjectId, bool notificationsEnabled,
                                  int notificationDaysBefore, bool dmsShowNextTask,
                                  bool dmsUseDefaultProject);
signals:
    void stateChanged();
    void filterChanged();
    void errorChanged();
    void countChanged();
    void projectIdChanged();
    void projectsChanged();
    void settingsChanged();
    void listFilterChanged();
    void tagFilterChanged();
private:
    void setError(const QString &error);
    void refreshTaskRows();
    TaskStore &store_;
    QVector<Task> tasks_;
    QString filter_ = "open";
    QString error_;
    QString projectId_;
    QVariantList projects_;
    QVariantList taskLists_;
    QVariantList tags_;
    QVariantMap settings_;
    QString listFilter_ = "*";
    bool showArchived_ = false;
    QString view_ = "project";
    QString searchText_;
    QString tagFilter_ = "*";
    QTimer timer_;
    QTimer searchDebounce_;
    int dataVersion_ = 0;
};
