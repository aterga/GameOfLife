#include "game_of_life.h"

void GameOfLife::init()
{
	n_x_nodes_ = 0;
	n_y_nodes_ = 0;
	start_time_ = MPI_Wtime();
	end_time_ = 0;

    node_x_size_ = new std::map<std::pair<int, int>, int>;
    node_y_size_ = new std::map<std::pair<int, int>, int>;
    redundant_nodes_ = new std::map<int, bool>;
}

GameOfLife::GameOfLife(int X, int Y, int N_nodes, int N_generations)
: n_nodes_ (N_nodes), n_gens_ (N_generations)
{
    field_ = new Matrix(X, Y);
    
    init();
    generate_random();
	print("initial");
    //linear_split();
    grid_split();
}

GameOfLife::GameOfLife(const Matrix &mat, int N_nodes, int N_generations)
: n_nodes_ (N_nodes), n_gens_ (N_generations)
{
    field_ = new Matrix(mat);
    	
    init();
    print("initial");
    //linear_split();
    grid_split();
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
    		
    		{
				LIFE *loc_job = new LIFE[x_size * y_size]();
				
				MPI_Recv(loc_job, x_size * y_size, MPI_INT, rank, MASS + rank, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	
				submat = new Matrix(x_size, y_size, loc_job);
						
				delete loc_job;
    		}
    		    		    						
    		for (int y = 0; y < y_size; y ++)
    		{
    			for (int x = 0; x < x_size; x ++)
    			{
    				field_->set(x + x_inc, y + y_inc, submat->get(x, y));
    			}
    		}
    		    		
    		if ((rank + 1) % n_x_nodes_ == 0)
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

int GameOfLife::find_neighbor(int my_x, int my_y, NEIGHBOR nei_dir)
{
// Generalized Geekie Coddie
    switch (nei_dir)
    {
    	case TOP:
    		my_y = (my_y != 0 ? (my_y - 1) : n_y_nodes_ - 1);
    		break;
    	case RIGHT:
    		my_x = (my_x + 1) % n_x_nodes_;
    		break;
    	case BOTTOM:
    		my_y = (my_y + 1) % n_y_nodes_;
    		break;
    	case LEFT:
    		my_x = (my_x != 0 ? (my_x - 1) : n_x_nodes_ - 1);
    		break;
    	case TOP_RIGHT:
    		my_x = (my_x + 1) % n_x_nodes_;
    		my_y = (my_y != 0 ? (my_y - 1) : n_y_nodes_ - 1);
    		break;
    	case BOTTOM_RIGHT:
    		my_x = (my_x + 1) % n_x_nodes_;
    		my_y = (my_y + 1) % n_y_nodes_;
    		break;
    	case BOTTOM_LEFT:
    		my_x = (my_x != 0 ? (my_x - 1) : n_x_nodes_ - 1);
    		my_y = (my_y + 1) % n_y_nodes_;
    		break;
    	case TOP_LEFT:
    		my_x = (my_x != 0 ? (my_x - 1) : n_x_nodes_ - 1);
    		my_y = (my_y != 0 ? (my_y - 1) : n_y_nodes_ - 1);
    		break;
    	default:
    		printf(">>> Error: wrong neighbor direction sent to find_neighbor function.\n");
    		return -1;
    }
                                                    
	int nei_rank = my_y * n_x_nodes_ + my_x;
	
    if ((*redundant_nodes_)[nei_rank])
    	return find_neighbor(my_x, my_y, nei_dir);
    
    return nei_rank;
}

inline void GameOfLife::send_init_data(int target_rank, int x_size, int y_size, int *neighbors, LIFE *jbuf)
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

inline void GameOfLife::send_init_data(int target_rank, int x_work, int y_work, LIFE *data)
{
	if ((*redundant_nodes_)[target_rank])
	{
		send_init_data(target_rank, 0, 0, NULL, NULL);
		printf("I'm here! (A)\n");
		return;
	}
	else
	{
		printf("target_rank = %d\n", target_rank);
	
		int *neighbors = new int[N_NEIGHBORS]();		    
						
		for (int i = 0; i < N_NEIGHBORS; i ++)
		{
			neighbors[(NEIGHBOR) i] = find_neighbor(node_x(target_rank), node_y(target_rank), (NEIGHBOR) i);
		}
		
		send_init_data(target_rank, x_work, y_work, neighbors, data);
	
		delete neighbors;
	}
}

void GameOfLife::linear_split()
{
    // Strategy:
    n_x_nodes_ = n_nodes_;
    n_y_nodes_ = 1;

    int workload_per_node = 0;
    int zero_node_workload = 0;
	int n_actual_nodes = n_nodes_;
	        
	// Taking into account the marginal cases:
    if (n_nodes_ > field_->size_x())
    {
    	workload_per_node = (int) ( field_->size_x() / n_nodes_ );
    	zero_node_workload = 0;
    	printf("\t\tI'm here! (1)\n");
    }
    else if (n_nodes_ > 1)
    {
    	workload_per_node = (int) (field_->size_x() / n_nodes_) * (n_nodes_ / (n_nodes_ - 1));
    	zero_node_workload = field_->size_x() - (n_nodes_ - 1) * workload_per_node;
    	printf("\t\tI'm here! (2)\n");
    }
    else
    {
    	workload_per_node = 1;
    	zero_node_workload = field_->size_x();
    	n_actual_nodes = field_->size_x() - zero_node_workload + 1;
    	printf("\t\tI'm here! (3)\n");
    }
	if (workload_per_node == 0)
	{
		workload_per_node = 1;
		n_actual_nodes = field_->size_x() - zero_node_workload;
    	printf("\t\tI'm here! (4)\n");
	}
	else if (zero_node_workload == 0)
	{
		n_actual_nodes --;
    	printf("\t\tI'm here! (5)\n");
	}
	
	printf("n_nodes_ = %d\nn_actual_nodes = %d\n", n_nodes_, n_actual_nodes);
	
	// Filling the Game's hyper torus with 2-cell-wide boarder overlap:
	std::map<int, Matrix *> jbuf;
	
	for (int n = 0; n < n_nodes_; n ++)
	{
		int workload = n == 0 ? zero_node_workload : workload_per_node;
			
		// Don't forget to init the redundant nodes as well:
		if (workload == 0)
		{
	        (*redundant_nodes_)[n] = true;
			jbuf[n] = new Matrix;
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
				jbuf[n]->set(loc_x, y + 1,
					field_->get(x>=0? (x < field_->size_x()? x : 0)
									: field_->size_x() - 1,
								y>=0? (y < field_->size_y()? y : 0)
									: field_->size_y() - 1) );
			}
		}
			
		(*node_x_size_)[std::pair<int, int>(n, 0)] = workload;
		(*node_y_size_)[std::pair<int, int>(n, 0)] = field_->size_y();
							
		(*redundant_nodes_)[n] = false;
	}
		
    // Generate per-node neighbor lists and send all per-node init data:
    for (int n = 0; n < n_nodes_; n ++)
    {    
    	// The zero node takes the extra-work-load.
		send_init_data(n, (n == 0 ? zero_node_workload : workload_per_node) + 2, field_->size_y() + 2, jbuf[n]->data());
		delete jbuf[n];
    }
}

void GameOfLife::grid_split()
{
	std::vector<num_t> *prime_factors = get_prime_factors(n_nodes_);
	
	int x_cells_per_node = field_->size_x(),
        y_cells_per_node = field_->size_y();
            
    n_x_nodes_ = 1;
    n_y_nodes_ = 1;
    	
	for (std::vector<num_t>::const_iterator iter = prime_factors->begin(); iter != prime_factors->end(); iter ++)
	{
		if (x_cells_per_node > y_cells_per_node)
		{
			x_cells_per_node /= *iter;
			n_x_nodes_ *= *iter;
		}
		else
		{
			y_cells_per_node /= *iter;
			n_y_nodes_ *= *iter;
		}
	}

	delete prime_factors;

    int x_extra_job = field_->size_x() - x_cells_per_node * n_x_nodes_,
	    y_extra_job = field_->size_y() - y_cells_per_node * n_y_nodes_;
	    
	int n_actual_nodes = n_nodes_;
	
	std::map<int, Matrix *> jbuf;
	
	for (int n = 0; n < n_actual_nodes; n ++)
	{	
		int x_workload = x_cells_per_node + (node_x(n) < n_x_nodes_ - 1 ? 0 : x_extra_job),
		    y_workload = y_cells_per_node + (node_y(n) < n_y_nodes_ - 1 ? 0 : y_extra_job);
		    
		if (x_workload * y_workload == 0)
		{
	        (*redundant_nodes_)[n] = true;
	        continue;
		}
		
		jbuf[n] = new Matrix(x_workload + 2, y_workload + 2);
						
		for (int loc_y = 0,
				 y = node_y(n) * y_cells_per_node - 1;
				 y < node_y(n) * y_cells_per_node + y_workload + 1;
		     loc_y ++, y ++)
		{
			for (int loc_x = 0,
					 x = node_x(n) * x_cells_per_node - 1;
					 x < node_x(n) * x_cells_per_node + x_workload + 1;
				 loc_x ++, x ++)
			{
				jbuf[n]->set(loc_x, loc_y, 
					field_->get(x>=0? (x < field_->size_x()? x : 0)
									: field_->size_x() - 1,
								y>=0? (y < field_->size_y()? y : 0)
									: field_->size_y() - 1) );
			}
		}
			
		(*node_x_size_)[std::pair<int, int>(node_x(n), node_y(n))] = x_workload;
		(*node_y_size_)[std::pair<int, int>(node_x(n), node_y(n))] = y_workload;

		(*redundant_nodes_)[n] = false;
	}
	
	// Don't forget to init the redundant nodes as well:
	for (int n = n_actual_nodes; n < n_nodes_; n ++)
	{
		(*redundant_nodes_)[n] = true;
	}
	
    // Generate per-node neighbor lists and send all per-node init data:
    for (int n = 0; n < n_nodes_; n ++)
    {
    	// The last node takes the extra-work-load.
		send_init_data(n, 2 + x_cells_per_node + (node_x(n) < n_x_nodes_ - 1 ? 0 : x_extra_job),
		                  2 + y_cells_per_node + (node_y(n) < n_y_nodes_ - 1 ? 0 : y_extra_job),
		                  jbuf[n]->data());
		delete jbuf[n];
    }	
}
