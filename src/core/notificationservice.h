#pragma once

#include "taskstore.h"
#include <QString>
#include <functional>
#include <QObject>

struct NotificationResult {
    int attempted = 0;
    int sent = 0;
    int failed = 0;
};

class NotificationService {
public:
    static NotificationResult run(TaskStore &store, const QString &throughDate = {},
                                  const QString &executable = {});
    static void runAsync(TaskStore &store, QObject *context,
                         std::function<void(NotificationResult)> finished = {});
};
