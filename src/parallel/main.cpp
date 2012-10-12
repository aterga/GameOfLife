/* This code is in public domain. 
 * There are no restrictions on use of any kind. 
 *
 * By Arshavir Ter-Gabrielyan
 * MIPT
 * 2012
 * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdlib.h>  
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <mpi.h>

#include "node.h"
#include "game_of_life.h"

Matrix load(char *fname)
{
	FILE *fp = fopen(fname, "r");
	if (!fp) printf(">>> Error opening file: %s\n", fname);

	int n_cells = 1, x_size = 0, y_size = 0;

	while (fgetc(fp) != '\n') x_size ++;
	while (fgetc(fp) != EOF) n_cells ++;

	y_size = (int) n_cells / x_size;
	
	fclose(fp);

	Matrix mat(x_size, y_size);

	fp = fopen(fname, "r");
	if (!fp) printf(">>> Error opening file: %s\n", fname);
	
	char buf = 0;
	for (int y = 0; ; y ++)
	{
		for (int x = 0; ; x ++)
		{
			buf = fgetc(fp);
			if (buf == EOF) { fclose(fp); return mat; }
			else if (buf != '\n' && x < x_size && y < y_size) mat.set(x, y, (LIFE) (buf - 48));
			else break;
		}
	}

	fclose(fp);
	return mat;
}

int main(int argc, char ** argv)
{
    enum MODE_ONE_ARGS {
        X_SIZE, Y_SIZE, N_GENERATIONS, NUM_ARGS
    };
    
    enum MODE_TWO_ARGS {
    	KEY, FILENAME
    };

    argc --; argv ++;

    int  numtasks = 0, rank = 0, len = 0;
    char hostname[MPI_MAX_PROCESSOR_NAME];
     
    int rc = MPI_Init(&argc, &argv);
    if (rc != MPI_SUCCESS)
    {   
        printf ("Error starting MPI program. Terminating.\n");
        MPI_Abort(MPI_COMM_WORLD, rc);
    }

    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Get_processor_name(hostname, &len);
    
    if (argc != NUM_ARGS)
	{
		if (rank == 0)
		{
			printf("Usage:\n");
			printf(" ./app.exe FIELD_SIZE_X FIELD_SIZE_Y N_ITERATIONS --- to generate random cell world\n");
			printf(" ./app.exe -L FILENAME.TXT           N_ITERATIONS --- to load a world from the file \"TEXTFILE.TXT\"\n");
		}
		MPI_Finalize();
		return -1;
	}

    GameOfLife *game = NULL;

    if (rank == 0)
    {   
    	int x_size = 0,
            y_size = 0,
            n_generations = 0;
        
        if ((n_generations = atoi(argv[N_GENERATIONS])) <= 0)
        {
        	printf(">>> Error: N_ITERATIONS must be greater than Null\n");
        	return -3;
        }
            
        printf("\n    The game will take %d generations\n\n", n_generations);
		
		if (argv[KEY][0] != '-') // Mode 1
		{
			x_size = atoi(argv[X_SIZE]);
			y_size = atoi(argv[Y_SIZE]);
			game = new GameOfLife(x_size, y_size, numtasks, n_generations);
		}
		else switch (argv[KEY][1]) // Mode 2
		{
			case 'L':
			{	
				game = new GameOfLife(load(argv[FILENAME]), numtasks, n_generations);							
				break;
			}
			
			default:
			{
				printf(">>> Unknown key: %s\n", argv[KEY]);
				MPI_Finalize();
				return -2;
			}
		}
    }
    
    {
		Node *node = new Node(rank);
		
		for (int gen = 0; gen < node->n_generations(); gen ++) node->iterate();
		
		node->end();
    }

    if (rank == 0) game->end();
        
    MPI_Finalize();
    return 0;
}
