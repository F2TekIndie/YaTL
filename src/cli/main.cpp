#include "taskstore.h"
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>
#include <stdexcept>

namespace {
void output(const QJsonObject &object, FILE *stream = stdout) {
    QTextStream(stream) << QJsonDocument(object).toJson(QJsonDocument::Compact) << '\n';
}
QJsonObject json(const Task &task) {
    return {{"id", task.id}, {"title", task.title}, {"created_at", task.createdAt},
            {"completed_at", task.completed() ? QJsonValue(task.completedAt) : QJsonValue(QJsonValue::Null)},
            {"project_id", task.projectId.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(task.projectId)},
            {"note", task.note}, {"status", task.completed() ? "completed" : "open"}};
}
}
int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("yatlctl");
    app.setApplicationVersion("0.2.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("YaTL local task commands. Successful commands return JSON on stdout; errors return JSON on stderr.");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addOption({"database", "Use a specific SQLite database.", "path", TaskStore::defaultPath()});
    parser.addOption({"filter", "Task filter: open, completed, all.", "filter", "open"});
    parser.addOption({"project", "Project ID or inbox; list also accepts all.", "project"});
    parser.addOption({"note", "Replacement note for edit (defaults to empty).", "note", ""});
    parser.addPositionalArgument("command", "add TITLE | list | complete ID | reopen ID | summary | project-add NAME | projects | edit ID TITLE --project ID|inbox --note NOTE");
    parser.addPositionalArgument("argument", "Title or task ID; use -- before titles starting with a dash.", "[argument]");
    if (!parser.parse(app.arguments())) {
        output({{"error", parser.errorText()}}, stderr);
        return 2;
    }
    if (parser.isSet("help")) parser.showHelp();
    if (parser.isSet("version")) parser.showVersion();
    const auto args = parser.positionalArguments();
    const QString command = args.value(0);
    const bool unary = command == "add" || command == "complete" || command == "reopen" || command == "project-add";
    const bool noArgs = command == "list" || command == "summary" || command == "projects";
    const bool editing = command == "edit";
    if ((!unary && !noArgs && !editing) || (unary && args.size() != 2) || (noArgs && args.size() != 1)
        || (editing && (args.size() != 3 || !parser.isSet("project") || !parser.isSet("note")))
        || (parser.isSet("filter") && command != "list")
        || (parser.isSet("project") && command != "list" && command != "add" && !editing)
        || (parser.isSet("note") && !editing)) {
        output({{"error", "Invalid command or options. Use yatlctl --help. Edit requires ID TITLE --project ID|inbox --note NOTE."}}, stderr);
        return 2;
    }
    QString project = parser.value("project");
    if (project == "inbox") project.clear();
    else if (command == "list" && (!parser.isSet("project") || project == "all")) project = "*";
    if (parser.isSet("project") && parser.value("project").isEmpty()) {
        output({{"error", "Project must be an ID, inbox, or (for list) all."}}, stderr);
        return 2;
    }
    try {
        TaskStore store(parser.value("database"));
        if (command == "add") output({{"task", json(store.add(args[1], project))}});
        else if (command == "complete") output({{"id", args[1]}, {"changed", store.complete(args[1])}});
        else if (command == "reopen") output({{"id", args[1]}, {"changed", store.reopen(args[1])}});
        else if (command == "edit") output({{"id", args[1]}, {"changed", store.edit(args[1], args[2], parser.value("note"), project)}});
        else if (command == "project-add") {
            const auto created = store.addProject(args[1]);
            output({{"project", QJsonObject{{"id", created.id}, {"name", created.name}}}});
        } else if (command == "projects") {
            QJsonArray projects;
            for (const auto &item : store.projects()) projects.append(QJsonObject{{"id", item.id}, {"name", item.name}});
            output({{"projects", projects}});
        } else if (command == "list") {
            QJsonArray tasks;
            for (const auto &task : store.tasks(parser.value("filter"), project)) tasks.append(json(task));
            output({{"tasks", tasks}});
        } else {
            const auto tasks = store.tasks();
            output({{"open_count", tasks.size()}, {"next_task", tasks.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(json(tasks.first()))}});
        }
        return 0;
    } catch (const std::exception &e) {
        output({{"error", QString::fromUtf8(e.what())}}, stderr);
        return 1;
    }
}
