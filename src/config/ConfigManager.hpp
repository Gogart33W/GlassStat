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

    Q_PROPERTY(double       uiOpacity      READ uiOpacity      NOTIFY configChanged)
    Q_PROPERTY(QString      bgColor        READ bgColor        NOTIFY configChanged)
    Q_PROPERTY(QString      accentColor    READ accentColor    NOTIFY configChanged)
    Q_PROPERTY(QString      textColor      READ textColor      NOTIFY configChanged)
    Q_PROPERTY(QString      graphColor     READ graphColor     NOTIFY configChanged)
    Q_PROPERTY(QString      fontFamily     READ fontFamily     NOTIFY configChanged)
    Q_PROPERTY(int          pollIntervalMs READ pollIntervalMs NOTIFY configChanged)
    Q_PROPERTY(QVariantList scriptDefs     READ scriptDefs     NOTIFY configChanged)

    // ── Window Mode & Desktop Pinning ─────────────────────────────────────────
    Q_PROPERTY(QString windowMode   READ windowMode   WRITE setWindowMode   NOTIFY windowModeChanged)
    Q_PROPERTY(bool    clickThrough READ clickThrough WRITE setClickThrough NOTIFY clickThroughChanged)
    Q_PROPERTY(bool    isX11        READ isX11        CONSTANT)

public:
    explicit ConfigManager(const QString& path, QObject* parent = nullptr);

    double       uiOpacity()      const noexcept { return m_opacity;      }
    QString      bgColor()        const          { return m_bgColor;      }
    QString      accentColor()    const          { return m_accentColor;  }
    QString      textColor()      const          { return m_textColor;    }
    QString      graphColor()     const          { return m_graphColor;   }
    QString      fontFamily()     const          { return m_fontFamily;   }
    int          pollIntervalMs() const noexcept { return m_pollMs;       }
    QVariantList scriptDefs()     const          { return m_scriptDefs;   }
    QString      windowMode()     const          { return m_windowMode;   }
    bool         clickThrough()   const noexcept { return m_clickThrough; }
    bool         isX11()          const noexcept { return m_isX11;        }

    Q_INVOKABLE void setWindowMode(const QString& mode);
    Q_INVOKABLE void setClickThrough(bool enabled);
    Q_INVOKABLE void setPreset(const QString& name);

    [[nodiscard]] static QString findConfigPath();

signals:
    void configChanged();
    void windowModeChanged(const QString& mode);
    void clickThroughChanged(bool enabled);
    void windowModeChangedByUser(const QString& mode);
    void clickThroughChangedByUser(bool enabled);

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

    double       m_opacity      = 0.85;
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
