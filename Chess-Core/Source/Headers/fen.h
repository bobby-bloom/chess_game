#pragma once
#ifndef FEN_H
#define FEN_H

#include <board.h>

bool isValidFEN(char* notation);
int  loadPositionsFromFEN(Board* board, char* notation);
int  loadCastlingStateFromFEN(GameState* state, char* notation);
int  loadEnPassantStateFromFEN(GameState* state, char* notation);
int  loadGameFromFEN(GameInstance* game, char* notation);

#endif // !FEN_H
