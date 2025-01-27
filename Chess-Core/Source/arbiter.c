#include <stdarg.h>
#include <string.h>

#include "arbiter.h"

static void addMove(GameInstance* game, Square* square, uint8_t targetSq, MoveFlags flags, uint8_t captureType, uint8_t promotionType) {
	Move move = createMove(
		square->nr,
		targetSq,
		square->piece.type,
		captureType,
		promotionType,
		flags
	);

	MoveArray* validMoves = &game->state.validMoves[square->nr];
	validMoves->moves[validMoves->size++] = move;
}

static bool pawnMakesPromotion(Square* square) {
	if ((square->piece.color == WHITE && square->nr / 8 == 6) 
	 || (square->piece.color == BLACK && square->nr / 8 == 1)) {
		return true;
	}
	return false;
}

static void addPawnMoves(GameInstance* game, Square* square) {
	MoveFlags moveFlags;
	int8_t	  direction, row;

	row = square->nr / 8;
	if ((square->piece.color == WHITE && row == BOARD_ROWS - 1) || (square->piece.color == BLACK && row == 0)) {
		return;
	}

	moveFlags = 0;
	direction = square->piece.color == WHITE ? 8 : -8;

	if (!isPieceOnSquareNr(&game->state.board, square->nr + direction)) {
		if (pawnMakesPromotion(square)) {
			moveFlags |= PROMOTION_FLAG;
		}
		addMove(game, square, square->nr + direction, moveFlags, NULL, NULL);

		moveFlags = 0;
		moveFlags |= EN_PASSANT_FLAG;
		if ((square->piece.color == WHITE && row == 1) || (square->piece.color == BLACK && row == 6)) {
			if (!isPieceOnSquareNr(&game->state.board, square->nr + (2 * direction))) {
				addMove(game, square, square->nr + (2 * direction), moveFlags, NULL, NULL);
			}
		}
	}

	Piece* capturedPiece;
	int8_t captureOffsets[] = { direction - 1, direction + 1 };

	for (int i = 0; i < 2; i++) {
		capturedPiece = NULL;
		moveFlags = 0;
		moveFlags |= CAPTURE_FLAG;

		if (pawnMakesPromotion(square)) {
			moveFlags |= PROMOTION_FLAG;
		}

		if ((i == 0 && square->piece.color == WHITE && square->nr % 8 <= 0)
			|| (i == 1 && square->piece.color == WHITE && square->nr % 8 >= BOARD_COLS - 1)
			|| (i == 0 && square->piece.color == BLACK && square->nr % 8 <= 0)
			|| (i == 1 && square->piece.color == BLACK && square->nr % 8 >= BOARD_COLS - 1))
		{
			continue;
		}

		if (GAME_FLAG_EN_PASSANT(game->state.flags)) {
			uint8_t moveTarget = GAME_FLAG_EN_PASSANT_MOVE(game->state.flags);
			if (moveTarget == square->nr + captureOffsets[i]) {
				moveFlags |= EN_PASSANT_FLAG;

				addMove(game, square, moveTarget, moveFlags, PAWN, NULL);
			}
		}
		moveFlags &= ~EN_PASSANT_FLAG;

		capturedPiece = pieceOnSquareNr(&game->state.board, square->nr + captureOffsets[i]);

		if (!isValidChessPiece(capturedPiece) || square->piece.color == capturedPiece->color) {
			continue;
		}

		if (capturedPiece->type == KING) {
			moveFlags ^= PROMOTION_FLAG;
			moveFlags |= CHECK_FLAG;
		}

		addMove(game, square, square->nr + captureOffsets[i], moveFlags, capturedPiece->type, NULL);
	}
}

static bool createMoveWhenPossible(GameInstance* game, Square* square, uint8_t offset) {
	Piece* pieceOnSquare;
	MoveFlags moveFlags;

	pieceOnSquare = NULL;
	pieceOnSquare = pieceOnSquareNr(&game->state.board, square->nr + offset);

	moveFlags = 0;

	if (!pieceOnSquare) {
		addMove(game, square, square->nr + offset, moveFlags, NULL, NULL);
		return true;
	}

	if (isValidChessPiece(pieceOnSquare)) {
		if (pieceOnSquare->color == square->piece.color) {
			return false;
		}

		moveFlags |= pieceOnSquare->type == KING ? CHECK_FLAG : CAPTURE_FLAG;

		addMove(game, square, square->nr + offset, moveFlags, pieceOnSquare->type, NULL);
		return false;
	}

	return false;
}

static void addMovesInDirection(GameInstance* game, Square* square, int8_t dx, int8_t dy) {
	uint8_t row = square->nr / BOARD_ROWS - dx;
	uint8_t column = square->nr % BOARD_COLS - dy;

	while (row >= 0 && row < BOARD_ROWS && column >= 0 && column < BOARD_COLS) {
		if (!createMoveWhenPossible(game, square, (row * BOARD_ROWS + column) - square->nr)) {
			break;
		}

		row -= dx;
		column -= dy;
	}
}

static void addOrthogonalMoves(GameInstance* game, Square* square) {
	addMovesInDirection(game, square, -1, 0);
	addMovesInDirection(game, square, 0, 1);
	addMovesInDirection(game, square, 1, 0);
	addMovesInDirection(game, square, 0, -1);
}

static void addDiagonalMoves(GameInstance* game, Square* square) {
	addMovesInDirection(game, square, -1, 1);
	addMovesInDirection(game, square, -1, -1);
	addMovesInDirection(game, square, 1, 1);
	addMovesInDirection(game, square, 1, -1);
}

static void addRookBishopQueenMoves(GameInstance* game, Square* square) {
	if (square->piece.type == QUEEN || square->piece.type == ROOK) {
		addOrthogonalMoves(game, square);
	}

	if (square->piece.type == QUEEN || square->piece.type == BISHOP) {
		addDiagonalMoves(game, square);
	}
}

static void addKnightMoves(GameInstance* game, Square* square) {
	uint8_t row = square->nr / BOARD_ROWS;
	uint8_t column = square->nr % BOARD_COLS;

	if (row < 6 && column > 0) {
		createMoveWhenPossible(game, square, 15);
	}
	if (row < 6 && column < 7) {
		createMoveWhenPossible(game, square, 17);
	}
	if (row < 7 && column > 1) {
		createMoveWhenPossible(game, square, 6);
	}
	if (row < 7 && column < 6) {
		createMoveWhenPossible(game, square, 10);
	}
	if (row > 0 && column > 1) {
		createMoveWhenPossible(game, square, -10);
	}
	if (row > 0 && column < 6) {
		createMoveWhenPossible(game, square, -6);
	}
	if (row > 1 && column > 0) {
		createMoveWhenPossible(game, square, -17);
	}
	if (row > 1 && column < 7) {
		createMoveWhenPossible(game, square, -15);
	}
}

static bool isConditionInMoveTable(GameState* state, bool (*condition)(Move, int), int extraArg) {
	bool isValid = false;
	
	for (int i = 0; i < BOARD_TILES; i++) {
		for (int j = 0; j < state->validMoves[i].size; j++) {
			if (!condition(state->validMoves[i].moves[j], extraArg)) {
				continue;
			}
			isValid = true;
			break;
		}
	}

	return isValid;
}

static bool isRangeFreeForCastling(Board* board, uint8_t firstSquare, uint8_t lastSquare) {
	int8_t range = lastSquare - firstSquare;

	if (!board || range < 1) {
		return false;
	}

	Square* square;
	for (int i = 0; i <= range; i++) {
		square = getSquareByNr(board, firstSquare + i);
		if (isValidChessPiece(&square->piece)) {
			return false;
		}
	}

	return true;
}

static void addCastlingMoves(GameInstance* game, Square* square) {
	if (GAME_FLAG_CHECK(game->state.flags)) {
		return;
	}
	
	ChessColor color = getPlayerColor(&game->state);

	MoveFlags flags = 0;
	flags |= CASTLING_FLAG;
	if (color == WHITE) {
		if (GAME_FLAG_CASTL_QUEEN_W(game->state.flags) && isRangeFreeForCastling(&game->state.board, 1, 3)) {
			addMove(game, square, 0, flags, NULL, NULL);
		}
		if (GAME_FLAG_CASTL_KING_W(game->state.flags) && isRangeFreeForCastling(&game->state.board, 5, 6)) {
			addMove(game, square, 7, flags, NULL, NULL);
		}
	}
	else {
		if (GAME_FLAG_CASTL_QUEEN_B(game->state.flags) && isRangeFreeForCastling(&game->state.board, 57, 59)) {
			addMove(game, square, 56, flags, NULL, NULL);
		}
		if (
			GAME_FLAG_CASTL_KING_B(game->state.flags) && isRangeFreeForCastling(&game->state.board, 61, 62)) {
			addMove(game, square, 63, flags, NULL, NULL);
		}
	}
}

static void addKingMoves(GameInstance* game, Square* square) {
	uint8_t column = square->nr % BOARD_COLS;
	uint8_t row	   = square->nr / BOARD_ROWS;

	if (column > 0) {
		createMoveWhenPossible(game, square, -1);
	}
	if (column < 7) {
		createMoveWhenPossible(game, square, 1);
	}
	if (row > 0) {
		createMoveWhenPossible(game, square, -8);
	}
	if (row < 7) {
		createMoveWhenPossible(game, square, 8);
	}
	if (column > 0 && row > 0) {
		createMoveWhenPossible(game, square, -9);
	}
	if (column > 0 && row < 7) {
		createMoveWhenPossible(game, square, 7);
	}
	if (column < 7 && row > 0) {
		createMoveWhenPossible(game, square, -7);
	}
	if (column < 7 && row < 7) {
		createMoveWhenPossible(game, square, 9);
	}

	addCastlingMoves(game, square);
}

void filterMoves(GameInstance* game, MoveArray* moves, bool (*condition)(GameInstance*, Move), bool evaluteMoves) {
	if (!moves || moves->size == 0) {
		return;
	}

	int newSize = 0;

	Board newBoard;
	Board oldBoard = game->state.board;
	for (int i = 0; i < moves->size; i++) {
		if (evaluteMoves) {
			memcpy(&newBoard, &oldBoard, sizeof(Board));
			updateBoard(&newBoard, moves->moves[i], getPlayerColor(&game->state));
			game->state.board = newBoard;
		}

		if (condition(game, moves->moves[i])) {
			continue;
		}

		moves->moves[newSize++] = moves->moves[i];
	}
	game->state.board = oldBoard;

	moves->size = newSize;
}

static bool moveHasCheckFlag(Move move, int _) {
	MoveFlags flags = MOVE_FLAGS(move);
	if (MOVE_FLAG_CHECK(flags)) {
		return true;
	}

	return false;
}

static bool isSquareUnderAttack(Move move, uint8_t squareNr) {
	if (MOVE_TARGET(move) == squareNr) {
		return true;
	}

	return false;
}

static void simulateGameTurn(GameInstance* game, GameInstance* evalGame) {
	memcpy(evalGame, game, sizeof(GameInstance));
	switchPlayerColorFlag(&evalGame->state);
	resetEnPassantFlags(&evalGame->state);

	findPossibleMoves(evalGame);
};

bool isKingInCheck(GameInstance* game, ...) {
	GameInstance tempGame;

	simulateGameTurn(game, &tempGame);

	if (isConditionInMoveTable(&tempGame.state, moveHasCheckFlag, NULL)) {
		return true;
	}

	return false;
}

static int* getQueenSideCastlDestSquares(ChessColor color) {
	static int whiteSquares[] = { 2, 3 };
	static int blackSquares[] = { 58, 59 };
	return (color == WHITE) ? whiteSquares : blackSquares;
}

static int* getKingSideCastlDestSquares(ChessColor color) {
	static int whiteSquares[] = { 5, 6 };
	static int blackSquares[] = { 61, 62 };
	return (color == WHITE) ? whiteSquares : blackSquares;
}

bool isCastlingPossible(GameInstance* game, Move move) {
	MoveFlags flags = MOVE_FLAGS(move);
	if (!MOVE_FLAG_CASTLING(flags)) {
		return false;
	}
	
	GameInstance tempGame;
	simulateGameTurn(game, &tempGame);

	ChessColor color = getPlayerColor(&game->state);

	bool isQueenSide =
		color == WHITE ?
			MOVE_TARGET(move) == 0 ?
				true
				: false
			: MOVE_TARGET(move) == 56 ?
				true
				: false;

	int* squares = isQueenSide ? getQueenSideCastlDestSquares(color) : getKingSideCastlDestSquares(color);

	for (int i = 0; i < 2; i++) {
		if (isConditionInMoveTable(&tempGame.state, isSquareUnderAttack, squares[i])) {
			return true;
		}
	}

	return false;
}

typedef void (*AddMoves)(GameInstance* game, Square* square);	

AddMoves addMoves[NUM_PIECE_TYPES] = {
	[NO_PIECE] = NULL,
	[KING]	   = addKingMoves,
	[QUEEN]	   = addRookBishopQueenMoves,
	[BISHOP]   = addRookBishopQueenMoves,
	[ROOK]	   = addRookBishopQueenMoves,
	[KNIGHT]   = addKnightMoves,
	[PAWN]	   = addPawnMoves,
};

void findPossibleMoves(GameInstance* game) {
	ChessColor playerColor = GAME_FLAG_WHITE_MOVES(game->state.flags) ? WHITE : BLACK;
	
	uint8_t x, y;
	Square* square;
	for (x = 0; x < BOARD_ROWS; x++) {
		for (y = 0; y < BOARD_COLS; y++) {
			square = &game->state.board.squares[x][y];
			game->state.validMoves[square->nr].size = 0;

			if (square->piece.color != playerColor || !isValidChessPiece(&square->piece)) {
				continue;
			}

			addMoves[square->piece.type](game, square);
		}
	}
}

Move findValidMove(MoveArray* moveTable, uint8_t sourcePos, uint8_t targetPos) {
	if (moveTable[sourcePos].size < 1) {
		return 0;
	}

	for (int i = 0; i < moveTable[sourcePos].size; i++) {
		if (MOVE_TARGET(moveTable[sourcePos].moves[i]) == targetPos) {
			return moveTable[sourcePos].moves[i];
		}
	}

	return 0;
}

bool isValidMove(MoveArray* moveTable, uint8_t sourcePos, Move move) {
	bool isValid = false;
	
	for (int i = 0; i < moveTable[sourcePos].size; i++) {
		if (moveTable[sourcePos].moves[i] == move) {
			isValid = true;
			break;
		}
	}

	MoveFlags flags = MOVE_FLAGS(move);
	if (MOVE_FLAG_PROMOTION(flags) == 1 && isValidPieceType(MOVE_PROMOTION(move))) {
		isValid = true;
	}

	return isValid;
}
