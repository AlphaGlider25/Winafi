#include "theme/Theme.h"
#include <QApplication>

namespace {
struct Palette { const char *bg0,*bg1,*bg2,*accent,*accent2,*text,*muted,*border; };
const Palette DARK  = {"#1c1830","#262038","#352c52","#8b5cf6","#6d28d9","#e6e1f5","#9d96b8","#352c52"};
const Palette LIGHT = {"#f4f1fb","#ffffff","#e6e1f5","#8b5cf6","#6d28d9","#241b3a","#6b6486","#d8d2ea"};
}

namespace winafi {

QString themeStylesheet(bool dark) {
    const Palette &p = dark ? DARK : LIGHT;
    return QString(
        "QWidget{background:%1;color:%6;font-family:'Inter','Segoe UI',sans-serif;font-size:13px;}"
        "QFrame#Nav{background:%1;border-right:1px solid %8;}"
        "QPushButton#Nav,QPushButton#Nav:checked{text-align:left;padding:8px 10px;border:none;"
            "border-radius:6px;color:%7;background:transparent;}"
        "QPushButton#Nav:checked{background:%3;color:%6;font-weight:600;}"
        "QFrame#Footer{background:%2;border-top:1px solid %8;}"
        "QLineEdit,QComboBox{background:%2;border:1px solid %8;border-radius:6px;padding:6px 8px;color:%6;}"
        "QPushButton{background:%4;color:#ffffff;border:none;border-radius:8px;padding:7px 16px;font-weight:600;}"
        "QPushButton:hover{background:%5;}"
        "QPushButton:disabled{background:%3;color:%7;}"
        "QPushButton#Ghost{background:transparent;border:1px solid %8;color:%6;}"
        "QGroupBox{border:1px solid %8;border-radius:6px;margin-top:8px;padding:8px;}"
        "QProgressBar{border:1px solid %8;border-radius:6px;background:%1;height:12px;text-align:center;color:%6;}"
        "QProgressBar::chunk{background:%4;border-radius:6px;}"
        "QCheckBox{spacing:7px;}"
    ).arg(p.bg0).arg(p.bg1).arg(p.bg2).arg(p.accent).arg(p.accent2)
     .arg(p.text).arg(p.muted).arg(p.border);
}

void applyTheme(QApplication *app, bool dark) {
    if (app) app->setStyleSheet(themeStylesheet(dark));
}

}
