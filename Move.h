#ifndef LIBCHESS_MOVE_H
#define LIBCHESS_MOVE_H

#include <algorithm>

#include "PieceType.h"
#include "Square.h"

namespace libchess {

class Move {
  private:
    enum BitOpLookup : std::uint32_t {
        TO_SQUARE_SHIFT = 6,
        PROMOTION_TYPE_SHIFT = 12,
        MOVE_TYPE_SHIFT = 15,

        PROMOTION_TYPE_MASK = 7 << PROMOTION_TYPE_SHIFT,
        MOVE_TYPE_MASK = 7 << MOVE_TYPE_SHIFT
    };

  public:
    class Value {
      public:
        enum MoveValue : std::uint32_t { MOVE_NONE };
    };
    enum class Type : std::uint8_t {
        NORMAL,
        CASTLING,
        ENPASSANT,
        PROMOTION,
        DOUBLE_PUSH,
        CAPTURE,
        CAPTURE_PROMOTION,
        NONE
    };

    using value_type = std::uint32_t;

    constexpr inline Move() : value_(static_cast<value_type>(Move::Value::MOVE_NONE)) {}
    constexpr inline Move(std::uint32_t value) : value_(value) {}
    constexpr inline Move(Square from_square, Square to_square,
                          PieceType promotion_pt = constants::PIECE_TYPE_NONE,
                          Move::Type type = Move::Type::NONE)
        : value_(from_square.value() | (to_square.value() << TO_SQUARE_SHIFT) |
                 (promotion_pt.value() << PROMOTION_TYPE_SHIFT) |
                 (std::uint32_t(type) << MOVE_TYPE_SHIFT)) {}
    inline Move(const std::string& move_str) : value_(from(move_str).value_) {}
    inline Move(const char* move_str) : value_(from(move_str).value_) {}

    static inline Move from(const std::string& str) {
        Square from = Square::from(str.substr(0, 2));
        Square to = Square::from(str.substr(2, 4));
        PieceType promotion_pt = constants::PIECE_TYPE_NONE;
        if (str.size() == 5) {
            promotion_pt = PieceType::from(str[4]);
        }
        return Move{from, to, promotion_pt};
    }

    constexpr inline bool operator==(const Move rhs) const {
        return value_sans_type() == rhs.value_sans_type();
    }
    constexpr inline bool operator!=(const Move rhs) const {
        return value_sans_type() != rhs.value_sans_type();
    }

    constexpr inline Square from_square() const { return value() & 0x3f; }
    constexpr inline Square to_square() const { return (value() & 0xfc0) >> 6; }
    constexpr inline Move::Type type() const {
        return static_cast<Move::Type>((value() & MOVE_TYPE_MASK) >> MOVE_TYPE_SHIFT);
    }
    constexpr inline PieceType promotion_piece_type() const {
        return (value() & PROMOTION_TYPE_MASK) >> PROMOTION_TYPE_SHIFT;
    }

    constexpr inline value_type value_sans_type() const { return value_ & ~MOVE_TYPE_MASK; }
    constexpr inline value_type value() const { return value_; }

  private:
    value_type value_;
};

class MoveList {
  protected:
    constexpr static int max_size = 256;

  public:
    using value_type = std::array<Move, max_size>;
    using iterator = value_type::iterator;
    using const_iterator = value_type::const_iterator;

    constexpr inline MoveList() : size_(0) {}

    constexpr inline iterator begin() { return values_.begin(); }
    constexpr inline iterator end() { return values_.begin() + size(); }
    constexpr inline const_iterator cbegin() const { return values_.begin(); }
    constexpr inline const_iterator cend() const { return values_.begin() + size(); }

    constexpr inline void decrement_size() { --size_; }
    constexpr inline void add(Move move) {
        values_[size()] = move;
        ++size_;
    }
    template <class F> constexpr inline void sort(F move_evaluator) {
        auto& moves = values_mut_ref();
        std::array<int, max_size> scores{};
        for (int i = 0; i < size(); ++i) {
            scores[i] = move_evaluator(moves[i]);
        }
        for (int i = 1; i < size(); ++i) {
            Move moving_move = moves[i];
            int moving_score = scores[i];
            int j = i;
            for (; j > 0; --j) {
                if (scores[j - 1] < moving_score) {
                    scores[j] = scores[j - 1];
                    moves[j] = moves[j - 1];
                } else {
                    break;
                }
            }
            scores[j] = moving_score;
            moves[j] = moving_move;
        }
    }
    constexpr inline int size() const { return size_; }
    constexpr inline const std::array<Move, max_size>& values() const { return values_; }

  protected:
    constexpr inline std::array<Move, max_size>& values_mut_ref() { return values_; }

  private:
    int size_;
    std::array<Move, max_size> values_;
};

inline std::ostream& operator<<(std::ostream& ostream, Move move) {
    ostream << move.from_square() << move.to_square();
    if (move.promotion_piece_type() != constants::PIECE_TYPE_NONE) {
        ostream << move.promotion_piece_type();
    }
    return ostream;
}

namespace constants {

constexpr static Move::Type MOVE_TYPE_NONE = Move::Type::NONE;
constexpr static Move MOVE_NONE{};

} // namespace constants

} // namespace libchess

namespace std {

template <> struct hash<libchess::Move> : public hash<libchess::Move::value_type> {};

} // namespace std

#endif // LIBCHESS_MOVE_H
