#include "app/cli_app.h"
#include "util/logger.h"

int main(int argc, char** argv) {
    Logger::init();

    // Create app specification
    diplodocus::AppParameters app_params = {
        .name = "Console app",
        .app_console_args =
            {
                .count = argc,
                .values = argv,
            },
    };

    // Create app
    diplodocus::CliApp app(app_params);

    // Run app
    app.Run();

    return 0;
}
