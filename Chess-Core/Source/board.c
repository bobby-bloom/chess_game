#include "board.h"
#include "fen.h"

char* getFilenameFromPiece(Piece piece) {
	return g_pieceFilenames[piece.type + (piece.color == WHITE ? 0 : 6)];
}

void initializeBoard(Board* board) {
	uint8_t x, y;
	Piece piece;
	
	for (y = 0; y < BOARD_COLS; y++) {
		for (x = 0; x < BOARD_ROWS; x++) {
			piece = initializeEmptyPiece();

			board->squares[x][y].piece = piece;
			board->squares[x][y].pos.x = y;
			board->squares[x][y].pos.y = x;
			board->squares[x][y].nr = (8 - x) * 8 - 7 + y - 1;
		}
	}
}

Piece initializeEmptyPiece() {
	Piece piece = {
		 .type  = NO_PIECE,
		 .color = NO_COLOR,
	};

	return piece;
}

static Square initializeEmptySquare() {
	Piece	  piece;
	SquarePos pos;
	Square	  square;
  
	piece = initializeEmptyPiece();
	pos.x = 0;
	pos.y = 0;

	square.piece = piece;
	square.pos	 = pos;

	return square;
}

bool isValidSquare(Square* square) {
	if (!square) {
		return false;
	}

	if (square->pos.x < 0 || square->pos.y < 0) {
		return false;
	}

	if (square->pos.x > BOARD_ROWS || square->pos.y > BOARD_COLS) {
		return false;
	}
	if (square->nr < 0 || square->nr > BOARD_COLS * BOARD_ROWS) {
		return false;
	}

	return true;
}

bool isValidPieceType(PieceType pieceType) {
	if (pieceType <= NO_PIECE || pieceType > NUM_PIECE_TYPES) {
		return false;
	}
	return true;
}

bool isValidChessPiece(Piece* piece) {
	if (!piece) {
		return false;
	}
	
	if (!isValidPieceType(piece->type)) {
		return false;
	}

	return true;
}

bool isPositionOnBoard(Board* board, int column, int row) {
	if ((column >= 0 && column <= BOARD_COLS) &&
		(row >= 0 && row <= BOARD_ROWS)) {
		return true;
	}
	return false;
}

static SquarePos squarePosByNr(uint8_t squareNr) {
	SquarePos pos;

	pos.x = BOARD_ROWS - 1 - squareNr / BOARD_ROWS;
	pos.y = squareNr % BOARD_COLS;

	return pos;
}

Piece* pieceOnSquareNr(Board* board, uint8_t squareNr) {
	SquarePos pos;
	pos = squarePosByNr(squareNr);

	if (isValidChessPiece(&board->squares[pos.x][pos.y].piece)) {
		return &board->squares[pos.x][pos.y].piece;
	}
	return NULL;
}

Square* getSquareByNr(Board* board, uint8_t squareNr) {
	uint8_t column = squareNr % 8;
	uint8_t row	   = BOARD_ROWS - 1 - squareNr / BOARD_ROWS;

	return &board->squares[row][column];
}

bool isPieceOnSquareNr(Board* board, uint8_t squareNr) {
	SquarePos pos;
	pos = squarePosByNr(squareNr);

	if (isValidChessPiece(&board->squares[pos.x][pos.y].piece)) {
			return true;
	}
	return false;
}

bool isValidSquareNr(Board* board, uint8_t squareNr) {
	if (squareNr >= 0 && squareNr < BOARD_ROWS * BOARD_COLS) {
		return true;
	}
	return false;
}

bool isValidChessColor(ChessColor color) {
	if (color > NO_PIECE && color <= BLACK) {
		return true;
	}
	return false;
}

bool isOppositePieceColor(ChessColor color1, ChessColor color2) {
	if (!isValidChessColor(color1) || !isValidChessColor(color2)) {
		return false;
	}
	if (color1 != color2) {
		return true;
	}
	return false;
}

bool isPlayersPiece(ChessColor color, Piece piece) {
	if (!isValidChessColor(color)) {
		return false;
	}
	if (color == piece.color) {
		return true;
	}
	return false;
}

Move createMove(
	uint8_t   source, 
	uint8_t   target, 
	PieceType piece, 
	PieceType capture, 
	PieceType promotion, 
	MoveFlags flags) {
	return (source	   & 0x3F)
		 | ((target	   & 0x3F) << 6)
		 | ((piece	   & 0x0F) << 12)
		 | ((capture   & 0x0F) << 16)
		 | ((promotion & 0x0F) << 20)
		 | ((flags	   & 0xFF) << 24);
}

void addPromotionToMove(Move* move, PieceType promotion) {
	*move &= ~(0x0F << 20);
	*move |= (promotion & 0x0F) << 20;
}

static void applyCastleMove(Board* board, Square* kingSource, Square* rookSource) {
	uint8_t kingDestNr = 0;
	uint8_t rookDestNr = 0;

	if (kingSource->piece.color == WHITE) {
		if (rookSource->nr == 0) {
			kingDestNr = 2;
			rookDestNr = 3;
		}
		else if (rookSource->nr == 7) {
			kingDestNr = 6;
			rookDestNr = 5;
		}
	}
	else {
		if (rookSource->nr == 56) {
			kingDestNr = 58;
			rookDestNr = 59;
		}
		else if (rookSource->nr == 63) {
			kingDestNr = 62;
			rookDestNr = 61;
		}
	}

	if (!kingDestNr) {
		return;
	}
	
	Square* kingDest = getSquareByNr(board, kingDestNr);
	Square* rookDest = getSquareByNr(board, rookDestNr);

	kingDest->piece = kingSource->piece;
	rookDest->piece = rookSource->piece;

	kingSource->piece = initializeEmptyPiece();
	rookSource->piece = initializeEmptyPiece();
}

void updateBoard(Board* board, Move move, ChessColor currPlayer) {
	if (!board) {
		return;
	}

	Square* source = getSquareByNr(board, MOVE_SOURCE(move));
	Square* target = getSquareByNr(board, MOVE_TARGET(move));
	
	MoveFlags flags = MOVE_FLAGS(move);

	if (MOVE_FLAG_CASTLING(flags)) {
		return applyCastleMove(board, source, target);
	}
	
	target->piece.type = source->piece.type;
	target->piece.color = source->piece.color;

	source->piece = initializeEmptyPiece();


	if (MOVE_FLAG_PROMOTION(flags)) {
		PieceType promotion = MOVE_PROMOTION(move);
		target->piece.type = promotion;
	}

	if (MOVE_FLAG_EN_PASSANT(flags) && MOVE_FLAG_CAPTURE(flags)) {
		int8_t direction = currPlayer == WHITE ? -BOARD_COLS : BOARD_COLS;
		Square* capturedSquare = getSquareByNr(board, MOVE_TARGET(move) + direction);

		capturedSquare->piece = initializeEmptyPiece();
	}
}

Square* findKing(Board* board, ChessColor kingColor) {
	if (!board || !isValidChessColor(kingColor)) {
		return NULL;
	}

	for (int column = 0; column < BOARD_COLS; column++) {
		for (int row = 0; row < BOARD_ROWS; row++) {
			if (!isValidChessPiece(&board->squares[row][column].piece)) {
				continue;
			}
			if (board->squares[row][column].piece.color != kingColor) {
				continue;
			}
			if (board->squares[row][column].piece.type != KING) {
				continue;
			}


			return &board->squares[row][column];
		}
	}

	return NULL;
}
