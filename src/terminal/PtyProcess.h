#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringList>

#include <memory>

class QSocketNotifier;
class QTimer;

class PtyProcess final : public QObject
{
    Q_OBJECT

public:
    explicit PtyProcess(QObject* parent = nullptr);
    ~PtyProcess() override;

    PtyProcess(const PtyProcess&) = delete;
    PtyProcess& operator=(const PtyProcess&) = delete;

    [[nodiscard]] bool isRunning() const noexcept;
    [[nodiscard]] qint64 processId() const noexcept;

    bool start(
        const QString& program,
        const QStringList& arguments,
        const QString& workingDirectory = {});

    qint64 write(const QByteArray& data);
    void resize(int rows, int columns);
    void sendSignal(int signalNumber);
    void terminate();

signals:
    void outputReady(const QByteArray& data);
    void started(qint64 pid);
    void finished(int exitCode, int termSignal);
    void errorOccurred(const QString& message);

private:
    void readAvailable();
    void checkChildState();
    void closeMaster();

    int m_masterFd{-1};
    qint64 m_pid{-1};
    bool m_running{false};
    std::unique_ptr<QSocketNotifier> m_readNotifier;
    std::unique_ptr<QTimer> m_childPollTimer;
};
