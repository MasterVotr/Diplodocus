#pragma once

#include <cassert>
#include <string>

#include "app/app_context.h"

namespace diplodocus {

struct AppConsoleArguments {
    int count;
    char** values;

    inline char* operator[](int index) {
        assert(index < count && "ConsoleArguments: index out-of-bounds");
        return values[index];
    }
};

struct AppParameters {
    std::string name;
    AppConsoleArguments app_console_args;
};

class App {
   public:
    App(AppParameters app_params) : app_params_(app_params) {}
    virtual ~App() = default;

    virtual void Run() = 0;

   protected:
    AppContext app_ctx_;
    AppParameters app_params_;
};

}  // namespace diplodocus
