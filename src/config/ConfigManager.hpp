#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QTimer>
#include <QVariantList>

#include <iostream>
#include <map>
#include <optional>
#include <string>

namespace gs {

class ConfigManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(double       uiOpacity      READ uiOpacity      WRITE setUiOpacity      NOTIFY configChanged)
    Q_PROPERTY(double       uiScale        READ uiScale        WRITE setUiScale        NOTIFY configChanged)
    Q_PROPERTY(QString      bgColor        READ bgColor        NOTIFY configChanged)
    Q_PROPERTY(QString      accentColor    READ accentColor    WRITE setAccentColor    NOTIFY configChanged)
    Q_PROPERTY(QString      textColor      READ textColor      NOTIFY configChanged)
    Q_PROPERTY(QString      graphColor     READ graphColor     NOTIFY configChanged)
    Q_PROPERTY(QString      fontFamily     READ fontFamily     NOTIFY configChanged)
    Q_PROPERTY(int          pollIntervalMs READ pollIntervalMs NOTIFY configChanged)
    Q_PROPERTY(QVariantList scriptDefs     READ scriptDefs     NOTIFY configChanged)

    // ── Window Mode & Desktop Pinning ─────────────────────────────────────────
    Q_PROPERTY(QString windowMode   READ windowMode   WRITE setWindowMode   NOTIFY windowModeChanged)
    Q_PROPERTY(bool    clickThrough READ clickThrough WRITE setClickThrough NOTIFY clickThroughChanged)
    Q_PROPERTY(bool    isX11        READ isX11        CONSTANT)

    // ── Autostart ─────────────────────────────────────────────────────────────
    Q_PROPERTY(bool autostartEnabled READ autostartEnabled WRITE setAutostartEnabled NOTIFY autostartChanged)

public:
    explicit ConfigManager(const QString& path, double autoScale = 1.0, QObject* parent = nullptr);

    double       uiOpacity()        const noexcept { return m_opacity;         }
    double       uiScale()          const noexcept { return m_uiScale;         }
    QString      bgColor()          const          { return m_bgColor;         }
    QString      accentColor()      const          { return m_accentColor;     }
    QString      textColor()        const          { return m_textColor;       }
    QString      graphColor()       const          { return m_graphColor;      }
    QString      fontFamily()       const          { return m_fontFamily;      }
    int          pollIntervalMs()   const noexcept { return m_pollMs;          }
    QVariantList scriptDefs()       const          { return m_scriptDefs;      }
    QString      windowMode()       const          { return m_windowMode;      }
    bool         clickThrough()     const noexcept { return m_clickThrough;    }
    bool         isX11()            const noexcept { return m_isX11;           }
    bool         autostartEnabled() const;

    // Live setters for Settings GUI (in-memory only, no file write)
    Q_INVOKABLE void setUiOpacity(double opacity);
    Q_INVOKABLE void setUiScale(double scale);
    Q_INVOKABLE void setAccentColor(const QString& color);

    // Tray-driven / user-driven setters (emit *ByUser signals for persistence)
    Q_INVOKABLE void setWindowMode(const QString& mode);
    Q_INVOKABLE void setClickThrough(bool enabled);
    Q_INVOKABLE void setAutostartEnabled(bool enabled);
    Q_INVOKABLE void setPreset(const QString& name);

    // Surgical file writer — preserves comments, only touches target key
    Q_INVOKABLE void setAndPersist(const QString& section, const QString& key, const QString& value);

    // Ensure user config file exists (creates from default if needed)
    Q_INVOKABLE QString ensureUserConfigPath();

    [[nodiscard]] static QString findConfigPath();


signals:
    void configChanged();
    void windowModeChanged(const QString& mode);
    void clickThroughChanged(bool enabled);
    void windowModeChangedByUser(const QString& mode);
    void clickThroughChangedByUser(bool enabled);
    void autostartChanged(bool enabled);

private slots:
    void onFileChanged(const QString& path);
    void processFileReload();

private:
    using TomlData = std::map<std::string, std::map<std::string, std::string>>;

    void                            load(const QString& path);
    static std::optional<TomlData>  parseToml(const QString& content);
    void                            applyData(const TomlData& data);
    void                            setWindowModeInternal(const QString& rawMode, bool fromUser);
    void                            setClickThroughInternal(bool enabled, bool fromUser);

    QFileSystemWatcher m_watcher;
    QTimer             m_debounceTimer;
    QString            m_configPath;

    double       m_autoScale    = 1.0;   // DPI-based default set before first load
    bool         m_scaleSetByFile = false; // true once file has an explicit scale key

    double       m_opacity      = 0.85;
    double       m_uiScale      = 1.0;
    QString      m_bgColor      = QStringLiteral("#0a0a16");
    QString      m_accentColor  = QStringLiteral("#8b5cf6");
    QString      m_textColor    = QStringLiteral("#e2e8f0");
    QString      m_graphColor   = QStringLiteral("#a78bfa");
    QString      m_fontFamily   = QStringLiteral("JetBrains Mono");
    int          m_pollMs       = 1000;
    QVariantList m_scriptDefs;

    QString      m_windowMode   = QStringLiteral("floating");
    bool         m_clickThrough = false;
    bool         m_isX11        = false;
};

} // namespace gs
