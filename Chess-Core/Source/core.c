#include "core.h"

int runDefaultGame() {
	GameVariant variant;
	TimeControl timeCtl;
	Opponent opponent;
	
	variant = Classic;
	timeCtl = Corresondence;
	opponent = Random;

	startGame(variant, timeCtl, opponent);
	
	return 0;
}	