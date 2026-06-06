// Forward declaration
extern int cli_main(int argc, char *argv[]);

// Entry point for CLI executable (winafi)
// GUI executable (winafi-gui) calls cli_main() directly
int main(int argc, char *argv[]) {
    return cli_main(argc, argv);
}
