#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <time.h>

#define MAX_X 200
#define MAX_Y 200

enum LIFE { DEAD, ALIVE };

int FIELD[MAX_X][MAX_Y];

int X_SIZE = 0;
int Y_SIZE = 0;

void print()
{
	// printf(">>> Debug: X_SIZE = %d, Y_SIZE = %d\n", X_SIZE, Y_SIZE);
	char buf = ' ';
	for (int y = 0; y < Y_SIZE; y ++)
	{
		printf("Y#%2d: ", y);
		for (int x = 0; x < X_SIZE; x ++)
		{
			buf = FIELD[x][y]?'O':' ';
			printf("%c", buf, buf);
		}
		printf("\n");
	}
}

int init()
{
	srand(time(0));	

	int cells_alive = 0;

	for (int x = 0; x < X_SIZE; x ++)
	{
		for (int y = 0; y < Y_SIZE; y ++)
		{
			cells_alive += FIELD[x][y] = rand() % 2;
		}
	}

	return cells_alive;
}

int cells_alive()
{
	int alive = 0;
	for (int x = 0; x < X_SIZE; x ++)
	{
		for (int y = 0; y < Y_SIZE; y ++)
		{
			if (FIELD[x][y] == ALIVE) alive ++;
		}
	}
	return alive;
}

bool load(char *fname)
{
	FILE *fp = fopen(fname, "r");
	if (!fp) return false;

	int n_cells = 1;

	while (fgetc(fp) != '\n') X_SIZE ++;
	while (fgetc(fp) != EOF) n_cells ++;

	Y_SIZE = (int) n_cells / X_SIZE;

	fclose(fp);


	fp = fopen(fname, "r");
	if (!fp) return false;
	
	int buf = 0;
	for (int y = 0; ; y ++)
	{
		for (int x = 0; ; x ++)
		{
			buf = fgetc(fp);
			if (buf == EOF) { fclose(fp); return true; }
			else if (buf != '\n') FIELD[x][y] = buf - 48;
			else break;
		}
	}

	fclose(fp);
	return true;	
}

inline int saw(int q, int T) { return q >= 0 ? q%T : (T + (q%T)); }

short int count(int x, int y)
{ // how many cells around (x, y) are alive
	int alive = 0;
	if (FIELD[saw(x-1, X_SIZE)][saw(y+1, Y_SIZE)]) alive ++; //  o----------+----------+----------o
	if (FIELD[saw( x , X_SIZE)][saw(y+1, Y_SIZE)]) alive ++; //  |(x-1; y+1)|(x;   y+1)|(x+1; y+1)|
	if (FIELD[saw(x+1, X_SIZE)][saw(y+1, Y_SIZE)]) alive ++; //  +----------+----------+----------+
	if (FIELD[saw(x-1, X_SIZE)][saw( y , Y_SIZE)]) alive ++; //  |(x-1; y  )|(x;   y  )|(x+1; y  )|
	if (FIELD[saw(x+1, X_SIZE)][saw( y , Y_SIZE)]) alive ++; //  +----------+----------+----------+
	if (FIELD[saw(x-1, X_SIZE)][saw(y-1, Y_SIZE)]) alive ++; //  |(x-1; y-1)|(x;   y-1)|(x+1; y-1)|
	if (FIELD[saw( x , X_SIZE)][saw(y-1, Y_SIZE)]) alive ++; //  o----------+----------+----------o
	if (FIELD[saw(x+1, X_SIZE)][saw(y-1, Y_SIZE)]) alive ++; 
	
	//printf(">>> Debug: x-1 = %3d; x = %3d; x+1 = %3d\n", saw(x-1, X_SIZE), x, saw(x+1, X_SIZE));
	return alive;
}

class IterStat {
private:
	int alive, revived, died, converted;
	static const int MSG_STR_LENGTH = 55;
public:
	IterStat() : alive(0), revived(0), died(0), converted(0) {}
	void born()  { alive ++; }
	void die()    { alive --; converted ++; died ++; }
	void revive() { alive ++; converted ++; revived ++; }
	bool gameover() { return alive == 0 || converted == 0; }
	char * print()
	{
		static char str[MSG_STR_LENGTH];
		sprintf(str, "(Cells alive: %d; revived: %d; died: %d; converted: %d)", alive, revived, died, converted);
		return str;
	}
};

IterStat iterate()
{
	int buf[MAX_X][MAX_Y];

	IterStat cells;

	for (int x = 0; x < X_SIZE; x ++)
	{
		for (int y = 0; y < Y_SIZE; y ++)
		{
			buf[x][y] = FIELD[x][y];

			if (FIELD[x][y]) // Alive
			{
				cells.born();
				if (count(x, y) <  2 ||
                                    count(x, y) >  3) { buf[x][y] = DEAD; cells.die(); }
			}
			else // Dead
			{
				if (count(x, y) == 3) { buf[x][y] = ALIVE; cells.revive(); }
			}
		}
	}

	for (int x = 0; x < X_SIZE; x ++)
	{
		for (int y = 0; y < Y_SIZE; y ++)
		{
			FIELD[x][y] = buf[x][y];
		}
	}
	
	return cells;
}

int main(int argc, char ** argv)
{ // Usage: ./app.exe N_ITERATIONS 
	argc --; argv ++;

	if (argc != 3)
	{
		printf("Usage:\n");
		printf(" ./app.exe FIELD_SIZE_X FIELD_SIZE_Y N_ITERATIONS --- to generate random cell world\n");
		printf(" ./app.exe -L FILENAME.TXT           N_ITERATIONS --- to load a world from the file \"TEXTFILE.TXT\"\n");
		return -1;
	}

	int n_iters = atoi(argv[2]);

	if (argv[0][0] != '-') // Mode 1
	{
		X_SIZE = atoi(argv[0]);
		Y_SIZE = atoi(argv[1]);
		init();
	}
	else switch (argv[0][1]) // Mode 2
	{
		case 'L':
			if (!load(argv[1])) { printf(">>> Error opening file: %s\n", argv[1]); return -3; }
			break;
		
		default:
			printf("Unknown key: %s\n", argv[0]);
			return -2;
	}

	printf("Starting game of life\n");
	printf(">>> Iteration 0 (Cells alive: %d)\n", cells_alive());
	print();

	// Loop
	IterStat cell_situation;
	for (int i = 0; i < n_iters; i ++)
	{
		getchar();
		cell_situation = iterate();
		printf(">>> Iteration %d %s\n", i+1, cell_situation.print());
		if (cell_situation.gameover()) { printf("Game Over\n"); return 0; }
		print();
	}

	return 0;
}

