#include <cstdlib>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_main.h>

#include "ayu_.h"
#include "logger.h"

int main(int argc, char **argv) {
    SDL_SetHint(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1");
    SDL_SetAppMetadataProperty(SDL_PROP_APP_METADATA_NAME_STRING, "io.github.tatakinov.ninix-kagari.ayu_builtin");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return 1;
    }
    atexit(SDL_Quit);

    Ayu ayu;

#if defined(DEBUG)
    ayu.create(0);
    ayu.show(0);
    ayu.setSurface(0, 0);
#endif // DEBUG

    while (ayu) {
        ayu.draw();
    }

	return 0;
}
