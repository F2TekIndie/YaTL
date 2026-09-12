#include "taskmodel.h"
#include <exception>

TaskModel::TaskModel(TaskStore &store, QObject *parent) : QAbstractListModel(parent), store_(store) {
    refresh();
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, [this] {
        try { if (store_.dataVersion() != dataVersion_) refresh(); }
        catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); }
    });
    timer_.start();
}
int TaskModel::rowCount(const QModelIndex &parent) const { return parent.isValid() ? 0 : tasks_.size(); }
QVariant TaskModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= tasks_.size()) return {};
    const auto &task = tasks_.at(index.row());
    switch (role) {
    case IdRole: return task.id;
    case TitleRole: return task.title;
    case CompletedRole: return task.completed();
    case CompletedAtRole: return task.completedAt;
    case ProjectRole: return task.projectId;
    case NoteRole: return task.note;
    default: return {};
    }
}
QHash<int, QByteArray> TaskModel::roleNames() const {
    return {{IdRole, "taskId"}, {TitleRole, "title"}, {CompletedRole, "completed"},
            {CompletedAtRole, "completedAt"}, {ProjectRole, "projectId"}, {NoteRole, "note"}};
}
void TaskModel::setError(const QString &error) {
    if (error_ == error) return;
    error_ = error;
    emit errorChanged();
}
void TaskModel::setFilter(const QString &filter) {
    if (filter != "open" && filter != "completed" && filter != "all") {
        setError("Unknown task filter.");
        return;
    }
    if (filter_ == filter) return;
    filter_ = filter;
    emit filterChanged();
    refresh();
}
void TaskModel::refresh() {
    try {
        // Read the version before the rows so concurrent changes trigger another refresh.
        const int version = store_.dataVersion();
        auto tasks = store_.tasks(filter_, projectId_);
        QVariantList projects{QVariantMap{{"id", ""}, {"name", "Inbox"}}};
        for (const auto &project : store_.projects())
            projects.append(QVariantMap{{"id", project.id}, {"name", project.name}});
        if (projects_ != projects) {
            projects_ = projects;
            emit projectsChanged();
        }
        beginResetModel();
        tasks_ = std::move(tasks);
        endResetModel();
        dataVersion_ = version;
        emit countChanged();
        setError({});
    } catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); }
}
bool TaskModel::add(const QString &title) {
    try { store_.add(title, projectId_); }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    setFilter("open");
    refresh();
    return true;
}
bool TaskModel::complete(const QString &id) {
    try { store_.complete(id); }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}

void TaskModel::setProjectId(const QString &id) {
    if (id == projectId_) return;
    bool found = false;
    for (const auto &project : projects_)
        if (project.toMap().value("id").toString() == id) found = true;
    if (!found) { setError("Project not found."); return; }
    projectId_ = id;
    emit projectIdChanged();
    refresh();
}
bool TaskModel::addProject(const QString &name) {
    try {
        const auto project = store_.addProject(name);
        refresh();
        setProjectId(project.id);
        setFilter("open");
        return true;
    } catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
}
bool TaskModel::edit(const QString &id, const QString &title, const QString &note, const QString &projectId) {
    try { store_.edit(id, title, note, projectId); }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}
bool TaskModel::reopen(const QString &id) {
    try { store_.reopen(id); }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}
