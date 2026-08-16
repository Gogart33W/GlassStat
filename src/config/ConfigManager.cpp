#include "config/ConfigManager.hpp"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>
#include <iostream>
#include <algorithm>

namespace gs {

// ── Constructor ──────────────────────────────────────────────────────────────

ConfigManager::ConfigManager(const QString& path, double autoScale, QObject* parent)
    : QObject(parent), m_configPath(path), m_autoScale(autoScale)
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

// ── Config Path Discovery ────────────────────────────────────────────────────

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

QString ConfigManager::ensureUserConfigPath() {
    const QString userPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                           + QStringLiteral("/glassstat/config.toml");
    if (QFile::exists(userPath)) return userPath;

    // Create from default config
    QDir().mkpath(QFileInfo(userPath).absolutePath());
    for (const char* rel : {"config/default_config.toml", "../config/default_config.toml"}) {
        const QString src = QLatin1String(rel);
        if (QFile::exists(src)) {
            QFile::copy(src, userPath);
            break;
        }
    }
    return userPath;
}

// ── File Watcher Slots ───────────────────────────────────────────────────────

void ConfigManager::onFileChanged(const QString& /*path*/) {
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

// ── File Loading ─────────────────────────────────────────────────────────────

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
        std::cerr << "[ConfigManager] Error: Config file is empty at "
                  << path.toStdString() << " (retaining current valid configuration)\n";
        return;
    }

    const auto dataOpt = parseToml(content);
    if (!dataOpt.has_value()) {
        std::cerr << "[ConfigManager] Error: TOML syntax error in "
                  << path.toStdString() << " (retaining current valid configuration)\n";
        return;
    }

    applyData(dataOpt.value());
}

// ── TOML Parser ──────────────────────────────────────────────────────────────

std::optional<ConfigManager::TomlData> ConfigManager::parseToml(const QString& content) {
    TomlData data;
    std::string section;
    int lineNumber = 0;

    for (const QString& rawLine : content.split(u'\n')) {
        lineNumber++;

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

        if (line.startsWith(u'[') && line.endsWith(u']')) {
            section = line.mid(1, line.size() - 2).trimmed().toStdString();
            if (section.empty()) {
                std::cerr << "[ConfigManager] Syntax Error line " << lineNumber
                          << ": Empty section header\n";
                return std::nullopt;
            }
            continue;
        }

        if (line.startsWith(u'[') && !line.endsWith(u']')) {
            std::cerr << "[ConfigManager] Syntax Error line " << lineNumber
                      << ": Malformed section header: " << line.toStdString() << "\n";
            return std::nullopt;
        }

        const int eq = line.indexOf(u'=');
        if (eq < 0) {
            std::cerr << "[ConfigManager] Syntax Error line " << lineNumber
                      << ": Expected key = value: " << line.toStdString() << "\n";
            return std::nullopt;
        }

        const std::string key = line.left(eq).trimmed().toStdString();
        if (key.empty()) {
            std::cerr << "[ConfigManager] Syntax Error line " << lineNumber
                      << ": Empty key: " << line.toStdString() << "\n";
            return std::nullopt;
        }

        QString val = line.mid(eq + 1).trimmed();
        if (val.size() >= 2 && val.startsWith(u'"') && val.endsWith(u'"')) {
            val = val.mid(1, val.size() - 2);
        }

        data[section][key] = val.toStdString();
    }

    return data;
}

// ── Data Application ─────────────────────────────────────────────────────────

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
    if (auto v = get("ui", "scale")) {
        const double sc = QString::fromStdString(*v).toDouble(&ok);
        if (ok && sc >= 0.6 && sc <= 2.5) {
            m_uiScale = sc;
            m_scaleSetByFile = true;
        }
    } else if (!m_scaleSetByFile) {
        // No explicit scale in file — use DPI-based auto default
        m_uiScale = m_autoScale;
    }
    if (auto v = get("ui", "font_family"))
        m_fontFamily = QString::fromStdString(*v);

    if (auto v = get("general", "poll_interval_ms")) {
        const int ms = QString::fromStdString(*v).toInt(&ok);
        if (ok && ms > 0) m_pollMs = ms;
    }

    // Window Mode from file hot-reload (fromUser = false)
    if (auto v = get("window", "mode")) {
        setWindowModeInternal(QString::fromStdString(*v).toLower().trimmed(), false);
    }
    if (auto v = get("window", "click_through")) {
        const std::string& s = *v;
        setClickThroughInternal(s == "true" || s == "1", false);
    }

    // Colors
    auto applyColor = [&](const char* key, QString& dst) {
        if (auto v = get("colors", key)) {
            const QString s = QString::fromStdString(*v);
            if (QColor::isValidColorName(s)) dst = s;
        }
    };
    applyColor("background", m_bgColor);
    applyColor("accent",     m_accentColor);
    applyColor("text",       m_textColor);
    applyColor("graph_line", m_graphColor);

    // Scripts
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

// ── Live In-Memory Setters (Settings GUI) ────────────────────────────────────

void ConfigManager::setUiOpacity(double opacity) {
    const double clamped = std::clamp(opacity, 0.1, 1.0);
    if (qFuzzyCompare(m_opacity, clamped)) return;
    m_opacity = clamped;
    emit configChanged();
}

void ConfigManager::setUiScale(double scale) {
    const double clamped = std::clamp(scale, 0.6, 2.5);
    if (qFuzzyCompare(m_uiScale, clamped)) return;
    m_uiScale = clamped;
    emit configChanged();
}

void ConfigManager::setAccentColor(const QString& color) {
    if (m_accentColor == color) return;
    if (!QColor::isValidColorName(color)) return;
    m_accentColor = color;
    emit configChanged();
}

// ── Window Mode Setters ──────────────────────────────────────────────────────

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
        if (fromUser) emit windowModeChangedByUser(m_windowMode);
    }
}

void ConfigManager::setClickThroughInternal(bool enabled, bool fromUser) {
    if (m_clickThrough != enabled) {
        m_clickThrough = enabled;
        emit clickThroughChanged(m_clickThrough);
        if (fromUser) emit clickThroughChangedByUser(m_clickThrough);
    }
}

// ── Autostart ────────────────────────────────────────────────────────────────

bool ConfigManager::autostartEnabled() const {
    const QString path = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                       + QStringLiteral("/autostart/glassstat.desktop");
    return QFile::exists(path);
}

void ConfigManager::setAutostartEnabled(bool enabled) {
    const QString autostartPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                                + QStringLiteral("/autostart/glassstat.desktop");
    if (enabled) {
        if (QFile::exists(autostartPath)) { emit autostartChanged(true); return; }
        QDir().mkpath(QFileInfo(autostartPath).absolutePath());

        // Find source .desktop
        QString src;
        for (const char* p : {"packaging/glassstat.desktop",
                               "../packaging/glassstat.desktop",
                               "/usr/share/applications/glassstat.desktop"}) {
            if (QFile::exists(QLatin1String(p))) { src = QLatin1String(p); break; }
        }
        if (src.isEmpty()) {
            // Write a minimal fallback
            QFile f(autostartPath);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream s(&f);
                s << "[Desktop Entry]\n"
                  << "Type=Application\n"
                  << "Name=GlassStat\n"
                  << "Exec=glassstat\n"
                  << "Icon=glassstat\n"
                  << "X-GNOME-Autostart-enabled=true\n"
                  << "Comment=System Monitor Desktop Widget\n";
            }
        } else {
            QFile::copy(src, autostartPath);
            // Append autostart key if not present
            QFile f(autostartPath);
            if (f.open(QIODevice::ReadWrite | QIODevice::Text)) {
                QString content = QString::fromUtf8(f.readAll());
                if (!content.contains(QStringLiteral("X-GNOME-Autostart-enabled"))) {
                    f.seek(f.size());
                    QTextStream s(&f);
                    s << "X-GNOME-Autostart-enabled=true\n";
                }
            }
        }
        emit autostartChanged(true);
    } else {
        QFile::remove(autostartPath);
        emit autostartChanged(false);
    }
}

// ── Surgical File Writer ─────────────────────────────────────────────────────

void ConfigManager::setAndPersist(const QString& section, const QString& key, const QString& value) {
    const QString targetPath = ensureUserConfigPath();
    if (targetPath.isEmpty()) {
        std::cerr << "[ConfigManager] setAndPersist: cannot determine user config path\n";
        return;
    }

    QFile f(targetPath);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        std::cerr << "[ConfigManager] setAndPersist: cannot open " << targetPath.toStdString() << "\n";
        return;
    }
    QStringList lines = QString::fromUtf8(f.readAll()).split(u'\n');
    f.close();

    const QString sectionHeader = u'[' + section + u']';
    const QString keyPrefix      = key + QStringLiteral("=");
    const QString keyPrefixSpace = key + QStringLiteral(" =");

    bool inTargetSection = false;
    bool keyFound = false;

    for (int i = 0; i < lines.size(); ++i) {
        const QString trimmed = lines[i].trimmed();

        // Detect section headers
        if (trimmed.startsWith(u'[') && trimmed.endsWith(u']')) {
            if (inTargetSection && !keyFound) {
                // We were in the section but key wasn't found — insert before this new section
                lines.insert(i, key + QStringLiteral(" = ") + value);
                keyFound = true;
                break;
            }
            inTargetSection = (trimmed.mid(1, trimmed.size() - 2).trimmed() == section);
            continue;
        }

        if (!inTargetSection) continue;

        // Match "key = ..." or "key=..." (with optional trailing comment)
        if (trimmed.startsWith(keyPrefix) || trimmed.startsWith(keyPrefixSpace)) {
            // Preserve any trailing inline comment
            const int eqPos = lines[i].indexOf(u'=');
            const QString afterEq = lines[i].mid(eqPos + 1);
            const int commentPos = [&]() -> int {
                bool inS = false;
                for (int j = 0; j < afterEq.size(); ++j) {
                    if (afterEq[j] == u'"') inS = !inS;
                    if (!inS && afterEq[j] == u'#') return j;
                }
                return -1;
            }();

            const QString trailingComment = commentPos >= 0
                ? QStringLiteral("   ") + afterEq.mid(commentPos).trimmed()
                : QString{};

            // Reconstruct: preserve leading whitespace from original key
            const int leadingSpaces = [&]() -> int {
                for (int j = 0; j < lines[i].size(); ++j)
                    if (lines[i][j] != u' ' && lines[i][j] != u'\t') return j;
                return 0;
            }();
            const QString indent = lines[i].left(leadingSpaces);
            lines[i] = indent + key + QStringLiteral(" = ") + value + trailingComment;
            keyFound = true;
            break;
        }
    }

    // Key not found and we were in section at end of file
    if (!keyFound) {
        if (inTargetSection) {
            lines.append(key + QStringLiteral(" = ") + value);
        } else {
            // Section itself not found — append section + key
            if (!lines.isEmpty() && !lines.last().trimmed().isEmpty())
                lines.append(QString{});
            lines.append(QStringLiteral("[") + section + QStringLiteral("]"));
            lines.append(key + QStringLiteral(" = ") + value);
        }
    }

    // Write back via QSaveFile (atomic)
    QSaveFile sf(targetPath);
    if (!sf.open(QIODevice::WriteOnly | QIODevice::Text)) {
        std::cerr << "[ConfigManager] setAndPersist: cannot open for writing " << targetPath.toStdString() << "\n";
        return;
    }
    QTextStream out(&sf);
    out << lines.join(u'\n');
    sf.commit();

    // If we're not watching the user path yet, add it
    if (!m_watcher.files().contains(targetPath)) {
        m_watcher.addPath(targetPath);
        m_configPath = targetPath;
    }
}

// ── Preset Themes ────────────────────────────────────────────────────────────

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
