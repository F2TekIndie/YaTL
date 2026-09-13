#pragma once

#include <QObject>
#include <QFileSystemWatcher>
#include <QMap>

class DmsThemeProvider : public QObject {
    Q_OBJECT
#define THEME_PROPERTY(name) Q_PROPERTY(QString name READ name NOTIFY themeChanged)
    THEME_PROPERTY(background) THEME_PROPERTY(surface) THEME_PROPERTY(surfaceContainerLow)
    THEME_PROPERTY(surfaceContainerHigh) THEME_PROPERTY(surfaceContainerHighest)
    THEME_PROPERTY(onSurface) THEME_PROPERTY(onSurfaceVariant) THEME_PROPERTY(primary)
    THEME_PROPERTY(onPrimary) THEME_PROPERTY(primaryContainer) THEME_PROPERTY(onPrimaryContainer)
    THEME_PROPERTY(outline) THEME_PROPERTY(outlineVariant) THEME_PROPERTY(error)
    THEME_PROPERTY(errorContainer) THEME_PROPERTY(onErrorContainer) THEME_PROPERTY(scrim)
    Q_PROPERTY(bool reducedMotion READ reducedMotion CONSTANT)
#undef THEME_PROPERTY
public:
    explicit DmsThemeProvider(QObject *parent = nullptr);
    QString background() const { return colors_.value("background"); }
    QString surface() const { return colors_.value("surface"); }
    QString surfaceContainerLow() const { return colors_.value("surfaceContainerLow"); }
    QString surfaceContainerHigh() const { return colors_.value("surfaceContainerHigh"); }
    QString surfaceContainerHighest() const { return colors_.value("surfaceContainerHighest"); }
    QString onSurface() const { return colors_.value("onSurface"); }
    QString onSurfaceVariant() const { return colors_.value("onSurfaceVariant"); }
    QString primary() const { return colors_.value("primary"); }
    QString onPrimary() const { return colors_.value("onPrimary"); }
    QString primaryContainer() const { return colors_.value("primaryContainer"); }
    QString onPrimaryContainer() const { return colors_.value("onPrimaryContainer"); }
    QString outline() const { return colors_.value("outline"); }
    QString outlineVariant() const { return colors_.value("outlineVariant"); }
    QString error() const { return colors_.value("error"); }
    QString errorContainer() const { return colors_.value("errorContainer"); }
    QString onErrorContainer() const { return colors_.value("onErrorContainer"); }
    QString scrim() const { return colors_.value("scrim"); }
    bool reducedMotion() const { return reducedMotion_; }
    Q_INVOKABLE QString color(const QString &role) const { return colors_.value(role); }
signals:
    void themeChanged();
private slots:
    void reload();
private:
    void setFallback();
    QFileSystemWatcher watcher_;
    QMap<QString, QString> colors_;
    QString filePath_;
    bool reducedMotion_ = false;
};
