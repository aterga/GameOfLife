#include "game_of_life.h"

GameOfLife::GameOfLife(int X, int Y, int N_nodes, int N_generations)
: n_nodes_ (N_nodes), n_x_nodes_ (0), n_y_nodes_ (0), n_gens_ (N_generations)
{
    field_ = new Matrix(X, Y);
    node_x_size_ = new std::map<std::pair<int, int>, int>;
    node_y_size_ = new std::map<std::pair<int, int>, int>;
    redundant_nodes_ = new std::map<int, bool>;
    
    init();
	print();
    linear_split();
}

GameOfLife::GameOfLife(const Matrix &mat, int N_nodes, int N_generations)
: n_nodes_ (N_nodes), n_x_nodes_ (0), n_y_nodes_ (0), n_gens_ (N_generations)
{
    field_ = new Matrix(mat);
    node_x_size_ = new std::map<std::pair<int, int>, int>;
    node_y_size_ = new std::map<std::pair<int, int>, int>;
    redundant_nodes_ = new std::map<int, bool>;
    	
    print();
    linear_split();
}

GameOfLife::~GameOfLife()
{
    delete field_;
    delete node_x_size_;
    delete node_y_size_;
    delete redundant_nodes_;
    printf("I'm (%d) here (RRRR)!\n", 0);
}

void GameOfLife::init()
{
    srand(time(0));

    for (int y = 0; y < field_->size_y(); y ++)
    {
    	for (int x = 0; x < field_->size_x(); x ++)
    	{
    		if (rand() % 2) field_->set(x, y, ALIVE);
    		else            field_->set(x, y, DEAD);
    	}
    }
}

void GameOfLife::collect()
{
    std::map<std::pair<int, int>, Node> *nodes_;
    
    int x_inc = 0, y_inc = 0, rank = 0;

    Matrix *submat = NULL;
    
    printf("I'm (%d) here (J)!\n", rank);
    		
    for (int n_y = 0; n_y < n_y_nodes_; n_y ++)
    {
    	for (int n_x = 0; n_x < n_x_nodes_; n_x ++)
    	{
    		rank = n_y * n_x_nodes_ + n_x;
    		
    		if ((*redundant_nodes_)[rank]) continue;
    	
    		//printf("I'm (%d) here (K)!\n", rank);
    			
    		int x_size = (*node_x_size_)[std::pair<int, int>(n_x, n_y)],
    			y_size = (*node_y_size_)[std::pair<int, int>(n_x, n_y)];
    			
    		if (x_size * y_size == 0) continue;
    			
    		//printf(">>> Node #%d@(%dX%d) is as large as (%dX%d)\n", rank, n_x, n_y, x_size, y_size);
    		//if (x_size == 0) printf(">>> Node #%d is redundant\n", rank);

    		LIFE *loc_job = alloc_mass(x_size * y_size, DEAD);
    		
    		//printf(">>> Main Node: waiting for Node #%d\n", rank);
    		MPI_Recv(loc_job, x_size * y_size, MPI_INT, rank, MASS + rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    		submat = new Matrix(x_size, y_size, loc_job);
    		
    		//printf("I'm (%d) here (L)!\n", rank);
    	
    		free(loc_job);
    		
    		//submat->print();
    						
    		for (int y = 0; y < y_size; y ++)
    		{
    			for (int x = 0; x < x_size; x ++)
    			{
    				field_->set(x + x_inc, y + y_inc, submat->get(x, y));
    			}
    		}
    		
    		//printf("I'm (%d) here (M)!\n", rank);
    		
    		if (rank != 0 && rank % n_x_nodes_ == 0)
    		{
    			x_inc = 0;
    			y_inc += y_size;
    			
    			//printf("I'm (%d) here (N)!\n", rank);
    		}
    		else
    		{
    			x_inc += x_size;
    			//printf("I'm (%d) here (O)!\n", rank);
    		}
    		
    		//printf("I'm (%d) here (P)!\n", rank);
    		
    		delete submat;
    	}
    }
    
    printf("I'm (%d) here (Q)!\n", rank);
}

void GameOfLife::send_init_data(int target_rank, int x_pos, int y_pos, int x_size, int y_size, int *neighbors, LIFE *jbuf)
{    
	bool red = (*redundant_nodes_)[target_rank];
    printf("Node#%d is %s\n", target_rank, red ? "redundant!" : "not redundant!");
    if (red)
    {
    	MPI_Send(&red, 1, MPI_INT, target_rank, REDUNDANCE, MPI_COMM_WORLD);
    	return;	
    }
    MPI_Send(&red, 1, MPI_INT, target_rank, REDUNDANCE, MPI_COMM_WORLD);

    //printf(">>> Main Node: sending following data to Node #%d: {x_pos=%d, y_pos=%d, x_size=%d, y_size=%d}\n", target_rank, x_pos, y_pos, x_size, y_size);
    // Job split data
    MPI_Send(&x_pos, 1, MPI_INT, target_rank, NODE_X_POS, MPI_COMM_WORLD);
    MPI_Send(&y_pos, 1, MPI_INT, target_rank, NODE_Y_POS, MPI_COMM_WORLD);
    MPI_Send(&x_size, 1, MPI_INT, target_rank, NODE_X_SIZE, MPI_COMM_WORLD);
    MPI_Send(&y_size, 1, MPI_INT, target_rank, NODE_Y_SIZE, MPI_COMM_WORLD);
    MPI_Send(neighbors, N_NEIGHBORS, MPI_INT, target_rank, NEIGHBORS, MPI_COMM_WORLD);
    MPI_Send(jbuf, x_size * y_size, MPI_INT, target_rank, MASS, MPI_COMM_WORLD);
    
    // Global data
    MPI_Send(&n_x_nodes_, 1, MPI_INT, target_rank, NODE_MAX_X_POS, MPI_COMM_WORLD);
    MPI_Send(&n_y_nodes_, 1, MPI_INT, target_rank, NODE_MAX_Y_POS, MPI_COMM_WORLD);
    MPI_Send(&n_gens_, 1, MPI_INT, target_rank, N_GENERATIONS, MPI_COMM_WORLD);
}

int GameOfLife::find_neighbor(int my_x, bool right_not_left)
{ // Geekie Coddie
    int nei = right_not_left ? (my_x + 1) % n_x_nodes_
                             : (my_x != 0 ? (my_x - 1) : n_x_nodes_ - 1);
    if (my_x == 10) printf("            my_x = 10, nei = %d, n_x_nodes_ = %d, (my_x + 1) mod n_x_nodes_ = %d, (*redundant_nodes_)[nei] = %d\n",
                           nei, n_x_nodes_, (my_x + 1) % n_x_nodes_, (*redundant_nodes_)[nei]);
    if ((*redundant_nodes_)[nei]) return find_neighbor(right_not_left ? my_x+1 : my_x-1, right_not_left);
    return nei;
}

void GameOfLife::linear_split()
{

    // Strategy:
    n_x_nodes_ = n_nodes_;
    printf("     n_x_nodes_ = %d\n", n_x_nodes_);
    n_y_nodes_ = 1;

    int workload_per_node = 0;
    int zero_node_workload = 0;
    
	std::map<int, Matrix *> jbuf;
    
    if (n_nodes_ > field_->size_x())
    {
    	workload_per_node = (int) ( field_->size_x() / n_nodes_ );
    	zero_node_workload = 0;
    }
    else if (n_nodes_ > 1)
    {
    	workload_per_node = (int) (field_->size_x() / n_nodes_) * (n_nodes_ / (n_nodes_ - 1));
    	zero_node_workload = field_->size_x() - (n_nodes_ - 1) * workload_per_node;
    }
    else
    {
    	workload_per_node = 0;
    	zero_node_workload = field_->size_x();
    }
    
    int workload = workload_per_node;

    // n = 0
    if (zero_node_workload > 0)
    {
    	jbuf[0] = new Matrix(zero_node_workload + 2, field_->size_y() + 2);
    
    	for (int y = -1; y < field_->size_y() + 1; y ++)
    	{
    		//printf("Y#%2d: ", y);
    		for (int x = -1; x < zero_node_workload + 1; x ++)
    		{
    			if (field_->get(x>=0? (x < field_->size_x()? x : 0)
    							   : field_->size_x()-1,
    							y>=0? (y < field_->size_y()? y : 0)
    							   : field_->size_y()-1) == ALIVE)
    			{
    				 (jbuf[0])->set(x + 1, y + 1, ALIVE);
    			} 
    			else (jbuf[0])->set(x + 1, y + 1, DEAD);
    		}
    	}        
    	    
    	(*node_x_size_)[std::pair<int, int>(0, 0)] = zero_node_workload;
    	(*node_y_size_)[std::pair<int, int>(0, 0)] = field_->size_y();
    	
        (*redundant_nodes_)[0] = false;
    }
    else
    {
        (*redundant_nodes_)[0] = true;
    }
    

    
    printf("zero_node_workload = %d\nworkload_per_node = %d\n", zero_node_workload, workload_per_node);

    if (workload_per_node >= 0)
    {   
        int n_actual_nodes = n_nodes_;
        if (workload == 0)
        {
            workload = 1;
            n_actual_nodes = field_->size_x() - zero_node_workload;
        }
        printf("field_->size_x() = %d\n", field_->size_x());
        printf("workload = %d\n", workload);
        printf("n_actual_nodes = %d\n", n_actual_nodes);
        
        for (int n = 1; n < n_actual_nodes + 1; n ++)
        {
        	jbuf[n] = new Matrix(workload + 2, field_->size_y() + 2);
        	
        	for (int y = -1; y < field_->size_y() + 1; y ++)
        	{
        		for (int loc_x = 0,
        				 x = zero_node_workload + (n - 1) * workload - 1;
        				 x < zero_node_workload +  n      * workload + 1;
        			 loc_x ++, x ++)
        		{
        	
        		/*
        		printf(">>> Params: {x = %d, y = %d,  get(x, y) = get(%d, %d)}\n",
        			   x, y, x>field_->size_x()-1? 0 : x, y>=0? (y < field_->size_y()? y : 0)
        							: (field_->size_y()-1));
        		printf(">>> Param: {workload_per_node = %d, zero_node_workload = %d}\n", workload_per_node, zero_node_workload);
        		*/
        	
        			if (field_->get(x>=0? (x < field_->size_x()? x : 0)
        								: field_->size_x()-1,
        							y>=0? (y < field_->size_y()? y : 0)
        								: field_->size_y()-1) == ALIVE)
        			{
        				jbuf[n]->set(loc_x, y + 1, ALIVE);
        			}
        			else
        			{
        				jbuf[n]->set(loc_x, y + 1, DEAD);
        			}
        		}
        	
        	}
        	    
        	(*node_x_size_)[std::pair<int, int>(n, 0)] = workload;
        	(*node_y_size_)[std::pair<int, int>(n, 0)] = field_->size_y();
                	        	
            (*redundant_nodes_)[n] = false;
        	
        }
        
        for (int n = n_actual_nodes + 1; n < n_nodes_; n ++)
        {
        	printf("Extra nodes found! (#%d)\n", n);
            (*redundant_nodes_)[n] = true;
        }
    }
    else
    {
        for (int n = 1; n < n_nodes_; n ++)
        {
            (*redundant_nodes_)[n] = true;
        }
    }
    
    // n = 0
    for (int n = 0; n < 1; n ++)
    {
    	if ((*redundant_nodes_)[0])
    	{
	        send_init_data(0, 0, 0, 0, 0, NULL, NULL);
	    	continue;    
	    }

		int *neighbors = new int[N_NEIGHBORS]();
		
		neighbors[TOP] = 0;
		neighbors[TOP_RIGHT] = n_x_nodes_ > 1 ? 1 : 0;
		neighbors[RIGHT] = n_x_nodes_ > 1 ? 1 : 0;
		neighbors[BOTTOM_RIGHT] = n_x_nodes_ > 1 ? 1 : 0;
		neighbors[BOTTOM] = 0;
		neighbors[BOTTOM_LEFT] = n_x_nodes_ - 1;
		neighbors[LEFT] = n_x_nodes_ - 1;
		neighbors[TOP_LEFT] = n_x_nodes_ - 1;
		
		send_init_data(0, 0, 0, zero_node_workload + 2, field_->size_y() + 2, neighbors, (jbuf[0])->data());
		
		delete neighbors;
		delete jbuf[0];
	}

    for (int n = 1; n < n_nodes_; n ++)
    {
    	if ((*redundant_nodes_)[n])
    	{
	        send_init_data(n, n, 0, 0, 0, NULL, NULL);
	    	continue;    
	    }    

        int *neighbors = new int[N_NEIGHBORS]();
        
		int right_nei = find_neighbor(n, true);
		int left_nei = find_neighbor(n, false);
        	
		neighbors[TOP] = n;
		neighbors[BOTTOM] = n;
		
		neighbors[TOP_RIGHT] = right_nei;
		neighbors[RIGHT] = right_nei;
		neighbors[BOTTOM_RIGHT] = right_nei;

		neighbors[BOTTOM_LEFT] = left_nei;
		neighbors[LEFT] = left_nei;
		neighbors[TOP_LEFT] = left_nei;
		
		send_init_data(n, n, 0, workload + 2, field_->size_y() + 2, neighbors, (jbuf[n])->data());
	
		delete neighbors;
		//delete jbuf[n];
    }
}

void GameOfLife::grid_split()
{

}