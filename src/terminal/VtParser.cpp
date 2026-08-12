#include "VtParser.h"

#include <QString>

#include <algorithm>
#include <utility>

VtParser::VtParser(TerminalScreen& screen)
    : m_screen(screen)
{
}

void VtParser::reset()
{
    m_state = State::Ground;
    m_textBytes.clear();
    m_csiBytes.clear();
    m_oscBytes.clear();
    m_utf8Decoder = QStringDecoder(QStringDecoder::Utf8);
}

void VtParser::consume(const QByteArray& bytes)
{
    for (const char rawByte : bytes) {
        const auto byte = static_cast<unsigned char>(rawByte);

        switch (m_state) {
        case State::Ground:
            if (byte == 0x1B) {
                flushText();
                m_state = State::Escape;
            } else if (byte < 0x20 || byte == 0x7F) {
                flushText();
                handleControl(byte);
            } else {
                m_textBytes.append(rawByte);
            }
            break;

        case State::Escape:
            handleEscape(byte);
            break;

        case State::Csi:
            if (byte >= 0x40 && byte <= 0x7E) {
                finishCsi(byte);
                m_csiBytes.clear();
                m_state = State::Ground;
            } else if (m_csiBytes.size() < 256) {
                m_csiBytes.append(rawByte);
            } else {
                m_csiBytes.clear();
                m_state = State::Ground;
            }
            break;

        case State::Osc:
            if (byte == 0x07) {
                finishOsc();
                m_oscBytes.clear();
                m_state = State::Ground;
            } else if (byte == 0x1B) {
                m_state = State::OscEscape;
            } else if (m_oscBytes.size() < 8192) {
                m_oscBytes.append(rawByte);
            }
            break;

        case State::OscEscape:
            if (byte == '\\') {
                finishOsc();
                m_oscBytes.clear();
                m_state = State::Ground;
            } else {
                if (m_oscBytes.size() < 8190) {
                    m_oscBytes.append('\x1B');
                    m_oscBytes.append(rawByte);
                }
                m_state = State::Osc;
            }
            break;
        }
    }

    if (m_state == State::Ground) {
        flushText();
    }
}

void VtParser::setResponseHandler(std::function<void(const QByteArray&)> handler)
{
    m_responseHandler = std::move(handler);
}

void VtParser::setTitleHandler(std::function<void(const QString&)> handler)
{
    m_titleHandler = std::move(handler);
}

void VtParser::flushText()
{
    if (m_textBytes.isEmpty()) {
        return;
    }
    const QString decoded = m_utf8Decoder(m_textBytes);
    m_textBytes.clear();
    if (!decoded.isEmpty()) {
        m_screen.writeText(decoded);
    }
}

void VtParser::handleControl(unsigned char byte)
{
    switch (byte) {
    case '\a':
        break;
    case '\b':
        m_screen.backspace();
        break;
    case '\t':
        m_screen.horizontalTab();
        break;
    case '\n':
    case '\v':
    case '\f':
        m_screen.lineFeed();
        break;
    case '\r':
        m_screen.carriageReturn();
        break;
    default:
        break;
    }
}

void VtParser::handleEscape(unsigned char byte)
{
    switch (byte) {
    case '[':
        m_csiBytes.clear();
        m_state = State::Csi;
        return;
    case ']':
        m_oscBytes.clear();
        m_state = State::Osc;
        return;
    case '7':
        m_screen.saveCursor();
        break;
    case '8':
        m_screen.restoreCursor();
        break;
    case 'D':
        m_screen.lineFeed();
        break;
    case 'M':
        m_screen.scrollDown(1);
        break;
    case 'E':
        m_screen.carriageReturn();
        m_screen.lineFeed();
        break;
    case 'c':
        m_screen.reset();
        break;
    default:
        break;
    }
    m_state = State::Ground;
}

void VtParser::finishCsi(unsigned char finalByte)
{
    QByteArray body = m_csiBytes;
    bool privateMode = false;
    if (!body.isEmpty() && body.front() == '?') {
        privateMode = true;
        body.remove(0, 1);
    }

    const QList<int> parameters = parseParameters(body);
    const int amount = parameterOr(parameters, 0, 1);

    switch (finalByte) {
    case 'A': m_screen.cursorUp(amount); break;
    case 'B': m_screen.cursorDown(amount); break;
    case 'C': m_screen.cursorForward(amount); break;
    case 'D': m_screen.cursorBackward(amount); break;
    case 'E': m_screen.cursorNextLine(amount); break;
    case 'F': m_screen.cursorPreviousLine(amount); break;
    case 'G': m_screen.setCursorColumn(parameterOr(parameters, 0, 1) - 1); break;
    case 'H':
    case 'f':
        m_screen.setCursorPosition(parameterOr(parameters, 0, 1) - 1,
                                   parameterOr(parameters, 1, 1) - 1);
        break;
    case 'd': m_screen.setCursorRow(parameterOr(parameters, 0, 1) - 1); break;
    case 'J': m_screen.eraseDisplay(parameterOr(parameters, 0, 0, false)); break;
    case 'K': m_screen.eraseLine(parameterOr(parameters, 0, 0, false)); break;
    case '@': m_screen.insertCharacters(amount); break;
    case 'P': m_screen.deleteCharacters(amount); break;
    case 'X': m_screen.eraseCharacters(amount); break;
    case 'L': m_screen.insertLines(amount); break;
    case 'M': m_screen.deleteLines(amount); break;
    case 'S': m_screen.scrollUp(amount); break;
    case 'T': m_screen.scrollDown(amount); break;
    case 'm': applySgr(parameters); break;
    case 'r': {
        const int top = parameterOr(parameters, 0, 1) - 1;
        const int bottom = parameterOr(parameters, 1, m_screen.rows()) - 1;
        m_screen.setScrollRegion(top, bottom);
        break;
    }
    case 's': m_screen.saveCursor(); break;
    case 'u': m_screen.restoreCursor(); break;
    case 'h':
    case 'l':
        if (privateMode) {
            applyPrivateMode(parameters, finalByte == 'h');
        }
        break;
    case 'n': {
        const int request = parameterOr(parameters, 0, 0, false);
        if (request == 5) {
            sendResponse(QByteArray("\x1b[0n"));
        } else if (request == 6) {
            sendResponse(QByteArray("\x1b[")
                         + QByteArray::number(m_screen.cursorRow() + 1)
                         + ';'
                         + QByteArray::number(m_screen.cursorColumn() + 1)
                         + 'R');
        }
        break;
    }
    case 'c':
        sendResponse(QByteArray("\x1b[?1;2c"));
        break;
    case 't':
        if (parameterOr(parameters, 0, 0, false) == 18) {
            sendResponse(QByteArray("\x1b[8;")
                         + QByteArray::number(m_screen.rows())
                         + ';'
                         + QByteArray::number(m_screen.columns())
                         + 't');
        }
        break;
    default:
        break;
    }
}

void VtParser::finishOsc()
{
    const qsizetype separator = m_oscBytes.indexOf(';');
    if (separator <= 0) {
        return;
    }

    bool ok = false;
    const int command = m_oscBytes.left(separator).toInt(&ok);
    if (!ok) {
        return;
    }

    if ((command == 0 || command == 2) && m_titleHandler) {
        const QByteArray titleBytes = m_oscBytes.mid(separator + 1);
        m_titleHandler(QString::fromUtf8(titleBytes));
    }
}

void VtParser::applySgr(const QList<int>& parameters)
{
    const QList<int> params = parameters.isEmpty() ? QList<int>{0} : parameters;
    for (int index = 0; index < params.size(); ++index) {
        const int code = params.at(index);
        if (code == 0) {
            m_screen.resetStyle();
        } else if (code == 1) {
            m_screen.setBold(true);
        } else if (code == 3) {
            m_screen.setItalic(true);
        } else if (code == 4) {
            m_screen.setUnderline(true);
        } else if (code == 7) {
            m_screen.setInverse(true);
        } else if (code == 22) {
            m_screen.setBold(false);
        } else if (code == 23) {
            m_screen.setItalic(false);
        } else if (code == 24) {
            m_screen.setUnderline(false);
        } else if (code == 27) {
            m_screen.setInverse(false);
        } else if (code >= 30 && code <= 37) {
            m_screen.setForeground(TerminalScreen::indexedColor(code - 30));
        } else if (code >= 40 && code <= 47) {
            m_screen.setBackground(TerminalScreen::indexedColor(code - 40));
        } else if (code >= 90 && code <= 97) {
            m_screen.setForeground(TerminalScreen::indexedColor((code - 90) + 8));
        } else if (code >= 100 && code <= 107) {
            m_screen.setBackground(TerminalScreen::indexedColor((code - 100) + 8));
        } else if (code == 39) {
            m_screen.setDefaultForeground();
        } else if (code == 49) {
            m_screen.setDefaultBackground();
        } else if ((code == 38 || code == 48) && index + 1 < params.size()) {
            const bool foreground = code == 38;
            const int mode = params.at(index + 1);
            if (mode == 5 && index + 2 < params.size()) {
                const QColor color = TerminalScreen::indexedColor(params.at(index + 2));
                foreground ? m_screen.setForeground(color) : m_screen.setBackground(color);
                index += 2;
            } else if (mode == 2 && index + 4 < params.size()) {
                const QColor color(std::clamp(params.at(index + 2), 0, 255),
                                   std::clamp(params.at(index + 3), 0, 255),
                                   std::clamp(params.at(index + 4), 0, 255));
                foreground ? m_screen.setForeground(color) : m_screen.setBackground(color);
                index += 4;
            }
        }
    }
}

void VtParser::applyPrivateMode(const QList<int>& parameters, bool enabled)
{
    for (const int mode : parameters) {
        switch (mode) {
        case 1: m_screen.setApplicationCursorKeys(enabled); break;
        case 7: m_screen.setAutoWrap(enabled); break;
        case 25: m_screen.setCursorVisible(enabled); break;
        case 47:
        case 1047: m_screen.useAlternateScreen(enabled, true); break;
        case 1049:
            if (enabled) {
                m_screen.saveCursor();
                m_screen.useAlternateScreen(true, true);
            } else {
                m_screen.useAlternateScreen(false, false);
                m_screen.restoreCursor();
            }
            break;
        case 2004: m_screen.setBracketedPaste(enabled); break;
        default: break;
        }
    }
}

void VtParser::sendResponse(const QByteArray& response)
{
    if (m_responseHandler) {
        m_responseHandler(response);
    }
}

QList<int> VtParser::parseParameters(const QByteArray& text) const
{
    if (text.isEmpty()) {
        return {};
    }

    QList<int> parameters;
    const QList<QByteArray> pieces = text.split(';');
    parameters.reserve(pieces.size());
    for (const QByteArray& piece : pieces) {
        if (piece.isEmpty()) {
            parameters.push_back(0);
            continue;
        }
        bool ok = false;
        const int value = piece.toInt(&ok);
        parameters.push_back(ok ? value : 0);
    }
    return parameters;
}

int VtParser::parameterOr(const QList<int>& parameters, int index, int fallback, bool zeroUsesFallback)
{
    if (index < 0 || index >= parameters.size()) {
        return fallback;
    }
    const int value = parameters.at(index);
    return (zeroUsesFallback && value == 0) ? fallback : value;
}
