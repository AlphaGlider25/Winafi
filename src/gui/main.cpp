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
    QApplication app(argc, argv);
    app.setApplicationName("Winafi");
    app.setApplicationVersion("4.0.0");

    MainWindow window;
    window.show();

    return app.exec();
}
