#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <vector>
#include <ctime>

//
// CONVENTIONS
//
// 'r' == should raid
// 's' == should stake
// 'n' == no available action

//
// GLOBALS/CONSTANTS
//

int N;
int SIZE;
char MODE[20];
char US, THEM;
char FREE = '.';
int N_FREE;
float TIME_LEFT;
float TIME_MAX;
int * VALUES;
char * START_BOARD;
int INF = 2000000000;
int OUT_OF_BOUNDS = 2000000000;
int MAX_DEPTH = 20;

//
// FORWARD DECLARATIONS
//

int max_value(char board[], int n_free, int depth, int a, int b);
int min_value(char board[], int n_free, int depth, int a, int b);

//
// TIME
//

bool should_stop() {
	return (std::clock() / (double)(CLOCKS_PER_SEC)) >= TIME_MAX;
}

//
// POSITION
//

int get_index(int row, int col) { return N * row + col; }

int row(int index) { return index / N; }

int col(int index) { return index % N; }

void letnum(int row, int col, char * position) {
	position[0] = 'A' + col;
	position[1] = '0' + row + 1;
}

int up(int index, int spaces) {
	int r = row(index);
	int c = col(index);
	r -= spaces;
	return (r >= 0) ? (get_index(r, c)) : OUT_OF_BOUNDS;
}

int down(int index, int spaces) {
	int r = row(index);
	int c = col(index);
	r += spaces;
	return (r < SIZE) ? (get_index(r, c)) : OUT_OF_BOUNDS;
}

int left(int index, int spaces) {
	int r = row(index);
	int c = col(index);
	c -= spaces;
	return (c >= 0) ? (get_index(r, c)) : OUT_OF_BOUNDS;
}

int right(int index, int spaces) {
	int r = row(index);
	int c = col(index);
	c += spaces;
	return (c < SIZE) ? (get_index(r, c)) : OUT_OF_BOUNDS;
}

//
// INPUT/OUTPUT
//

void retrieve_arguments(std::ifstream & inf, int & N, char * MODE,
						char & US, char & THEM, float & TIME_LEFT) {
	inf >> N;
	inf >> MODE;
	inf >> US;
	THEM = US == 'O' ? 'X' : 'O';
	inf >> TIME_LEFT;
}

void retrieve_board_values(std::ifstream & inf, int v[], int N) {
	for (int i = 0; i < SIZE; ++i) {
		inf >> v[i];
	}
}

int retrieve_board_start_state(std::ifstream & inf, char s[], int N) {
	int n_free = 0;
	char line[N+1];
	for (int i = 0; i < N; ++i) {
		inf >> line;
		for (int j = 0; j < N; ++j) {
			s[i*N+j] = line[j];
			if (line[j] == FREE) { ++n_free; }
		}
	}
	return n_free;
}

void write_output(int index, char action, char state[]) {
	std::ofstream outf("output.txt");
	char position[3];
	position[2] = NULL;
	letnum(row(index), col(index), position);
	outf << position << ' ';
	if (action == 's') {
		outf << "Stake";
	}
	else {
		outf << "Raid";
	}
	for (int i = 0; i < N; ++i) {
		outf << std::endl;
		for (int j = 0; j < N; ++j) {
			outf << state[i*N+j];
		}
	}
}

void print_debug(int depth, char player, int position, char action, char next_board[]) {
	std::cout << "Dp" << depth << " -- " << player << " -- " << position << action << std::endl;
	for (int i = 0; i < N; ++i) {
		for (int j = 0; j < N; ++j) {
			std::cout << next_board[N*i+j];
		}
		std::cout << std::endl;
	}
}

//
// GAME ACTIONS
//

void raid(char board[], int position, char player) {
	char other = (player == US) ? THEM : US;
	board[position] = player;
	int u = up(position, 1);
	int d = down(position, 1);
	int l = left(position, 1);
	int r = right(position, 1);
	if (u != OUT_OF_BOUNDS && board[u] == other) { board[u] = player; }
	if (d != OUT_OF_BOUNDS && board[d] == other) { board[d] = player; }
	if (l != OUT_OF_BOUNDS && board[l] == other) { board[l] = player; }
	if (r != OUT_OF_BOUNDS && board[r] == other) { board[r] = player; }
}

int raidability(const char board[], int position, char player) {
	// returns -1 if not raidable
	// returns gained score * 1 + num opponent adjacent if raidable
	if (board[position] != FREE) {
		return -1;
	}
	char other = (player == US) ? THEM : US;
	int player_adjacent = 0;
	int opponent_adjacent = 0;
	int gain = VALUES[position];
	int u = up(position, 1);
	int d = down(position, 1);
	int l = left(position, 1);
	int r = right(position, 1);
	if (u != OUT_OF_BOUNDS) {
		if (board[u] == player) { ++player_adjacent; }
		else if (board[u] == other) { ++opponent_adjacent; gain += VALUES[u]; }
	}
	if (d != OUT_OF_BOUNDS) {
		if (board[d] == player) { ++player_adjacent; }
		else if (board[d] == other) { ++opponent_adjacent; gain += VALUES[d]; }
	}
	if (l != OUT_OF_BOUNDS) {
		if (board[l] == player) { ++player_adjacent; }
		else if (board[l] == other) { ++opponent_adjacent; gain += VALUES[l]; }
	}
	if (r != OUT_OF_BOUNDS) {
		if (board[r] == player) { ++player_adjacent; }
		else if (board[r] == other) { ++opponent_adjacent; gain += VALUES[r]; }
	}
	return (player_adjacent == 0) ? -1 : gain * (1 + opponent_adjacent);
}

//
// EVALUATION
//

int evaluate(char board[]) {
	int score = 0;
	for (int i = 0; i < SIZE; ++i) {
		if (board[i] == US) { score += VALUES[i]; }
		else if (board[i] == THEM) { score -= VALUES[i]; }
	}
	return score;
}

//
// ALPHA-BETA PRUNING
//

int get_moves(const char board[], char player,
			   int priority[], char action[]) {
	// prioritize higher raidability; then high value stake?
	// returns n_free
	int n_free = 0;
	int ratings[SIZE];
	for (int i = 0; i < SIZE; ++i) {
		int raid_score = raidability(board, i, player);
		int stake_score = (board[i] == FREE) ? VALUES[i] : -1;
//		std::cout << raid_score << '/' << stake_score << ' ';
		n_free += (board[i] == FREE) ? 1 : 0;
		if (raid_score < 0 && stake_score < 0) {
			ratings[i] = -1;
			action[i] = 'n';
		}
		else if (raid_score >= stake_score) {
			ratings[i] = raid_score;
			action[i] = 'r';
		}
		else {
			ratings[i] = stake_score;
			action[i] = 's';
		}
	}
	// sort moves
	std::vector<int> ratings_vector(ratings, ratings + sizeof ratings / sizeof ratings[0]);
	std::vector<int> ranking(ratings_vector.size());
	std::size_t count(0);
	std::generate(std::begin(ranking), std::end(ranking), [&]{ return count++; });
	std::sort(std::begin(ranking), std::end(ranking),
			  [&](int x, int y){ return ratings_vector[x] > ratings_vector[y]; });
	for (int i = 0; i < SIZE; ++i) {
		priority[i] = ranking[i];
		if (action[ranking[i]] == 'n') { priority[i] = -1; }
	}
//	for (int i = 0; i < SIZE; ++i) {
//		std::cout << priority[i] << ' ';
//	}
//	std::cout << std::endl;
	return n_free;
}

int max_value(char board[], int n_free, int depth, int a, int b) {
	if (n_free == 0 || depth == 0 || should_stop()) {
//		std::cout << "LEAF NODE ==> " << evaluate(board) << std::endl;
		return evaluate(board);
	}
	int v = -INF;
	int priority[SIZE];
	char action[SIZE];
	get_moves(board, US, priority, action);
	for (int i = 0; i < SIZE && priority[i] >= 0; ++i) {
		char next_board[SIZE];
		for (int j = 0; j < SIZE; ++j) {
			next_board[j] = board[j];
		}
		if (action[priority[i]] == 'r') {
			raid(next_board, priority[i], US);
//			print_debug(depth, US, priority[i], 'r', next_board);
		}
		else {
			next_board[priority[i]] = US;
//			print_debug(depth, US, priority[i], 's', next_board);
		}
		int w = min_value(next_board, n_free-1, depth-1, a, b);
		v = (w > v) ? w : v;
		if (v >= b) {
//			std::cout << v << " >= " << b << " So Pruned" << std::endl;
			return v;
		}
		a = (v > a) ? v : a;
	}
	return v;
}

int min_value(char board[], int n_free, int depth, int a, int b) {
	if (n_free == 0 || depth == 0 || should_stop()) {
//		std::cout << "LEAF NODE ==> " << evaluate(board) << std::endl;
		return evaluate(board);
	}
	int v = INF;
	int priority[SIZE];
	char action[SIZE];
	get_moves(board, THEM, priority, action);
	for (int i = 0; i < SIZE && priority[i] >= 0; ++i) {
		char next_board[SIZE];
		for (int j = 0; j < SIZE; ++j) {
			next_board[j] = board[j];
		}
		if (action[priority[i]] == 'r') {
			raid(next_board, priority[i], THEM);
//			print_debug(depth, THEM, priority[i], 'r', next_board);
		}
		else {
			next_board[priority[i]] = THEM;
//			print_debug(depth, THEM, priority[i], 's', next_board);
		}
		int w = max_value(next_board, n_free-1, depth-1, a, b);
		v = (w < v) ? w : v;
		if (v <= a) {
//			std::cout << v << " <= " << a << " So Pruned" << std::endl;
			return v;
		}
		b = (v < b) ? v : b;
	}
	return v;
}

void decide(int * best_position, char * best_action) {
	// iterative deepening; best at each depth
	*best_position = -1;
	*best_action = 'n';
	int best_value = -INF;
	int old_best_position = -1;
	char old_best_action = 'n';
	int old_best_value = -INF;
	for (int d = 1; d < MAX_DEPTH; ++d) {
//		std::cout << "START DEPTH " << d << std::endl;
		old_best_position = *best_position;
		old_best_action = *best_action;
		old_best_value = best_value;
//		std::cout << "UPDATE START " << "Dp" << d << " : " << old_best_position << old_best_action << " FOR " << old_best_value << std::endl;
		*best_position = -1;
		*best_action = 'n';
		best_value = -INF;
//		std::cout << "*** DEPTH *** " << d << std::endl;
		int priority[SIZE];
		char action[SIZE];
		get_moves(START_BOARD, US, priority, action);
//		for (int i = 0; i < SIZE; ++i) {
//			std::cout << priority[i] << action[priority[i]] << std::endl;
//		}
		for (int i = 0; i < SIZE && priority[i] >= 0; ++i) {
			if (should_stop()) {
//				std::cout << "STOPPING AT: " << std::clock() / (double)(CLOCKS_PER_SEC) << std::endl;
				if (old_best_position >= 0) {
					*best_position = old_best_position;
					*best_action = old_best_action;
				}
				else if (*best_position < 0) {
					*best_position = priority[i];
					*best_action = action[priority[i]];
				}
				return;
			}
			// update if half of nodes are checked
			if (i == N_FREE/2 && *best_position >= 0) {
				old_best_position = *best_position;
				old_best_action = *best_action;
				old_best_value = best_value;
//				std::cout << "UPDATE MIDWAY " << "Dp" << d << " : " << old_best_position << old_best_action << " FOR " << old_best_value << std::endl;
			}
			if (i == N_FREE*3/4 && *best_position >= 0) {
				old_best_position = *best_position;
				old_best_action = *best_action;
				old_best_value = best_value;
//								std::cout << "UPDATE ALMOST DONE " << "Dp" << d << " : " << old_best_position << old_best_action << " FOR " << old_best_value << std::endl;
			}
			char next_board[SIZE];
			for (int j = 0; j < SIZE; ++j) {
				next_board[j] = START_BOARD[j];
			}
			if (action[priority[i]] == 'r') {
				raid(next_board, priority[i], US);
//				print_debug(d, US, priority[i], 'r', next_board);
			}
			else {
				next_board[priority[i]] = US;
//				print_debug(d, US, priority[i], 's', next_board);
			}
//			std::cout << priority[i] << '-'<< action[i] << ':' << std::endl;
//			for (int x = 0; x < N; ++x) {
//				for (int y = 0; y < N; ++y) {
//					std::cout << next_board[x*N+y];
//				}
//				std::cout << std::endl;
//			}
			int v = min_value(next_board, N_FREE-1, d-1, -INF, INF);
//			std::cout << priority[i] << action[priority[i]] << " ==> " << v << std::endl;
//			std::cout << "BEST VALUE IS " << best_value << " V IS " << v << std::endl;
			if (v > best_value) {
				best_value = v;
				*best_position = priority[i];
				*best_action = action[priority[i]];
//				std::cout << "CURRENT BEST " << "Dp" << d << " : " << *best_position << *best_action << " FOR " << v << std::endl;
			}
		}
	}
}

int main(int argc, const char * argv[]) {
	// retrieve data
	std::ifstream inf("input.txt");
	retrieve_arguments(inf, N, MODE, US, THEM, TIME_LEFT);
	SIZE = N * N;
	VALUES = new int[SIZE];
	START_BOARD = new char[SIZE];
	retrieve_board_values(inf, VALUES, N);
	N_FREE = retrieve_board_start_state(inf, START_BOARD, N);
	if (N_FREE >= SIZE * 3 / 4) {
		TIME_MAX = TIME_LEFT / 8;
	}
	else {
		TIME_MAX = TIME_LEFT * 0.8f * (1.0f / ((N_FREE + 1) / 2));
	}
	inf.close();

	int best_position;
	char best_action;
	decide(&best_position, &best_action);
	if (best_action == 's') {
		START_BOARD[best_position] = US;
	}
	else {
		raid(START_BOARD, best_position, US);
	}
	write_output(best_position, best_action, START_BOARD);
	std::cout << TIME_LEFT - std::clock() / (double)(CLOCKS_PER_SEC) << std::endl;

    return 0;
}
