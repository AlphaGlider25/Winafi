#ifndef WINAFI_THEME_H
#define WINAFI_THEME_H
#include <QString>
class QApplication;
namespace winafi {
// Returns a complete Qt stylesheet for the app. dark==true => violet dark theme
// (default), false => light variant sharing the #8b5cf6 accent.
QString themeStylesheet(bool dark);
// Applies palette + stylesheet to the whole application.
void applyTheme(QApplication *app, bool dark);
}
#endif
