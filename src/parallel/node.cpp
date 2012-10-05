#include "node.h"

Node::Node(int rank)
: neighbors_ (new int[N_NEIGHBORS]()), rank_ (rank), gen_ (0), n_gens_ (0), is_redundant_ (false)
{
    MPI_Recv(&is_redundant_, 1, MPI_INT, 0, REDUNDANCE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
    if (is_redundant_)
	{
		is_redundant_ = true;

		return;		
	}

	MPI_Recv(&n_gens_, 1, MPI_INT, 0, N_GENERATIONS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			
	int x_size = 0, y_size = 0;
	MPI_Recv(&x_size, 1, MPI_INT, 0, NODE_X_SIZE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&y_size, 1, MPI_INT, 0, NODE_Y_SIZE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	
	MPI_Recv(neighbors_, N_NEIGHBORS, MPI_INT, 0, NEIGHBORS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	one_row_ = neighbors_[TOP] == rank_;
	one_col_ = neighbors_[RIGHT] == rank_;
	
	//if (neighbors_[TOP] == rank_) printf(">>> My (%d) top neighbor is me!\n", rank_);
	//if (neighbors_[BOTTOM] == rank_) printf(">>> My (%d) bottom neighbor is me!\n", rank_);
	//if (neighbors_[LEFT] == rank_) printf(">>> My (%d) left neighbor is me!\n", rank_);
	//if (neighbors_[RIGHT] == rank_) printf(">>> My (%d) right neighbor is me!\n", rank_);
	
	LIFE *loc_job = new LIFE[x_size * y_size]();//alloc_mass(x_size * y_size, DEAD);
	MPI_Recv(loc_job, x_size * y_size, MPI_INT, 0, MASS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		
	field_ = new Matrix(x_size, y_size, loc_job);
	free(loc_job);
}

Node::~Node()
{
	if (!is_redundant_) delete field_;
}

void Node::end()
{
	if (!is_redundant_)
	{
		MPI_Send(field_->inner_data(), field_->inner_size(), MPI_INT, 0, (int) MASS + rank_, MPI_COMM_WORLD);
	}
	
	delete this;
}

short int Node::count(int x, int y)
{ // how many cells around (x, y) are alive
	int alive = 0;
	if (field_->get(x-1, y+1)) alive ++; //  o----------+----------+----------o
	if (field_->get( x , y+1)) alive ++; //  |(x-1; y+1)|(x;   y+1)|(x+1; y+1)|
	if (field_->get(x+1, y+1)) alive ++; //  +----------+----------+----------+
	if (field_->get(x-1,  y )) alive ++; //  |(x-1; y  )|(x;   y  )|(x+1; y  )|
	if (field_->get(x+1,  y )) alive ++; //  +----------+----------+----------+
	if (field_->get(x-1, y-1)) alive ++; //  |(x-1; y-1)|(x;   y-1)|(x+1; y-1)|
	if (field_->get( x , y-1)) alive ++; //  o----------+----------+----------o		
	if (field_->get(x+1, y-1)) alive ++;
	return alive;
}

inline void Node::count()
{
	Matrix buf = *field_;
	
	for (int x = 1; x < field_->size_x() - 1; x ++)
	{
		for (int y = 1; y < field_->size_y() - 1; y ++)
		{
			buf.set(x, y, field_->get(x, y));

			if (field_->get(x, y) == ALIVE)
			{
				if (count(x, y) <  2
				||  count(x, y) >  3) buf.die(x, y);
			}
			else // Dead
			{
				if (count(x, y) == 3) buf.revive(x, y);
			}
		}
	}

	delete field_;
	field_ = new Matrix(buf);
}

void Node::send()
{
	Matrix *inmat = field_->inner();
			
	if (!(one_row_ && one_col_))
	{
		MPI_Send(inmat->angle_nw(), 1, MPI_INT, neighbors_[TOP_LEFT],     ANGLE_SE_TO_NW, MPI_COMM_WORLD);
		MPI_Send(inmat->angle_ne(), 1, MPI_INT, neighbors_[TOP_RIGHT],    ANGLE_SW_TO_NE, MPI_COMM_WORLD);
		MPI_Send(inmat->angle_se(), 1, MPI_INT, neighbors_[BOTTOM_RIGHT], ANGLE_NW_TO_SE, MPI_COMM_WORLD);
		MPI_Send(inmat->angle_sw(), 1, MPI_INT, neighbors_[BOTTOM_LEFT],  ANGLE_NE_TO_SW, MPI_COMM_WORLD);
	}
	else
	{
		field_->set(                 0,                  0, field_->get(field_->size_x()-2, field_->size_y()-2));
		field_->set(                 0, field_->size_y()-1, field_->get(field_->size_x()-2,                  1));
		field_->set(field_->size_x()-1,                  0, field_->get(                 1, field_->size_y()-2));
		field_->set(field_->size_x()-1, field_->size_y()-1, field_->get(                 1,                  1));
	}
	if (!one_col_)
	{
		MPI_Send(inmat->border_left(),  inmat->size_y(), MPI_INT, neighbors_[LEFT],  BORDER_RIGHT_TO_LEFT, MPI_COMM_WORLD);		
		MPI_Send(inmat->border_right(), inmat->size_y(), MPI_INT, neighbors_[RIGHT], BORDER_LEFT_TO_RIGHT, MPI_COMM_WORLD);
	}
	else
	{
		for (int y = 1; y < field_->size_y()-1; y ++)
		{
			field_->set(                 0, y, field_->get(field_->size_x()-2, y));
			field_->set(field_->size_x()-1, y, field_->get(1,                  y));
		}
	}
	if (!one_row_)
	{
		MPI_Send(inmat->border_top(),    inmat->size_x(), MPI_INT, neighbors_[TOP],    BORDER_BOTTOM_TO_TOP, MPI_COMM_WORLD);
		MPI_Send(inmat->border_bottom(), inmat->size_x(), MPI_INT, neighbors_[BOTTOM], BORDER_TOP_TO_BOTTOM, MPI_COMM_WORLD);
	}
	else
	{
		for (int x = 1; x < field_->size_x()-1; x ++)
		{
			field_->set(x,                  0, field_->get(x, field_->size_y()-2));
			field_->set(x, field_->size_y()-1, field_->get(x,                  1));
		}
	}
	
	delete inmat;
}

void Node::receive()
{
	int xbordsize = field_->size_x() - 2,
		ybordsize = field_->size_y() - 2;
			
	LIFE *xbordbuf = new LIFE[xbordsize],
		 *ybordbuf = new LIFE[ybordsize];

	if (!(one_row_ && one_col_))
	{	
		LIFE lbuf = DEAD;
		
		MPI_Recv(&lbuf, 1, MPI_INT, neighbors_[TOP_LEFT], ANGLE_NW_TO_SE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		field_->set(0, 0, lbuf);

		MPI_Recv(&lbuf, 1, MPI_INT, neighbors_[BOTTOM_LEFT], ANGLE_SW_TO_NE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		field_->set(0, field_->size_y()-1, lbuf);

		MPI_Recv(&lbuf, 1, MPI_INT, neighbors_[TOP_RIGHT], ANGLE_NE_TO_SW, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		field_->set(field_->size_x()-1, 0, lbuf);
				 
		MPI_Recv(&lbuf, 1, MPI_INT, neighbors_[BOTTOM_RIGHT], ANGLE_SE_TO_NW, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		field_->set(field_->size_x()-1, field_->size_y()-1, lbuf);
	}
	
	if (!one_col_)
	{
		MPI_Recv(ybordbuf, ybordsize, MPI_INT, neighbors_[LEFT], BORDER_LEFT_TO_RIGHT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		for (int y = 0; y < ybordsize; y ++) field_->set(0, y+1, ybordbuf[y]);

		MPI_Recv(ybordbuf, ybordsize, MPI_INT, neighbors_[RIGHT], BORDER_RIGHT_TO_LEFT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		for (int y = 0; y < ybordsize; y ++) field_->set(field_->size_x()-1, y+1, ybordbuf[y]);
	}

	if (!one_row_)
	{
		MPI_Recv(xbordbuf, xbordsize, MPI_INT, neighbors_[TOP], BORDER_TOP_TO_BOTTOM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		for (int x = 0; x < xbordsize; x ++) field_->set(x+1, 0, xbordbuf[x]);
		
		MPI_Recv(xbordbuf, xbordsize, MPI_INT, neighbors_[BOTTOM], BORDER_BOTTOM_TO_TOP, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		for (int x = 0; x < xbordsize; x ++) field_->set(x+1, field_->size_y()-1, xbordbuf[x]);
	}
	
	delete xbordbuf;
	delete ybordbuf;
}
