#include <flexy/env/application.h>

int main(int argc, char** argv) {
    flexy::Application app;
    if (!app.init(argc, argv)) {
        return -1;
    }
    return app.run();
}