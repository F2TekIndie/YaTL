#include "dmsthemeprovider.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QColor>
#include <QPalette>
#include <QStandardPaths>

namespace {
const QStringList roles{"background", "surface", "surfaceContainerLow", "surfaceContainerHigh",
    "surfaceContainerHighest", "onSurface", "onSurfaceVariant", "primary", "onPrimary",
    "primaryContainer", "onPrimaryContainer", "outline", "outlineVariant", "error",
    "errorContainer", "onErrorContainer", "scrim"};
QString snake(const QString &value) {
    QString result;
    for (const auto c : value) { if (c.isUpper()) result += '_' + QString(c).toLower(); else result += c; }
    return result;
}
}

DmsThemeProvider::DmsThemeProvider(QObject *parent) : QObject(parent) {
    const auto reducedMotion = qEnvironmentVariable("YATL_REDUCED_MOTION").toLower();
    const auto qtReducedMotion = qEnvironmentVariable("QT_REDUCED_MOTION").toLower();
    reducedMotion_ = reducedMotion == "1" || reducedMotion == "true" || reducedMotion == "yes"
        || qtReducedMotion == "1" || qtReducedMotion == "true" || qtReducedMotion == "yes";
    const auto cache = qEnvironmentVariable("XDG_CACHE_HOME").isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation)
        : qEnvironmentVariable("XDG_CACHE_HOME");
    filePath_ = cache + "/DankMaterialShell/dms-colors.json";
    connect(&watcher_, &QFileSystemWatcher::fileChanged, this, &DmsThemeProvider::reload);
    connect(&watcher_, &QFileSystemWatcher::directoryChanged, this, &DmsThemeProvider::reload);
    setFallback();
    reload();
}

void DmsThemeProvider::setFallback() {
    QPalette palette;
    if (auto *gui = qobject_cast<QGuiApplication *>(QCoreApplication::instance())) palette = gui->palette();
    const auto value = [&](QPalette::ColorRole role, const QColor &fallback) {
        const QColor color = palette.color(role);
        return (color.isValid() ? color : fallback).name(QColor::HexRgb);
    };
    const QColor window = palette.color(QPalette::Window).isValid() ? palette.color(QPalette::Window) : QColor(Qt::white);
    const QColor surface = palette.color(QPalette::Base).isValid() ? palette.color(QPalette::Base) : window;
    const QColor high = palette.color(QPalette::AlternateBase).isValid() ? palette.color(QPalette::AlternateBase) : palette.color(QPalette::Button);
    const QColor highest = palette.color(QPalette::Midlight).isValid() ? palette.color(QPalette::Midlight) : high;
    const QColor text = palette.color(QPalette::Text).isValid() ? palette.color(QPalette::Text) : QColor(Qt::black);
    const QColor variant = palette.color(QPalette::ButtonText).isValid() ? palette.color(QPalette::ButtonText) : text;
    const QColor primary = palette.color(QPalette::Highlight).isValid() ? palette.color(QPalette::Highlight) : palette.color(QPalette::Link);
    const QColor onPrimary = palette.color(QPalette::HighlightedText).isValid() ? palette.color(QPalette::HighlightedText) : text;
    const QColor error = palette.color(QPalette::BrightText).isValid() ? palette.color(QPalette::BrightText) : palette.color(QPalette::Text);
    const QColor errorContainer = error.lighter(170);
    colors_ = {{"background", value(QPalette::Window, window)}, {"surface", value(QPalette::Base, surface)},
        {"surfaceContainerLow", value(QPalette::Window, window)}, {"surfaceContainerHigh", high.name(QColor::HexRgb)},
        {"surfaceContainerHighest", highest.name(QColor::HexRgb)}, {"onSurface", value(QPalette::Text, text)},
        {"onSurfaceVariant", variant.name(QColor::HexRgb)}, {"primary", primary.name(QColor::HexRgb)},
        {"onPrimary", onPrimary.name(QColor::HexRgb)}, {"primaryContainer", high.name(QColor::HexRgb)},
        {"onPrimaryContainer", text.name(QColor::HexRgb)}, {"outline", value(QPalette::Mid, variant)},
        {"outlineVariant", value(QPalette::Midlight, high)}, {"error", error.name(QColor::HexRgb)},
        {"errorContainer", errorContainer.name(QColor::HexRgb)}, {"onErrorContainer", text.name(QColor::HexRgb)},
        {"scrim", QColor(Qt::black).name(QColor::HexRgb)}};
}

void DmsThemeProvider::reload() {
    const auto previous = colors_;
    setFallback();
    QJsonObject source;
    QFile file(filePath_);
    QJsonParseError parseError;
    if (file.open(QIODevice::ReadOnly)) source = QJsonDocument::fromJson(file.readAll(), &parseError).object();
    if (!source.isEmpty() && parseError.error == QJsonParseError::NoError) {
        QJsonObject selected = source;
        const QString mode = source.value("mode").toString();
        if (!mode.isEmpty() && source.value(mode).isObject()) selected = source.value(mode).toObject();
        else if (source.value("colors").isObject()) {
            const auto colors = source.value("colors").toObject();
            selected = !mode.isEmpty() && colors.value(mode).isObject() ? colors.value(mode).toObject() : colors;
        }
        QMap<QString, QString> next = colors_;
        for (const auto &role : roles) {
            const auto value = selected.value(role).toString(selected.value(snake(role)).toString());
            if (QColor(value).isValid()) next[role] = QColor(value).name(QColor::HexRgb);
        }
        colors_ = next;
    }
    const QFileInfo info(filePath_);
    if (info.exists() && !watcher_.files().contains(filePath_)) watcher_.addPath(filePath_);
    const QFileInfo themeDirectory(info.absolutePath());
    const auto watchDirectory = themeDirectory.exists()
        ? themeDirectory.absoluteFilePath()
        : themeDirectory.dir().absolutePath();
    if (QDir(watchDirectory).exists() && !watcher_.directories().contains(watchDirectory)) watcher_.addPath(watchDirectory);
    if (colors_ != previous) emit themeChanged();
}
