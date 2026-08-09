#pragma once

#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>

namespace gs {

class ScriptRunner : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)

public:
    explicit ScriptRunner(QObject* parent = nullptr);

    // Called by ConfigManager hot-reload or initial setup.
    void         setScripts(const QVariantList& defs);
    QVariantList results() const { return m_results; }

public slots:
    void runAll();

signals:
    void resultsChanged();

private:
    static constexpr int kRunIntervalMs = 5000;
    static constexpr int kTimeoutMs     = 10000;

    QTimer       m_timer;
    QVariantList m_defs;    // [{name, command}]
    QVariantList m_results; // [{name, command, output, exitCode}]
};

} // namespace gs
