//
// Created by Mikael Zayenz Lagerkvist
//

#ifndef NMBR9_SYMMETRY_H
#define NMBR9_SYMMETRY_H

#include <vector>

namespace nmbr9::symmetry {
        /** Return index of (\a h, \a w) in the row-major layout of a matrix with
         * width \a w1 and height \a h1.
         */
        int pos(int h, int w, int h1, int w1);

        /** \name Symmetry functions
         *
         * These functions implement the 8 symmetries of 2D planes. The
         * functions are templatized so that they can be used both for the
         * pieces (defined using C-strings) and for arrays of variables.
         */

        /// Type for tile vector<int> symmetry functions
        typedef void (*vsymmfunc)(const std::vector<int> &, int, int, std::vector<int> &, int &, int &);

        /// Identity symmetry
        template<class CArray, class Array>
        void id(CArray t1, int w1, int h1, Array t2, int &w2, int &h2) {
            w2 = w1;
            h2 = h1;
            for (int h = 0; h < h1; ++h)
                for (int w = 0; w < w1; ++w)
                    t2[pos(h, w, h2, w2)] = t1[pos(h, w, h1, w1)];
        }

        /// Rotate 90 degrees
        template<class CArray, class Array>
        void rot90(CArray t1, int w1, int h1, Array t2, int &w2, int &h2) {
            w2 = h1;
            h2 = w1;
            for (int h = 0; h < h1; ++h)
                for (int w = 0; w < w1; ++w)
                    t2[pos(w, w2 - h - 1, h2, w2)] = t1[pos(h, w, h1, w1)];
        }

        /// Rotate 180 degrees
        template<class CArray, class Array>
        void rot180(CArray t1, int w1, int h1, Array t2, int &w2, int &h2) {
            w2 = w1;
            h2 = h1;
            for (int h = 0; h < h1; ++h)
                for (int w = 0; w < w1; ++w)
                    t2[pos(h2 - h - 1, w2 - w - 1, h2, w2)] = t1[pos(h, w, h1, w1)];
        }

        /// Rotate 270 degrees
        template<class CArray, class Array>
        void rot270(CArray t1, int w1, int h1, Array t2, int &w2, int &h2) {
            w2 = h1;
            h2 = w1;
            for (int h = 0; h < h1; ++h)
                for (int w = 0; w < w1; ++w)
                    t2[pos(h2 - w - 1, h, h2, w2)] = t1[pos(h, w, h1, w1)];
        }

}

#endif //NMBR9_SYMMETRY_H
