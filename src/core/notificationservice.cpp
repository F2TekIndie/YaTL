#include "notificationservice.h"

#include <QDate>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QSharedPointer>

NotificationResult NotificationService::run(TaskStore &store, const QString &throughDate,
                                             const QString &executable) {
    const auto settings = store.settings();
    if (!settings.notificationsEnabled) return {};
    const QString date = throughDate.isEmpty()
        ? QDate::currentDate().addDays(settings.notificationDaysBefore).toString(Qt::ISODate)
        : throughDate;
    const auto pending = store.pendingNotifications(date);
    NotificationResult result;
    result.attempted = pending.size();
    QString notifier = executable;
    if (notifier.isEmpty()) notifier = qEnvironmentVariable("YATL_NOTIFY_EXECUTABLE");
    if (notifier.isEmpty()) notifier = QStandardPaths::findExecutable("notify-send");
    for (const auto &notification : pending) {
        if (notifier.isEmpty()) { ++result.failed; continue; }
        QProcess process;
        const QString heading = notification.kind == "due" ? "YaTL — Task due"
                                                            : "YaTL — Task scheduled";
        process.start(notifier, {"--app-name=YaTL", "--icon=view-task", heading,
                                 notification.title});
        const bool success = process.waitForFinished(5000) && process.exitStatus() == QProcess::NormalExit
                             && process.exitCode() == 0;
        if (success) {
            store.markNotificationSent(notification);
            ++result.sent;
        } else {
            if (process.state() != QProcess::NotRunning) process.kill();
            ++result.failed;
        }
    }
    return result;
}

void NotificationService::runAsync(TaskStore &store, QObject *context,
                                    std::function<void(NotificationResult)> finished) {
    QVector<TaskNotification> pending;
    try {
        const auto settings = store.settings();
        if (!settings.notificationsEnabled) { if (finished) finished({}); return; }
        const QString date = QDate::currentDate().addDays(settings.notificationDaysBefore).toString(Qt::ISODate);
        pending = store.pendingNotifications(date);
    } catch (...) {
        // Notification delivery is best effort; storage failures must not escape
        // the timer callback and terminate the Qt event loop.
        NotificationResult failed;
        failed.failed = 1;
        if (finished) finished(failed);
        return;
    }
    const QString notifier = qEnvironmentVariable("YATL_NOTIFY_EXECUTABLE").isEmpty()
        ? QStandardPaths::findExecutable("notify-send") : qEnvironmentVariable("YATL_NOTIFY_EXECUTABLE");
    struct State { QVector<TaskNotification> pending; NotificationResult result; int index = 0; };
    auto state = QSharedPointer<State>::create();
    state->pending = pending;
    state->result.attempted = pending.size();
    auto processNext = QSharedPointer<std::function<void()>>::create();
    *processNext = [&, state, processNext, context, notifier, finished]() {
        try {
            if (state->index >= state->pending.size()) {
                const auto result = state->result;
                *processNext = {};
                if (finished) {
                    try { finished(result); } catch (...) { /* completion callbacks are best effort */ }
                }
                return;
            }
            const auto notification = state->pending.at(state->index++);
            if (notifier.isEmpty()) { ++state->result.failed; QTimer::singleShot(0, context, *processNext); return; }
            auto *process = new QProcess(context);
            const QString heading = notification.kind == "due" ? "YaTL — Task due" : "YaTL — Task scheduled";
            QObject::connect(process, qOverload<int,QProcess::ExitStatus>(&QProcess::finished), context,
                             [process, state, notification, processNext, &store](int code, QProcess::ExitStatus status) {
                try {
                    if (status == QProcess::NormalExit && code == 0) {
                        store.markNotificationSent(notification);
                        ++state->result.sent;
                    } else ++state->result.failed;
                } catch (...) {
                    ++state->result.failed;
                }
                process->deleteLater();
                QTimer::singleShot(0, process->parent(), *processNext);
            });
            QTimer::singleShot(5000, process, [process]() {
                if (process->state() != QProcess::NotRunning) process->kill();
            });
            process->start(notifier, {"--app-name=YaTL", "--icon=view-task", heading, notification.title});
        } catch (...) {
            ++state->result.failed;
            QTimer::singleShot(0, context, *processNext);
        }
    };
    QTimer::singleShot(0, context, *processNext);
}
