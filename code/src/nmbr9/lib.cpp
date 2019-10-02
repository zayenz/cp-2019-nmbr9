#include <utility>

//
// Created by Mikael Zayenz Lagerkvist
//

#include "lib.h"
#include "symmetry.h"
#include "tiles.h"

#include <gecode/driver.hh>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>

#include <iostream>
#include <iomanip>

using namespace Gecode;

namespace nmbr9 {

    namespace {
        /**
         * Utility function that takes a square matrix as an array in row-major order, and produces a new array
         * with the elements in sprial order, starting form the center.
         */
        template <class C>
        C anti_spiral(const C& in, int wh) {
            C reversed_out;
            Matrix<C> m(in, wh, wh);

            int start_row = 0;
            int end_row = wh;
            int start_col = 0;
            int end_col = wh;

            while (start_row < end_row && start_col < end_col) {
                // First of remaining upper row
                for (int c = start_col; c < end_col; ++c) {
                    reversed_out << m(c, start_row);
                }
                start_row += 1;

                // Last remaining end column
                for (int r = start_row; r < end_row; ++r) {
                    reversed_out << m(end_col-1, r);
                }
                end_col -= 1;

                // Last of remaining lower row (if any)
                if (start_row < end_row) {
                    for (int c = end_col-1; c >= start_col; --c) {
                        reversed_out << m(c, end_row-1);
                    }
                    end_row -= 1;
                }

                // Last of remaining start column (if any)
                if (start_col < end_col) {
                    for (int r = end_row-1; r >= start_row; --r) {
                        reversed_out << m(start_col, r);
                    }
                    start_col += 1;
                }
            }

            assert(reversed_out.size() == in.size());

            C out;
            for (int i = in.size()-1; i >= 0; --i) {
                out << reversed_out[i];
            }

            assert(reversed_out.size() == out.size());

            return out;
        }
    }

    //
    // Main code setting up the model.
    //
    
    Nmbr9Board::Nmbr9Board(const Nmbr9Options& options)
            : IntMaximizeScript(options),
              instance_(options.instance()),
              wh_(options.grid_size()),
              nlevels_(options.max_layers()),
              nparts_(options.number_of_parts()),
              ncolors_(nparts_ + 1),
              ncards_(options.deck_size()),
              nsquares_(wh_ * wh_),
              empty_color_(0),
              boards_(), // Initialized in body
              value_boards_(), // Initialized in body
              tile_is_used_(*this, nparts_, 0, 1),
              tile_is_not_used_(*this, nparts_, 0, 1),
              tile_is_on_level_(*this, nparts_*nlevels_, 0, 1),
              tile_value_(nparts_), // Initialized in body
              tile_level_(*this, nparts_, 0, nlevels_),
              placement_boards_(), // Initialized in body
              part_boards_(), // Initialized in body
              around_boards_(), // Initialized in body
              before_(*this, nparts_ * nparts_, 0, 1),
              deck_(*this, ncards_, 0, nparts_),
              order_(*this, nparts_, 0, nparts_),
              score_(*this, 0, options.max_value() * options.copies() * options.max_layers())
    {
        // Initialization of variables
        //

        boards_.reserve(nlevels_);
        value_boards_.reserve(nlevels_);
        placement_boards_.reserve(nlevels_);
        part_boards_.reserve(nlevels_);
        around_boards_.reserve(nlevels_);
        for (int l = 0; l < nlevels_; ++l) {
            boards_.emplace_back(IntVarArray(*this, nsquares_, 0, ncolors_-1));
            value_boards_.emplace_back(IntVarArray(*this, nsquares_, 0, 9));
            placement_boards_.emplace_back(std::vector<IntVarArray>());
            placement_boards_.rbegin()->reserve(nparts_);
            part_boards_.emplace_back(std::vector<BoolVarArray>());
            part_boards_.rbegin()->reserve(nparts_);
            around_boards_.emplace_back(std::vector<BoolVarArray>());
            around_boards_.rbegin()->reserve(nparts_);
            for (int p = 0; p < nparts_; ++p) {
                placement_boards_.rbegin()->emplace_back(IntVarArray(*this, nsquares_, 0, 2));
                part_boards_.rbegin()->emplace_back(BoolVarArray(*this, nsquares_, 0, 1));
                around_boards_.rbegin()->emplace_back(BoolVarArray(*this, nsquares_, 0, 1));
            }
        }
        for (int p = 0; p < nparts_; ++p) {
            tile_value_[p] = nmbr9::tile(options.instance(), p+1).value();
        }


        // Setting upp access to variables
        //

        Matrix<BoolVarArray> mbefore(before_, nparts_, nparts_);
        Matrix<BoolVarArray> mtile_is_on_level(tile_is_on_level_, nparts_, nlevels_);


        // Constraints. Numbers are with reference to the constraints in
        // "Nmbr9 as a Constraint Programming Challenge"
        //

        // Outer columns and rows must be all zeroes
        for (int l = 0; l < nlevels_; ++l) {
            Matrix mboard(boards_[l], wh_, wh_);
            IntVarArgs first_column = mboard.col(0);
            IntVarArgs final_column = mboard.col(wh_-1);
            IntVarArgs first_row = mboard.row(0);
            IntVarArgs final_row = mboard.row(wh_-1);
            for (int i = 0; i < wh_; ++i) {
                rel(*this, first_column[i], IRT_EQ, 0);
                rel(*this, final_column[i], IRT_EQ, 0);
                rel(*this, first_row[i], IRT_EQ, 0);
                rel(*this, final_row[i], IRT_EQ, 0);
            }
        }


        // Deck is a shuffle and placements of parts
        //

        // (1) Deck is a shuffle
        IntVarArgs tile_is_used_as_int;
        for (int p = 0; p < nparts_; ++p) {
            tile_is_used_as_int << channel(*this, tile_is_used_[p]);
        }
        count(*this, deck_, tile_is_used_as_int, options.ipl());

        // (2) Placement constraints
        for (int p = 0; p < nparts_; ++p) {
            const TileSource &tile_source = nmbr9::tile(options.instance(), p+1);
            const REG &placement_expression = tile_source.as_placement_expression();
            // Make placement expression reified with an additional first control variable
            REG reified_placement =
                    (REG(1) + // Placement for control variable
                     placement_expression // Placement on board
                    ) |
                    (REG(0) + // No placement for control variable
                     REG(0)(nsquares_, nsquares_) // No placement on board
                    );
            for (int l = 0; l < nlevels_; ++l) {
                const IntVar tile_is_on_level = channel(*this, mtile_is_on_level(p, l));
                const IntVarArgs reified_tile_variables = tile_is_on_level + placement_boards_[l][p];
                extensional(*this, reified_tile_variables, reified_placement);
            }
        }


        // Basic channeling constraints
        //

        // (3) Deck to order
        IntVarArgs extended_deck;
        extended_deck << deck_;
        while (extended_deck.size() < nparts_) {
            extended_deck << IntVar(*this, 0, nparts_);
        }
        channel(*this, order_, extended_deck, options.ipl());

        // (4, pt1) Order to before
        for (int p1 = 0; p1 < nparts_; ++p1) {
            for (int p2 = 0; p2 < nparts_; ++p2) {
                if (p1 != p2) {
                    rel(*this, order_[p1], IRT_LE, order_[p2], Reify(mbefore(p1, p2)));
                }
            }
        }
        for (int p = 0; p < nparts_; ++p) {
            rel(*this, mbefore(p, p), IRT_EQ, 0);
        }

        // (4, pt2) Order to is placed
        for (int p = 0; p < nparts_; ++p) {
            rel(*this, order_[p], IRT_LE, ncards_, Reify(tile_is_used_[p]));
        }

        // (5) Level to is on is on level
        for (int p = 0; p < nparts_; ++p) {
            BoolVarArgs no_and_levels;
            no_and_levels << tile_is_not_used_[p];
            no_and_levels << mtile_is_on_level.col(p);
            channel(*this, no_and_levels, tile_level_[p]);
        }

        // (6) Y_p to N_p
        for (int p = 0; p < nparts_; ++p) {
            rel(*this, tile_is_used_[p], IRT_NQ, tile_is_not_used_[p]);
        }

        // (7) Aspects of placement boards
        for (int l = 0; l < nlevels_; ++l) {
            for (int p = 0; p < nparts_; ++p) {
                for (int s = 0; s < nsquares_; ++s) {
                    channel(*this,
                            BoolVarArgs{BoolVar(*this, 0, 1), part_boards_[l][p][s], around_boards_[l][p][s]},
                            placement_boards_[l][p][s]);
                }
            }
        }

        // (8) Placement boards connected to actual boards
        for (int l = 0; l < nlevels_; ++l) {
            for (int p = 0; p < nparts_; ++p) {
                for (int s = 0; s < nsquares_; ++s) {
                    rel(*this, boards_[l][s], IRT_EQ, p+1, Reify(part_boards_[l][p][s]));
                }
            }
        }

        // Connect value boards and actual board
        IntArgs values;
        values << 0;
        for (int p = 1; p < ncolors_; ++p) {
            values << nmbr9::tile(options.instance(), p).value();
        }
        for (int l = 0; l < nlevels_; ++l) {
            for (int s = 0; s < nsquares_; ++s) {
                element(*this, values, boards_[l][s], value_boards_[l][s]);
            }
        }


        // On-top and connectedness constraints
        //

        // (9) Connectedness constraints
        for (int p = 0; p < nparts_; ++p) {
            for (int l = 0; l < nlevels_; ++l) {
                // Guard (is on this level, is not first on level)
                BoolVar guard(*this, 0, 1);
                {
                    BoolVar not_first(*this, 0, 1);
                    BoolVarArgs is_before_and_on_level;

                    for (int p2 = 0; p2 < nparts_; ++p2) {
                        if (p2 != p) {
                            BoolVar p2_before_and_on_level(*this, 0, 1);
                            rel(*this, mbefore(p2, p), BOT_AND, mtile_is_on_level(p2, l), p2_before_and_on_level);
                            is_before_and_on_level << p2_before_and_on_level;
                        }
                    }

                    rel(*this, BOT_OR, is_before_and_on_level, not_first);
                    rel(*this, mtile_is_on_level(p, l), BOT_AND, not_first, guard);
                }

                // Requirement (connected to another part)
                BoolVar requirement(*this, 0, 1);
                {
                    BoolVarArgs connected_squares;
                    for (int s = 0; s < nsquares_; ++s) {
                        BoolVar before_part_placed_on_square(*this, 0, 1);
                        {
                            BoolVarArgs before_parts_placed_on_square;
                            for (int p2 = 0; p2 < nparts_; ++p2) {
                                if (p2 != p) {
                                    before_parts_placed_on_square
                                            << expr(*this, mbefore(p2, p) && part_boards_[l][p2][s]);
                                }
                            }
                            rel(*this, BOT_OR, before_parts_placed_on_square, before_part_placed_on_square);
                        }
                        BoolVar square_is_connected(*this, 0, 1);
                        rel(*this, before_part_placed_on_square, BOT_AND, around_boards_[l][p][s], square_is_connected);
                        connected_squares << square_is_connected;
                    }

                    rel(*this, BOT_OR, connected_squares, requirement);
                }
                // Actual constraint,
                //   if there is a part placed before the current part on this level and
                //   the current part is on this level,
                // then
                //   there must exist a square that is around the current part that is occupied by one of the
                //   parts before this part on this level
                rel(*this, guard >> requirement);
            }
        }

        // (10) On top requirements
        for (int l = 1; l < nlevels_; ++l) {
            for (int p = 0; p < nparts_; ++p) {
                for (int s = 0; s < nsquares_; ++s) {
                    BoolVarArgs all_before_part_squares;
                    for (int p2 = 0; p2 < nparts_; ++p2) {
                        if (p2 != p) {
                            BoolVar before_part_square(*this, 0, 1);
                            rel(*this, mbefore(p2, p), BOT_AND, part_boards_[l - 1][p2][s], before_part_square);
                            all_before_part_squares << before_part_square;
                        }
                    }
                    BoolVar before_part_underneath(*this, 0, 1);
                    rel(*this, BOT_OR, all_before_part_squares, before_part_underneath);
                    //rel(*this, part_boards_[l][p][s], BOT_IMP, before_part_underneath, 1);
                    rel(*this, part_boards_[l][p][s] >> before_part_underneath);
                }
            }
        }

        // (11) On top of at least two different parts
        for (int l = 1; l < nlevels_; ++l) {
            for (int p = 0; p < nparts_; ++p) {
                // Guard (is on this level)
                BoolVar guard = mtile_is_on_level(p, l);

                // Requirement (sum of part type underneath is at least 2)
                BoolVar requirement(*this, 0, 1);

                BoolVarArgs is_on_top_of_part;
                for (int p2 = 0; p2 < nparts_; ++p2) {
                    if (p2 != p) {
                        BoolVarArgs is_on_top_square;
                        for (int s = 0; s < nsquares_; ++s) {
                            is_on_top_square << expr(*this, part_boards_[l][p][s] && part_boards_[l-1][p2][s]);
                        }
                        BoolVar is_on_top(*this, 0, 1);
                        rel(*this, BOT_OR, is_on_top_square, is_on_top);
                        is_on_top_of_part << expr(*this, is_on_top && mbefore(p2, p));
                    }
                }
                linear(*this, is_on_top_of_part, IRT_GQ, 2, Reify(requirement));

                // Actual constraint,
                //   If the part is on this level
                // then
                //   the number of parts it rests upon must be at least 2
                rel(*this, guard, BOT_IMP, requirement, 1);
            }
        }


        // Implied constraints
        //

        // Each level needs two cards before the next level can be filled
        for (int i = 0; i < ncards_; ++i) {
            int max_level = (int) ceil(((double) i+1) / 2);
            rel(*this, element(tile_level_, deck_[i]) <= max_level);
        }

        // Each layer must have at most the are of the previous layer
        IntArgs tile_area;
        for (int p = 0; p < nparts_; ++p) {
            tile_area << nmbr9::tile(instance_, p+1).value();
        }
        IntVarArgs level_areas;
        for (int l = 0; l < nlevels_; ++l) {
            IntVar level_area(*this, 0, wh_*wh_);
            linear(*this, tile_area, mtile_is_on_level.row(l), IRT_EQ, level_area);

            level_areas << level_area;
        }
        for (int i = 0; i < level_areas.size() - 1; ++i) {
            rel(*this, level_areas[i], IRT_GQ, level_areas[i+1]);
        }


        // Symmetry breaking constraints
        //

        // Copies of the same value can be fixed order in the deck
        int copies = (int) options.copies();
        for (int p = 0; p < nparts_; p += copies) {
            IntArgs same_values;
            for (int c = 0; c < copies; ++c) {
                same_values << (p + c);
            }
            precede(*this, deck_, same_values);
        }

        // Rotational symmetry on the base grid

        // Type for tile vector<int> symmetry functions
        typedef void (*varsymmfunc)(const IntVarArgs &, int, int, IntVarArgs &, int &, int &);
        const std::vector<varsymmfunc> symmetries {
                symmetry::rot90, symmetry::rot180, symmetry::rot270
        };
        for (const auto& symmetry : symmetries) {
            IntVarArgs rotated_grid(nsquares_);
            int gs = (int) options.grid_size();
            symmetry(boards_[0], gs, gs, rotated_grid, gs, gs);
            rel(*this, boards_[0], IRT_GQ, rotated_grid);
        }

        if (options.use_deck_level_symmetry()) {
            // Symmetry breaking suggested by Ciaran McCreesh at CP2019
            // When solving the free play type, we can decide that all the tiles on the bottom level are first in the deck.
            // Unfortunately, this did not seem to help, even though it should.
            if (options.play_type() == PT_FREE) {
                for (int c = 0; c < ncards_ - 1; ++c) {
                    rel(*this, element(tile_level_, deck_[c]) <= element(tile_level_, deck_[c + 1]));
                }
            }
        }

        
        // Calculate the score of the solution
        //

        // (12) Score summation
        IntVarArgs tile_score_levels;
        for (int p = 0; p < nparts_; ++p) {
            tile_score_levels << expr(*this, tile_level_[p] - tile_is_used_as_int[p]);
        }
        linear(*this, tile_value_, tile_score_levels, IRT_EQ, score_);


        // Set up heuristics
        //

        if (options.play_type() == PT_KNOWN) {
            // A known deck is simulated by a random assignment.
            // Adjust the seed parameter to get different instances.
            assign(*this, deck_, INT_ASSIGN_RND(Rnd(options.seed())));
        }

        // Use spiral pattern to get placements close to the center.
        IntVarArgs all_levels_bottom_to_top;
        for (int l = 0; l < nlevels_; ++l) {
            all_levels_bottom_to_top << anti_spiral(IntVarArgs(boards_[l]), wh_);
        }

        // First, decide the cards and their order in the deck
        branch(*this, deck_, INT_VAR_NONE(), INT_VAL_MIN());
//        branch(*this, IntVarArgs(deck_.rbegin(), deck_.rend()), INT_VAR_NONE(), INT_VAL_MAX());

        // Then, decide the level for the different cards.
        // This uniquely determines the score.
        branch(*this, tile_level_, INT_VAR_NONE(), INT_VAL_MAX());

        // Find placements for the parts
        branch(*this, all_levels_bottom_to_top, INT_VAR_NONE(),
                INT_VAL([](const Space& home, IntVar x, int i){
                    // Choose the minimum part value. That is, a value that is not empty (0).
                    if (x.min() > 0) {
                        return x.min();
                    }
                    IntVarValues values(x);
                    ++values;
                    return values.val();
                }));

        // Assign the order variables (is the deck does not contain all parts,
        // some are left undetermined by above branchings).
        assign(*this, order_, INT_ASSIGN_MIN());
    }


    Nmbr9Board::Nmbr9Board(Nmbr9Board &s) :
            IntMaximizeScript(s), instance_(s.instance_), wh_(s.wh_), nlevels_(s.nlevels_),
            nparts_(s.nparts_), ncolors_(s.ncolors_), ncards_(s.ncards_), nsquares_(s.nsquares_),
            empty_color_(s.empty_color_),
            boards_(nlevels_, IntVarArray()),
            value_boards_(nlevels_, IntVarArray()),
            tile_value_(s.tile_value_),
            placement_boards_(nlevels_, std::vector(nparts_, IntVarArray())),
            part_boards_(nlevels_, std::vector(nparts_, BoolVarArray())),
            around_boards_(nlevels_, std::vector(nparts_, BoolVarArray()))

    {
        for (int l = 0; l < nlevels_; ++l) {
            boards_[l].update(*this, s.boards_[l]);
            value_boards_[l].update(*this, s.value_boards_[l]);
            for (int p = 0; p < nparts_; ++p) {
                placement_boards_[l][p].update(*this, s.placement_boards_[l][p]);
                part_boards_[l][p].update(*this, s.part_boards_[l][p]);
                around_boards_[l][p].update(*this, s.around_boards_[l][p]);
            }
        }

        tile_is_used_.update(*this, s.tile_is_used_);
        tile_is_not_used_.update(*this, s.tile_is_not_used_);
        tile_is_on_level_.update(*this, s.tile_is_on_level_);
        tile_level_.update(*this, s.tile_level_);
        before_.update(*this, s.before_);
        deck_.update(*this, s.deck_);
        order_.update(*this, s.order_);
        score_.update(*this, s.score_);
    }

    Nmbr9Board *Nmbr9Board::copy() {
        return new Nmbr9Board(*this);
    }


#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCSimplifyInspection"
    void Nmbr9Board::print(std::ostream &os) const {
        const bool PRINT_PARTS = false;

        for (int l = 0; l < nlevels_; ++l) {
            os << "Level " << l << std::endl;
            for (int h = 0; h < wh_; ++h) {
                os << "\t";
                for (int w = 0; w < wh_; ++w) {
                    print_square(os, boards_[l][h * wh_ + w]);
                }
                if (PRINT_PARTS) {
                    for (int p = 0; p < nparts_; ++p) {
                        os << "  |  ";
                        for (int w = 0; w < wh_; ++w) {
                            print_square_part_board(os, part_boards_[l][p][h * wh_ + w]);
                        }
                    }
                }
                os << std::endl;
            }
            os << std::endl;
        }
        os << "Deck used :  {";// << deck_ << std::endl;
        for (int i = 0; i < deck_.size(); ++i) {
            os << std::setw(2) << deck_[i];
            if (i < deck_.size()-1) {
                os << ", ";
            }
        }
        os << "}" << std::endl;
        os << "Part values: {";
        for (int i = 0; i < deck_.size(); ++i) {
            if (deck_[i].assigned()) {
                os << std::setw(2) << nmbr9::tile(instance_, deck_[i].val()+1).value();
            } else {
                os << " ?";
            }
            if (i < deck_.size()-1) {
                os << ", ";
            }
        }
        os << "}" << std::endl;
        os << "Order of parts: " << order_ << std::endl;
        os << "Tiles used : " << tile_is_used_ << std::endl;
        os << "Tiles levels : " << tile_level_ << std::endl;
        os << "Score : " << score_ << std::endl;
    }
#pragma clang diagnostic pop

    IntVar Nmbr9Board::cost() const {
        return score_;
    }

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
    void print_square_part_board(std::ostream &os, const IntVar &square) {
        if (square.assigned()) {
            switch (square.val()) {
                case 0:
                    os << " ";
                    break;
                case 1:
                    os << "X";
                    break;
                case 2:
                    os << "-";
                    break;
            }
        } else {
            if (square.min() > 0) {
                os << "?";
            } else {
                os << "·";
            }
        }
    }
#pragma clang diagnostic pop

    void print_square_part_board(std::ostream &os, const BoolVar &square) {
        if (square.assigned()) {
            switch (square.val()) {
                case 0:
                    os << " ";
                    break;
                case 1:
                    os << "X";
                    break;
                case 2:
                    os << "-";
                    break;
            }
        } else {
            if (square.min() > 0) {
                os << "?";
            } else {
                os << "·";
            }
        }
    }

    void print_square(std::ostream &os, const IntVar &square) {
        if (square.assigned()) {
            int val = square.val();
            if (val > 0) {
                --val;
                char c = static_cast<char>(val < 10 ? '0' + val : 'A' + (val - 10));
                os << c;
            } else {
                os << '_';
            }
        } else {
            os << "·";
        }
    }


}
