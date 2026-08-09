#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QVariantList>

#include <map>
#include <optional>
#include <string>

namespace gs {

class ConfigManager : public QObject {
    Q_OBJECT

    Q_PROPERTY(double       uiOpacity     READ uiOpacity     NOTIFY configChanged)
    Q_PROPERTY(QString      bgColor       READ bgColor       NOTIFY configChanged)
    Q_PROPERTY(QString      accentColor   READ accentColor   NOTIFY configChanged)
    Q_PROPERTY(QString      textColor     READ textColor     NOTIFY configChanged)
    Q_PROPERTY(QString      graphColor    READ graphColor    NOTIFY configChanged)
    Q_PROPERTY(QString      fontFamily    READ fontFamily    NOTIFY configChanged)
    Q_PROPERTY(int          pollIntervalMs READ pollIntervalMs NOTIFY configChanged)
    Q_PROPERTY(QVariantList scriptDefs    READ scriptDefs    NOTIFY configChanged)

public:
    explicit ConfigManager(const QString& path, QObject* parent = nullptr);

    double       uiOpacity()      const noexcept { return m_opacity;     }
    QString      bgColor()        const          { return m_bgColor;     }
    QString      accentColor()    const          { return m_accentColor; }
    QString      textColor()      const          { return m_textColor;   }
    QString      graphColor()     const          { return m_graphColor;  }
    QString      fontFamily()     const          { return m_fontFamily;  }
    int          pollIntervalMs() const noexcept { return m_pollMs;      }
    QVariantList scriptDefs()     const          { return m_scriptDefs;  }

    [[nodiscard]] static QString findConfigPath();

signals:
    void configChanged();

private slots:
    void onFileChanged(const QString& path);

private:
    using TomlData = std::map<std::string, std::map<std::string, std::string>>;

    void             load(const QString& path);
    static TomlData  parseToml(const QString& content);
    void             applyData(const TomlData& data);

    QFileSystemWatcher m_watcher;
    QString            m_configPath;

    double       m_opacity     = 0.85;
    QString      m_bgColor     = QStringLiteral("#0a0a16");
    QString      m_accentColor = QStringLiteral("#8b5cf6");
    QString      m_textColor   = QStringLiteral("#e2e8f0");
    QString      m_graphColor  = QStringLiteral("#a78bfa");
    QString      m_fontFamily  = QStringLiteral("JetBrains Mono");
    int          m_pollMs      = 1000;
    QVariantList m_scriptDefs;
};

} // namespace gs
