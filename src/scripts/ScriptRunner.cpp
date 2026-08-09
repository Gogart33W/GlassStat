#include "scripts/ScriptRunner.hpp"

#include <QStringLiteral>

namespace gs {

ScriptRunner::ScriptRunner(QObject* parent) : QObject(parent) {
    m_timer.setInterval(kRunIntervalMs);
    connect(&m_timer, &QTimer::timeout, this, &ScriptRunner::runAll);
}

void ScriptRunner::setScripts(const QVariantList& defs) {
    m_defs = defs;

    // Rebuild result list with placeholder output
    m_results.clear();
    for (const auto& defVar : m_defs) {
        const QVariantMap def = defVar.toMap();
        QVariantMap result;
        result[QStringLiteral("name")]     = def[QStringLiteral("name")];
        result[QStringLiteral("command")]  = def[QStringLiteral("command")];
        result[QStringLiteral("output")]   = QStringLiteral("…");
        result[QStringLiteral("exitCode")] = -1;
        m_results.append(result);
    }
    emit resultsChanged();

    if (!m_defs.isEmpty()) {
        runAll();
        m_timer.start();
    } else {
        m_timer.stop();
    }
}

void ScriptRunner::runAll() {
    for (int i = 0; i < m_defs.size(); ++i) {
        const QVariantMap def = m_defs[i].toMap();
        const QString name    = def[QStringLiteral("name")].toString();
        const QString cmd     = def[QStringLiteral("command")].toString();

        auto* proc = new QProcess(this);

        // Hard timeout — kill runaway scripts so they don't accumulate
        QTimer::singleShot(kTimeoutMs, proc, [proc]() {
            if (proc->state() == QProcess::Running)
                proc->kill();
        });

        connect(proc, &QProcess::finished, this,
            [this, proc, name](int exitCode, QProcess::ExitStatus) {
                const QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
                const QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();

                for (int j = 0; j < m_results.size(); ++j) {
                    QVariantMap r = m_results[j].toMap();
                    if (r[QStringLiteral("name")].toString() == name) {
                        r[QStringLiteral("output")]   = out.isEmpty() ? err : out;
                        r[QStringLiteral("exitCode")] = exitCode;
                        m_results[j] = r;
                        break;
                    }
                }
                emit resultsChanged();
                proc->deleteLater();
            });

        proc->start(QStringLiteral("/bin/bash"), {QStringLiteral("-c"), cmd});
    }
}

} // namespace gs
