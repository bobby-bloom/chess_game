#pragma once
#ifndef BOARD_H
#define BOARD_H

#include <stdint.h> 
#include <stdbool.h>

#include "game-types.h"

#define ArraySize(array) sizeof(array)/sizeof(array[0])

static const struct piece_lookup {
	ChessColor color; PieceType type;
} fenPiecesLookup[128] = {
	['k'] = {BLACK, KING},	 ['q'] = {BLACK, QUEEN},  ['r'] = {BLACK, ROOK},
	['b'] = {BLACK, BISHOP}, ['n'] = {BLACK, KNIGHT}, ['p'] = {BLACK, PAWN},
	['K'] = {WHITE, KING},	 ['Q'] = {WHITE, QUEEN},  ['R'] = {WHITE, ROOK},
	['B'] = {WHITE, BISHOP}, ['N'] = {WHITE, KNIGHT}, ['P'] = {WHITE, PAWN},
};

static char g_piecesPath[] = "Assets/svg/pieces/default/";

static const char* g_pieceFilenames[13] = {
  [1] = "wK.svg",
		"wQ.svg",
		"wR.svg",
		"wB.svg",
		"wN.svg",
		"wP.svg",
		"bK.svg",
		"bQ.svg",
		"bR.svg",
		"bB.svg",
		"bN.svg",
		"bP.svg",
};

static const char startPosThreefold[] = "q7/8/8/8/8/8/8/7Q w -- - 0 1";
static const char startPosDoubleCheck[] = "7q/4p3/3k4/3P4/8/8/3R3B/7K w -- - 0 1";
static const char startPosProm[] = "rnbqkbnr/pPppppp-/8/8/8/8/P-PPPPPp/RNBQKBNR w KQkq - 0 1";
static const char startPosEnPa[] = "rnbqkbnr/pppppp--/8/-P-P----/------p-/------p/P-P-PPPP/RNBQKBNR w KQkq - 0 1";

static const char startPosTest1[] = "5rk1/1p1Bb2p/3p1npP/3P1p2/3Qp3/1q2P3/1B3PP1/2r2RK1 w - - 0 24";
static const char startPosTest2[] = "5r1k/4b2p/3pBQpP/1p1P1p2/4p3/1q2P3/1B3PP1/2R3K1 b - - 0 26";

static const char startPos[]  = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
static const char startPos1[] = "r---kbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
static const char startPos2[] = "qqqqqqqq/qqqqqqqq/8/8/8/8/QQQQQQQQ/QQQQQQQQ w KQkq - 0 1";

static const char startPos3[] = "4k1rr/6r1/8/8/8/8/3R4/R2RK3 w Qk - 0 1";
static const char startPos4[] = "qqqqqqqq/P1P1P111/8/8/8/8/8/8 w - - 0 1";
static const char startPos5[] = "8/8/8/4p1K1/2k1P3/8/8/8 b - - 0 1";

void       initializeBoard(Board* board);
void	   updateBoard(Board* board, Move move, ChessColor currPlayer);
Piece      initializeEmptyPiece();
char*      getFilenameFromPiece(Piece piece);
bool       isValidSquare(Square* square);
bool	   isValidPieceType(PieceType pieceType);
bool       isValidChessPiece(Piece* piece);
Piece*     pieceOnSquareNr(Board* board, uint8_t squareNr);
bool       isPieceOnSquareNr(Board* board, uint8_t squareNr);
Square*    getSquareByNr(Board* board, uint8_t squareNr);
bool       isPositionOnBoard(Board* board, int x, int y);
bool       isValidSquareNr(Board* board, uint8_t squareNr);
bool	   isValidChessColor(ChessColor color);
bool	   isOppositePieceColor(ChessColor color1, ChessColor color2);
bool	   isPlayersPiece(ChessColor color, Piece piece);
Move       createMove(uint8_t   source,
					  uint8_t   target,
					  PieceType piece,
					  PieceType capture,
					  PieceType promotion,
					  MoveFlags flags);
void	   addPromotionToMove(Move* move, PieceType promotion);
Square*    findKing(Board* board, ChessColor kingColor);

#endif // BOARD_H