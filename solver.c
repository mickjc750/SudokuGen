/*

All positions are indexed from 0 (column 0-8, row 0-8 etc).
All values are referenced as actual values 1-9 (no 0)

2 cell addressing schemes

	X-Y coodrdinates

	N-I
	N is row/column/square (0-8)
	I is cell index(0-8) within that row/column/square
	Squares are numbered top left to bottom right, accross then down
	Columns are left to right (same X)
	Rows are top to bottom (same Y)

	Terms
		RCS	Row Column or Square
		RC	Row or Column

*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

//*******************************************************
// Defines
//*******************************************************

 //   #define printf(...)     do{}while(false)
 //   #define GETCH()         do{}while(false)

	#define ROW		0
	#define COLUMN	1
	#define SQUARE	2

//	macro to get cells row/column/square n, by specefying it's index within a different column/square/row
	#define TRANS_TYPE_N(out_type, in_type, in_n, in_i)	n_of(out_type, x_of(in_type, in_n, in_i), y_of(in_type, in_n, in_i))

//	macro to get cells index within it's row/column/square by specefying it's index within a different column/square/row
	#define TRANS_TYPE_I(out_type, in_type, in_n, in_i) i_of(out_type, x_of(in_type, in_n, in_i), y_of(in_type, in_n, in_i))

	//for each cell store possible 1-9 and solved value
	//0 indicates not possible/not solved
	struct cell_struct
	{
		uint8_t	possible[10];	//index 0 not used, index 1-9 loaded with 1-9 to represent possible
		uint8_t	certain;		//solved value
	};

	//used to avoid multi-dimensional arrays
	struct mask_struct
	{
		uint8_t mask[9];
		uint8_t count;
	};

//*******************************************************
// Public Variables
//*******************************************************

//*******************************************************
// Private Variables
//*******************************************************

    static char solver_map[82];

	static struct cell_struct map[81];

	static uint8_t	cells_solved;

//*******************************************************
// Prototypes
//*******************************************************

	static void 		init_map(void);
	static void			cell_solve(uint8_t x, uint8_t y, uint8_t value);
	static bool		    read_map(void);
	static uint8_t 		cell_possible_count(struct cell_struct *cell);
	static bool 		remove_from(uint8_t type, uint8_t n, uint8_t *list, uint8_t list_size, uint8_t *mask);
	static uint8_t		count_mask(uint8_t *x_ptr);
	static uint8_t 		population_in_rcs(uint8_t type, uint8_t n, uint8_t value, uint8_t *mask);
	static uint8_t 		finger_step(uint8_t *thumb_ptr, uint8_t *finger_ptr, uint8_t last_index);
	static void 		mask_or(uint8_t *target, uint8_t *source);

//	Type conversion
	static uint8_t		n_of(uint8_t type, uint8_t x, uint8_t y);
	static uint8_t		i_of(uint8_t type, uint8_t x, uint8_t y);
	static uint8_t		x_of(uint8_t type, uint8_t n, uint8_t i);
	static uint8_t		y_of(uint8_t type, uint8_t n, uint8_t i);

//	map addressing
	static struct cell_struct* 	map_get(uint8_t type, uint8_t n, uint8_t cellno);

//	solving methods
	static bool 		sole_candidate(void);
	static bool 		unique_candidate(void);
	static bool 		pointing(uint8_t type_a, uint8_t type_b);
	static bool 		naked_set(void);
	static bool		    hidden_set(void);
	static bool 		xwing(void);

//*******************************************************
// Functions
//*******************************************************

uint8_t solver(char* map)
{
	bool success = true;
	uint8_t retval=0;
    uint8_t difficulty=0;

	memcpy(solver_map, map, 81);

	init_map();
    cells_solved=0;

	success = read_map();
	if(!success)
		printf("Error in map\r\n\r\n");

	if(success)
	{
		while(success && (cells_solved < 81))
		{
			difficulty=1;
            success = sole_candidate();
            if(!success)
            {
                difficulty=2;
                success = unique_candidate();
            };
			if(!success)
            {
                difficulty=3;
				success = pointing(SQUARE, COLUMN);
            };
			if(!success)
            {
                difficulty=3;
				success = pointing(SQUARE, ROW);
            };
			if(!success)
            {
                difficulty=4;
				success = pointing(ROW, SQUARE);
            };
			if(!success)
            {
                difficulty=4;
 				success = pointing(COLUMN, SQUARE);
            };
			if(!success)
            {
                difficulty=5;
				success = naked_set();
            };
			if(!success)
            {
                difficulty=6;
				success = hidden_set();
            };
			if(!success)
            {
                difficulty=7;
				success = xwing();
            };
			if(!success)
                difficulty=99;
			if(difficulty > retval)
                retval = difficulty;
        };
	};

    return retval;
}

//*******************************************************
// misc functions
//*******************************************************

//remove list of candidates from RCS 'n'
//the list may contain 0's which will be ignored, and may be another cells possible[] array
//no text is output, returns true if any candidates were removed
//only acts on cells with non0 values in the mask[9] array
static bool remove_from(uint8_t type, uint8_t n, uint8_t *list, uint8_t list_size, uint8_t *mask)
{
	uint8_t i=0, list_index;
	bool success=false;

	while(i != 9)
	{
		if(mask[i])
		{
			list_index=0;
			while(list_index != list_size)
			{
				if(list[list_index])
				{
					if(map_get(type, n,i)->possible[list[list_index]])	//candidate to be removed?
					{
						success=true;
						map_get(type, n,i)->possible[list[list_index]]=0;
					};
				};
				list_index++;
			};
		};
		i++;
	};

	return success;
}

static void init_map(void)
{
	uint8_t x,y,z;
	y=0;
	while(y != 9)
	{
		x=0;
		while(x != 9)
		{
			map_get(COLUMN, x,y)->certain = 0;
			z = 0;
			while(z != 10)
			{
				map_get(COLUMN, x,y)->possible[z]=z;
				z++;
			};
			x++;
		};
		y++;
	};
}

// read file, each digit 1-9 represents a solved cell
// unsolved cells are represented by . or 0
// ignores all other characters, fails if < 81 cells found
// returns success
static bool read_map(void)
{
	char tempchar;
	bool finished=false;
	bool success=true;
	uint8_t tempu8;
	uint8_t x=0,y=0;

	do
	{
		tempchar = solver_map[x+y*9];

		if(tempchar==0)
		{
			finished=true;
			success=false;
		}
		else
		{
			//insert digit?
			if(tempchar == '.')
				tempchar = '0';
			if(('1' <= tempchar) && (tempchar <= '9'))
			{
				tempu8 = tempchar & 0x0F;
				if(map_get(COLUMN, x,y)->possible[tempu8])	//if value possible
					cell_solve(x,y,tempu8);				//solve cell
				else
				{
					finished=true;
					success=false;
				};
			};
			//advance to next cell?
			if(('0' <= tempchar) && (tempchar <= '9'))
			{
				x++;
				if(x==9)
				{
					x=0;
					y++;
					if(y==9)
					{
						finished=true;
						success=true;
					};
				};
			};
		};
	}while(!finished);

	return success;
}

static void cell_solve(uint8_t x, uint8_t y, uint8_t value)
{
	uint8_t	n;

	cells_solved++;

	//solve cell
	map_get(COLUMN, x,y)->certain = value;

	n=0;
	while(n!=9)
	{
		map_get(COLUMN, x,y)->possible[n+1]=0;						//remove all candidates within solved cell
		map_get(ROW,    n_of(ROW,x,y), n)->possible[value]=0;		//remove all candidates from row
		map_get(COLUMN, n_of(COLUMN,x,y), n)->possible[value]=0;	//remove all candidates from column
		map_get(SQUARE, n_of(SQUARE,x,y), n)->possible[value]=0;	//remove all candidates from square
		n++;
	};
}

//get RCS n 0-8 of x-y coordinate
static uint8_t	n_of(uint8_t type, uint8_t x, uint8_t y)
{
	uint8_t retval;

	if(type == SQUARE)
	{
		x /=3;
		y /=3;
		retval = x+y*3;
	}
	else if(type == ROW)
		retval = y;
	else
		retval = x;

	return retval;
}

//get cell index 0-8 of x-y coordinate within it's RCS
static uint8_t	i_of(uint8_t type, uint8_t x, uint8_t y)
{
	uint8_t retval;

	if(type == SQUARE)
	{
		x = x%3;
		y = y%3;
		retval = x+y*3;
	}
	else if(type == ROW)
		retval = x;
	else
		retval = y;

	return retval;
}

//get x coordinate of cell index i within RCS n
static uint8_t	x_of(uint8_t type, uint8_t n, uint8_t i)
{
	uint8_t retval;

	if(type == SQUARE)
	{
		i = i%3;
		n = n%3;
		retval = n*3+i;
	}
	else if(type == ROW)
		retval = i;
	else
		retval = n;

	return retval;
}

//get y coordinate of cell index i within RCS n
static uint8_t	y_of(uint8_t type, uint8_t n, uint8_t i)
{
	uint8_t retval;

	if(type == SQUARE)
	{
		i /=3;
		n /=3;
		retval = n*3+i;
	}
	else if(type == ROW)
		retval = n;
	else
		retval = i;

	return retval;
}

//count how many candidates are in a cell
static uint8_t cell_possible_count(struct cell_struct *cell)
{
	uint8_t count=0;
	uint8_t index=1;
	while(index != 10)
	{
		if(cell->possible[index])
			count++;
		index++;
	};

	return count;
}

//count how many true values are in a mask uint8_t[9]
static uint8_t	count_mask(uint8_t *x_ptr)
{
	uint8_t	index=0;
	uint8_t retval=0;

	while(index != 9)
	{
		if(x_ptr[index])
			retval++;
		index++;
	};

	return retval;
}

//get population count of a candidate within a rcs, also builds a mask if 'mask' is not NULL
static uint8_t population_in_rcs(uint8_t type, uint8_t n, uint8_t value, uint8_t *mask)
{
	uint8_t retval=0;
	uint8_t i=0;

	while(i != 9)
	{
		if(map_get(type, n, i)->possible[value])
		{
			retval++;
			if(mask)
				mask[i]=true;
		}
		else if(mask)
			mask[i]=false;

		i++;
	};
	return retval;
}

//used to generate all combinations of a specific size within 'last_index'
static uint8_t finger_step(uint8_t *thumb_ptr, uint8_t *finger_ptr, uint8_t last_index)
{
	uint8_t retval=0;

	if(finger_ptr[0] < last_index)
		finger_ptr[0]++;
	else
	{
		if(finger_ptr != thumb_ptr)
		{
			retval = finger_step(thumb_ptr, &finger_ptr[1], last_index-1);
			finger_ptr[0] = finger_ptr[1]+1;
		}
		else
			retval=1;//end of search
	};

	return retval;
}

//or source mask onto target mask
static void mask_or(uint8_t *target, uint8_t *source)
{
	uint8_t index=0;
	while(index != 9)
	{
		if(source[index])
			target[index]=true;
		index++;
	};
}

//***********************************************************************************************
// Map addressing (row/column/square) map array should only be accessed via this function
//***********************************************************************************************

//get cell# from row/col/square 'n'
static struct cell_struct* map_get(uint8_t type, uint8_t n, uint8_t cellno)
{
	struct cell_struct* ptr;
	uint8_t x,y,sqx,sqy;

	if(type == COLUMN)
		ptr = &map[n+cellno*9];
	else if(type == ROW)
		ptr = &map[cellno+n*9];
	else
	{
		x = n%3;
		y = n/3;

		sqx = cellno%3;
		sqy = cellno/3;

		ptr = &map[(x*3+sqx)+(y*3+sqy)*9];
	};
	return ptr;
}

//*******************************************************
// Methods
//*******************************************************

//Sole candidate, if a cell has only one possibility
static bool sole_candidate(void)
{
	uint8_t x, y;
	uint8_t value;
	uint8_t index;
	bool success=false, finished=false;
	struct cell_struct *cell_ptr;

	x=0;
	y=0;
	while(x!=9 && !finished)
	{
		y=0;
		while(y!=9 && !finished)
		{
			cell_ptr = map_get(COLUMN, x, y);
			if(cell_possible_count(cell_ptr)==1)
			{
				value=0;
				index=1;
				while(index != 10)	//get value
				{
					value += cell_ptr->possible[index];
					index++;
				};
				cell_solve(x,y,value);
				finished=true;
				success=true;
			};
			y++;
		};
		x++;
	};

	return success;
}

//Unique candidate
//If a particular number can only be in one place within a row column or square
static bool unique_candidate(void)
{
	uint8_t value=1, type;
	uint8_t n, i, count, posx, posy;
	bool finished=false, success=false;

	while((value !=10) && (!finished))
	{
		//check for unique candidate
		n=0;
		while((n != 9) && (!finished))
		{
			type=0;
			while(type !=3)
			{
				//count candidates within row/col/square
				i=0;
				count=0;
				while((i != 9) && (count < 2) && !finished)
				{
					if(map_get(type, n, i)->possible[value])
					{
						count++;
						posx = x_of(type, n, i);
						posy = y_of(type, n, i);
					};
					i++;
				};
				//unique candidate found?
				if(count==1)
				{
					cell_solve(posx, posy, value);
					finished = true;
					success = true;
				};
				type++;
			};
			n++;
		};

		value++;
	};

	return success;
}

//remove from type_b due to pointing in type_a
static bool pointing(uint8_t type_a, uint8_t type_b)
{
	bool	finished=false;
	bool	success=false;
	uint8_t	value, n, n_b, i, count;
	uint8_t	mask[9];

	value=1;
	while((value !=10) && !finished)
	{
		n=0;
		while(n!=9 && !finished)
		{
			count=0;
			i=0;
			memset(mask, 1, 9);
			while((i != 9) && (count !=2))
			{
				if(map_get(type_a, n, i)->possible[value])
				{
					mask[TRANS_TYPE_I(type_b, type_a, n, i)]=0;
					if(!count)
					{
						n_b = TRANS_TYPE_N(type_b, type_a, n, i);
						count=1;
					}
					else if(n_b != TRANS_TYPE_N(type_b, type_a, n, i))
						count=2;
				};
				i++;
			};
			if(count == 1)
			{
				if(remove_from(type_b, n_b, &value, 1, mask))
				{
					finished=true;
					success=true;
				};
			};
			n++;
		};
		value++;
	};
	return success;
}

//naked set (of any size)
static bool naked_set(void)
{
	uint8_t i_a, i_b, n=0;
	uint8_t set_size=0;
	uint8_t set_mask[9];
	uint8_t type;
	bool finished=false, success=false;

	while((n != 9) && !finished)
	{
		type=0;
		while(type!=3)
		{
			i_a=0;
			while((i_a != 8) && !finished)
			{
				set_size = 1;
				memset(set_mask, 1, 9);
				i_b = i_a+1;
				while(i_b !=9)
				{
					//if matching cell, and not solved
					if(!memcmp(map_get(type, n, i_a)->possible, map_get(type, n, i_b)->possible, 10) && !(map_get(type, n, i_a)->certain))
					{
						set_mask[i_a]=0;		//count matching cells and remove from mask
						set_mask[i_b]=0;
						set_size++;
					};
					i_b++;
				};
				if((set_size > 1) && (set_size == cell_possible_count(map_get(type, n, i_a))))	//complete naked set?
				{
					if(remove_from(type, n, map_get(type, n, i_a)->possible, 10, set_mask))
					{
						finished=true;
						success=true;
					};
				};
				i_a++;
			};
			type++;
		};
		n++;
	};

	return success;
}

//hidden set
// if N numbers are limited to N cells, then other candidates in those cells may be removed
// set size is 2-8
// if 1,2,3,4,5,6,7,8 only occur in cells a,b,c,d,e,f,g,h, any 9's within a-h can be eliminated
static bool hidden_set(void)
{
	uint8_t type=0;
	uint8_t n=0;
	uint8_t value;
	uint8_t set_size;
	uint8_t index;
	bool finished=false, success=false;
	struct mask_struct mask[10];	// access with mask[value 1-9].mask
	uint8_t fingers[8];
	uint8_t tempmask[9];
	uint8_t remove_list[10];
	bool flag;
	type=0;
	while(type !=3)
	{
		n=0;
		while(n!=9)
		{
			//get location masks for each candidate in rcs n
			value = 1;
			while(value != 10)
			{
				mask[value].count = population_in_rcs(type, n, value, mask[value].mask);
				value++;
			};

			set_size=2;
			while((set_size < 9) && !finished)
			{
				//init fingers
				index = 0;
				while(index != set_size)
				{
					fingers[index] = set_size-index;
					index++;
				};

				do
				{
				// fingers[0] -> fingers[set_size-1] are values we need to test for hidden set
				//	tempmask is or'd masks of values indexed by fingers
					memset(tempmask, 0, 9);
					index=0;
					flag=true;
					while(index != set_size)
					{
						mask_or(tempmask, mask[fingers[index]].mask);
						if(mask[fingers[index]].count==0)
							flag=false;
						index++;
					};
					//if hidden set found
					if(flag && (count_mask(tempmask) == set_size))
					{
						//build remove list (all values bar ones in set)
						index=0;
						while(index !=10)
						{
							remove_list[index]=index;
							index++;
						};
						index = 0;
						while(index != set_size)
						{
							remove_list[fingers[index]] = 0;
							index++;
						};

						if(remove_from(type, n, remove_list, 10, tempmask))
						{
							finished=true;
							success=true;
						};
					};
				}while(!finished && !finger_step(&fingers[set_size-1], fingers, 9));

				set_size++;
			};
			n++;
		};
		type++;
	};

	return success;
}

//xwing
//eg. if 7 appears twice only in 2 rows, and in the same positions, eliminate 7 from the columns
static bool xwing(void)
{
	struct mask_struct mask[9];	// access with mask[value].mask
	uint8_t type=0;
	uint8_t remove_type;
	uint8_t n=0;
	uint8_t value;
	uint8_t index, i1, i2;
	bool finished=false, success=false;
	uint8_t removemask[9];
	bool flag;
	uint8_t fingers[2];	//reference positions, so fingers explore values 0-8

	type=0;
	while(type !=2)
	{
		value=1;
		while(value != 10)
		{
			n=0;
			while(n!=9)
			{
				mask[n].count = population_in_rcs(type, n, value, mask[n].mask);
				n++;
			};
			fingers[1]=0;
			fingers[0]=1;
			do
			{
				//if x-wing found
				if( (mask[fingers[0]].count == 2) && !memcmp(mask[fingers[0]].mask, mask[fingers[1]].mask, 9))
				{
					index=0;
					i1=99;
					while(i1==99)
					{
						if(mask[fingers[0]].mask[index])
							i1=index;
						index++;
					};
					i2=99;
					while(i2==99)
					{
						if(mask[fingers[0]].mask[index])
							i2=index;
						index++;
					};
					memset(removemask, 1, 9);
					removemask[fingers[0]]=0;
					removemask[fingers[1]]=0;
					if(type==ROW)
						remove_type=COLUMN;
					else
						remove_type=ROW;
					flag = false;
					if( remove_from(remove_type, i1, &value, 1, removemask) )
						flag=true;
					if( remove_from(remove_type, i2, &value, 1, removemask) )
						flag=true;
					if(flag)
					{
						success=true;
						finished=true;
					};
				};
			}while(!finished && !finger_step(&fingers[1], fingers, 8));
			n++;
			value++;
		};
		type++;
	};
	return success;
}

