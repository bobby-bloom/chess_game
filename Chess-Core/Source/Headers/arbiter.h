#pragma once
#ifndef ARBITER_H
#define ARBITER_H

#include <math.h>
#include "board.h"

#define ArraySize(array) sizeof(array)/sizeof(array[0])

static ChessColor inline getPlayerColor(GameState* state) {
	return GAME_FLAG_WHITE_MOVES(state->flags) == 1 ? WHITE : BLACK;
}

void	  findPossibleMoves(GameInstance* game);
Move	  findValidMove(MoveArray* moveTable, uint8_t sourcePos, uint8_t targetPos);
bool	  isValidMove(MoveArray* moveTable, uint8_t sourcePos, Move move);
void	  filterMoves(GameInstance* game, MoveArray* moves, bool (*condition)(GameInstance*, Move), bool evaluteMoves);
bool	  isKingInCheck(GameInstance* game, ...);
bool	  isCastlingPossible(GameInstance* game, Move move);

#endif // !ARBITER_H
