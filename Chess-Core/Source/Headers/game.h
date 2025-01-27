#pragma once
#ifndef CHESS_GAME_H
#define CHESS_GAME_H

#include "board.h"
#include "arbiter.h"

static const inline char* getGameVariantStr(GameVariant variant) {
	if (variant < 0 || variant > GameVariantCount) {
		return "";
	}
	static const unsigned short* strings[] = {
		L"Classic", L"Chess960",
		L"KingOfTheHill", L"TreeCheck",
		L"RacingKings", L"Horde"
	};

	return (const char*)strings[variant];
}

static const inline char* getTimeControlStr(TimeControl timeCtl) {
	if (timeCtl < 0 || timeCtl > TimeControlCount) {
		return "";
	}
	static const unsigned short* strings[] = {
		L"Real Time", L"Correspondence", L"Unlimited"
	};

	return (const char*)strings[timeCtl];
}

int			  initializeGameFromFEN(GameInstance* game, char* notation);
void		  startGame(GameVariant variant, TimeControl timeCtl, Opponent opponent);
size_t		  countPossibleMoves(MoveArray* moveTable);
GameInstance* getGameInstance();
bool		  addMove(GameInstance* game, Move move);
void		  iterateAndMove(
				GameInstance* game, 
				Move move, 
				int depth, 
				int maxDepth, 
				uint64_t* cMoveArray, 
				uint64_t* cCheckArray
			  );

#endif // CHESS_GAME_H