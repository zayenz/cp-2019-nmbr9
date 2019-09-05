//
// Created by Mikael Zayenz Lagerkvist
//

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedGlobalDeclarationInspection"
#ifndef NMBR9_TILES_H
#define NMBR9_TILES_H

#include <vector>
#include <gecode/minimodel.hh>
#include "base.h"

namespace nmbr9 {

    /** \brief Specification of one tile
     *
     * This structure can be used to specify a tile with specified width
     * and height, number of such tiles (all with unique values), and a
     * char-array tile showing the tile in row-major order.x
     *
     * \relates Nmbr9Model
     */
    class Tile {
        const int width;  ///< Width of tile
        const int height; ///< Height of tile
        const std::vector<int> marks; ///< The row-major marks of the tile in a minimum bounding box
    public:
        Tile(int width, int height, std::vector<int> marks);

        const Gecode::REG make_placement_expression(Instance instance) const;

        inline int operator()(int x, int y) const {
            return at(x, y);
        }

        /**
         *
         * @param x The x-coordinate (the column)
         * @param y The y-coordinate (the row)
         * @return The value at (x, y)
         */
        inline int at(int x, int y) const {
            assert(0 <= x && x < width);
            assert(0 <= y && y < height);
            return marks[y * width + x];
        }

        inline bool operator==(const Tile &rhs) const noexcept {
            if (width != rhs.width || height != rhs.height) {
                return false;
            }
            for (int i = 0; i < marks.size(); ++i) {
                if (marks[i] != rhs.marks[i]) {
                    return false;
                }
            }
            return true;
        }

        inline bool operator!=(const Tile &rhs) const noexcept {
            return !(*this == rhs);
        }
    };


    /** \brief Specification of one tile
     *
     * This structure can be used to specify a tile with specified width
     * and height, number of such tiles (all with unique values), and a
     * char-array tile showing the tile in row-major order.
     *
     * \relates Nmbr9Model
     */
    class TileSource {
        const int id_; ///< The id for this source
        const int value_; ///< Number of copies of the tile
        const int area_; ///< The area occupied by the tile
        const std::vector<Tile> alternatives_; ///< The tile alternatives (id, rot90, flip vertical, ...)
        const Gecode::REG placement_expression_; ///< A placement expression for placing on a 9x9 grid of Boolean variables with 1 eol column
    public:
        /**
         *
         * @param width The width of the tile pattern
         * @param height The height of the tile pattern
         * @param amount The number of copies of the tile
         * @param button_cost The button cost for the tile
         * @param time_cost The time cost for the tile
         * @param buttons The number of buttons of income for the tile
         * @param tile_pattern A row-major tile pattern
         */
        TileSource(Instance instance, int id, int width, int height, int value, const char *tile_pattern);

        /**
         *
         * @return A placement expression for the tile on a 9x9 grid of Boolean variables with 1 eol column.
         */
        const Gecode::REG as_placement_expression() const;

        const int id() const;

        const int value() const;

        const int area() const;
    private:
        /**
         *
         * @param width The width of the tile pattern
         * @param height The height of the tile pattern
         * @param tile_pattern A row-major tile pattern
         * @return A vector of all the uniquely transformed tiles
         */
        static std::vector<Tile> make_unique_tiles(int width, int height, const char *tile_pattern);

        /**
         *
         * @param width The width of the tile pattern
         * @param height The height of the tile pattern
         * @param tile_pattern A row-major tile pattern
         * @return The area occupied by the tile
         */
        static const int count_area(int width, int height, const char *pattern);

        /**
         * Create the expression for placing this tile, with the alternative index as the first 8 variables.
         *
         * @param alternatives The alternatives to make the expression over.
         * @return A placement expression on a 9x9 grid with 1 eol column, preceeded by the alternative index
         */
        static const Gecode::REG make_placement_expression(Instance instance, const std::vector<Tile>& alternatives);

        static int pattern_square_value(char i1);
    };


    /** \brief Abstract specification of one tile source
     *
     * This structure can be used to specify a tile source with specified width
     * and height, a value, and a char-array tile showing the tile in row-major order.
     */
    class AbstractTileSource {
        const int id_; ///< The id for this source
        const int width_; ///< The width of this tile
        const int height_; ///< The height of this tile
        const int value_; ///< The value of this tile
        const char *tile_pattern_; ///< The pattern for this tile
    public:
        /**
         *
         * @param width The width of the tile pattern
         * @param height The height of the tile pattern
         * @param amount The number of copies of the tile
         * @param button_cost The button cost for the tile
         * @param time_cost The time cost for the tile
         * @param buttons The number of buttons of income for the tile
         * @param tile_pattern A row-major tile pattern
         */
        AbstractTileSource(int id, int width, int height, int value, const char *tile_pattern)
                : id_(id),
                width_(width),
                height_(height),
                value_(value),
                tile_pattern_(tile_pattern)
        {}

        TileSource as_tile_source(Instance instance) const {
            return TileSource {
                instance,
                id_,
                width_,
                height_,
                value_,
                tile_pattern_
            };
        }

        const int value() const {
            return value_;
        }
    };

    /**
     * All the tiles in Nmbr9
     */
    extern const std::vector<AbstractTileSource> base_tiles;

    /**
     *
     * @return The number of unique tiles
     */
    int tile_count(nmbr9::Instance instance);

    /**
     *
     * @return The number of unique tiles that are choosable (that is, not including the 1x1 tiles)
     */
    int tile_count_choosable();

    /**
     * Guarantees that that the tile in postion 0 is the start tile, and the rest are all tiles to choose from
     * and not the 1x1 bonus tiles.
     *
     * @return The unique tiles that are choosable (that is, not including the 1x1 tiles)
     */
    const std::vector<int>& choosable_tiles();

    /**
     * Only the bonus tiles.
     *
     * @return The unique bonus/single tiles.
     */
    const std::vector<int>& single_tiles();

    /**
     *
     * @param tile The tile index to get. Must be between 0 and tile_count()
     * @return The source for tile number \a tile
     */
    TileSource tile(Instance instance, int tile);
}

#endif //NMBR9_TILES_H

#pragma clang diagnostic pop