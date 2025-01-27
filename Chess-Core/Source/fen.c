#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "fen.h"

static bool isValidLineupFEN(char* notation) {
	if (!notation) {
		return false;
	}

	char validLetters[] = { 'K', 'Q', 'B', 'N', 'R', 'P', 'k', 'q', 'b', 'n', 'r', 'p' };
	int cols = 0;
	int rows = 0;
	int i, j;
	for (i = 0; i < strlen(notation); i++) {
		for (j = 0; j < ArraySize(validLetters); j++) {
			if (notation[i] == validLetters[j]) {
				cols++;
				break;
			}
		}
		if (isdigit(notation[i])) {
			cols += notation[i] - '0';
		}
		else if (notation[i] == '/') {
			if (cols != BOARD_COLS) {
				return false;
			}
			cols = 0;
			rows++;
		}
	}

	if (rows != BOARD_ROWS - 1) {
		return false;
	}

	return true;
}

static bool isValidCastlingFEN(char* notation) {
	if (!notation) {
		return false;
	}
	size_t length = strlen(notation);
	if (length == 1 && notation[0] == '-') {
		return true;
	}
	else if (length == 1) {
		return false;
	}

	char validLetters[] = { 'k', 'q', 'K', 'Q' };
	int matches = 0;
	for (int i = 0; i < strlen(notation); i++) {
		for (int j = 0; j < ArraySize(validLetters); j++) {
			if (notation[i] == validLetters[j]) {
				matches++;
				break;
			}
		}
	}

	if (matches == length) {
		return true;
	}

	return false;
}

static bool isValidEnPassantFEN(char* notation) {
	if (!notation) {
		return false;
	}

	size_t length = strlen(notation);
	if (length == 1 && notation[0] == '-') {
		return true;
	}
	else if (length == 1) {
		return false;
	}

	if (length != 2) {
		return false;
	}

	bool isValid = false;
	int i;
	for (i = 0; i < BOARD_ROWS; i++) {
		if ('a' + i == notation[0]) {
			isValid = true;
			break;
		}
	}
	for (i = 0; i < BOARD_COLS; i++) {
		if (i == notation[1]) {
			isValid = true;
			break;
		}
	}
	return isValid;
}

bool isValidFEN(char* notation) {
	if (!notation) {
		return false;
	}

	size_t length = strlen(notation);
	if (length > 65 || length < 28) {
		return false;
	}

	int i = 0;
	int spaces = 0;
	while (notation[i] != '\0' && spaces < 5 && i < length) {
		if (notation[i] == ' ') {
			spaces++;
		}
		i++;
	}

	if (spaces != 5) {
		return false;
	}

	char copy[65];
	strcpy(copy, notation);
	
	char* pch;
	pch = strtok(copy, " ");

	if (!pch) {
		return false;
	}

	if (!isValidLineupFEN(pch)) {
		return false;
	}

	pch = strtok(NULL, " ");
	if (!strcmp(pch, "w") && !strcmp(pch, "b")) {
		return false;
	}

	pch = strtok(NULL, " ");
	if (!isValidCastlingFEN(pch)) {
		return false;
	}

	pch = strtok(NULL, " ");
	if (!isValidEnPassantFEN(pch)) {
		return false;
	}

	pch = strtok(NULL, " ");
	if (!isdigit(pch[0])) {
		return false;
	}

	pch = strtok(NULL, " ");
	if (!isdigit(pch[0])) {
		return false;
	}

	return true;
}

static void setPieceOnBoardByLetter(Board* board, int column, int row, int letter) {
	Square* square = &board->squares[row][column];
	Piece* piece = &square->piece;

	piece->type  = fenPiecesLookup[letter].type;
	piece->color = fenPiecesLookup[letter].color;
}

int loadPositionsFromFEN(Board* board, char* notation) {
	if (!notation) {
		return -1;
	}

	size_t length = strlen(notation);

	int column = 0;
	int row = 0;

	for (int i = 0; i < length; i++) {
		if (isdigit(notation[i])) {
			column += notation[i] - '0';
			continue;
		}
		if (notation[i] == '/') {
			column = 0;
			row++;
			continue;
		}

		setPieceOnBoardByLetter(board, column, row, notation[i]);
		column++;
	}

	return 1;
}

int loadCastlingStateFromFEN(GameState* state, char* notation) {
	if (!notation) {
		return -1;
	}

	size_t length = strlen(notation);

	if (length == 1) {
		return 1;
	}

	for (int i = 0; i < length; i++) {
		if (notation[i] == 'K') {
			state->flags |= GAME_FLAG_CASTL_KING_W;
		}
		else if (notation[i] == 'Q') {
			state->flags |= GAME_FLAG_CASTL_QUEEN_W;
		}
		else if (notation[i] == 'k') {
			state->flags |= GAME_FLAG_CASTL_KING_B;
		}
		else if (notation[i] == 'q') {
			state->flags |= GAME_FLAG_CASTL_QUEEN_B;
		}
	}
	
	return 1;
}

int loadEnPassantStateFromFEN(GameState* state, char* notation) {
	if (!notation) {
		return -1;
	}
	
	if (notation[0] == '-') {
		return 1;
	}

	int row = notation[0] - 'a';
	int column = notation[1];
	int square = row * BOARD_ROWS + column;

	state->flags |= GAME_FLAG_EN_PASSANT;
	state->flags |= (square & 0x3F) << 16;

	return 1;
}

int loadGameFromFEN(GameInstance* game, char* notation) {
	if (!isValidFEN(notation)) {
		return -1;
	}

	char copy[65];
	strcpy(copy, notation);

	char* fenToken;
	fenToken = strtok(copy, " ");

	loadPositionsFromFEN(&game->state.board, fenToken);

	fenToken = strtok(NULL, " ");
	if (fenToken[0] == 'w') {
		game->state.flags |= GAME_FLAG_WHITE_MOVES;
	}
	else {
		game->state.flags &= ~GAME_FLAG_WHITE_MOVES;
	}

	fenToken = strtok(NULL, " ");
	loadCastlingStateFromFEN(&game->state, fenToken);

	fenToken = strtok(NULL, " ");
	loadEnPassantStateFromFEN(&game->state, fenToken);

	// half move clock
	fenToken = strtok(NULL, " ");
	game->state.flags |= ((int)fenToken & 0xFF) << 22;

	fenToken = strtok(NULL, " ");
	int fullMoves = atoi(fenToken);
	game->moveCount = fullMoves * 2;

	return 1;
}
