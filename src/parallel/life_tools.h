#ifndef _LIFE_TOOLS_H_
#define _LIFE_TOOLS_H_

#include <stdlib.h>
#include <map>
#include <vector>

enum LIFE { DEAD, ALIVE };

typedef int num_t;

std::vector<num_t> *get_prime_factors(num_t num)
{
        std::vector<num_t> *list = new std::vector<num_t>();

        num_t start_num = num;

        for (num_t i = 2; i <= num; i ++)
        {
                while (num % i == 0)
                {
                        //if (i != start_num)
                        	list->push_back(i);
                        num /= i;
                }
        }

        return list;
}

#if 0
			 0 1 2 3 4 5
			0[][][][][][]
			1[][][][][][]
			2[][][]<>[][]--------------------+
			3[][][][][][]		             |
								             V
			 0 1 2 3 4 5  6 7 8 9 1011 121314151617 181920212223
			 [][][][][][] [][][]<>[][] [][][][][][] [][][][][][]
			
			0) x_size = 6, y_size = 4, x_max = 5, y_max = 3
			1) size of lenear mass: x_size * y_size = 24.
			2) i from 0 to 23, x from 0 to 5, y from 0 to 3
			3) i = x + y * x_size = 3 + 2 * 6 = 3 + 12 = 15
#endif

class Matrix {
private:
	LIFE *data_;
	int x_size_, y_size_;
	
	inline LIFE *get_data_element(int x, int y)
	{
		if (x < 0 || y < 0 || x >= x_size_ || y >= y_size_)
		{
			printf("Error: incorrect (x, y) coords: (%3d%3d)\n", x, y);
			return NULL;
		}
		return &(data_[x + y * x_size_]);
	}
	
	friend void swap(Matrix& first, Matrix& second)
    {
        std::swap(first.x_size_, second.x_size_); 
		std::swap(first.y_size_, second.y_size_);
        std::swap(first.data_, second.data_);
    }
    
    void init()
    {
    	for (int i = 0 ; i < x_size_ * y_size_; i ++)
    	{
    		data_[i] = DEAD;
    	}
    }

public:	
	Matrix(int x_size, int y_size)
	: x_size_ (x_size), y_size_ (y_size),
	  data_ (x_size * y_size ? new LIFE[x_size * y_size]() : 0) 
	{

	}
	Matrix(int x_size, int y_size, LIFE *data)
	: x_size_ (x_size), y_size_ (y_size),
	  data_ (x_size * y_size ? new LIFE[x_size * y_size]() : 0)
	{
		std::copy(data, data + x_size * y_size, data_);
	}
	Matrix(const Matrix& other)
	: x_size_ (other.x_size_), y_size_ (other.y_size_),
	  data_ (other.x_size_ * other.y_size_ ? new LIFE[other.x_size_ * other.y_size_]() : 0)
	{
		std::copy(other.data_, other.data_ + other.x_size_ * other.y_size_, data_);
	}
	~Matrix()
	{
		free(data_);
	}
	
	Matrix& operator=(Matrix other)
	{
		swap(*this, other);
		return *this;
	} 
	
	void print()
	{
		printf("(%dX%d)\n", x_size_, y_size_);
		    	
		for (int y = 0; y < y_size_; y ++)
		{	    	
			printf ("Y#%2d ", y);
			for (int x = 0; x < x_size_; x++)
			{
				printf("%c", get(x, y)==ALIVE?'O':' ');
			}
			printf("\n");
		}
	}
	
	void die(int x, int y)
	{
		set(x, y, DEAD);
	}	
	void revive(int x, int y)
	{
		set(x, y, ALIVE);
	}
	const int size_x()
	{
		return x_size_;
	}
	const int size_y()
	{
		return y_size_;
	}
	const int inner_size()
	{
		return size() - 2 * (x_size_ + y_size_ - 2);
	}
	const int size()
	{
		return x_size_ * y_size_;
	}
	inline LIFE *angle_nw() { return new LIFE(get(          0,           0)); }
	inline LIFE *angle_ne() { return new LIFE(get(x_size_ - 1,           0)); }
	inline LIFE *angle_se() { return new LIFE(get(x_size_ - 1, y_size_ - 1)); }
	inline LIFE *angle_sw() { return new LIFE(get(          0, y_size_ - 1)); }
	LIFE *border_top()
	{
		LIFE *bt = new LIFE[x_size_]();
		for (int x = 0; x < x_size_; x ++) bt[x] = get(x, 0);
		return bt;
	}
	LIFE *border_right()
	{
		LIFE *br = new LIFE[y_size_]();
		for (int y = 0; y < y_size_; y ++) br[y] = get(x_size_ - 1, y);
		return br;
	}
	LIFE *border_bottom()
	{
		LIFE *bb = new LIFE[x_size_]();
		for (int x = 0; x < x_size_; x ++) bb[x] = get(x, y_size_ - 1);
		return bb;
	}
	LIFE *border_left()
	{
		LIFE *bl = new LIFE[y_size_]();
		for (int y = 0; y < y_size_; y ++) bl[y] = get(0, y);
		return bl;
	}
	inline Matrix *inner()
	{
		return new Matrix(x_size_ - 2, y_size_ - 2, inner_data());
	}
	LIFE *inner_data()
	{
		if (x_size_ <= 2 || y_size_ <= 2)
		{
			printf(">>> Error: trying to get inner data from Matrix[%dX%d]\n", x_size_, y_size_);
			return NULL;
		}
		
		Matrix *tmpmat = new Matrix(x_size_ - 2, y_size_ - 2);
		
		for (int y = 1; y < y_size_ - 1; y ++)
		{
			for (int x = 1; x < x_size_ - 1; x ++)
			{
				tmpmat->set(x - 1, y - 1, get(x, y));
			}
		}
		
		return tmpmat->data();
	}
	LIFE *data()
	{
		return data_;
	}
	void set(int x, int y, LIFE val)
	{
		*get_data_element(x, y) = val;
	}
	const LIFE get(int x, int y)
	{
		return *get_data_element(x, y);
	}
};

#endif // _LIFE_TOOLS_H_
