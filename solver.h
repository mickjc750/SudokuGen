/*

*/

//*******************************************************
// Defines
//*******************************************************

//*******************************************************
// Public Variables
//*******************************************************

//*******************************************************
// Prototypes
//*******************************************************
/*
    returns difficulty
    0 complete (nothing to do)
    1 sole candidate
    2 unique candidate
    3 squ pointing
    4 row or col pointing
    5 naked set
    6 hidden set
    7 xwing
    99 unsolved
*/
	uint8_t solver(char *map);
