#ifndef LIBCHESS_MOVEINTEGRATION_H
#define LIBCHESS_MOVEINTEGRATION_H

namespace libchess {

bool Position::is_legal_move(Move move) const {
    Color c = side_to_move();
    Square from = move.from_square();
    Square king_sq = king_square(c);
    if (move.type() == Move::Type::ENPASSANT) {
        Bitboard ep_bb = Bitboard{enpassant_square()};
        Bitboard post_ep_occupancy =
            (occupancy_bb() ^ Bitboard{from} ^ lookups::pawn_shift(ep_bb, !c)) | ep_bb;

        return !(lookups::rook_attacks(king_sq, post_ep_occupancy) & color_bb(!c) &
                 (piece_type_bb(constants::QUEEN) | piece_type_bb(constants::ROOK))) &&
               !(lookups::bishop_attacks(king_sq, post_ep_occupancy) & color_bb(!c) &
                 ((piece_type_bb(constants::QUEEN) | piece_type_bb(constants::BISHOP))));
    } else if (from == king_sq) {
        return move.type() == Move::Type::CASTLING || !attackers_to(move.to_square(), !c);
    } else {
        return !(pinned_pieces(c) & Bitboard{from}) ||
               (Bitboard{move.to_square()} & lookups::direction_xray(king_sq, from));
    }
}
Move::Type Position::move_type_of(Move move) const {
    Move::Type move_type = move.type();
    if (move_type != constants::MOVE_TYPE_NONE) {
        return move_type;
    } else {
        Square to_square = move.to_square();
        Square from_square = move.from_square();
        PieceType moving_pt = piece_type_on(from_square);
        PieceType captured_pt = piece_type_on(to_square);
        PieceType promotion_pt = move.promotion_piece_type();
        if (promotion_pt != constants::PIECE_TYPE_NONE) {
            return captured_pt != constants::PIECE_TYPE_NONE ? Move::Type::CAPTURE_PROMOTION
                                                             : Move::Type::PROMOTION;
        } else if (captured_pt != constants::PIECE_TYPE_NONE) {
            return Move::Type::CAPTURE;
        } else if (moving_pt == constants::PAWN) {
            int sq_diff = std::abs(to_square - from_square);
            if (sq_diff == 16) {
                return Move::Type::DOUBLE_PUSH;
            } else if (sq_diff == 9 || sq_diff == 7) {
                return Move::Type::ENPASSANT;
            } else {
                return Move::Type::NORMAL;
            }
        } else if (moving_pt == constants::KING && std::abs(to_square - from_square) == 2) {
            return Move::Type::CASTLING;
        } else {
            return Move::Type::NORMAL;
        }
    }
}
bool Position::is_capture_move(Move move) const {
    Move::Type move_type = move.type();
    switch (move_type) {
    case Move::Type::CAPTURE:
    case Move::Type::CAPTURE_PROMOTION:
    case Move::Type::ENPASSANT:
        return true;
    default:
        return false;
    }
}
bool Position::is_promotion_move(Move move) const {
    Move::Type move_type = move.type();
    switch (move_type) {
    case Move::Type::PROMOTION:
    case Move::Type::CAPTURE_PROMOTION:
        return true;
    default:
        return false;
    }
}
void Position::unmake_move() {
    if (side_to_move() == constants::WHITE) {
        --fullmoves_;
    }
    Move move = state().previous_move_;
    Move::Type move_type = state().move_type_;
    PieceType captured_pt = state().captured_pt_;
    --ply_;
    reverse_side_to_move();
    if (move == constants::MOVE_NONE) {
        return;
    }
    Color stm = side_to_move();

    Square from_square = move.from_square();
    Square to_square = move.to_square();

    PieceType moving_pt = piece_type_on(to_square);
    switch (move_type) {
    case Move::Type::NORMAL:
        move_piece(to_square, from_square, moving_pt, stm);
        break;
    case Move::Type::CAPTURE:
        move_piece(to_square, from_square, moving_pt, stm);
        put_piece(to_square, captured_pt, !stm);
        break;
    case Move::Type::DOUBLE_PUSH:
        move_piece(to_square, from_square, constants::PAWN, stm);
        break;
    case Move::Type::ENPASSANT:
        put_piece(stm == constants::WHITE ? Square{to_square - 8} : Square(to_square + 8),
                  constants::PAWN, !stm);
        move_piece(to_square, from_square, constants::PAWN, stm);
        break;
    case Move::Type::CASTLING:
        move_piece(to_square, from_square, constants::KING, stm);
        switch (to_square) {
        case constants::C1:
            move_piece(constants::D1, constants::A1, constants::ROOK, stm);
            break;
        case constants::G1:
            move_piece(constants::F1, constants::H1, constants::ROOK, stm);
            break;
        case constants::C8:
            move_piece(constants::D8, constants::A8, constants::ROOK, stm);
            break;
        case constants::G8:
            move_piece(constants::F8, constants::H8, constants::ROOK, stm);
            break;
        default:
            break;
        }
        break;
    case Move::Type::PROMOTION:
        remove_piece(to_square, move.promotion_piece_type(), stm);
        put_piece(from_square, constants::PAWN, stm);
        break;
    case Move::Type::CAPTURE_PROMOTION:
        remove_piece(to_square, move.promotion_piece_type(), stm);
        put_piece(from_square, constants::PAWN, stm);
        put_piece(to_square, captured_pt, !stm);
        break;
    case Move::Type::NONE:
        break;
    }
}
void Position::make_move(Move move) {
    Color stm = side_to_move();
    if (stm == constants::BLACK) {
        ++fullmoves_;
    }
    State& prev_state = state_mut_ref();
    ++ply_;
    State& next_state = state_mut_ref();
    next_state.halfmoves_ = prev_state.halfmoves_ + 1;
    next_state.previous_move_ = move;
    next_state.enpassant_square_ = constants::SQUARE_NONE;

    Square from_square = move.from_square();
    Square to_square = move.to_square();

    next_state.castling_rights_ = CastlingRights{prev_state.castling_rights_.value() &
                                                 castling_spoilers[from_square.value()] &
                                                 castling_spoilers[to_square.value()]};

    PieceType moving_pt = piece_type_on(from_square);
    PieceType captured_pt = piece_type_on(to_square);
    PieceType promotion_pt = move.promotion_piece_type();

    Move::Type move_type = move_type_of(move);

    if (moving_pt == constants::PAWN || captured_pt != constants::PIECE_TYPE_NONE) {
        next_state.halfmoves_ = 0;
    }

    switch (move_type) {
    case Move::Type::NORMAL:
        move_piece(from_square, to_square, moving_pt, stm);
        break;
    case Move::Type::CAPTURE:
        remove_piece(to_square, captured_pt, !stm);
        move_piece(from_square, to_square, moving_pt, stm);
        break;
    case Move::Type::DOUBLE_PUSH:
        move_piece(from_square, to_square, constants::PAWN, stm);
        next_state.enpassant_square_ =
            stm == constants::WHITE ? Square(from_square + 8) : Square(from_square - 8);
        break;
    case Move::Type::ENPASSANT:
        move_piece(from_square, to_square, constants::PAWN, stm);
        remove_piece(stm == constants::WHITE ? Square(to_square - 8) : Square(to_square + 8),
                     constants::PAWN, !stm);
        break;
    case Move::Type::CASTLING:
        move_piece(from_square, to_square, constants::KING, stm);
        switch (to_square) {
        case constants::C1:
            move_piece(constants::A1, constants::D1, constants::ROOK, stm);
            break;
        case constants::G1:
            move_piece(constants::H1, constants::F1, constants::ROOK, stm);
            break;
        case constants::C8:
            move_piece(constants::A8, constants::D8, constants::ROOK, stm);
            break;
        case constants::G8:
            move_piece(constants::H8, constants::F8, constants::ROOK, stm);
            break;
        default:
            break;
        }
        break;
    case Move::Type::PROMOTION:
        remove_piece(from_square, constants::PAWN, stm);
        put_piece(to_square, promotion_pt, stm);
        break;
    case Move::Type::CAPTURE_PROMOTION:
        remove_piece(to_square, captured_pt, !stm);
        remove_piece(from_square, constants::PAWN, stm);
        put_piece(to_square, promotion_pt, stm);
        break;
    case Move::Type::NONE:
        break;
    }
    next_state.captured_pt_ = captured_pt;
    next_state.move_type_ = move_type;
    reverse_side_to_move();
    next_state.hash_ = calculate_hash();
}
void Position::make_null_move() {
    if (side_to_move() == constants::BLACK) {
        ++fullmoves_;
    }
    State& prev = state_mut_ref();
    ++ply_;
    State& next = state_mut_ref();
    reverse_side_to_move();
    next.previous_move_ = constants::MOVE_NONE;
    next.halfmoves_ = prev.halfmoves_ + 1;
    next.enpassant_square_ = constants::SQUARE_NONE;
    next.castling_rights_ = prev.castling_rights_;
    next.hash_ = calculate_hash();
}

} // namespace libchess

#endif // LIBCHESS_MOVEINTEGRATION_H
