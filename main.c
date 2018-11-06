
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "solver.h"

//*******************************************************
// Defines
//*******************************************************


//*******************************************************
// Prototypes
//*******************************************************

    static void reposition(char *map);
    static void resymbol(char *map);

//*******************************************************
// Variables
//*******************************************************

int main(int argc, char *argv[])
{
    uint8_t mandatory_cells = 0;
    uint8_t remaining_cells = 81;
    uint8_t target;
    uint8_t tempu8;
    uint8_t difficulty=0;
    char    tempchar;

    bool    mandatory[81] = {[0 ... 80]=false};

    int     target_difficulty;

    char    map[82];
    char    remap[82] = "\
123456789\
789123456\
456789123\
912345678\
678912345\
345678912\
891234567\
567891234\
234567891\
";

   	srand(time(NULL));
    resymbol(remap);
    reposition(remap);

    printf("\r\n\r\n\
*******************************************\r\n\
    Sudoku generator by Michael.J Clift    \r\n\
*******************************************\r\n\
\r\n");
    printf("Select difficulty 1-7 >");
    scanf ("%d",&target_difficulty);
    target_difficulty = 1+((target_difficulty-1)%7);    //limit 1-7
    printf("Wait...\r\n");
    while(difficulty != target_difficulty)
    {
        strcpy(map, remap);
        mandatory_cells=0;
        remaining_cells=81;
        difficulty = 0;
        memset(mandatory, false, 81 * sizeof(bool));

        while(mandatory_cells != remaining_cells)
        {
            //choose random cell
            target = rand()%81;
            //if present, and not mandatory
            if(!mandatory[target] && (map[target]!='.'))
            {
                tempchar = map[target];
                map[target]='.';
                tempu8 = solver(map);
                if(tempu8 > 7)
                {
                    map[target]=tempchar;
                    mandatory[target]=true;
                    mandatory_cells++;
                }
                else
                {
                    if(tempu8 > difficulty)
                        difficulty=tempu8;
                    remaining_cells--;
                };
            };
        };
    };

    printf("\r\nDifficulty %i out of 7\r\n\r\n", (int)difficulty);
    printf("May require:\r\n     Sole candidate\r\n");
    if(difficulty > 1)
        printf("     Unique candidate\r\n");
    if(difficulty > 2)
        printf("     Eliminate in row/column due to pointing in square\r\n");
    if(difficulty > 3)
        printf("     Eliminate in square, due to pointing in row or column\r\n");
    if(difficulty > 4)
        printf("     Naked set\r\n");
    if(difficulty > 5)
        printf("     Hidden set\r\n");
    if(difficulty > 6)
        printf("     X wing\r\n");

    printf("\r\n%s", &map[9*8]);printf("\r\n");map[9*8]=0;
    printf("%s", &map[9*7]);printf("\r\n");map[9*7]=0;
    printf("%s", &map[9*6]);printf("\r\n");map[9*6]=0;
    printf("%s", &map[9*5]);printf("\r\n");map[9*5]=0;
    printf("%s", &map[9*4]);printf("\r\n");map[9*4]=0;
    printf("%s", &map[9*3]);printf("\r\n");map[9*3]=0;
    printf("%s", &map[9*2]);printf("\r\n");map[9*2]=0;
    printf("%s", &map[9*1]);printf("\r\n");map[9*1]=0;
    printf("%s", map);printf("\r\n\r\n(Press enter to exit)");

    fgets(map, 9, stdin);
    fgets(map, 9, stdin);

	return 0;
}

//perform random symbol arrangement
static void resymbol(char *map)
{
    uint8_t tempu8;
    char    subst_x[10]="123456789";
    char    subst_y[10]="";
    char    tempbuf[2]=" ";

//  create symbol subst table
	while(strlen(subst_x))
    {
        tempu8 = rand()%strlen(subst_x);    //choose a random digit from x
        tempbuf[0] = subst_x[tempu8];
        strcat(subst_y, tempbuf);           //add to y
        memmove(&subst_x[tempu8], &subst_x[tempu8+1], strlen(&subst_x[tempu8])); //remove digit from x
    };

//  shuffle symbols
    tempu8=0;
    while(tempu8 != 81)
    {
        map[tempu8] = subst_y[(map[tempu8]&0x0F) -1];
        tempu8++;
    };
}

//reorder rows within squares, cols within squares, and squares themselves
static void reposition(char *map)
{
    //  used to shuffle rows/cols and also squares
    const uint8_t reorder[6][3] = {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};

    //to be loaded with rand values 0-5
    uint8_t order_col[3];
    uint8_t order_sqy;
    uint8_t order_row[3];
    uint8_t order_sqx;

    uint8_t sqx, sqy, col, row;

    char result[82];

    order_col[0] = rand()%6;
    order_col[1] = rand()%6;
    order_col[2] = rand()%6;
    order_row[0] = rand()%6;
    order_row[1] = rand()%6;
    order_row[2] = rand()%6;
    order_sqy    = rand()%6;
    order_sqx    = rand()%6;

    #define newsqy  (reorder[order_sqy][sqy])
    #define newsqx  (reorder[order_sqx][sqx])
    #define newrow  (reorder[order_row[sqy]][row])
    #define newcol  (reorder[order_col[sqx]][col])

    for(sqy=0; sqy<3; sqy++)
        for(sqx=0; sqx<3; sqx++)
            for(row=0; row<3; row++)
                for(col=0; col<3; col++)
    result[newsqy*27 + newrow*9 + newsqx*3 + newcol] = map[sqy*27 + row*9 + sqx*3 + col];
    result[81]=0;
    strcpy(map, result);
}
