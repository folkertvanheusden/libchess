#ifndef LIBCHESS_PIECETYPE_H
#define LIBCHESS_PIECETYPE_H

#include "internal/MetaValueType.h"

namespace libchess {

class PieceType : public MetaValueType<int> {
  public:
    class Value {
      public:
        enum PieceTypeValue : value_type {
            PAWN,
            KNIGHT,
            BISHOP,
            ROOK,
            QUEEN,
            KING,
            NUM_PIECE_TYPES,
            NONE
        };
    };
    constexpr inline PieceType(value_type value) : MetaValueType<value_type>(value) {}
    constexpr inline PieceType() : PieceType(Value::NONE) {}

    constexpr inline char to_char() const {
        switch (value()) {
        case Value::PAWN:
            return 'p';
        case Value::KNIGHT:
            return 'n';
        case Value::BISHOP:
            return 'b';
        case Value::ROOK:
            return 'r';
        case Value::QUEEN:
            return 'q';
        case Value::KING:
            return 'k';
        default:
            return '-';
        }
    }

    constexpr inline static PieceType from(char c) {
        switch (c) {
        case 'p':
        case 'P':
            return Value::PAWN;
        case 'n':
        case 'N':
            return Value::KNIGHT;
        case 'b':
        case 'B':
            return Value::BISHOP;
        case 'r':
        case 'R':
            return Value::ROOK;
        case 'q':
        case 'Q':
            return Value::QUEEN;
        case 'k':
        case 'K':
            return Value::KING;
        default:
            return Value::NONE;
        }
    }
};

inline std::ostream& operator<<(std::ostream& ostream, PieceType piece_type) {
    return ostream << piece_type.to_char();
}

namespace constants {

constexpr inline PieceType PAWN{PieceType::Value::PAWN};
constexpr inline PieceType KNIGHT{PieceType::Value::KNIGHT};
constexpr inline PieceType BISHOP{PieceType::Value::BISHOP};
constexpr inline PieceType ROOK{PieceType::Value::ROOK};
constexpr inline PieceType QUEEN{PieceType::Value::QUEEN};
constexpr inline PieceType KING{PieceType::Value::KING};
constexpr inline PieceType PIECE_TYPE_NONE{PieceType::Value::NONE};
constexpr inline PieceType PIECE_TYPES[]{PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
constexpr inline int NUM_PIECE_TYPES = PieceType::Value::NUM_PIECE_TYPES;

} // namespace constants

} // namespace libchess

#endif // LIBCHESS_PIECETYPE_H
