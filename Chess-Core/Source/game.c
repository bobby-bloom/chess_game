#include <stdlib.h>

#include "game.h"
#include "fen.h"

static GameInstance g_gameInstance;

static void initializeDefaultGameState(GameState* state) {
	state->flags = 0;
	state->flags |= GAME_FLAG_WHITE_MOVES;

	state->flags |= GAME_FLAG_CASTL_KING_W;
	state->flags |= GAME_FLAG_CASTL_QUEEN_W;
	state->flags |= GAME_FLAG_CASTL_KING_B;
	state->flags |= GAME_FLAG_CASTL_QUEEN_B;
}

static void initializeGameHistory(GameInstance* game) {
	game->stateHistory = (GameState*)calloc(HISTORY_LENGTH, sizeof(GameState));
	game->stateCount = 0;

	game->moveHistory = (Move*)malloc(sizeof(Move) * HISTORY_LENGTH);
	game->moveCount = 0;
}

int initializeGameFromFEN(GameInstance* game, char* notation) {
	initializeBoard(&game->state.board);
	initializeGameHistory(game);

	if (!loadGameFromFEN(game, notation)) {
		return -1;
	}

	findPossibleMoves(game);
	updateCheckFlag(&game->state, isKingInCheck(game));

	for (int i = 0; i < BOARD_TILES; i++) {
		filterMoves(game, &game->state.validMoves[i], isKingInCheck, true);
		filterMoves(game, &game->state.validMoves[i], isCastlingPossible, false);
	}

	memcpy(&game->stateHistory[game->stateCount++], &game->state, sizeof(GameState));

	return 1;
}

void startGame(GameVariant variant, TimeControl timeCtl, Opponent opponent) {

	initializeGameFromFEN(&g_gameInstance, startPos);
}

size_t countPossibleMoves(MoveArray* moveTable) {
	size_t count = 0;
	for (int i = 0; i < BOARD_TILES; i++) {
		count += moveTable[i].size;
	}

	return count;
}

void iterateAndMove(GameInstance* game, Move move, int depth, int maxDepth, uint64_t* cMoveArray, uint64_t* cCheckArray) {
	GameInstance evalGame;
	memcpy(&evalGame, game, sizeof(GameInstance));
	addMove(&evalGame, move);

	if (GAME_FLAG_CHECKMATE(evalGame.state.flags)) {
		cCheckArray[depth - 1]++;
	}

	if (depth >= maxDepth) {
		return;
	}

	for (int i = 0; i < BOARD_TILES; i++) {
		for (int j = 0; j < evalGame.state.validMoves[i].size; j++) {
			iterateAndMove(
				&evalGame, 
				evalGame.state.validMoves[i].moves[j], 
				depth + 1,
				maxDepth, 
				cMoveArray,
				cCheckArray
			);
		}

		cMoveArray[depth] += evalGame.state.validMoves[i].size;
	}
}

GameInstance* getGameInstance() {
	return &g_gameInstance;
}

static void updateEnPassantState(GameState* state, Move move) {
	resetEnPassantFlags(state);

	MoveFlags flags = MOVE_FLAGS(move);
	
	if (MOVE_FLAG_EN_PASSANT(flags) && !MOVE_FLAG_CAPTURE(flags)) {
		ChessColor color = GAME_FLAG_WHITE_MOVES(state->flags) ? WHITE : BLACK;
		int direction = color == WHITE ? 8 : -8;
		uint8_t moveSource = MOVE_SOURCE(move);
		uint8_t enPassantSqNr = moveSource + direction;
		
		state->flags |= GAME_FLAG_EN_PASSANT;
		state->flags |= (enPassantSqNr & 0x3F) << 16;
	}
}

static void updateCastlingState(GameState* state, Move move) {
	ChessColor color = GAME_FLAG_WHITE_MOVES(state->flags) ? WHITE : BLACK;
	
	uint8_t kingSource = color == WHITE ? 4 : 60;
	uint8_t queenRookSource = color == WHITE ? 0 : 56;
	uint8_t kingRookSource  = color == WHITE ? 7 : 63;

	GameFlags queenCastleFlag = color == WHITE ? GAME_FLAG_CASTL_QUEEN_W : GAME_FLAG_CASTL_QUEEN_B;
	GameFlags kingCastleFlag = color == WHITE ? GAME_FLAG_CASTL_KING_W : GAME_FLAG_CASTL_KING_B;

	if (!(state->flags & queenCastleFlag) && !(state->flags & kingCastleFlag)) {
		return;
	}

	PieceType piece = MOVE_PIECE(move);

	if (piece == KING && MOVE_SOURCE(move) == kingSource) {
		state->flags &= ~queenCastleFlag;
		state->flags &= ~kingCastleFlag;
	} 
	else if (piece == ROOK) {
		if (MOVE_SOURCE(move) == queenRookSource) {
			state->flags &= ~queenCastleFlag;
		}
		else if (MOVE_SOURCE(move) == kingRookSource) {
			state->flags &= ~kingCastleFlag;
		}
	}
}

static void updateHalfMoveClock(GameState* state, Move move) {
	PieceType piece = MOVE_PIECE(move);
	MoveFlags flags = MOVE_FLAGS(move);

	uint8_t count = GAME_FLAG_HALF_MOVE_CLOCK(state->flags);
	resetHalfMoveClock(state);

	if (piece == PAWN || MOVE_FLAG_CAPTURE(flags)) {
		return;
	}

	state->flags |= (++count & 0xFF) << 22;
}

static bool movesAvailable(MoveArray* moveTable) {
	bool moveFound = false;
	for (int i = 0; i < BOARD_TILES; i++) {
		if (!moveTable[i].size) {
			continue;
		}
		moveFound = true;
		break;
	}

	return moveFound;
}

static bool isFiftiethMove(GameState* state) {
	uint8_t moveCount = GAME_FLAG_HALF_MOVE_CLOCK(state->flags);

	if (moveCount == 100) {
		return true;
	}
	return false;
}

static bool isThreefoldRepetition(GameInstance* game) {
	GameFlags historyFlags, currentFlags;
	int repetitionCount = 0;

	currentFlags  = game->state.flags;
	currentFlags &= ~GAME_FLAG_WHITE_MOVES;
	currentFlags &= ~(0xFF << 22);

	for (int i = 0; i < game->stateCount; i++) {
		historyFlags = game->stateHistory[i].flags;
		historyFlags &= ~GAME_FLAG_WHITE_MOVES;
		historyFlags &= ~(0xFF << 22);

		if (historyFlags == currentFlags) {
			if (memcmp(&game->state.board, &game->stateHistory[i].board, sizeof(Board))) {
				continue;
			}

			repetitionCount++;
		}
	}

	if (repetitionCount >= 3) {
		return true;
	}

	return false;
}

static void updateGameState(GameInstance* game, Move move) {
	updateHalfMoveClock(&game->state, move);
	
	if (isFiftiethMove(&game->state)) {
		game->state.flags |= GAME_FLAG_DRAW;
		printf("/###########/ - DRAW - /###########/");
	}

	if (isThreefoldRepetition(game)) {
		game->state.flags |= GAME_FLAG_DRAW;
		printf("/###########/ - DRAW - /###########/");
	}

	updateEnPassantState(&game->state, move);
	updateCastlingState(&game->state, move);
	
	switchPlayerColorFlag(&game->state);

	updateCheckFlag(&game->state, isKingInCheck(game));
}

static void updateGameHistory(GameInstance* game, Move move) {
	memcpy(&game->stateHistory[game->stateCount++], &game->state, sizeof(GameState));
	game->moveHistory[game->moveCount++] = move;
}

static void updateGame(GameInstance* game, Move move) {
	updateBoard(&game->state.board, move, getPlayerColor(&game->state));
	updateGameState(game, move);

	findPossibleMoves(game);

	for (int i = 0; i < BOARD_TILES; i++) {
		filterMoves(game, &game->state.validMoves[i], isKingInCheck, true);
		filterMoves(game, &game->state.validMoves[i], isCastlingPossible, false);
	}

	if (!movesAvailable(game->state.validMoves)) {
		if (!GAME_FLAG_CHECK(game->state.flags)) {
			game->state.flags |= GAME_FLAG_STALEMATE;
			printf("/###########/ - STALEMATE - /###########/");
		}
		else {
			game->state.flags |= GAME_FLAG_CHECKMATE;
			printf("/###########/ - CHECKMATE - /###########/");
		}
	}

	updateGameHistory(game, move);
}

bool addMove(GameInstance* game, Move move) {
	if (GAME_FLAG_DRAW(game->state.flags) 
	 || GAME_FLAG_CHECKMATE(game->state.flags) 
	 || GAME_FLAG_STALEMATE(game->state.flags)) {
		return false;
	}
	
	uint8_t source = MOVE_SOURCE(move);

	if (!isValidSquareNr(&game->state.board, source)) {
		return false;
	}

	if (!isValidMove(game->state.validMoves, source, move)) {
		return false;
	}
	
	updateGame(game, move);
	return true;
}