//
// Created by Mikael Zayenz Lagerkvist
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#pragma ide diagnostic ignored "misc-static-assert"

#include <map>
#include <iostream>
#include <gecode/minimodel.hh>
#include <gecode/int.hh>
#include <gecode/driver.hh>
#include <utility>

#include "lib.h"
#include "tiles.h"
#include "symmetry.h"
#include "base.h"

namespace nmbr9 {
    Tile::Tile(int width, int height, std::vector<int> marks) : width(width), height(height), marks(std::move(marks)) {
        assert(this->marks.size() == width * height);
    }

    const Gecode::REG Tile::make_placement_expression(Instance instance) const {
        using Gecode::REG;
        REG around(2);
        REG mark(1);
        REG empty(0);

        // The fixed separation between rows
        const auto fixed_separation_length = instance.wh() - width;
        const REG fixed_separation = empty(fixed_separation_length, fixed_separation_length);

        // Start anywhere (that is, with arbitrary number of empty squares)
        REG result = *empty;

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                switch (at(x, y)) {
                    case 0:
                        result += empty;
                        break;
                    case 1:
                        result += mark;
                        break;
                    case 2:
                        result += around;
                        break;
                    default:
                        GECODE_NEVER
                }
            }
            if (y < height - 1) {
                // Between rows, add the fixed separation
                result += fixed_separation;
            }
        }

        result += *empty;

        return result;
    }


    TileSource::TileSource(Instance instance,
                           const int id,
                           const int width, const int height, const int value,
                           const char *tile_pattern)
            : id_(id),
              value_(value),
              area_(count_area(width, height, tile_pattern)),
              alternatives_(make_unique_tiles(width, height, tile_pattern)),
              placement_expression_(make_placement_expression(instance, alternatives_))
              {}

    std::vector<Tile> TileSource::make_unique_tiles(const int width, const int height, const char *tile_pattern) {
        assert(width * height == strlen(tile_pattern));


        const std::vector<symmetry::vsymmfunc> symmetries {
            symmetry::id, symmetry::rot90, symmetry::rot180, symmetry::rot270
        };

        std::vector<int> base_tile;
        base_tile.reserve(width * height);
        for (int i = 0; i < width * height; ++i) {
            base_tile.emplace_back(pattern_square_value(tile_pattern[i]));
        }

        std::vector<Tile> result;
        result.reserve(symmetries.size());

        for (const auto &symmetry : symmetries) {
            std::vector<int> transformed(width * height);
            int transformed_width, transformed_height;
            symmetry(base_tile, width, height,
                    transformed, transformed_width, transformed_height);

            Tile tile(transformed_width, transformed_height, transformed);

            bool unique = true;
            for (const auto &alternative : result) {
                if (tile == alternative) {
                    unique = false;
                    break;
                }
            }
            if (unique) {
                result.emplace_back(tile);
            }
        }

        return result;
    }

    int TileSource::pattern_square_value(char mark) {
        switch (mark) {
            case ' ':
                return 0;
            case 'X':
                return 1;
            case '.':
                return 2;
            default:
                GECODE_NEVER
                return -1;
        }
    }

    const int TileSource::count_area(const int width, const int height, const char *pattern) {
        int result = 0;
        for (int i = 0; i < width * height; ++i) {
            if (pattern[i] == 'X') {
                ++result;
            }
        }
        return result;
    }



    const Gecode::REG TileSource::make_placement_expression(Instance instance, const std::vector<Tile>& alternatives) {
        using Gecode::REG;

        REG result = alternatives[0].make_placement_expression(instance);

        for (int i = 1; i < alternatives.size(); ++i) {
            REG placement_expression = alternatives[i].make_placement_expression(instance);
            result = result | placement_expression;
        }
        
        return result;
    }

    
    const Gecode::REG TileSource::as_placement_expression() const {
        return placement_expression_;
    }

    const int TileSource::id() const {
        return id_;
    }

    const int TileSource::value() const {
        return value_;
    }

    const int TileSource::area() const {
        return area_;
    }


    const std::vector<AbstractTileSource> base_tiles = { // NOLINT(cert-err58-cpp)
            {0,
             5, 6, 0,
             " ... "
             ".XXX."
             ".X X."
             ".X X."
             ".XXX."
             " ... "
            },
            {1,
             4, 6, 1,
             " .. "
             ".XX."
             " .X."
             " .X."
             " .X."
             "  . "
            },
            {2,
             5, 6, 2,
             "  .. "
             " .XX."
             " .XX."
             ".XX. "
             ".XXX."
             " ... "
            },
            {3,
             5, 6, 3,
             " ... "
             ".XXX."
             " ..X."
             " .XX."
             ".XXX."
             " ... "
            },
            {4,
             5, 6, 4,
             "  .. "
             " .XX."
             " .X. "
             ".XXX."
             " .XX."
             "  .. "
            },
            {5,
             5, 6, 5,
             " ... "
             ".XXX."
             ".XXX."
             " ..X."
             ".XXX."
             " ... "
            },
            {6,
             5, 6, 6,
             " ..  "
             ".XX. "
             ".X.  "
             ".XXX."
             ".XXX."
             " ... "
            },
            {7,
             5, 6, 7,
             " ... "
             ".XXX."
             " .X. "
             ".XX. "
             ".X.  "
             " .   "
            },
            {8,
             5, 6, 8,
             "  .. "
             " .XX."
             " .XX."
             ".XX. "
             ".XX. "
             " ..  "
            },
            {9,
             5, 6, 9,
             " ... "
             ".XXX."
             ".XXX."
             ".XX. "
             ".XX. "
             " ..  "
            },

    };

    class TileSources
    {
    private:
        std::map<Instance, const std::vector<TileSource>> sources_;
    public:
        static TileSources& instance()
        {
            static TileSources instance; // Guaranteed to be destroyed.
                                         // Instantiated on first use.
            return instance;
        }
    private:
        static const std::vector<TileSource> collect_sources(Instance instance) noexcept {
            std::vector<TileSource> result;
            for (const auto &abstract_tile : base_tiles) {
                if (abstract_tile.value() <= instance.max_value()) {
                    TileSource tile = abstract_tile.as_tile_source(instance);
                    for (int i = 0; i < instance.copies(); ++i) {
                        result.emplace_back(tile);
                    }
                }
            }
            assert(instance.number_of_parts() == result.size());
            return result;
        }

        void ensure_exists(const Instance &instance) {
            if (sources_.count(instance) == 0) {
                sources_.insert(std::make_pair(instance, collect_sources(instance)));
            }
        }


        TileSources() :
                sources_()
        {
        }
        friend TileSource tile(Instance instance, int tile);
    public:
        TileSources(TileSources const&) = delete;
        void operator=(TileSources const&)  = delete;
    };

    TileSource tile(Instance instance, int tile) {
        TileSources::instance().ensure_exists(instance);
        assert(0 < tile && tile <= instance.number_of_parts());
        return TileSources::instance().sources_[instance][tile-1];
    }

}

#pragma clang diagnostic pop