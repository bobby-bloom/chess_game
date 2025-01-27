#include <stdio.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#include "game.h"

#define DEPTH_COUNT 2
#define NUM_THREADS 64

static void printComputationResults(int depthCount, uint64_t* cMoveArray, uint64_t* cCheckArray, int cpu_time) {
	printf("\n\n");
	printf("/#################################################/\n");
	printf("//###########/ - Computation result - /###########/\n");
	printf("/#################################################/\n\n");

	printf("Half-moves \t| Checkmates    | Number of possible positions\n");

	for (int i = 0; i < depthCount; i++) {
		printf("%15i | %13I64d | %28I64d\n", i + 1, cCheckArray[i], cMoveArray[i]);
	}

	printf("\n");
	printf("CPU time: \t  %f in minutes.\n", cpu_time / 60);
	printf("CPU time: \t  %i in seconds.\n", cpu_time);
}

typedef struct shannon_thread_data {
	GameInstance* game;
	int startIdx;
	int endIdx;
	int depth;
	int maxDepth;
	uint64_t cMoveArray[DEPTH_COUNT];
	uint64_t cCheckArray[DEPTH_COUNT];
} ShannonThreadData;

static void iterateAndMoveThreaded(void* args) {
	ShannonThreadData* data = (ShannonThreadData*)args;
	for (int i = 0; i < data->endIdx - data->startIdx; i++) {
		for (int j = 0; j < data->game->state.validMoves[data->startIdx + i].size; j++) {
			iterateAndMove(
				data->game,
				data->game->state.validMoves[data->startIdx + i].moves[j],
				data->depth,
				data->maxDepth,
				data->cMoveArray,
				data->cCheckArray
			);
		}
	}
}

void runShannonComputationParallel() {
	GameInstance game;
	initializeGameFromFEN(&game, startPos);

	uint64_t cMoveArray[DEPTH_COUNT] = { 0 };
	uint64_t cCheckArray[DEPTH_COUNT] = { 0 };

	cMoveArray[0] = countPossibleMoves(game.state.validMoves);

	pthread_t threads[NUM_THREADS];
	ShannonThreadData threadData[NUM_THREADS];

	int tilesPerThread = BOARD_TILES / NUM_THREADS;
	int i;
	for (i = 0; i < NUM_THREADS; i++) {
		int startIdx = i * tilesPerThread;
		int endIdx = (i == (NUM_THREADS - 1)) ? BOARD_TILES - 1 : startIdx + tilesPerThread;

		threadData[i].game = &game;
		threadData[i].startIdx = startIdx;
		threadData[i].endIdx = endIdx;
		threadData[i].depth = 1;
		threadData[i].maxDepth = DEPTH_COUNT;

		memset(threadData[i].cMoveArray, 0, sizeof(uint64_t) * DEPTH_COUNT);
		memset(threadData[i].cCheckArray, 0, sizeof(uint64_t) * DEPTH_COUNT);

		pthread_create(&threads[i], NULL, iterateAndMoveThreaded, &threadData[i]);
	}

	for (i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	for (i = 0; i < NUM_THREADS; i++) {
		for (int d = 0; d < DEPTH_COUNT; d++) {
			cMoveArray[d] += threadData[i].cMoveArray[d];
			cCheckArray[d] += threadData[i].cCheckArray[d];
		}
	}

	printComputationResults(DEPTH_COUNT, cMoveArray, cCheckArray, NULL);
}

void runShannonComputation() {
	clock_t time = clock();

	GameInstance game;
	initializeGameFromFEN(&game, startPos);

	uint64_t cMoveArray[DEPTH_COUNT] = { 0 };
	uint64_t cCheckArray[DEPTH_COUNT] = { 0 };

	cMoveArray[0] = countPossibleMoves(game.state.validMoves);

	for (int i = 0; i < BOARD_TILES; i++) {
		for (int j = 0; j < game.state.validMoves[i].size; j++) {
			iterateAndMove(&game, game.state.validMoves[i].moves[j], 1, DEPTH_COUNT, cMoveArray, cCheckArray);
		}
	}

	time = clock() - time;
	double cpu_time = ((double)time) / CLOCKS_PER_SEC;

	printComputationResults(DEPTH_COUNT, cMoveArray, cCheckArray, cpu_time);
}
