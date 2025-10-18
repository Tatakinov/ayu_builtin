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
