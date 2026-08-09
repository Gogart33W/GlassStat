#include "config/ConfigManager.hpp"

#include <QColor>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>
#include <QTimer>

namespace gs {

ConfigManager::ConfigManager(const QString& path, QObject* parent)
    : QObject(parent), m_configPath(path)
{
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

void ConfigManager::onFileChanged(const QString& path) {
    // Editors that use atomic-write (delete + create) need the watcher re-armed.
    // A short delay lets the new file appear before we re-watch and reload.
    QTimer::singleShot(120, this, [this, path]() {
        if (!m_watcher.files().contains(path))
            m_watcher.addPath(path);
        load(path);
        emit configChanged();
    });
}

void ConfigManager::load(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;

    QTextStream in(&f);
    applyData(parseToml(in.readAll()));
}

// Minimal TOML subset: [sections], key = value, # comments (inline + full-line),
// string values in double-quotes, bare int/float/bool otherwise.
ConfigManager::TomlData ConfigManager::parseToml(const QString& content) {
    TomlData data;
    std::string section;

    for (const QString& rawLine : content.split(u'\n')) {
        // Strip inline comment respecting quoted strings
        QString line;
        bool inStr = false;
        for (const QChar ch : rawLine) {
            if (ch == u'"') inStr = !inStr;
            if (!inStr && ch == u'#') break;
            line += ch;
        }
        line = line.trimmed();
        if (line.isEmpty()) continue;

        // [section]
        if (line.startsWith(u'[') && line.endsWith(u']')) {
            section = line.mid(1, line.size() - 2).trimmed().toStdString();
            continue;
        }

        const int eq = line.indexOf(u'=');
        if (eq < 0) continue;

        const std::string key = line.left(eq).trimmed().toStdString();
        QString val = line.mid(eq + 1).trimmed();

        // Unquote strings
        if (val.size() >= 2 && val.startsWith(u'"') && val.endsWith(u'"'))
            val = val.mid(1, val.size() - 2);

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

} // namespace gs
