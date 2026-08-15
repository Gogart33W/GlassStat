#include "config/ConfigManager.hpp"

#include <QColor>
#include <QFile>
#include <QGuiApplication>
#include <QStandardPaths>
#include <QTextStream>
#include <iostream>

namespace gs {

ConfigManager::ConfigManager(const QString& path, QObject* parent)
    : QObject(parent), m_configPath(path)
{
    m_isX11 = (QGuiApplication::platformName() == QStringLiteral("xcb"));

    m_debounceTimer.setSingleShot(true);
    m_debounceTimer.setInterval(250);
    connect(&m_debounceTimer, &QTimer::timeout, this, &ConfigManager::processFileReload);

    if (path.isEmpty()) return;

    load(path);
    m_watcher.addPath(path);
    connect(&m_watcher, &QFileSystemWatcher::fileChanged,
            this, &ConfigManager::onFileChanged);
}

QString ConfigManager::findConfigPath() {
    const auto exists = [](const QString& p) -> QString {
        return QFile::exists(p) ? p : QString{};
    };

    // 1. Environment variable override
    const QByteArray env = qgetenv("GLASSSTAT_CONFIG");
    if (!env.isEmpty()) return exists(QString::fromLocal8Bit(env));

    // 2. User XDG config dir
    const QString user = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                       + QStringLiteral("/glassstat/config.toml");
    if (auto p = exists(user); !p.isEmpty()) return p;

    // 3. Relative to CWD (dev: run from source root or one level up from build/)
    for (const char* rel : {"config/default_config.toml", "../config/default_config.toml"}) {
        if (auto p = exists(QLatin1String(rel)); !p.isEmpty()) return p;
    }
    return {};
}

void ConfigManager::onFileChanged(const QString& /*path*/) {
    // 250ms debounce to handle multi-write / atomic-write file save events
    m_debounceTimer.start(250);
}

void ConfigManager::processFileReload() {
    if (m_configPath.isEmpty()) return;

    if (!m_watcher.files().contains(m_configPath) && QFile::exists(m_configPath)) {
        m_watcher.addPath(m_configPath);
    }

    load(m_configPath);
    emit configChanged();
}

void ConfigManager::load(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "[ConfigManager] Error: Unable to open config file at "
                  << path.toStdString() << " (retaining current valid configuration)\n";
        return;
    }

    QTextStream in(&f);
    const QString content = in.readAll();
    if (content.trimmed().isEmpty()) {
        std::cerr << "[ConfigManager] Error: Config file is empty or missing content at "
                  << path.toStdString() << " (retaining current valid configuration)\n";
        return;
    }

    const auto dataOpt = parseToml(content);
    if (!dataOpt.has_value()) {
        std::cerr << "[ConfigManager] Error: TOML syntax validation failed for "
                  << path.toStdString() << " (retaining current valid configuration)\n";
        return;
    }

    applyData(dataOpt.value());
}

std::optional<ConfigManager::TomlData> ConfigManager::parseToml(const QString& content) {
    TomlData data;
    std::string section;
    int lineNumber = 0;

    for (const QString& rawLine : content.split(u'\n')) {
        lineNumber++;

        // Strip comments
        QString line;
        bool inStr = false;
        for (const QChar ch : rawLine) {
            if (ch == u'"') inStr = !inStr;
            if (!inStr && (ch == u'#' || (ch == u'/' && line.endsWith(u'/')))) {
                if (ch == u'/') line.chop(1);
                break;
            }
            line += ch;
        }
        line = line.trimmed();
        if (line.isEmpty()) continue;

        // [section]
        if (line.startsWith(u'[') && line.endsWith(u']')) {
            section = line.mid(1, line.size() - 2).trimmed().toStdString();
            if (section.empty()) {
                std::cerr << "[ConfigManager] Syntax Error line " << lineNumber
                          << ": Empty section header bracket\n";
                return std::nullopt;
            }
            continue;
        }

        if (line.startsWith(u'[') && !line.endsWith(u']')) {
            std::cerr << "[ConfigManager] Syntax Error line " << lineNumber
                      << ": Malformed section header (missing ']'): " << line.toStdString() << "\n";
            return std::nullopt;
        }

        const int eq = line.indexOf(u'=');
        if (eq < 0) {
            std::cerr << "[ConfigManager] Syntax Error line " << lineNumber
                      << ": Expected key = value assignment: " << line.toStdString() << "\n";
            return std::nullopt;
        }

        const std::string key = line.left(eq).trimmed().toStdString();
        if (key.empty()) {
            std::cerr << "[ConfigManager] Syntax Error line " << lineNumber
                      << ": Empty key name before '=': " << line.toStdString() << "\n";
            return std::nullopt;
        }

        QString val = line.mid(eq + 1).trimmed();

        // Unquote strings
        if (val.size() >= 2 && val.startsWith(u'"') && val.endsWith(u'"')) {
            val = val.mid(1, val.size() - 2);
        }

        data[section][key] = val.toStdString();
    }

    return data;
}

void ConfigManager::applyData(const TomlData& d) {
    auto get = [&](const char* sec, const char* key) -> std::optional<std::string> {
        auto s = d.find(sec);
        if (s == d.end()) return std::nullopt;
        auto k = s->second.find(key);
        return k != s->second.end() ? std::optional{k->second} : std::nullopt;
    };

    bool ok;

    if (auto v = get("ui", "opacity")) {
        const double op = QString::fromStdString(*v).toDouble(&ok);
        if (ok && op > 0.0 && op <= 1.0) m_opacity = op;
    }
    if (auto v = get("ui", "font_family"))
        m_fontFamily = QString::fromStdString(*v);

    if (auto v = get("general", "poll_interval_ms")) {
        const int ms = QString::fromStdString(*v).toInt(&ok);
        if (ok && ms > 0) m_pollMs = ms;
    }

    // Window Mode from file hot-reload (fromUser = false)
    if (auto v = get("window", "mode")) {
        const QString modeStr = QString::fromStdString(*v).toLower().trimmed();
        setWindowModeInternal(modeStr, false);
    }

    // Click Through from file hot-reload (fromUser = false)
    if (auto v = get("window", "click_through")) {
        const std::string s = *v;
        const bool ct = (s == "true" || s == "1");
        setClickThroughInternal(ct, false);
    }

    // Validate color strings before applying
    auto applyColor = [&](const char* key, QString& dst) {
        if (auto v = get("colors", key)) {
            const QString s = QString::fromStdString(*v);
            if (QColor(s).isValid()) dst = s;
        }
    };
    applyColor("background", m_bgColor);
    applyColor("accent",     m_accentColor);
    applyColor("text",       m_textColor);
    applyColor("graph_line", m_graphColor);

    // Scripts: each key = "command" pair becomes a runnable script entry
    m_scriptDefs.clear();
    auto sit = d.find("scripts");
    if (sit != d.end()) {
        for (const auto& [name, cmd] : sit->second) {
            if (cmd.empty()) continue;
            QVariantMap entry;
            entry[QStringLiteral("name")]    = QString::fromStdString(name);
            entry[QStringLiteral("command")] = QString::fromStdString(cmd);
            m_scriptDefs.append(entry);
        }
    }
}

void ConfigManager::setWindowMode(const QString& rawMode) {
    setWindowModeInternal(rawMode, true);
}

void ConfigManager::setClickThrough(bool enabled) {
    setClickThroughInternal(enabled, true);
}

void ConfigManager::setWindowModeInternal(const QString& rawMode, bool fromUser) {
    QString targetMode = rawMode.toLower().trimmed();
    if (targetMode != QStringLiteral("desktop") &&
        targetMode != QStringLiteral("floating") &&
        targetMode != QStringLiteral("top")) {
        targetMode = QStringLiteral("floating");
    }

    if (targetMode == QStringLiteral("desktop") && !m_isX11) {
        std::cerr << "[GlassStat] Desktop mode requires X11 — falling back to floating window on Wayland.\n";
        targetMode = QStringLiteral("floating");
    }

    if (m_windowMode != targetMode) {
        m_windowMode = targetMode;
        emit windowModeChanged(m_windowMode);
        if (fromUser) {
            emit windowModeChangedByUser(m_windowMode);
        }
    }
}

void ConfigManager::setClickThroughInternal(bool enabled, bool fromUser) {
    if (m_clickThrough != enabled) {
        m_clickThrough = enabled;
        emit clickThroughChanged(m_clickThrough);
        if (fromUser) {
            emit clickThroughChangedByUser(m_clickThrough);
        }
    }
}

void ConfigManager::setPreset(const QString& name) {
    if (name == QStringLiteral("dark")) {
        m_bgColor     = QStringLiteral("#0a0a16");
        m_accentColor = QStringLiteral("#8b5cf6");
        m_textColor   = QStringLiteral("#e2e8f0");
        m_graphColor  = QStringLiteral("#a78bfa");
    } else if (name == QStringLiteral("cyberpunk")) {
        m_bgColor     = QStringLiteral("#0d0221");
        m_accentColor = QStringLiteral("#00f6ff");
        m_textColor   = QStringLiteral("#ffffff");
        m_graphColor  = QStringLiteral("#00f6ff");
    } else if (name == QStringLiteral("emerald")) {
        m_bgColor     = QStringLiteral("#022c22");
        m_accentColor = QStringLiteral("#10b981");
        m_textColor   = QStringLiteral("#ecfdf5");
        m_graphColor  = QStringLiteral("#34d399");
    } else if (name == QStringLiteral("amber")) {
        m_bgColor     = QStringLiteral("#1c0a00");
        m_accentColor = QStringLiteral("#f97316");
        m_textColor   = QStringLiteral("#fff7ed");
        m_graphColor  = QStringLiteral("#fbbf24");
    }
    emit configChanged();
}

} // namespace gs
