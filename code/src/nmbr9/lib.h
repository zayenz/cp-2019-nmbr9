//
// Created by Mikael Zayenz Lagerkvist
//

#ifndef NMBR9_LIB_H
#define NMBR9_LIB_H

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"

#include "tiles.h"
#include "base.h"

#include <gecode/driver.hh>
#include <gecode/int.hh>

#include <vector>
#include <cassert>
#include <optional>

namespace nmbr9 {
        class Nmbr9Options : public Gecode::Options {
        Gecode::Driver::StringOption play_type_;
        Gecode::Driver::UnsignedIntOption max_value_;
        Gecode::Driver::UnsignedIntOption copies_;
        Gecode::Driver::UnsignedIntOption deck_size_;
        unsigned int number_of_parts_;

        Gecode::Driver::UnsignedIntOption grid_size_;
        Gecode::Driver::UnsignedIntOption max_layers_;

        Gecode::Driver::BoolOption use_deck_level_symmetry_;
    public:
        Nmbr9Options()
        : Options("Nmbr9"),
          play_type_("play-type", "play type to use, default is free", PT_FREE),
          max_value_("max-value", "maximum value to use for parts in 0-9, default 9", 9),
          copies_("copies", "number of copies of each part, default 2", 2),
          deck_size_("deck-size", "the number of cards in the deck, max max-value * copies, default 20", 20),
          number_of_parts_(20),
          grid_size_("grid-size", "the size of the grid, default 20", 20),
          max_layers_("max-layers", "the maximum layer to use, default 7", 7),
          use_deck_level_symmetry_("deck-level-symmetry",
                  "When true and in free play type, force the levels of cards in deck to be ordered.", false)
        {
            add(play_type_);
            add(max_value_);
            add(copies_);
            add(deck_size_);

            add(grid_size_);
            add(max_layers_);

            add(use_deck_level_symmetry_);

            play_type_.add(PT_FREE, "free");
            play_type_.add(PT_KNOWN, "known");
        }


        void parse(int& argc, char* argv[]) {
            Options::parse(argc, argv);
            if (max_value_.value() > 9) {
                std::cerr << "max-value can be at most 9, " << max_value_.value()
                          << " supplied." << std::endl;
                std::exit(EXIT_FAILURE);
            }
            number_of_parts_ = (max_value_.value()+1) * copies_.value();
            if (number_of_parts_ < deck_size_.value()) {
                std::cerr << "deck-size can be at most max-value*copies (("
                          << max_value_.value() << "+1)*" << copies_.value() << "="
                          << ((max_value_.value()+1) * copies_.value()) << "), "
                          << deck_size_.value() << " supplied." << std::endl;
                std::exit(EXIT_FAILURE);
            }
        }

        [[nodiscard]] PlayType play_type() const {
            return static_cast<const PlayType>(play_type_.value());
        }

        [[nodiscard]] int max_value() const {
            return static_cast<int>(max_value_.value());
        }

        [[nodiscard]] int copies() const {
            return static_cast<int>(copies_.value());
        }

        [[nodiscard]] int deck_size() const {
            return static_cast<int>(deck_size_.value());
        }

        [[nodiscard]] int number_of_parts() const {
            return static_cast<int>(number_of_parts_);
        }

        [[nodiscard]] int grid_size() const {
            return static_cast<int>(grid_size_.value());
        }

        [[nodiscard]] int max_layers() const {
            return static_cast<int>(max_layers_.value());
        }

        [[nodiscard]] bool use_deck_level_symmetry() const {
            return static_cast<int>(use_deck_level_symmetry_.value());
        }


        [[nodiscard]] Instance instance() const {
            return Instance(play_type(), max_value(), copies(), deck_size(), grid_size());
        }
    };

    class Nmbr9Board : public Gecode::IntMaximizeScript {
    private:
        /// The current instance used
        const Instance instance_;

        /// Size (width/height) of the board (wh_ is s)
        const int wh_;
        /// Number of board levels (nlevels_ is l_\top)
        const int nlevels_;
        /// Number of tiles that can be placed (nparts_ is n)
        const int nparts_;
        /// Number of colors for board squares
        const int ncolors_;
        /// Number of cards in the deck (ncards_ is k)
        const int ncards_;
        /// Number of board squares, wh_*wh_
        const int nsquares_;
        /// The value for empty squares
        const int empty_color_;

        /// The variables for the board. (boards[l] is G_l)
        std::vector<Gecode::IntVarArray> boards_;

        /// The values for variables for the board.
        std::vector<Gecode::IntVarArray> value_boards_;

        /// The variables representing the chosen tiles. (tile_is_used_[p] is Y_p)
        Gecode::BoolVarArray tile_is_used_;

        /// The variables representing the chosen tiles. (tile_is_not_used_[p] is N_p)
        Gecode::BoolVarArray tile_is_not_used_;

        /// The variables representing is a tile is on . (matrix(tile_is_on_level_, nparts_, nlevels_)(p, l) is L_pl)
        Gecode::BoolVarArray tile_is_on_level_;


        /// Tile value (tile_value_[p] is v(p))
        Gecode::IntSharedArray tile_value_;

        /// Tile level (tile_level_[p] is L_p)
        Gecode::IntVarArray tile_level_;

        /// Variables representing placement and surrounding area of different tiles. (placement_boards_[l][p] is G_pl)
        std::vector<std::vector<Gecode::IntVarArray>> placement_boards_;

        /// Boolean variables representing placement of different tiles. (placement_boards_[l][p] is G_pl^1)
        std::vector<std::vector<Gecode::BoolVarArray>> part_boards_;

        /// Boolean variables representing surrounding area of different tiles. (placement_boards_[l][p] is G_pl^2)
        std::vector<std::vector<Gecode::BoolVarArray>> around_boards_;

        /// Boolean variables representing the before relation (matrix(before_, nparts_, nparts_)(p1, p2) is B(p1, p2))
        Gecode::BoolVarArray before_;

        /// The deck of cards. (deck_[i] is D(i))
        Gecode::IntVarArray deck_;

        /// The order of parts in the deck, or >= ncards_ if the part is not used (order_[p] is O(p)).
        Gecode::IntVarArray order_;

        /// The score of the solution (score_ is S)
        Gecode::IntVar score_;

    public:
        /// Construction of the model.
        explicit Nmbr9Board(const Nmbr9Options& opts);

        /// Constructor for cloning \a s
        Nmbr9Board(Nmbr9Board &s);

        /// Copy space during cloning
        Nmbr9Board *copy() override;

        /// Print solution
        void print(std::ostream &os) const override;

        Gecode::IntVar cost() const override;
    };

    /**
     * Prints a variable form one of the part boards as a square
     *
     * @param os The stream to print to
     * @param square The variable representing the square to print
     */
    void print_square_part_board(std::ostream &os, const Gecode::IntVar &square);

    /**
     * Prints a variable form one of the part boards as a square
     *
     * @param os The stream to print to
     * @param square The variable representing the square to print
     */
    void print_square_part_board(std::ostream &os, const Gecode::BoolVar &square);

    /**
     * Prints a variable as a square
     *
     * @param os The stream to print to
     * @param square The variable representing the square to print
     */
    void print_square(std::ostream &os, const Gecode::IntVar &square);
}

#pragma clang diagnostic pop

#endif //NMBR9_LIB_H

