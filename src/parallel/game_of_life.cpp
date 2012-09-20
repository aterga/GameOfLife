#include "game_of_life.h"

GameOfLife::GameOfLife(int X, int Y, int N_nodes, int N_generations)
: n_nodes_ (N_nodes), n_x_nodes_ (0), n_y_nodes_ (0), n_gens_ (N_generations)
{
	field_ = new Matrix(X, Y);
	node_x_size_ = new std::map<std::pair<int, int>, int>;
	node_y_size_ = new std::map<std::pair<int, int>, int>;
	
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
	
	//printf(">>> Params: {x_size = %d, y_size = %d, inner_size = %d}\n",
	//       field_->size_x(), field_->size_y(), field_->inner_size());
	
	//(new Matrix(8, 8, field_->inner_data()))->print();
	
	print();
	linear_split();
}

GameOfLife::~GameOfLife()
{
	delete field_;
	delete node_x_size_;
	delete node_y_size_;
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
	
	//printf("I'm (%d) here (J)!\n", rank);
			
	for (int n_y = 0; n_y < n_y_nodes_; n_y ++)
	{
		for (int n_x = 0; n_x < n_x_nodes_; n_x ++)
		{
			rank = n_y * n_x_nodes_ + n_x;
		
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
	
	//printf("I'm (%d) here (Q)!\n", rank);
}

void GameOfLife::send_init_data(int target_rank, int x_pos, int y_pos, int x_size, int y_size, LIFE *jbuf)
{	
	//printf(">>> Main Node: sending following data to Node #%d: {x_pos=%d, y_pos=%d, x_size=%d, y_size=%d}\n", target_rank, x_pos, y_pos, x_size, y_size);
	
	// Job split data
	MPI_Send(&x_pos, 1, MPI_INT, target_rank, NODE_X_POS, MPI_COMM_WORLD);
	MPI_Send(&y_pos, 1, MPI_INT, target_rank, NODE_Y_POS, MPI_COMM_WORLD);
	MPI_Send(&x_size, 1, MPI_INT, target_rank, NODE_X_SIZE, MPI_COMM_WORLD);
	MPI_Send(&y_size, 1, MPI_INT, target_rank, NODE_Y_SIZE, MPI_COMM_WORLD);
	MPI_Send(jbuf, x_size * y_size, MPI_INT, target_rank, MASS, MPI_COMM_WORLD);
	
	// Global data
	MPI_Send(&n_x_nodes_, 1, MPI_INT, target_rank, NODE_MAX_X_POS, MPI_COMM_WORLD);
	MPI_Send(&n_y_nodes_, 1, MPI_INT, target_rank, NODE_MAX_Y_POS, MPI_COMM_WORLD);
	MPI_Send(&n_gens_, 1, MPI_INT, target_rank, N_GENERATIONS, MPI_COMM_WORLD);
}

void GameOfLife::linear_split()
{
	// Strategy:
	n_x_nodes_ = n_nodes_;
	n_y_nodes_ = 1;

	int workload_per_node = 0;
	int zero_node_workload = 0;
	
	if (n_nodes_ > 1)
	{
		workload_per_node = int(field_->size_x() / n_nodes_) * (n_nodes_ / (n_nodes_ - 1));
		zero_node_workload = field_->size_x() - (n_nodes_ - 1) * workload_per_node;
		
		if (workload_per_node < 3) workload_per_node = 0;
		if (zero_node_workload < 3) zero_node_workload = 0;
	}
	else
	{
		workload_per_node = 0;
		zero_node_workload = field_->size_x();
	}

	// n = 0
	if (zero_node_workload > 0)
	{
		Matrix *jbuf = new Matrix(zero_node_workload + 2, field_->size_y() + 2);
	
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
					 jbuf->set(x + 1, y + 1, ALIVE);
				} 
				else jbuf->set(x + 1, y + 1, DEAD);
			}
		}        
		
		//jbuf->print();
	
		(*node_x_size_)[std::pair<int, int>(0, 0)] = zero_node_workload;
		(*node_y_size_)[std::pair<int, int>(0, 0)] = field_->size_y();
		
		send_init_data(0, 0, 0, zero_node_workload + 2, field_->size_y() + 2, jbuf->data());
    
	    delete jbuf;
    }
    else
    {
    	send_init_data(0, 0, 0, 0, 0, NULL);
    }
    
    if (workload_per_node > 0)
    {    
		for (int n = 1; n < n_nodes_; n ++)
		{
			Matrix *jbuf = new Matrix(workload_per_node + 2, field_->size_y() + 2);
			
			for (int y = -1; y < field_->size_y() + 1; y ++)
			{
				for (int loc_x = 0,
						 x = zero_node_workload + (n - 1) * workload_per_node - 1;
						 x < zero_node_workload +  n      * workload_per_node + 1;
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
						jbuf->set(loc_x, y + 1, ALIVE);
					}
					else
					{
						jbuf->set(loc_x, y + 1, DEAD);
					}
				}
			}
			
			//jbuf->print();
	
			(*node_x_size_)[std::pair<int, int>(n, 0)] = workload_per_node;    
			(*node_y_size_)[std::pair<int, int>(n, 0)] = field_->size_y();
			
			send_init_data(n, n, 0, workload_per_node + 2, field_->size_y() + 2, jbuf->data());
		
			delete jbuf;
		}
    }
}

void GameOfLife::grid_split()
{

}
