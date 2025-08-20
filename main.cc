/* ----------------------------------------------------------------------------
// Simple sample to prove that Assimp is easy to use with OpenGL.
// It takes a file name as command line parameter, loads it using standard
// settings and displays it.
//
// If you intend to _use_ this code sample in your app, do yourself a favour
// and replace immediate mode calls with VBOs ...
//
// The vc8 solution links against assimp-release-dll_win32 - be sure to
// have this configuration built.
// ----------------------------------------------------------------------------
*/

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "ayu_.h"
#include "logger.h"
#include "program.h"
#include "vertex.h"

int main(int argc, char **argv) {
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
