#include <QApplication>
#include "MainWindow.h"

// Forward declare the CLI main function from cli/main.c
extern "C" int cli_main(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    // If any command-line arguments provided, run in CLI mode
    if (argc > 1) {
        return cli_main(argc, argv);
    }

    // Otherwise, launch GUI mode
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    app.setApplicationName("Winafi");
#ifdef WINAFI_VERSION
    app.setApplicationVersion(WINAFI_VERSION);
#else
    app.setApplicationVersion("0.0.5");
#endif

    MainWindow window;
    window.show();

    return app.exec();
}
