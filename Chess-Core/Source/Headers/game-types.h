#pragma once
#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#define BOARD_COLS  8
#define BOARD_ROWS  8
#define BOARD_TILES (BOARD_ROWS * BOARD_COLS)
#define MAX_PIECES  32

#define NUM_PIECE_TYPES 7

typedef enum piece_type {
	NO_PIECE,
	KING,
	QUEEN,
	ROOK,
	BISHOP,
	KNIGHT,
	PAWN,
} PieceType;

typedef enum chess_color {
	NO_COLOR,
	WHITE,
	BLACK
} ChessColor;

typedef struct piece {
	uint8_t	type : 4;
	uint8_t	color : 4;

} Piece;

typedef struct square_pos {
	uint8_t x : 4;
	uint8_t y : 4;
} SquarePos;

typedef struct square {
	Piece	  piece;
	SquarePos pos;
	uint8_t	  nr;
} Square;

typedef struct board {
	Square squares[BOARD_ROWS][BOARD_COLS];
} Board;

typedef enum move_flags {
	NO_MOVE_FLAG	= 0,
	CAPTURE_FLAG	= 1,
	CASTLING_FLAG	= 1 << 1,
	EN_PASSANT_FLAG = 1 << 2,
	CHECK_FLAG		= 1 << 3,
	CHECKMATE_FLAG	= 1 << 4,
	PROMOTION_FLAG	= 1 << 5,
	// remaining	= 2
} MoveFlags;

#define MOVE_FLAG_CAPTURE(flag)    ((flag) & 0x01)
#define MOVE_FLAG_CASTLING(flag)   ((flag >> 1) & 0x01)
#define MOVE_FLAG_EN_PASSANT(flag) ((flag >> 2) & 0x01)
#define MOVE_FLAG_CHECK(flag)	   ((flag >> 3) & 0x01)
#define MOVE_FLAG_CHECKMATE(flag)  ((flag >> 4) & 0x01)
#define MOVE_FLAG_PROMOTION(flag)  ((flag >> 5) & 0x01)

/******
 * 01 - 06 Bit = source
 * 07 - 12 Bit = target
 * 13 - 16 Bit = piece
 * 17 - 20 Bit = captured piece
 * 21 - 24 Bit = promoted piece
 * 25 - 32 Bit = move flags
 ******/
typedef uint32_t Move;

#define MOVE_SOURCE(move)	 ((move) & 0x3F)
#define MOVE_TARGET(move)	 ((move >> 6) & 0x3F)
#define MOVE_PIECE(move)	 ((move >> 12) & 0x0F)
#define MOVE_CAPTURE(move)   ((move >> 16) & 0x0F)
#define MOVE_PROMOTION(move) ((move >> 20) & 0x0F)
#define MOVE_FLAGS(move)	 ((move >> 24) & 0xFF)

typedef struct move_array {
	size_t size;
	Move   moves[BOARD_TILES / 2];
} MoveArray;

typedef enum game_variant {
	Classic,
	Chess960,
	KingOfTheHill,
	TreeCheck,
	RacingKings,
	Horde,
	GameVariantCount
} GameVariant;

typedef enum timectl {
	RealTime,
	Corresondence,
	Unlimited,
	TimeControlCount
} TimeControl;

typedef enum opponent {
	Random,
	Friend,
	Computer
} Opponent;

typedef struct game_info2 {
	char event[256];
	char site[256];
	char date[10];
	char utcTime[8];
	char white[256];
	char black[256];
	char result[7];
} GameInfo2;

typedef struct game_info {
	GameVariant	variant;
	Opponent	opponent;
} GameInfo;

/******
 * 01 - 16 Bit = flags
 * 17 - 22 Bit = en_passant move
 * 23 - 30 Bit = half moves clock
 * 31 - 32 Bit = empty
 ******/
typedef enum game_flags {
	GAME_FLAG_DRAW			 = 1,
	GAME_FLAG_CHECK          = 1 << 1,
	GAME_FLAG_CHECKMATE      = 1 << 2,
	GAME_FLAG_STALEMATE	     = 1 << 3,
	GAME_FLAG_WHITE_MOVES    = 1 << 4,
	// En passant possiblity
	GAME_FLAG_EN_PASSANT     = 1 << 5,
	// Castling rights
	GAME_FLAG_CASTL_QUEEN_W  = 1 << 6,
	GAME_FLAG_CASTL_KING_W   = 1 << 7,
	GAME_FLAG_CASTL_QUEEN_B  = 1 << 8,
	GAME_FLAG_CASTL_KING_B   = 1 << 9,
	// remaining_flag_space	 = 6
} GameFlags;

typedef struct game_state {
	Board	  board;
	MoveArray validMoves[BOARD_TILES];
	GameFlags flags;
} GameState;

#define GAME_FLAG_DRAW(flags)            ((flags) & 0x01)
#define GAME_FLAG_CHECK(flags)		     (((flags) >> 1) & 0x01)
#define GAME_FLAG_CHECKMATE(flags)       (((flags) >> 2) & 0x01)
#define GAME_FLAG_STALEMATE(flags)       (((flags) >> 3) & 0x01)
#define GAME_FLAG_WHITE_MOVES(flags)     (((flags) >> 4) & 0x01)
#define GAME_FLAG_EN_PASSANT(flags)      (((flags) >> 5) & 0x01)
#define GAME_FLAG_CASTL_QUEEN_W(flags)   (((flags) >> 6) & 0x01)
#define GAME_FLAG_CASTL_KING_W(flags)    (((flags) >> 7) & 0x01)
#define GAME_FLAG_CASTL_QUEEN_B(flags)   (((flags) >> 8) & 0x01)
#define GAME_FLAG_CASTL_KING_B(flags)    (((flags) >> 9) & 0x01)

#define GAME_FLAG_EN_PASSANT_MOVE(flags) (((flags) >> 16) & 0x3F)
#define GAME_FLAG_HALF_MOVE_CLOCK(flags) (((flags) >> 22) & 0xFF)

#define HISTORY_LENGTH 512

typedef struct game_instance {
	GameInfo	info;
	GameState	state;
	GameState*	stateHistory;
	size_t      stateCount;
	Move*	    moveHistory;
	size_t      moveCount;
} GameInstance;

static inline void switchPlayerColorFlag(GameState* state) {
	state->flags ^= GAME_FLAG_WHITE_MOVES;
}

static inline void resetEnPassantFlags(GameState* state) {
	state->flags &= ~GAME_FLAG_EN_PASSANT;
	state->flags &= ~(0x3F << 16);
}

static inline void resetHalfMoveClock(GameState* state) {
	state->flags &= ~(0xFF << 22);
}

static inline void updateCheckFlag(GameState* state, bool isCheck) {
	if (isCheck) {
		state->flags |= GAME_FLAG_CHECK;
	}
	else {
		state->flags &= ~GAME_FLAG_CHECK;
	}
}

#endif //!GAME_TYPES_H
