#include "game_of_life.h"

void GameOfLife::init()
{
    node_x_size_ = new std::map<std::pair<int, int>, int>;
    node_y_size_ = new std::map<std::pair<int, int>, int>;
    redundant_nodes_ = new std::map<int, bool>;
}

GameOfLife::GameOfLife(int X, int Y, int N_nodes, int N_generations)
: n_nodes_ (N_nodes), n_x_nodes_ (0), n_y_nodes_ (0), n_gens_ (N_generations)
{
    field_ = new Matrix(X, Y);
    
    init();
    generate_random();
	print();
    linear_split();
}

GameOfLife::GameOfLife(const Matrix &mat, int N_nodes, int N_generations)
: n_nodes_ (N_nodes), n_x_nodes_ (0), n_y_nodes_ (0), n_gens_ (N_generations)
{
    field_ = new Matrix(mat);
    	
    init();
    print();
    linear_split();
}

GameOfLife::~GameOfLife()
{
    delete field_;
    delete node_x_size_;
    delete node_y_size_;
    delete redundant_nodes_;
}

void GameOfLife::generate_random()
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
    int x_inc = 0, y_inc = 0, rank = 0;

    Matrix *submat = NULL;
        		
    for (int n_y = 0; n_y < n_y_nodes_; n_y ++)
    {
    	for (int n_x = 0; n_x < n_x_nodes_; n_x ++)
    	{
    		rank = n_y * n_x_nodes_ + n_x;
    		
    		if ((*redundant_nodes_)[rank]) continue;
    	    			
    		int x_size = (*node_x_size_)[std::pair<int, int>(n_x, n_y)],
    			y_size = (*node_y_size_)[std::pair<int, int>(n_x, n_y)];
    		
    		LIFE *loc_job = new LIFE[x_size * y_size]();
    		
    		MPI_Recv(loc_job, x_size * y_size, MPI_INT, rank, MASS + rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    		submat = new Matrix(x_size, y_size, loc_job);
    		    	
    		delete loc_job;
    		    						
    		for (int y = 0; y < y_size; y ++)
    		{
    			for (int x = 0; x < x_size; x ++)
    			{
    				field_->set(x + x_inc, y + y_inc, submat->get(x, y));
    			}
    		}
    		    		
    		if (rank != 0 && rank % n_x_nodes_ == 0)
    		{
    			x_inc = 0;
    			y_inc += y_size;
    		}
    		else
    		{
    			x_inc += x_size;
    		}
    		    		
    		delete submat;
    	}
    }
}

void GameOfLife::send_init_data(int target_rank, int x_pos, int y_pos, int x_size, int y_size, int *neighbors, LIFE *jbuf)
{    
	bool red = (*redundant_nodes_)[target_rank];
	
    //printf("Node#%d is %s\n", target_rank, red ? "redundant!" : "not redundant!");
    
    if (red)
    {
    	MPI_Send(&red, 1, MPI_INT, target_rank, REDUNDANCE, MPI_COMM_WORLD);
    	return;	
    }
    MPI_Send(&red, 1, MPI_INT, target_rank, REDUNDANCE, MPI_COMM_WORLD);
    
    MPI_Send(&n_gens_, 1, MPI_INT, target_rank, N_GENERATIONS, MPI_COMM_WORLD);
    MPI_Send(&x_size, 1, MPI_INT, target_rank, NODE_X_SIZE, MPI_COMM_WORLD);
    MPI_Send(&y_size, 1, MPI_INT, target_rank, NODE_Y_SIZE, MPI_COMM_WORLD);
    MPI_Send(neighbors, N_NEIGHBORS, MPI_INT, target_rank, NEIGHBORS, MPI_COMM_WORLD);
    MPI_Send(jbuf, x_size * y_size, MPI_INT, target_rank, MASS, MPI_COMM_WORLD);
}

int GameOfLife::find_neighbor(int my_x, bool right_not_left)
{ // Geekie Coddie
    int nei = right_not_left ? (my_x + 1) % n_x_nodes_
                             : (my_x != 0 ? (my_x - 1) : n_x_nodes_ - 1);
                    
    if ((*redundant_nodes_)[nei]) return find_neighbor(nei, right_not_left);
    return nei;
}

void GameOfLife::linear_split()
{
    // Strategy:
    n_x_nodes_ = n_nodes_;
    n_y_nodes_ = 1;

    int workload_per_node = 0;
    int zero_node_workload = 0;
	int n_actual_nodes = n_nodes_;
	        
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
	if (workload_per_node == 0)
	{
		workload_per_node = 1;
		n_actual_nodes = field_->size_x() - zero_node_workload;
	}
	
	std::map<int, Matrix *> jbuf;
       
	for (int n = 0; n < n_actual_nodes + 1; n ++)
	{
		int workload = n == 0 ? zero_node_workload : workload_per_node;
			
		if (workload == 0)
		{
	        (*redundant_nodes_)[n] = true;
	        continue;
		}
		
		jbuf[n] = new Matrix(workload + 2, field_->size_y() + 2);
		
		for (int y = -1; y < field_->size_y() + 1; y ++)
		{
			for (int loc_x = 0,
					 x = zero_node_workload + (n - 1) * workload - 1;
					 x < zero_node_workload +  n      * workload + 1;
				 loc_x ++, x ++)
			{        	
				if (field_->get(x>=0? (x < field_->size_x()? x : 0)
									: field_->size_x() - 1,
								y>=0? (y < field_->size_y()? y : 0)
									: field_->size_y() - 1) == ALIVE)
				
					jbuf[n]->set(loc_x, y + 1, ALIVE);
				else
					jbuf[n]->set(loc_x, y + 1, DEAD);
			}
		}
			
		(*node_x_size_)[std::pair<int, int>(n, 0)] = workload;
		(*node_y_size_)[std::pair<int, int>(n, 0)] = field_->size_y();
							
		(*redundant_nodes_)[n] = false;
	}
	
	for (int n = n_actual_nodes + 1; n < n_nodes_; n ++)
	{
		(*redundant_nodes_)[n] = true;
	}
    
    for (int n = 0; n < n_nodes_; n ++)
    {
    	if ((*redundant_nodes_)[n])
    	{
	        send_init_data(n, n, 0, 0, 0, NULL, NULL);
	    	continue;    
	    }    

        int *neighbors = new int[N_NEIGHBORS]();
        
		int right_nei = find_neighbor(n, true);
		int left_nei = find_neighbor(n, false);
		        	
		neighbors[   TOP] = n;
		neighbors[BOTTOM] = n;
		
		neighbors[   TOP_RIGHT] = right_nei;
		neighbors[       RIGHT] = right_nei;
		neighbors[BOTTOM_RIGHT] = right_nei;
		
		neighbors[BOTTOM_LEFT] = left_nei;
		neighbors[       LEFT] = left_nei;
		neighbors[   TOP_LEFT] = left_nei;
		
		send_init_data(n, n, 0, (n == 0 ? zero_node_workload : workload_per_node) + 2, field_->size_y() + 2, neighbors, jbuf[n]->data());
	
		delete neighbors;
		delete jbuf[n];
    }
}

void GameOfLife::grid_split()
{

}
