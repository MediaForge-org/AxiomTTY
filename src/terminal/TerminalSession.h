#pragma once

#include "PtyProcess.h"
#include "TerminalScreen.h"
#include "VtParser.h"

#include <QObject>
#include <QString>

class TerminalSession final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString title READ title NOTIFY titleChanged)
    Q_PROPERTY(QString shell READ shell NOTIFY shellChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(qint64 processId READ processId NOTIFY processIdChanged)
    Q_PROPERTY(bool applicationCursorKeys READ applicationCursorKeys NOTIFY terminalModesChanged)
    Q_PROPERTY(bool bracketedPaste READ bracketedPaste NOTIFY terminalModesChanged)
    Q_PROPERTY(int rows READ rows NOTIFY terminalSizeChanged)
    Q_PROPERTY(int columns READ columns NOTIFY terminalSizeChanged)

public:
    explicit TerminalSession(QObject* parent = nullptr);

    [[nodiscard]] QString title() const;
    [[nodiscard]] QString shell() const;
    [[nodiscard]] bool running() const;
    [[nodiscard]] qint64 processId() const;
    [[nodiscard]] bool applicationCursorKeys() const;
    [[nodiscard]] bool bracketedPaste() const;
    [[nodiscard]] int rows() const noexcept;
    [[nodiscard]] int columns() const noexcept;
    [[nodiscard]] const TerminalScreen& screen() const noexcept;

public slots:
    void startDefaultShell();
    void startShell(const QString& shellPath);
    void sendText(const QString& text);
    void pasteClipboard();
    void sendInterrupt();
    void sendSuspend();
    void clearDisplay();
    void resizeTerminal(int rows, int columns);

signals:
    void screenChanged();
    void titleChanged();
    void shellChanged();
    void runningChanged();
    void processIdChanged();
    void terminalModesChanged();
    void terminalSizeChanged();

private:
    void consumeOutput(const QByteArray& bytes);
    void writeParserResponse(const QByteArray& bytes);
    void setTitle(const QString& title);

    PtyProcess m_process;
    TerminalScreen m_screen;
    VtParser m_parser;
    QString m_title{QStringLiteral("Shell")};
    QString m_shell;
};
