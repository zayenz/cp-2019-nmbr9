//
// Created by Mikael Zayenz Lagerkvist
//

#ifndef NMBR9_BASE_H
#define NMBR9_BASE_H

namespace nmbr9 {
    typedef enum {
        PT_UNKNOWN,
        PT_FREE,
        PT_KNOWN,
    } PlayType;

    /**
     * Unique representation of a a problem instance, including the size of the grid
     * since that affects the placement expressions produced.
     */
    class Instance {
        const PlayType play_type_;
        const int max_value_;
        const int copies_;
        const int deck_size_;
        const int number_of_parts_;
        const int wh_;
    public:
        Instance(PlayType play_type, int max_value, int copies, int deck_size, int wh);
        const PlayType play_type() const;
        const int max_value() const;
        const int copies() const;
        const int deck_size() const;
        const int number_of_parts() const;
        const int wh() const;
        bool operator==(const Instance &rhs) const;
        bool operator!=(const Instance &rhs) const;
        bool operator<(const Instance &rhs) const;
        bool operator>(const Instance &rhs) const;
        bool operator<=(const Instance &rhs) const;
        bool operator>=(const Instance &rhs) const;
    };
}

#endif //NMBR9_BASE_H
