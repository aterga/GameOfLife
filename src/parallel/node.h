#ifndef _NODE_H_
#define _NODE_H_

#include <stddef.h>
#include <utility>
#include <vector>
#include <map>

#include "life_tools.h"

enum NEIGHBOR {
	TOP,
	TOP_RIGHT,
	RIGHT,
	BOTTOM_RIGHT,
	BOTTOM,
	BOTTOM_LEFT,
	LEFT,
	TOP_LEFT,
	
	N_NEIGHBORS
};

enum MES_TAGS {
    DEBUG,
    
    NODE_X_POS,
    NODE_Y_POS,
    NODE_X_SIZE,
    NODE_Y_SIZE,
    
    NODE_MAX_X_POS,
    NODE_MAX_Y_POS,
    
    BORDER_TOP_TO_BOTTOM,
    BORDER_RIGHT_TO_LEFT,
    BORDER_BOTTOM_TO_TOP,
    BORDER_LEFT_TO_RIGHT,
    
    ANGLE_SE_TO_NW,
    ANGLE_SW_TO_NE,
    ANGLE_NW_TO_SE,
    ANGLE_NE_TO_SW,
    
    N_GENERATIONS,
    
    MASS,
    NEIGHBORS,
    
    REDUNDANCE
};

class Node {

private:
    Matrix *field_;
    int *neighbors_;
    int     x_pos_,     y_pos_,
        max_x_pos_, max_y_pos_,
        rank_, gen_, n_gens_;
    bool is_redundant_, one_col_, one_row_;
    
    short int count(int x, int y);
    inline void count();
    /*
    inline int rank(int x_pos, int y_pos)
    {
    	return (x_pos < 0? max_x_pos_-1 : (x_pos >= max_x_pos_? 0: x_pos))
    	      +(y_pos < 0? max_y_pos_-1 : (y_pos >= max_y_pos_? 0: y_pos)) * max_x_pos_;
    }
    */
    void send();
    void receive();

public:
    Node(int rank);
    
    ~Node();
    
    void print()
    {
    	printf(">>> Generation %d of Node#%d ", gen_, rank_);
		field_->print();
		printf("\n");
    }

    void revive(int x, int y)
    {
		field_->set(x, y, ALIVE);
    }

    void die(int x, int y)
    {
		field_->set(x, y, DEAD);
    }

    void iterate()
    {
    	if (is_redundant_) return;
    	
    	gen_ ++;
    	
    	//printf(">>> Node's #%d generation %d\n", rank_, gen_);
    	
		count();
		send();
		receive();
    }
    
    const int n_generations()
    {
    	return n_gens_;
    }
    
    void end();
};

#include "node.cpp"

#endif // _NODE_H_
