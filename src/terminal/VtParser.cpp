#include "VtParser.h"

#include <QChar>
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
    m_g0SpecialGraphics = false;
    m_g1SpecialGraphics = false;
    m_useG1 = false;
    m_charsetTarget = 0;
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
                const bool specialGraphics = m_useG1 ? m_g1SpecialGraphics : m_g0SpecialGraphics;
                if (specialGraphics && byte >= 0x20 && byte <= 0x7E) {
                    flushText();
                    m_screen.writeText(mapDecSpecial(byte));
                } else {
                    m_textBytes.append(rawByte);
                }
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

        case State::Charset:
            finishCharset(byte);
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

void VtParser::setDefaultColorHandler(std::function<QColor(bool)> handler)
{
    m_defaultColorHandler = std::move(handler);
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
    case 0x0E: // SO: select G1
        m_useG1 = true;
        break;
    case 0x0F: // SI: select G0
        m_useG1 = false;
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
    case '(':
        m_charsetTarget = 0;
        m_state = State::Charset;
        return;
    case ')':
        m_charsetTarget = 1;
        m_state = State::Charset;
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
    case 'H':
        m_screen.setTabStopAtCursor();
        break;
    case 'c':
        m_screen.reset();
        m_g0SpecialGraphics = false;
        m_g1SpecialGraphics = false;
        m_useG1 = false;
        break;
    default:
        break;
    }
    m_state = State::Ground;
}

void VtParser::finishCharset(unsigned char byte)
{
    const bool specialGraphics = byte == '0';
    if (m_charsetTarget == 0) {
        m_g0SpecialGraphics = specialGraphics;
    } else {
        m_g1SpecialGraphics = specialGraphics;
    }
    m_state = State::Ground;
}

QString VtParser::mapDecSpecial(unsigned char byte) const
{
    switch (byte) {
    case '`': return QStringLiteral("◆");
    case 'a': return QStringLiteral("▒");
    case 'b': return QStringLiteral("␉");
    case 'c': return QStringLiteral("␌");
    case 'd': return QStringLiteral("␍");
    case 'e': return QStringLiteral("␊");
    case 'f': return QStringLiteral("°");
    case 'g': return QStringLiteral("±");
    case 'h': return QStringLiteral("␤");
    case 'i': return QStringLiteral("␋");
    case 'j': return QStringLiteral("┘");
    case 'k': return QStringLiteral("┐");
    case 'l': return QStringLiteral("┌");
    case 'm': return QStringLiteral("└");
    case 'n': return QStringLiteral("┼");
    case 'o': return QStringLiteral("⎺");
    case 'p': return QStringLiteral("⎻");
    case 'q': return QStringLiteral("─");
    case 'r': return QStringLiteral("⎼");
    case 's': return QStringLiteral("⎽");
    case 't': return QStringLiteral("├");
    case 'u': return QStringLiteral("┤");
    case 'v': return QStringLiteral("┴");
    case 'w': return QStringLiteral("┬");
    case 'x': return QStringLiteral("│");
    case 'y': return QStringLiteral("≤");
    case 'z': return QStringLiteral("≥");
    case '{': return QStringLiteral("π");
    case '|': return QStringLiteral("≠");
    case '}': return QStringLiteral("£");
    case '~': return QStringLiteral("·");
    default: return QString(QChar::fromLatin1(static_cast<char>(byte)));
    }
}

void VtParser::finishCsi(unsigned char finalByte)
{
    QByteArray body = m_csiBytes;
    QByteArray intermediates;

    qsizetype intermediateStart = -1;
    for (qsizetype index = 0; index < body.size(); ++index) {
        const auto byte = static_cast<unsigned char>(body.at(index));
        if (byte >= 0x20 && byte <= 0x2F) {
            intermediateStart = index;
            break;
        }
    }
    if (intermediateStart >= 0) {
        intermediates = body.mid(intermediateStart);
        body = body.left(intermediateStart);
    }

    char privatePrefix = '\0';
    if (!body.isEmpty()) {
        const char first = body.front();
        if (first == '?' || first == '>' || first == '<' || first == '=') {
            privatePrefix = first;
            body.remove(0, 1);
        }
    }

    const QList<int> parameters = parseParameters(body);
    const int amount = parameterOr(parameters, 0, 1);

    switch (finalByte) {
    case 'A': m_screen.cursorUp(amount); break;
    case 'B':
    case 'e': m_screen.cursorDown(amount); break;
    case 'C':
    case 'a': m_screen.cursorForward(amount); break;
    case 'D': m_screen.cursorBackward(amount); break;
    case 'E': m_screen.cursorNextLine(amount); break;
    case 'F': m_screen.cursorPreviousLine(amount); break;
    case 'G': m_screen.setCursorColumn(parameterOr(parameters, 0, 1) - 1); break;
    case 'I': m_screen.cursorForwardTab(amount); break;
    case 'Z': m_screen.cursorBackwardTab(amount); break;
    case 'H':
    case 'f': {
        int row = parameterOr(parameters, 0, 1) - 1;
        const int column = parameterOr(parameters, 1, 1) - 1;
        if (m_screen.originMode()) {
            row += m_screen.scrollTop();
            row = std::clamp(row, m_screen.scrollTop(), m_screen.scrollBottom());
        }
        m_screen.setCursorPosition(row, column);
        break;
    }
    case 'd': {
        int row = parameterOr(parameters, 0, 1) - 1;
        if (m_screen.originMode()) {
            row += m_screen.scrollTop();
            row = std::clamp(row, m_screen.scrollTop(), m_screen.scrollBottom());
        }
        m_screen.setCursorRow(row);
        break;
    }
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
    case 'b': m_screen.repeatLastCharacter(amount); break;
    case 'g': {
        const int mode = parameterOr(parameters, 0, 0, false);
        if (mode == 0) {
            m_screen.clearTabStopAtCursor();
        } else if (mode == 3) {
            m_screen.clearAllTabStops();
        }
        break;
    }
    case 'r': {
        const int top = parameterOr(parameters, 0, 1) - 1;
        const int bottom = parameterOr(parameters, 1, m_screen.rows()) - 1;
        m_screen.setScrollRegion(top, bottom);
        break;
    }
    case 's': m_screen.saveCursor(); break;
    case 'u': m_screen.restoreCursor(); break;
    case 'q':
        if (intermediates == QByteArray(" ")) {
            m_screen.setCursorStyle(parameterOr(parameters, 0, 0, false));
        }
        break;
    case 'h':
    case 'l': {
        const bool enabled = finalByte == 'h';
        if (privatePrefix == '?') {
            applyPrivateMode(parameters, enabled);
        } else if (privatePrefix == '\0') {
            for (const int mode : parameters) {
                if (mode == 4) {
                    m_screen.setInsertMode(enabled);
                }
            }
        }
        break;
    }
    case 'n': {
        const int request = parameterOr(parameters, 0, 0, false);
        if (privatePrefix == '?' && request == 6) {
            sendResponse(QByteArray("\x1b[?")
                         + QByteArray::number(m_screen.cursorRow() + 1)
                         + ';'
                         + QByteArray::number(m_screen.cursorColumn() + 1)
                         + 'R');
        } else if (privatePrefix == '\0' && request == 5) {
            sendResponse(QByteArray("\x1b[0n"));
        } else if (privatePrefix == '\0' && request == 6) {
            sendResponse(QByteArray("\x1b[")
                         + QByteArray::number(m_screen.cursorRow() + 1)
                         + ';'
                         + QByteArray::number(m_screen.cursorColumn() + 1)
                         + 'R');
        }
        break;
    }
    case 'c':
        if (privatePrefix == '>') {
            // Secondary DA: identify as a modern VT-compatible terminal without
            // claiming a public semantic version during pre-alpha.
            sendResponse(QByteArray("\x1b[>0;1;0c"));
        } else {
            sendResponse(QByteArray("\x1b[?1;2c"));
        }
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

    const QByteArray payload = m_oscBytes.mid(separator + 1);
    if ((command == 0 || command == 2) && m_titleHandler) {
        m_titleHandler(QString::fromUtf8(payload));
        return;
    }

    // xterm-compatible default color queries. Editors occasionally probe these
    // instead of trusting terminfo, so answering them avoids stray query text
    // and lets applications derive a sensible light/dark palette.
    if ((command == 10 || command == 11) && payload == QByteArray("?")) {
        const bool foreground = command == 10;
        const QColor color = m_defaultColorHandler
            ? m_defaultColorHandler(foreground)
            : (foreground ? TerminalScreen::defaultForeground() : TerminalScreen::defaultBackground());
        const auto component = [](int value) {
            return QStringLiteral("%1").arg(value * 257, 4, 16, QLatin1Char('0'));
        };
        const QString response = QStringLiteral("\x1b]%1;rgb:%2/%3/%4\x1b\\")
            .arg(command)
            .arg(component(color.red()))
            .arg(component(color.green()))
            .arg(component(color.blue()));
        sendResponse(response.toUtf8());
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
        } else if (code == 2) {
            m_screen.setFaint(true);
        } else if (code == 3) {
            m_screen.setItalic(true);
        } else if (code == 4) {
            m_screen.setUnderline(true);
        } else if (code == 7) {
            m_screen.setInverse(true);
        } else if (code == 9) {
            m_screen.setStrikethrough(true);
        } else if (code == 22) {
            m_screen.setBold(false);
            m_screen.setFaint(false);
        } else if (code == 23) {
            m_screen.setItalic(false);
        } else if (code == 24) {
            m_screen.setUnderline(false);
        } else if (code == 27) {
            m_screen.setInverse(false);
        } else if (code == 29) {
            m_screen.setStrikethrough(false);
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
        case 1:
            m_screen.setApplicationCursorKeys(enabled);
            break;
        case 6:
            m_screen.setOriginMode(enabled);
            break;
        case 7:
            m_screen.setAutoWrap(enabled);
            break;
        case 12:
            m_screen.setCursorBlinking(enabled);
            break;
        case 25:
            m_screen.setCursorVisible(enabled);
            break;
        case 47:
        case 1047:
            m_screen.useAlternateScreen(enabled, true);
            break;
        case 1048:
            enabled ? m_screen.saveCursor() : m_screen.restoreCursor();
            break;
        case 1049:
            if (enabled) {
                m_screen.saveCursor();
                m_screen.useAlternateScreen(true, true);
            } else {
                m_screen.useAlternateScreen(false, false);
                m_screen.restoreCursor();
            }
            break;
        case 1000:
            if (enabled) {
                m_screen.setMouseTrackingMode(TerminalMouseTrackingMode::Normal);
            } else if (m_screen.mouseTrackingMode() == TerminalMouseTrackingMode::Normal) {
                m_screen.setMouseTrackingMode(TerminalMouseTrackingMode::None);
            }
            break;
        case 1002:
            if (enabled) {
                m_screen.setMouseTrackingMode(TerminalMouseTrackingMode::ButtonEvent);
            } else if (m_screen.mouseTrackingMode() == TerminalMouseTrackingMode::ButtonEvent) {
                m_screen.setMouseTrackingMode(TerminalMouseTrackingMode::None);
            }
            break;
        case 1003:
            if (enabled) {
                m_screen.setMouseTrackingMode(TerminalMouseTrackingMode::AnyEvent);
            } else if (m_screen.mouseTrackingMode() == TerminalMouseTrackingMode::AnyEvent) {
                m_screen.setMouseTrackingMode(TerminalMouseTrackingMode::None);
            }
            break;
        case 1004:
            m_screen.setFocusReporting(enabled);
            break;
        case 1006:
            m_screen.setSgrMouseMode(enabled);
            break;
        case 1007:
            m_screen.setAlternateScroll(enabled);
            break;
        case 2004:
            m_screen.setBracketedPaste(enabled);
            break;
        case 2026:
            m_screen.setSynchronizedOutput(enabled);
            break;
        default:
            break;
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
