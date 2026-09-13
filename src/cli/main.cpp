#include "taskstore.h"
#include "desktopipc.h"
#include "notificationservice.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QFileInfo>
#include <QSaveFile>
#include <QProcess>
#include <QSet>
#include <QStandardPaths>
#include <QTextStream>
#include <stdexcept>

namespace {
void output(const QJsonObject &object, FILE *stream = stdout) {
    QTextStream(stream) << QJsonDocument(object).toJson(QJsonDocument::Compact) << '\n';
}
QJsonObject json(const Task &task) {
    QJsonArray tags;
    for (const auto &tag : task.tags)
        tags.append(QJsonObject{{"id", tag.id}, {"name", tag.name}, {"color", tag.color}});
    return {{"id", task.id}, {"title", task.title}, {"created_at", task.createdAt},
            {"completed_at", task.completed() ? QJsonValue(task.completedAt) : QJsonValue(QJsonValue::Null)},
            {"project_id", task.projectId.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(task.projectId)},
            {"list_id", task.listId.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(task.listId)},
            {"sort_order", task.sortOrder},
            {"scheduled_date", task.scheduledDate.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(task.scheduledDate)},
            {"due_date", task.dueDate.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(task.dueDate)},
            {"priority", task.priority}, {"project_name", task.projectName}, {"list_name", task.listName},
            {"note", task.note}, {"archived", task.archived},
            {"recurrence", task.recurrence},
            {"recurrence_source_id", task.recurrenceSourceId.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(task.recurrenceSourceId)},
            {"tags", tags},
            {"status", task.archived ? "archived" : (task.completed() ? "completed" : "open")}};
}
QJsonObject jsonTasks(const QVector<Task> &items) {
    QJsonArray tasks;
    for (const auto &task : items) tasks.append(json(task));
    return {{"tasks", tasks}};
}
QJsonObject json(const Settings &settings) {
    return {{"default_project_id", settings.defaultProjectId.isEmpty() ? QJsonValue(QJsonValue::Null)
                                                                       : QJsonValue(settings.defaultProjectId)},
            {"notifications_enabled", settings.notificationsEnabled},
            {"notification_days_before", settings.notificationDaysBefore},
            {"dms_show_next_task", settings.dmsShowNextTask},
            {"dms_use_default_project", settings.dmsUseDefaultProject}};
}
QJsonObject json(const Project &project) {
    return {{"id", project.id}, {"name", project.name}, {"color", project.color},
            {"archived", project.archived}, {"sort_order", project.sortOrder}};
}
QJsonObject json(const TaskList &list) {
    return {{"id", list.id}, {"project_id", list.projectId}, {"name", list.name},
            {"sort_order", list.sortOrder}};
}
QJsonObject json(const Tag &tag) {
    return {{"id", tag.id}, {"name", tag.name}, {"color", tag.color}};
}
QJsonObject exportData(const TaskStore &store) {
    QJsonArray projects;
    for (const auto &project : store.projects(true)) projects.append(json(project));
    QJsonArray lists;
    for (const auto &project : store.projects(true))
        for (const auto &list : store.lists(project.id)) lists.append(json(list));
    QJsonArray tags;
    for (const auto &tag : store.tags()) tags.append(json(tag));
    QJsonArray tasks;
    for (const auto &task : store.tasks("all", "*", "*", "project")) tasks.append(json(task));
    return {{"format", "yatl-export-v1"}, {"schema_version", TaskStore::SchemaVersion},
            {"exported_at", QDateTime::currentDateTimeUtc().toString(Qt::ISODate)},
            {"settings", json(store.settings())}, {"projects", projects}, {"lists", lists},
            {"tags", tags}, {"tasks", tasks}};
}

bool focusNiriWindow(bool quickCapture) {
    QProcess windows;
    windows.start("niri", {"msg", "-j", "windows"});
    if (!windows.waitForFinished(1500) || windows.exitCode() != 0) return false;
    const auto document = QJsonDocument::fromJson(windows.readAllStandardOutput());
    if (!document.isArray()) return false;
    const QString appId = quickCapture ? "org.yatl.YaTL.QuickCapture" : "org.yatl.YaTL";
    qint64 windowId = -1;
    for (const auto &value : document.array()) {
        const auto window = value.toObject();
        if (window.value("app_id").toString() != appId) continue;
        if (window.value("is_focused").toBool()) return true;
        windowId = window.value("id").toInteger(-1);
        break;
    }
    if (windowId < 0) return false;
    QProcess focus;
    focus.start("niri", {"msg", "action", "focus-window", "--id", QString::number(windowId)});
    return focus.waitForFinished(1500) && focus.exitCode() == 0;
}

QJsonObject activateOrLaunch(const QString &databasePath, QString view, bool quickCapture) {
    const QString serverName = DesktopIpc::serverName(databasePath, quickCapture);
    const QByteArray command = (quickCapture || view.isEmpty())
        ? QByteArrayLiteral("focus") : QByteArray("view:") + view.toUtf8();
    const QByteArray reply = DesktopIpc::request(serverName, command);
    if (!reply.isEmpty()) {
        const auto acknowledgement = QJsonDocument::fromJson(reply).object();
        return {{"action", "activated"},
                {"view", acknowledgement.value("view").toString(quickCapture ? "capture" : view)},
                {"niri_focused", focusNiriWindow(quickCapture)}};
    }

    QString executable = qEnvironmentVariable("YATL_APP_EXECUTABLE");
    if (executable.isEmpty()) {
        const QString sibling = QCoreApplication::applicationDirPath() + "/yatl";
        executable = QFileInfo(sibling).isExecutable() ? sibling : QStandardPaths::findExecutable("yatl");
    }
    if (executable.isEmpty()) throw std::runtime_error("Cannot find the yatl application executable.");
    QStringList arguments{"--database", databasePath};
    if (quickCapture) arguments.append("--quick-capture");
    else arguments.append({"--view", view.isEmpty() ? "project" : view});
    qint64 pid = 0;
    if (!QProcess::startDetached(executable, arguments, {}, &pid))
        throw std::runtime_error("Cannot launch the yatl application.");
    return {{"action", "launched"},
            {"view", quickCapture ? "capture" : (view.isEmpty() ? "project" : view)}, {"pid", pid}};
}
}
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("yatlctl");
    app.setApplicationVersion("0.9.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("YaTL local task commands. Successful commands return JSON on stdout; errors return JSON on stderr.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"database", "Use a specific SQLite database.", "path", TaskStore::defaultPath()});
    parser.addOption({"filter", "Task filter: open, completed, archived, all.", "filter", "open"});
    parser.addOption({"project", "Project ID or inbox; list also accepts all.", "project"});
    parser.addOption({"note", "Replacement note for edit (defaults to empty).", "note", ""});
    parser.addOption({"list", "Task-list ID, none, or all where supported.", "list"});
    parser.addOption({"color", "Project or tag color in #RRGGBB format.", "color"});
    parser.addOption({"archived", "Include archived projects in projects output."});
    parser.addOption({"scheduled", "Scheduled date YYYY-MM-DD, or none.", "date"});
    parser.addOption({"due", "Due date YYYY-MM-DD, or none.", "date"});
    parser.addOption({"priority", "Priority from 0 (none) to 3 (high).", "priority"});
    parser.addOption({"tags", "Comma-separated tag IDs, or none.", "ids"});
    parser.addOption({"tag", "Filter tasks by tag ID, or all.", "id"});
    parser.addOption({"recurrence", "Recurrence: none, daily, weekdays, weekly, monthly.", "pattern"});
    parser.addOption({"use-default", "Use the configured default project for add."});
    parser.addOption({"default-project", "Default project ID or inbox.", "project"});
    parser.addOption({"notifications", "Desktop notifications: on or off.", "state"});
    parser.addOption({"notification-days", "Notify 0 to 30 days before a task date.", "days"});
    parser.addOption({"dms-next", "Show the next task in the DMS bar: on or off.", "state"});
    parser.addOption({"dms-capture", "DMS quick add destination: inbox or default.", "destination"});
    parser.addOption({"output", "Write export JSON to a file instead of stdout.", "path"});
    parser.addPositionalArgument("command", "add TITLE | list | today | upcoming | search QUERY | complete ID | reopen ID | archive ID | restore ID | summary | notify | export [PATH] | open [project|today|upcoming|search] | focus | capture | project-add NAME | projects | edit ID TITLE --project ID|inbox --note NOTE | project-edit ID NAME --color COLOR | project-archive ID | project-restore ID | project-move ID up|down | list-add PROJECT_ID NAME | lists PROJECT_ID | list-rename ID NAME | list-move ID up|down | move ID up|down | tag-add NAME --color COLOR | tag-edit ID NAME --color COLOR | tags");
    parser.addPositionalArgument("argument", "Title or task ID; use -- before titles starting with a dash.", "[argument]");
    if (!parser.parse(app.arguments())) {
        output({{"error", parser.errorText()}}, stderr);
        return 2;
    }
    if (parser.isSet("help")) parser.showHelp();
    if (parser.isSet("version")) parser.showVersion();
    const auto args = parser.positionalArguments();
    const QString command = args.value(0);
    const QMap<QString,int> arity{{"add",2},{"complete",2},{"reopen",2},{"archive",2},{"restore",2},{"project-add",2},
        {"list",1},{"today",1},{"upcoming",1},{"search",2},{"summary",1},{"projects",1},{"edit",3},{"project-edit",3},{"project-archive",2},
        {"project-restore",2},{"project-move",3},{"list-add",3},{"lists",2},{"list-rename",3},{"list-move",3},{"move",3},
        {"tag-add",2},{"tag-edit",3},{"tags",1},{"focus",1},{"capture",1},{"notify",1},{"export",1},
        {"settings",1},{"settings-set",1}};
    const QMap<QString,QSet<QString>> allowed{{"add",{"project","list","scheduled","due","priority","tags","recurrence","use-default"}},
        {"list",{"project","filter","list","tag"}},
        {"edit",{"project","note","list","scheduled","due","priority","tags","recurrence"}},
        {"projects",{"archived"}}, {"project-edit",{"color"}}, {"move",{"list"}},
        {"tag-add",{"color"}}, {"tag-edit",{"color"}},
        {"settings-set",{"default-project","notifications","notification-days","dms-next","dms-capture"}},
        {"export",{"output"}}};
    bool valid = (arity.contains(command) && args.size() == arity.value(command))
                 || (command == "open" && (args.size() == 1 || args.size() == 2))
                 || (command == "export" && (args.size() == 1 || args.size() == 2));
    for (const auto &option : {"project","filter","note","list","archived","color","scheduled","due","priority","tags","tag","recurrence",
                               "use-default","default-project","notifications","notification-days","dms-next","dms-capture","output"})
        if (parser.isSet(option) && !allowed.value(command).contains(option)) valid = false;
    if (command == "export" && args.size() == 2 && parser.isSet("output")) valid = false;
    if (command == "edit" && (!parser.isSet("project") || !parser.isSet("note"))) valid = false;
    if ((command == "project-edit" || command == "tag-add" || command == "tag-edit")
        && !parser.isSet("color")) valid = false;
    if (parser.isSet("list") && parser.value("list").isEmpty()) valid = false;
    if (parser.isSet("tag") && parser.value("tag").isEmpty()) valid = false;
    if (command == "settings-set" && !parser.isSet("default-project") && !parser.isSet("notifications")
        && !parser.isSet("notification-days") && !parser.isSet("dms-next") && !parser.isSet("dms-capture")) valid = false;
    if (parser.isSet("use-default") && parser.isSet("project")) valid = false;
    if (!valid) {
        output({{"error", "Invalid command or options. Use yatlctl --help."}}, stderr);
        return 2;
    }
    QString requestedView = args.value(1, "project");
    if (requestedView == "inbox") requestedView = "project";
    if (command == "open" && requestedView != "project" && requestedView != "today"
        && requestedView != "upcoming" && requestedView != "search") {
        output({{"error", "View must be project, today, upcoming, or search."}}, stderr);
        return 2;
    }
    QString listId = parser.value("list");
    if (listId == "none") listId.clear();
    else if ((command == "list" || command == "move") && (!parser.isSet("list") || listId == "all")) listId = "*";
    QString project = parser.value("project");
    if (project == "inbox") project.clear();
    else if (command == "list" && (!parser.isSet("project") || project == "all")) project = "*";
    if (parser.isSet("project") && parser.value("project").isEmpty()) {
        output({{"error", "Project must be an ID, inbox, or (for list) all."}}, stderr);
        return 2;
    }
    QString tagFilter = parser.value("tag");
    if (!parser.isSet("tag") || tagFilter == "all") tagFilter = "*";
    try {
        TaskStore store(parser.value("database"));
        if (command == "add" && parser.isSet("use-default"))
            project = store.settings().defaultProjectId;
        const auto dateOption = [&parser](const QString &name, const QString &fallback) {
            if (!parser.isSet(name)) return fallback;
            return parser.value(name) == "none" ? QString() : parser.value(name);
        };
        const auto priorityOption = [&parser](int fallback) {
            if (!parser.isSet("priority")) return fallback;
            bool ok = false;
            const int value = parser.value("priority").toInt(&ok);
            if (!ok) throw std::runtime_error("Priority must be between 0 and 3.");
            return value;
        };
        const auto tagOption = [&parser](const QStringList &fallback) {
            if (!parser.isSet("tags")) return fallback;
            const QString value = parser.value("tags").trimmed();
            if (value == "none") return QStringList{};
            if (value.isEmpty()) throw std::runtime_error("Tags must be comma-separated IDs or none.");
            QStringList result;
            for (const auto &part : value.split(',', Qt::KeepEmptyParts)) {
                const auto id = part.trimmed();
                if (id.isEmpty()) throw std::runtime_error("Tags must be comma-separated IDs or none.");
                result.append(id);
            }
            return result;
        };
        if (command == "add")
            output({{"task", json(store.add(args[1], project, listId,
                                             dateOption("scheduled", {}), dateOption("due", {}),
                                             priorityOption(0), tagOption({}),
                                             parser.isSet("recurrence") ? parser.value("recurrence") : "none"))}});
        else if (command == "complete") output({{"id", args[1]}, {"changed", store.complete(args[1])}});
        else if (command == "reopen") output({{"id", args[1]}, {"changed", store.reopen(args[1])}});
        else if (command == "archive" || command == "restore")
            output({{"id", args[1]}, {"changed", store.archiveTask(args[1], command == "archive")}});
        else if (command == "edit") {
            const auto existing = store.task(args[1]);
            QStringList existingTags;
            for (const auto &tag : existing.tags) existingTags.append(tag.id);
            output({{"id", args[1]}, {"changed", store.edit(args[1], args[2], parser.value("note"),
                project, listId, dateOption("scheduled", existing.scheduledDate),
                dateOption("due", existing.dueDate), priorityOption(existing.priority),
                tagOption(existingTags), parser.isSet("recurrence") ? parser.value("recurrence") : existing.recurrence)}});
        }
        else if (command == "project-edit") output({{"changed", store.editProject(args[1],args[2],parser.value("color"))}});
        else if (command == "project-archive" || command == "project-restore") output({{"changed", store.archiveProject(args[1], command == "project-archive")}});
        else if (command == "project-move") output({{"changed", store.moveProject(args[1],args[2])}});
        else if (command == "list-add") {
            const auto list = store.addList(args[1],args[2]);
            output({{"list", QJsonObject{{"id",list.id},{"project_id",list.projectId},{"name",list.name},{"sort_order",list.sortOrder}}}});
        } else if (command == "list-rename") output({{"changed",store.renameList(args[1],args[2])}});
        else if (command == "list-move") output({{"changed",store.moveList(args[1],args[2])}});
        else if (command == "lists") {
            QJsonArray lists;
            for (const auto &list : store.lists(args[1]))
                lists.append(QJsonObject{{"id",list.id},{"project_id",list.projectId},
                                         {"name",list.name},{"sort_order",list.sortOrder}});
            output({{"lists",lists}});
        } else if (command == "move") output({{"changed",store.moveTask(args[1],args[2],listId)}});
        else if (command == "project-add") {
            const auto created = store.addProject(args[1]);
            output({{"project", QJsonObject{{"id", created.id}, {"name", created.name},
                                              {"color", created.color}, {"archived", created.archived},
                                              {"sort_order", created.sortOrder}}}});
        } else if (command == "projects") {
            QJsonArray projects;
            for (const auto &item : store.projects(parser.isSet("archived")))
                projects.append(QJsonObject{{"id", item.id}, {"name", item.name},
                                            {"color", item.color}, {"archived", item.archived},
                                            {"sort_order", item.sortOrder}});
            output({{"projects", projects}});
        } else if (command == "list") {
            output(jsonTasks(store.tasks(parser.value("filter"), project, listId, "project", {}, tagFilter)));
        } else if (command == "today") {
            output(jsonTasks(store.tasks("open", "*", "*", "today")));
        } else if (command == "upcoming") {
            output(jsonTasks(store.tasks("open", "*", "*", "upcoming")));
        } else if (command == "search") {
            output(jsonTasks(store.tasks("all", "*", "*", "search", args[1])));
        } else if (command == "tag-add") {
            const auto tag = store.addTag(args[1], parser.value("color"));
            output({{"tag", QJsonObject{{"id",tag.id},{"name",tag.name},{"color",tag.color}}}});
        } else if (command == "tag-edit") {
            output({{"changed", store.editTag(args[1], args[2], parser.value("color"))}});
        } else if (command == "tags") {
            QJsonArray tags;
            for (const auto &tag : store.tags())
                tags.append(QJsonObject{{"id",tag.id},{"name",tag.name},{"color",tag.color}});
            output({{"tags", tags}});
        } else if (command == "open") {
            output(activateOrLaunch(parser.value("database"), requestedView, false));
        } else if (command == "focus") {
            output(activateOrLaunch(parser.value("database"), {}, false));
        } else if (command == "capture") {
            output(activateOrLaunch(parser.value("database"), {}, true));
        } else if (command == "notify") {
            const auto result = NotificationService::run(store);
            output({{"attempted", result.attempted}, {"sent", result.sent}, {"failed", result.failed}});
        } else if (command == "export") {
            const auto document = QJsonDocument(exportData(store));
            const QString path = parser.isSet("output") ? parser.value("output") : args.value(1);
            if (path.isEmpty()) {
                QTextStream(stdout) << document.toJson(QJsonDocument::Indented);
            } else {
                QSaveFile file(path);
                if (!file.open(QIODevice::WriteOnly) || file.write(document.toJson(QJsonDocument::Indented)) < 0 || !file.commit())
                    throw std::runtime_error(QString("Cannot write export file: %1").arg(path).toStdString());
                output({{"path", QFileInfo(path).absoluteFilePath()}, {"format", "yatl-export-v1"}});
            }
        } else if (command == "settings") {
            output({{"settings", json(store.settings())}});
        } else if (command == "settings-set") {
            Settings settings = store.settings();
            const auto booleanOption = [&parser](const QString &name, bool fallback) {
                if (!parser.isSet(name)) return fallback;
                const QString value = parser.value(name);
                if (value != "on" && value != "off")
                    throw std::runtime_error(QString("%1 must be on or off.").arg(name).toStdString());
                return value == "on";
            };
            if (parser.isSet("default-project")) {
                settings.defaultProjectId = parser.value("default-project");
                if (settings.defaultProjectId == "inbox") settings.defaultProjectId.clear();
            }
            settings.notificationsEnabled = booleanOption("notifications", settings.notificationsEnabled);
            settings.dmsShowNextTask = booleanOption("dms-next", settings.dmsShowNextTask);
            if (parser.isSet("notification-days")) {
                bool ok = false;
                settings.notificationDaysBefore = parser.value("notification-days").toInt(&ok);
                if (!ok) throw std::runtime_error("notification-days must be an integer from 0 to 30.");
            }
            if (parser.isSet("dms-capture")) {
                const QString value = parser.value("dms-capture");
                if (value != "inbox" && value != "default")
                    throw std::runtime_error("dms-capture must be inbox or default.");
                settings.dmsUseDefaultProject = value == "default";
            }
            store.saveSettings(settings);
            output({{"settings", json(store.settings())}});
        } else {
            const auto tasks = store.tasks();
            const auto today = store.tasks("open", "*", "*", "today");
            const auto &next = today.isEmpty() ? tasks : today;
            output({{"open_count", tasks.size()}, {"today_count", today.size()},
                    {"next_task", next.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(json(next.first()))}});
        }
        return 0;
    } catch (const std::exception &e) {
        output({{"error", QString::fromUtf8(e.what())}}, stderr);
        return 1;
    }
}
