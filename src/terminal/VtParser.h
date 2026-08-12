#pragma once

#include "TerminalScreen.h"

#include <QByteArray>
#include <QList>
#include <QStringDecoder>

#include <functional>

class VtParser final
{
public:
    explicit VtParser(TerminalScreen& screen);

    void reset();
    void consume(const QByteArray& bytes);

    void setResponseHandler(std::function<void(const QByteArray&)> handler);
    void setTitleHandler(std::function<void(const QString&)> handler);

private:
    enum class State {
        Ground,
        Escape,
        Csi,
        Osc,
        OscEscape,
        Charset
    };

    void flushText();
    void handleControl(unsigned char byte);
    void handleEscape(unsigned char byte);
    void finishCsi(unsigned char finalByte);
    void finishOsc();
    void applySgr(const QList<int>& parameters);
    void applyPrivateMode(const QList<int>& parameters, bool enabled);
    void sendResponse(const QByteArray& response);
    void finishCharset(unsigned char byte);
    [[nodiscard]] QString mapDecSpecial(unsigned char byte) const;

    [[nodiscard]] QList<int> parseParameters(const QByteArray& text) const;
    [[nodiscard]] static int parameterOr(const QList<int>& parameters, int index, int fallback, bool zeroUsesFallback = true);

    TerminalScreen& m_screen;
    State m_state{State::Ground};
    QByteArray m_textBytes;
    QByteArray m_csiBytes;
    QByteArray m_oscBytes;
    QStringDecoder m_utf8Decoder{QStringDecoder::Utf8};
    std::function<void(const QByteArray&)> m_responseHandler;
    std::function<void(const QString&)> m_titleHandler;
    bool m_g0SpecialGraphics{false};
    bool m_g1SpecialGraphics{false};
    bool m_useG1{false};
    int m_charsetTarget{0};
};
