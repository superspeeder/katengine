#include "game/main.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[]) {
    kat::init();

    kat::setValidationLayersEnabled(true);
//    kat::setApiDumpEnabled(true);

    kat::startup();



    kat::run();

    kat::terminate();
    return EXIT_SUCCESS;
}
