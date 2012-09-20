#include "node.h"

Node::Node(int rank)
: rank_ (rank), gen_ (0), n_gens_ (0), is_redundant_ (false)
{
    //printf("I'm (%d) here (D)!\n", rank);

	MPI_Recv(&x_pos_, 1, MPI_INT, 0, NODE_X_POS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&y_pos_, 1, MPI_INT, 0, NODE_Y_POS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&max_x_pos_, 1, MPI_INT, 0, NODE_MAX_X_POS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&max_y_pos_, 1, MPI_INT, 0, NODE_MAX_Y_POS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&n_gens_, 1, MPI_INT, 0, N_GENERATIONS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	
	//printf("I'm (%d) here (I)!\n", rank);
	
	int x_size = 0, y_size = 0;
	MPI_Recv(&x_size, 1, MPI_INT, 0, NODE_X_SIZE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Recv(&y_size, 1, MPI_INT, 0, NODE_Y_SIZE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	
	//printf("I'm (%d) here (E)!\n", rank);
	
	if (x_size * y_size == 0)
	{
		is_redundant_ = true;	
		return;
	}
	
	LIFE *loc_job = new LIFE[x_size * y_size]();//alloc_mass(x_size * y_size, DEAD);
	MPI_Recv(loc_job, x_size * y_size, MPI_INT, 0, MASS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	
	printf("I'm (%d) here (H)!\n", rank);
	
	field_ = new Matrix(x_size, y_size, loc_job);
	free(loc_job);

	//print();
	
	//printf("I'm (%d) here (F)!\n", rank);
}

Node::~Node()
{
	if (!is_redundant_) delete field_;
	
	//printf("I'm (%d) here (Z)!\n", rank_);
}

void Node::end()
{
	if (!is_redundant_)
	{
		MPI_Send(field_->inner_data(), field_->inner_size(), MPI_INT, 0, (int) MASS + rank_, MPI_COMM_WORLD);
	}
	
	delete this;
	//printf("I'm (%d) here (X)!\n", rank_);
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
	//printf("I'm (%d) here (S)!\n", rank_);

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
	
	print();
}

void Node::send()
{
	Matrix *inmat = field_->inner();

	//print();
	//inmat->print();

	MPI_Send(inmat->border_top(),    inmat->size_x(), MPI_INT, rank(x_pos_  , y_pos_-1), BORDER_TOP_TO_BOTTOM, MPI_COMM_WORLD);
	MPI_Send(inmat->border_right(),  inmat->size_y(), MPI_INT, rank(x_pos_+1, y_pos_  ), BORDER_RIGHT_TO_LEFT, MPI_COMM_WORLD);
	MPI_Send(inmat->border_bottom(), inmat->size_x(), MPI_INT, rank(x_pos_  , y_pos_+1), BORDER_BOTTOM_TO_TOP, MPI_COMM_WORLD);
	MPI_Send(inmat->border_left(),   inmat->size_y(), MPI_INT, rank(x_pos_-1, y_pos_  ), BORDER_LEFT_TO_RIGHT, MPI_COMM_WORLD);

	MPI_Send(inmat->angle_nw(), 1, MPI_INT, rank(x_pos_  , y_pos_+1), ANGLE_NW_TO_SE, MPI_COMM_WORLD);
	MPI_Send(inmat->angle_ne(), 1, MPI_INT, rank(x_pos_-1, y_pos_  ), ANGLE_NE_TO_SW, MPI_COMM_WORLD);
	MPI_Send(inmat->angle_se(), 1, MPI_INT, rank(x_pos_  , y_pos_-1), ANGLE_SE_TO_NW, MPI_COMM_WORLD);
	MPI_Send(inmat->angle_sw(), 1, MPI_INT, rank(x_pos_+1, y_pos_  ), ANGLE_SW_TO_NE, MPI_COMM_WORLD);
	
	delete inmat;
}

void Node::receive()
{
	int xbordsize = field_->size_x() - 2,
		ybordsize = field_->size_y() - 2;
		
	LIFE *xbordbuf = new LIFE[field_->size_x() - 2],
		 *ybordbuf = new LIFE[field_->size_y() - 2];
		 
	MPI_Recv(xbordbuf, xbordsize, MPI_INT, rank(x_pos_  , y_pos_+1),
			 BORDER_TOP_TO_BOTTOM, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	for (int x = 1; x < xbordsize - 1; x ++) field_->set(x, field_->size_y()-1, *xbordbuf);
	
	MPI_Recv(ybordbuf, ybordsize, MPI_INT, rank(x_pos_-1, y_pos_  ),
			 BORDER_RIGHT_TO_LEFT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	for (int y = 1; y < ybordsize - 1; y ++) field_->set(0, y, *ybordbuf);
	
	MPI_Recv(xbordbuf, xbordsize, MPI_INT, rank(x_pos_  , y_pos_-1),
			 BORDER_BOTTOM_TO_TOP, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	for (int x = 1; x < xbordsize - 1; x ++) field_->set(x, 0, *xbordbuf);
	
	MPI_Recv(ybordbuf, ybordsize, MPI_INT, rank(x_pos_+1, y_pos_  ),
			 BORDER_LEFT_TO_RIGHT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	for (int y = 1; y < ybordsize - 1; y ++) field_->set(field_->size_x()-1, y, *ybordbuf);

	delete xbordbuf;
	delete ybordbuf;

	LIFE lbuf = DEAD;
	MPI_Recv(&lbuf, 1, MPI_INT, rank(x_pos_, y_pos_-1),
			 ANGLE_NW_TO_SE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	field_->set(field_->size_x()-1, field_->size_y()-1, lbuf);
			 
	MPI_Recv(&lbuf, 1, MPI_INT, rank(x_pos_+1, y_pos_),
			 ANGLE_NE_TO_SW, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	field_->set(0, field_->size_y()-1, lbuf);
			 
	MPI_Recv(&lbuf, 1, MPI_INT, rank(x_pos_, y_pos_+1),
			 ANGLE_SE_TO_NW, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	field_->set(0, 0, lbuf);
			 
	MPI_Recv(&lbuf, 1, MPI_INT, rank(x_pos_-1, y_pos_),
			 ANGLE_SW_TO_NE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	field_->set(field_->size_x()-1, 0, lbuf);     
}
