//
// Created by Mikael Zayenz Lagerkvist
//

#include "symmetry.h"

#include <cassert>

namespace nmbr9::symmetry {
    int pos(int h, int w, int h1, int w1) {
        assert(0 <= h && h < h1);
        assert(0 <= w && w < w1);

        return h * w1 + w;
    }
}
