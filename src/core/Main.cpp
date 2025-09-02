#include <iostream>

#include "Application.h"

int main(int argc, char *argv[]) {
    try {
        Application app(1280, 720, "GLCraft");
        app.run();
    } catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
