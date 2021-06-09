#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ncursesw/ncurses.h>

#define ROWS 10
#define COLS 10
#define EASY 500000
#define HARD 250000
#define SAVEFILE "tetris.sav"
#define MAXSAVE 100

char table[ROWS][COLS] = {0};
char *saves[MAXSAVE] = {NULL};
int score = 0;
bool GameOn = true;
bool MenuOn = false;
useconds_t timer = EASY; //half second
int decrease = 1000;

typedef struct
{
    char **array;
    int width, row, col;
} piece;
piece current;

const piece pieces[] = {
    {(char *[]){(char[]){0, 1, 1}, (char[]){1, 1, 0}, (char[]){0, 0, 0}}, 3},                               //S_shape
    {(char *[]){(char[]){1, 1, 0}, (char[]){0, 1, 1}, (char[]){0, 0, 0}}, 3},                               //Z_shape
    {(char *[]){(char[]){0, 1, 0}, (char[]){1, 1, 1}, (char[]){0, 0, 0}}, 3},                               //T_shape
    {(char *[]){(char[]){0, 0, 1}, (char[]){1, 1, 1}, (char[]){0, 0, 0}}, 3},                               //L_shape
    {(char *[]){(char[]){1, 0, 0}, (char[]){1, 1, 1}, (char[]){0, 0, 0}}, 3},                               //ML_shape
    {(char *[]){(char[]){1, 1}, (char[]){1, 1}}, 2},                                                        //SQ_shape
    {(char *[]){(char[]){0, 0, 0, 0}, (char[]){1, 1, 1, 1}, (char[]){0, 0, 0, 0}, (char[]){0, 0, 0, 0}}, 4} //R_shape
};

void menu_print();
bool load_game();
bool load_saves();
void save_game();
piece piece_copy(piece);
void piece_del(piece);
bool check_pos(piece);
void piece_new();
void piece_rotate(piece);
void table_add();
void tetris_move();
void table_print();
void piece_move(int);

int main()
{
    srand(time(0));
    score = 0;
    int c;
    initscr();
    menu_print();

    struct timeval before, after;
    gettimeofday(&before, NULL);
    timeout(1);
    inline int is_later()
    {
        return ((useconds_t)(after.tv_sec * 1000000 + after.tv_usec) - ((useconds_t)before.tv_sec * 1000000 + before.tv_usec)) > timer;
    }
    piece_new();
    // menu_print();
    table_print();
    while (GameOn)
    {
        if (!MenuOn)
        {
            fflush(stdin);
            if ((c = getch()) != ERR)
            {
                piece_move(c);
            }
            gettimeofday(&after, NULL);
            if (is_later())
            { //time difference in microsec accuracy
                piece_move('s');
                gettimeofday(&before, NULL);
            }
        }
    }
    piece_del(current);
    endwin();
    int i, j;
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            printf("%c ", table[i][j] ? '#' : '.');
        }
        printf("\n");
    }
    printf("\nGame over!\n");
    printf("\nScore: %d\n", score);
    return 0;
}

void menu_print()
{
    int cc;
    bool flag = true;
    do
    {
        fflush(stdin);
        printw("[1] Play Game with EASY\n");
        printw("[2] Play Game with HARD\n");
        printw("[3] Load Game\n");
        printw("Enter your choice: ");
        cc = getch();
        switch (cc)
        {
        case '1':
            timer = EASY;
            flag = false;
            break;
        case '2':
            timer = HARD;
            flag = false;
            break;
        case '3':
            if (load_saves())
            {
                printw("\nGame Loaded\n\n");
            }
            else
                printw("\nError game loading\n\n");
            // flag = false;
            break;
        }
    } while (flag);
}

bool load_saves()
{
    FILE *fptr = fopen(SAVEFILE, "r");
    if (!fptr)
    {
        printw("There are no save files!\n");
        return false;
    }
    char *line_buf = NULL;
    size_t line_buf_size = 0;
    // int line_count = 0;
    ssize_t line_size;

    /* Get the first line of the file. */
    line_size = getline(&line_buf, &line_buf_size, fptr);

    /* Loop through until we are done with the file. */
    for (int i = 0; line_size >= 0 && i < MAXSAVE; i++)
    {
        saves[i] = strdup(line_buf);
        saves[i][strlen(saves[i]) - 1] = '\0';
        /* Increment our line count */
        // line_count++;
        /* Show the line details */
        // printf("line[%06d]: chars=%06zd, buf size=%06zu, contents: %s", line_count,
        //    line_size, line_buf_size, line_buf);

        /* Get the next line */
        line_size = getline(&line_buf, &line_buf_size, fptr);
    }

    /* Free the allocated line buffer */
    free(line_buf);
    line_buf = NULL;
    fclose(fptr);
    printw("\n");
    endwin();
    for (int i = 0; saves[i] != NULL; i++)
    {
        printf("[%i]: %s\n", i, saves[i]);
    }
    printf("\nEnter file to load: ");
    int f;
    // clear();
    fflush(stdin);
    f = getchar();
    fflush(stdin);

    printw("\n");
    initscr();
    // scanf("%i", &f);
    printw("Loading [%s]\n", saves[(int)f - 48]);
    return load_game((int)f - 48);
}

bool load_game(int file)
{
    char *fname = saves[file];
    FILE *fptr = fopen(fname, "r");
    if (!fptr)
    {
        printw("Error: File '%s' not opening\n", fname);
        return false;
    }
    int score = 1;
    fscanf(fptr, "%d", &score);
    // printf("score: %d\n", score);
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            fscanf(fptr, "%d", &table[i][j]);
        }
    }
    // for (int i = 0; i < 5; i++)
    // {
    //     for (int j = 0; j < 5; j++)
    //     {
    //         printf("%c ", table[i][j] ? '#' : '.');
    //     }
    //     printf("\n");
    // }
    return true;
}

void save_game()
{
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    char str[50];
    sprintf(str, "%02d%02d%d_%02d-%02d", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min);
    printw("Saving.... %s\n", str);
    FILE *fptr = fopen(str, "w");
    if (!fptr)
    {
        printw("Error:101\n");
        return;
    }
    fprintf(fptr, "%d\n", score);
    // printw("score: %d\n", score);
    for (int i = 0; i < ROWS; i++)
    {
        for (int j = 0; j < COLS; j++)
        {
            // printw("%d %d\n", i, j);
            fprintf(fptr, "%d ", table[i][j]);
        }
        fprintf(fptr, "\n");
    }
    fclose(fptr);
    fptr = fopen(SAVEFILE, "a");
    if (!fptr)
    {
        printw("There is no SAVEFILE!\n");
        return;
    }
    fprintf(fptr, "%s\n", str);
    fclose(fptr);
}

piece piece_copy(piece shape)
{
    piece new_shape = shape;
    char **copyshape = shape.array;
    new_shape.array = (char **)malloc(new_shape.width * sizeof(char *));
    int i, j;
    for (i = 0; i < new_shape.width; i++)
    {
        new_shape.array[i] = (char *)malloc(new_shape.width * sizeof(char));
        for (j = 0; j < new_shape.width; j++)
        {
            new_shape.array[i][j] = copyshape[i][j];
        }
    }
    return new_shape;
}

void piece_del(piece shape)
{
    int i;
    for (i = 0; i < shape.width; i++)
    {
        free(shape.array[i]);
    }
    free(shape.array);
}

bool check_pos(piece shape)
{ //Check the position of the copied shape
    char **array = shape.array;
    int i, j;
    for (i = 0; i < shape.width; i++)
    {
        for (j = 0; j < shape.width; j++)
        {
            if ((shape.col + j < 0 || shape.col + j >= COLS || shape.row + i >= ROWS))
            {                    //Out of borders
                if (array[i][j]) //but is it just a phantom?
                    return false;
            }
            else if (table[shape.row + i][shape.col + j] && array[i][j])
                return false;
        }
    }
    return true;
}

void piece_new()
{ //returns random shape
    piece new_shape = piece_copy(pieces[rand() % 7]);

    new_shape.col = rand() % (COLS - new_shape.width + 1);
    new_shape.row = 0;
    piece_del(current);
    current = new_shape;
    if (!check_pos(current))
    {
        GameOn = false;
    }
}

void piece_rotate(piece shape)
{ //rotates clockwise
    piece temp = piece_copy(shape);
    int i, j, k, width;
    width = shape.width;
    for (i = 0; i < width; i++)
    {
        for (j = 0, k = width - 1; j < width; j++, k--)
        {
            shape.array[i][j] = temp.array[k][i];
        }
    }
    piece_del(temp);
}

void table_add()
{
    int i, j;
    for (i = 0; i < current.width; i++)
    {
        for (j = 0; j < current.width; j++)
        {
            if (current.array[i][j])
                table[current.row + i][current.col + j] = current.array[i][j];
        }
    }
}

void tetris_move()
{ //count lines
    int i, j, sum, count = 0;
    for (i = 0; i < ROWS; i++)
    {
        sum = 0;
        for (j = 0; j < COLS; j++)
        {
            sum += table[i][j];
        }
        if (sum == COLS)
        {
            count++;
            int l, k;
            for (k = i; k >= 1; k--)
                for (l = 0; l < COLS; l++)
                    table[k][l] = table[k - 1][l];
            for (l = 0; l < COLS; l++)
                table[k][l] = 0;
            timer -= decrease--;
        }
    }
    if (count <= 0)
        count = 1;
    score += 10 * count;
}

void table_print()
{
    char Buffer[ROWS][COLS] = {0};
    int i, j;
    for (i = 0; i < current.width; i++)
    {
        for (j = 0; j < current.width; j++)
        {
            if (current.array[i][j])
                Buffer[current.row + i][current.col + j] = current.array[i][j];
        }
    }
    clear();
    for (i = 0; i < COLS - 9; i++)
        printw(" ");
    printw("Hunzlah Malik\n");
    for (i = 0; i < ROWS; i++)
    {
        for (j = 0; j < COLS; j++)
        {
            printw("%c ", (table[i][j] + Buffer[i][j]) ? '#' : '.');
        }
        printw("\n");
    }
    printw("\nScore: %d\n", score);
}

void piece_move(int action)
{
    piece temp = piece_copy(current);
    switch (action)
    {
    case 'm':
        MenuOn = true;
        // clear();
        // endwin();
        printw("\n\nDo you want to save and quit the game? y/n: ");
        // int c;
        // fflush(stdin);
        // c = getc(stdin);
        // fflush(stdin);

        char c;
        scanw("%c", c);
        if (c == 'y')
        {
            save_game();
            GameOn = false;
        }
        // initscr();
        MenuOn = false;
    case 's':
        temp.row++; //move down
        if (check_pos(temp))
            current.row++;
        else
        {
            table_add();
            tetris_move(); //check full lines, after putting it down
            piece_new();
        }
        break;
    case 'd':
        temp.col++; //move right
        if (check_pos(temp))
            current.col++;
        break;
    case 'a':
        temp.col--; //move left
        if (check_pos(temp))
            current.col--;
        break;
    case 'w':
        piece_rotate(temp); //yes
        if (check_pos(temp))
            piece_rotate(current);
        break;
    }
    piece_del(temp);
    table_print();
}