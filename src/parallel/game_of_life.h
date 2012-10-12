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
    std::map<int, bool> *redundant_nodes_;
    
    double start_time_, end_time_;

	inline const int node_x(const int rank) const { return       rank % n_x_nodes_; }
	inline const int node_y(const int rank) const { return (int) rank / n_x_nodes_; }
	inline const int node_rank(const int x, const int y) const { return y * n_x_nodes_ + x; }

	inline const int x_size(const int x, const int y) const { return (*node_x_size_)[std::pair<int, int>(x, y)]; }
	inline const int y_size(const int x, const int y) const { return (*node_y_size_)[std::pair<int, int>(x, y)]; }
	inline const int size(const int x, const int y) const { return x_size(x, y) * y_size(x, y); }		
	
	inline void set_x_size(const int x, const int y, const int x_size) { (*node_x_size_)[std::pair<int, int>(x, y)] = x_size; }
	inline void set_y_size(const int x, const int y, const int y_size) { (*node_x_size_)[std::pair<int, int>(x, y)] = y_size; }	
	
	inline const int x_size(const int rank) const { return (*node_x_size_)[std::pair<int, int>(node_x(rank), node_y(rank))]; }
	inline const int y_size(const int rank) const { return (*node_y_size_)[std::pair<int, int>(node_x(rank), node_y(rank))]; }
	
	inline void set_x_size(const int rank, const int x_size) { (*node_x_size_)[std::pair<int, int>(node_x(rank), node_y(rank))] = x_size; }
	inline void set_y_size(const int rank, const int y_size) { (*node_y_size_)[std::pair<int, int>(node_x(rank), node_y(rank))] = y_size; }	

	inline const int x_size() const { return field_->size_x(); }
	inline const int y_size() const { return field_->size_y(); }
	inline const int size() const { return field_->size_x() * field_->size_y(); }	
	
	inline const bool set_redundance(const int rank) { return (*redundant_nodes_)[rank] = x_size(rank) * y_size(rank) == 0 ? true : false; }
	inline const bool is_redundant(const int rank) const { return (*redundant_nodes_)[rank]; }
	inline const bool is_redundant(const int x, const int y) const { return (*redundant_nodes_)[node_rank(x, y)]; }
	
    void init();
    void generate_random();
    inline void send_init_data(int target_rank, int x_size, int y_size, int *neighbors, LIFE *data);
    inline void send_init_data(int target_rank, int x_work, int y_work, LIFE *data);
                                                
    void linear_split();
    
	inline int find_neighbor(int my_x, int my_y, NEIGHBOR nei_dir);    
    void grid_split();
    
    void collect();

public:
    GameOfLife(int X, int Y, int N_nodes, int N_generations);
    GameOfLife(const Matrix &mat, int N_nodes, int N_generations);
	~GameOfLife();

	void print(const char* state = "current")
	{
		printf(">>> The %s state of the World ", state);
		field_->print();
		printf("\n");
	}

	void end()
	{
		collect();
		end_time_ = MPI_Wtime();
		print("final");
		printf(">>> Computation time: %20f(sec)\n", end_time_ - start_time_);
		delete this;
	}
};

#include "game_of_life.cpp"

#endif // _GAME_OF_LIFE_H_
