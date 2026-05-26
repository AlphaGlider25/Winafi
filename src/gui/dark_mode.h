#ifndef DARK_MODE_H
#define DARK_MODE_H

#include <QApplication>

/**
 * Apply dark mode palette to the application
 * @param app The QApplication instance
 * @param enable True to enable dark mode, false for light mode
 */
void dark_mode_apply(QApplication *app, bool enable);

/**
 * Check if dark mode is currently enabled
 * @return true if dark mode is enabled, false otherwise
 */
bool dark_mode_is_enabled(void);

#endif // DARK_MODE_H
