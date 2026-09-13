#include "taskmodel.h"
#include <exception>

TaskModel::TaskModel(TaskStore &store, QObject *parent) : QAbstractListModel(parent), store_(store) {
    projectId_ = store_.settings().defaultProjectId;
    refresh();
    timer_.setInterval(1000);
    connect(&timer_, &QTimer::timeout, this, [this] {
        try { if (store_.dataVersion() != dataVersion_) refresh(); }
        catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); }
    });
    timer_.start();
    searchDebounce_.setSingleShot(true);
    searchDebounce_.setInterval(250);
    connect(&searchDebounce_, &QTimer::timeout, this, &TaskModel::refreshTaskRows);
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
    case ListRole: return task.listId;
    case ScheduledRole: return task.scheduledDate;
    case DueRole: return task.dueDate;
    case PriorityRole: return task.priority;
    case ProjectNameRole: return task.projectName;
    case ListNameRole: return task.listName;
    case ArchivedRole: return task.archived;
    case RecurrenceRole: return task.recurrence;
    case RecurrenceSourceRole: return task.recurrenceSourceId;
    case TagsRole: {
        QVariantList tags;
        for (const auto &tag : task.tags)
            tags.append(QVariantMap{{"id", tag.id}, {"name", tag.name}, {"color", tag.color}});
        return tags;
    }
    default: return {};
    }
}

QVariantMap TaskModel::get(int row) const {
    QVariantMap result;
    if (row < 0 || row >= rowCount()) return result;
    const auto item = index(row);
    const auto roles = roleNames();
    for (auto it = roles.cbegin(); it != roles.cend(); ++it)
        result.insert(QString::fromUtf8(it.value()), data(item, it.key()));
    return result;
}
QHash<int, QByteArray> TaskModel::roleNames() const {
    return {{IdRole, "taskId"}, {TitleRole, "title"}, {CompletedRole, "completed"},
            {CompletedAtRole, "completedAt"}, {ProjectRole, "projectId"},
            {NoteRole, "note"}, {ListRole, "listId"}, {ScheduledRole, "scheduledDate"},
            {DueRole, "dueDate"}, {PriorityRole, "priority"},
            {ProjectNameRole, "projectName"}, {ListNameRole, "listName"},
            {ArchivedRole, "archived"}, {TagsRole, "tags"},
            {RecurrenceRole, "recurrence"}, {RecurrenceSourceRole, "recurrenceSourceId"}};
}
void TaskModel::setError(const QString &error) {
    if (error_ == error) return;
    error_ = error;
    emit errorChanged();
}
void TaskModel::setFilter(const QString &filter) {
    if (filter != "open" && filter != "completed" && filter != "archived" && filter != "all") {
        setError("Unknown task filter.");
        return;
    }
    if (filter_ == filter) return;
    filter_ = filter;
    emit filterChanged();
    refresh();
}
void TaskModel::setView(const QString &view) {
    if (view != "project" && view != "today" && view != "upcoming" && view != "search") {
        setError("Unknown task view.");
        return;
    }
    if (view_ == view) return;
    view_ = view;
    refresh();
}
void TaskModel::setSearchText(const QString &text) {
    if (searchText_ == text) return;
    searchText_ = text;
    if (view_ == "search") {
        // Invalidate the visible result immediately so callers never observe a
        // previous query's rows while the debounced lookup is pending.
        if (!tasks_.isEmpty()) {
            beginResetModel();
            tasks_.clear();
            endResetModel();
            emit countChanged();
        }
        searchDebounce_.start();
        emit stateChanged();
    }
    else emit stateChanged();
}
void TaskModel::refreshTaskRows() {
    if (view_ != "search") return;
    try {
        const auto next = store_.tasks("all", "*", "*", "search", searchText_);
        bool same = next.size() == tasks_.size();
        if (same) {
            for (int i = 0; i < next.size() && same; ++i) {
                const auto &a = next.at(i); const auto &b = tasks_.at(i);
                same = a.id == b.id && a.title == b.title && a.note == b.note &&
                       a.completedAt == b.completedAt && a.projectId == b.projectId &&
                       a.listId == b.listId && a.archived == b.archived && a.tags.size() == b.tags.size();
                for (int j = 0; same && j < a.tags.size(); ++j) same = a.tags.at(j).id == b.tags.at(j).id;
            }
        }
        if (same) return;
        beginResetModel(); tasks_ = next; endResetModel();
        dataVersion_ = store_.dataVersion();
        emit countChanged();
    } catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); }
}
void TaskModel::setTagFilter(const QString &id) {
    if (tagFilter_ == id) return;
    bool found = id == "*";
    for (const auto &tag : tags_)
        if (tag.toMap().value("id").toString() == id) found = true;
    if (!found) { setError("Tag not found."); return; }
    tagFilter_ = id;
    emit tagFilterChanged();
    refresh();
}
void TaskModel::refresh() {
    try {
        // Read the version before the rows so concurrent changes trigger another refresh.
        const int version = store_.dataVersion();
        QVariantList projects{QVariantMap{{"id", ""}, {"name", "Inbox"}, {"displayName", "Inbox"},
                                           {"color", "#376548"}, {"archived", false}, {"sortOrder", -1}}};
        for (const auto &project : store_.projects(showArchived_))
            projects.append(QVariantMap{{"id", project.id}, {"name", project.name},
                                        {"displayName", project.name + (project.archived ? " (archived)" : "")},
                                        {"color", project.color}, {"archived", project.archived},
                                        {"sortOrder", project.sortOrder}});
        if (projects_ != projects) {
            projects_ = projects;
            emit projectsChanged();
        }
        bool found = false;
        for (const auto &item : projects_) if (item.toMap().value("id").toString() == projectId_) found = true;
        if (!found) {
            projectId_.clear();
            if (listFilter_ != "*") { listFilter_ = "*"; emit listFilterChanged(); }
            emit projectIdChanged();
        }
        taskLists_.clear();
        for (const auto &project : store_.projects(true))
            for (const auto &list : store_.lists(project.id))
                taskLists_.append(QVariantMap{{"id", list.id}, {"projectId", list.projectId},
                                              {"name", list.name}, {"sortOrder", list.sortOrder}});
        tags_.clear();
        for (const auto &tag : store_.tags())
            tags_.append(QVariantMap{{"id", tag.id}, {"name", tag.name}, {"color", tag.color}});
        const auto storedSettings = store_.settings();
        const QVariantMap settings{{"defaultProjectId", storedSettings.defaultProjectId},
                                   {"notificationsEnabled", storedSettings.notificationsEnabled},
                                   {"notificationDaysBefore", storedSettings.notificationDaysBefore},
                                   {"dmsShowNextTask", storedSettings.dmsShowNextTask},
                                   {"dmsUseDefaultProject", storedSettings.dmsUseDefaultProject}};
        if (settings_ != settings) { settings_ = settings; emit settingsChanged(); }
        bool tagFound = tagFilter_ == "*";
        for (const auto &tag : tags_)
            if (tag.toMap().value("id").toString() == tagFilter_) tagFound = true;
        if (!tagFound && tagFilter_ != "*") { tagFilter_ = "*"; emit tagFilterChanged(); }
        const QString effectiveFilter = view_ == "project" ? filter_ : (view_ == "search" ? "all" : "open");
        auto tasks = store_.tasks(effectiveFilter, projectId_, listFilter_, view_, searchText_,
                                  view_ == "project" ? tagFilter_ : "*");
        bool rowsChanged = tasks.size() != tasks_.size();
        for (int i = 0; !rowsChanged && i < tasks.size(); ++i) {
            const auto &a = tasks.at(i);
            const auto &b = tasks_.at(i);
            rowsChanged = a.id != b.id || a.title != b.title || a.completedAt != b.completedAt
                || a.projectId != b.projectId || a.note != b.note || a.listId != b.listId
                || a.sortOrder != b.sortOrder || a.scheduledDate != b.scheduledDate
                || a.dueDate != b.dueDate || a.priority != b.priority || a.archived != b.archived
                || a.recurrence != b.recurrence || a.recurrenceSourceId != b.recurrenceSourceId
                || a.tags.size() != b.tags.size();
            for (int j = 0; !rowsChanged && j < a.tags.size(); ++j)
                rowsChanged = a.tags.at(j).id != b.tags.at(j).id
                    || a.tags.at(j).name != b.tags.at(j).name
                    || a.tags.at(j).color != b.tags.at(j).color;
        }
        if (rowsChanged) {
            beginResetModel();
            tasks_ = std::move(tasks);
            endResetModel();
        }
        emit stateChanged();
        dataVersion_ = version;
        if (rowsChanged) emit countChanged();
        setError({});
    } catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); }
}

bool TaskModel::saveSettings(const QString &defaultProjectId, bool notificationsEnabled,
                             int notificationDaysBefore, bool dmsShowNextTask,
                             bool dmsUseDefaultProject) {
    try {
        store_.saveSettings({defaultProjectId, notificationsEnabled, notificationDaysBefore,
                             dmsShowNextTask, dmsUseDefaultProject});
    } catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}
bool TaskModel::add(const QString &title) {
    try { store_.add(title, projectId_, listFilter_ == "*" ? QString() : listFilter_); }
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
    listFilter_ = "*";
    emit listFilterChanged();
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
bool TaskModel::edit(const QString &id, const QString &title, const QString &note,
                     const QString &projectId, const QString &listId,
                     const QString &scheduledDate, const QString &dueDate, int priority,
                     const QStringList &tagIds, const QString &recurrence) {
    try { store_.edit(id, title, note, projectId, listId, scheduledDate, dueDate, priority, tagIds, recurrence); }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}

bool TaskModel::addTag(const QString &name, const QString &color) {
    try {
        const auto previous = tagFilter_;
        tagFilter_ = store_.addTag(name, color).id;
        if (tagFilter_ != previous) emit tagFilterChanged();
    }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}

bool TaskModel::editTag(const QString &id, const QString &name, const QString &color) {
    try { store_.editTag(id, name, color); }
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

bool TaskModel::archiveTask(const QString &id, bool archived) {
    try { store_.archiveTask(id, archived); }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}

QVariantMap TaskModel::projectInfo() const {
    for (const auto &item : projects_) if (item.toMap().value("id").toString() == projectId_) return item.toMap();
    return {};
}
QVariantList TaskModel::listsFor(const QString &projectId) const {
    QVariantList result{QVariantMap{{"id", ""}, {"name", "No list"}}};
    for (const auto &item : taskLists_) if (item.toMap().value("projectId").toString() == projectId) result.append(item);
    return result;
}
void TaskModel::setShowArchived(bool show) {
    if (showArchived_ == show) return;
    showArchived_ = show;
    refresh();
}
void TaskModel::setListFilter(const QString &id) {
    if (id == listFilter_) return;
    bool found = id == "*";
    for (const auto &item : listsFor(projectId_)) if (item.toMap().value("id").toString() == id) found = true;
    if (!found) { setError("Task list not found in this project."); return; }
    listFilter_ = id;
    emit listFilterChanged();
    refresh();
}

bool TaskModel::editProject(const QString &name, const QString &color) {
    try { store_.editProject(projectId_, name, color); }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}

bool TaskModel::archiveProject(bool archived) {
    try { store_.archiveProject(projectId_, archived); }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}

bool TaskModel::moveProject(const QString &direction) {
    try { store_.moveProject(projectId_, direction); }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}

bool TaskModel::addList(const QString &name) {
    try {
        const auto list = store_.addList(projectId_, name);
        if (listFilter_ != list.id) {
            listFilter_ = list.id;
            emit listFilterChanged();
        }
    }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}

bool TaskModel::renameList(const QString &name) {
    try { store_.renameList(listFilter_, name); }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}

bool TaskModel::moveList(const QString &direction) {
    try { store_.moveList(listFilter_, direction); }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}

bool TaskModel::moveTask(const QString &id, const QString &direction) {
    try { store_.moveTask(id, direction, listFilter_); }
    catch (const std::exception &e) { setError(QString::fromUtf8(e.what())); return false; }
    refresh();
    return true;
}
