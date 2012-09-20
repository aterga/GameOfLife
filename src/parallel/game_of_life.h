#ifndef _GAME_OF_LIFE_H_
#define _GAME_OF_LIFE_H_

#include <stddef.h>
#include <utility>
#include <vector>
#include <map>

#include "life_tools.h"
#include "node.h"

class GameOfLife {

private:
	Matrix *field_;
    int n_nodes_, n_gens_, n_x_nodes_, n_y_nodes_;
    std::map<std::pair<int, int>, int> *node_x_size_;
    std::map<std::pair<int, int>, int> *node_y_size_;

    void init();
    inline void send_init_data(int target_rank, int x_pos,  int y_pos,
                                                int x_size, int y_size, LIFE *data);
    void linear_split();
    void grid_split();
    
    void collect();

public:
    GameOfLife(int X, int Y, int N_nodes, int N_generations);
    GameOfLife(const Matrix &mat, int N_nodes, int N_generations);
	~GameOfLife();

	void print()
	{
		printf(">>> The current state of the World ");
		field_->print();
		printf("\n");
	}

	void end()
	{
		//printf("I'm (%d) here (R)!\n", 0);
		collect();
		print();
		delete this;
	}
};

#include "game_of_life.cpp"

#endif _GAME_OF_LIFE_H_
