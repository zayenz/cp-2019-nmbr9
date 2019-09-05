//
// Created by Mikael Zayenz Lagerkvist
//


#include <sstream>
#include <vector>
#include <iostream>
#include <chrono>

#include <gecode/driver.hh>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>

#include "config.h"
#include "nmbr9/lib.h"

int main(int argc, char **argv) {
    // Clock function used.
    auto now = [] { return std::chrono::steady_clock::now(); };

    const auto script_start = now();

    nmbr9::Nmbr9Options opt;
    opt.parse(argc,argv);
    Gecode::Script::run<
            nmbr9::Nmbr9Board,
            Gecode::BAB,
            nmbr9::Nmbr9Options>(opt);


    // Report results
    //
    const auto script_end = now();
    const std::chrono::duration<double, std::milli> script_duration =
            script_end - script_start;

    std::cout << "Ran Nmbr9 in " <<  script_duration.count()
              << std::endl;

    return EXIT_SUCCESS;
}

