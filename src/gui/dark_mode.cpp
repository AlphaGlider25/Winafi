#include "dark_mode.h"
#include <QPalette>
#include <QColor>
#include <QStyle>

static bool g_dark_mode_enabled = false;

void dark_mode_apply(QApplication *app, bool enable) {
    if (!app) return;

    if (enable) {
        // Dark palette
        QPalette dark;
        dark.setColor(QPalette::Window,          QColor(0x2b, 0x2b, 0x2b));
        dark.setColor(QPalette::WindowText,      QColor(0xff, 0xff, 0xff));
        dark.setColor(QPalette::Base,            QColor(0x1e, 0x1e, 0x1e));
        dark.setColor(QPalette::AlternateBase,   QColor(0x35, 0x35, 0x35));
        dark.setColor(QPalette::ToolTipBase,     QColor(0x2b, 0x2b, 0x2b));
        dark.setColor(QPalette::ToolTipText,     QColor(0xff, 0xff, 0xff));
        dark.setColor(QPalette::Text,            QColor(0xff, 0xff, 0xff));
        dark.setColor(QPalette::Button,          QColor(0x3c, 0x3c, 0x3c));
        dark.setColor(QPalette::ButtonText,      QColor(0xff, 0xff, 0xff));
        dark.setColor(QPalette::BrightText,      QColor(0xff, 0x00, 0x00));
        dark.setColor(QPalette::Link,            QColor(0x42, 0xa5, 0xf5));
        dark.setColor(QPalette::Highlight,       QColor(0x42, 0x85, 0xf4));
        dark.setColor(QPalette::HighlightedText, QColor(0xff, 0xff, 0xff));
        dark.setColor(QPalette::Disabled, QPalette::Text,       QColor(0x80, 0x80, 0x80));
        dark.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(0x80, 0x80, 0x80));
        app->setPalette(dark);
    } else {
        // Light palette - use default style palette
        app->setPalette(app->style()->standardPalette());
    }

    g_dark_mode_enabled = enable;
}

bool dark_mode_is_enabled(void) {
    return g_dark_mode_enabled;
}
