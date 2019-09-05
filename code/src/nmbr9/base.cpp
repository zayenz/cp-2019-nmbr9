//
// Created by Mikael Zayenz Lagerkvist
//

#include "base.h"

namespace nmbr9 {

    Instance::Instance(const PlayType play_type, const int max_value, const int copies, const int deck_size, const int wh)
            : play_type_(play_type),
              max_value_(max_value),
              copies_(copies),
              deck_size_(deck_size),
              number_of_parts_((max_value_ + 1) * copies_),
              wh_(wh) {}

    bool Instance::operator==(const Instance &rhs) const {
        return play_type_ == rhs.play_type_ &&
               max_value_ == rhs.max_value_ &&
               copies_ == rhs.copies_ &&
               deck_size_ == rhs.deck_size_ &&
               wh_ == rhs.wh_;
    }

    bool Instance::operator!=(const Instance &rhs) const {
        return !(rhs == *this);
    }

    bool Instance::operator<(const Instance &rhs) const {
        if (play_type_ < rhs.play_type_)
            return true;
        if (rhs.play_type_ < play_type_)
            return false;
        if (max_value_ < rhs.max_value_)
            return true;
        if (rhs.max_value_ < max_value_)
            return false;
        if (copies_ < rhs.copies_)
            return true;
        if (rhs.copies_ < copies_)
            return false;
        if (rhs.deck_size_ < deck_size_)
            return false;
        return wh_ < rhs.wh_;
    }

    bool Instance::operator>(const Instance &rhs) const {
        return rhs < *this;
    }

    bool Instance::operator<=(const Instance &rhs) const {
        return !(rhs < *this);
    }

    bool Instance::operator>=(const Instance &rhs) const {
        return !(*this < rhs);
    }

    const PlayType Instance::play_type() const {
        return play_type_;
    }

    const int Instance::max_value() const {
        return max_value_;
    }

    const int Instance::copies() const {
        return copies_;
    }

    const int Instance::deck_size() const {
        return deck_size_;
    }

    const int Instance::number_of_parts() const {
        return number_of_parts_;
    }

    const int Instance::wh() const {
        return wh_;
    }

}