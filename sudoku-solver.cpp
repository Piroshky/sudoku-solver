#include<iostream>
#include<fstream>
#include<unistd.h>
using namespace std;

void print_puzzle();
void reprint_puzzle();
void place_number(uint8_t y, uint8_t x, uint8_t value, bool reprint);
void integrity_check();

void onexit() {
  printf("\E[?25h"); //sets cursor to show again, some terminals don't reset automatically when a program closes
}

uint8_t remaining = 81;
uint8_t grid[9][9] = {0};

// first number is how many possible numbers that cell could be
uint8_t potential[9][9][10];

uint8_t inferred[9][10] = {0}; // or known
uint8_t updated = 0;

uint8_t found = 0;
uint8_t found_row[81];
uint8_t found_col[81];

// option related variables
bool animate = false;
bool plain = false;
int sleep_time = 36000;
int cells_to_find = -1;

int main (int argc, char *argv[]) {
  char *filename = NULL;  

  // parse commandline args
  for (int ix = 1; ix < argc; ++ix) {
    if (argv[ix][0] == '-') {
      if(argv[ix][1] == 'a'){
	animate = true;
      } else if (argv[ix][1] == 'A') {
	animate = true;
	if (ix+1 >= argc || !isdigit(argv[ix+1][0])) {
	  printf("error: -A option (animation time in ms) requires numeric argument\n");
	  exit(1);
	}
	int ms = stoi(argv[ix+1], nullptr);
	sleep_time = ms*1000;
	++ix;
      } else if (argv[ix][1] == 'p') {
	plain = true;
      } else if (argv[ix][1] == 'n'){
	if (ix+1 >= argc || !isdigit(argv[ix+1][0])) {
	  printf("error: -n option (number of cells to find) requires numeric argument\n");
	  exit(1);
	}
	cells_to_find = stoi(argv[ix+1], nullptr);
	++ix;
      } else {
	printf("error: unrecognized argument, '-%c'\n", argv[ix][1]);
	exit(1);
      }
    } else if (filename == NULL) {
      filename = argv[ix];
    }
  }
  if (filename == NULL) {
    printf("usage: %s file [options]\n"
	   "options: a     animate, turn on animation\n"
	   "         A <x> turn on animation and set animation time to <x>\n"
	   "         p     plain, only print numbers (and spaces for unfilled characters)\n"
	   "         n <x> only find <x> more cells\n", argv[0]);
    exit(1);
  }
  
  fstream input;
  input.open(filename, ios::in);

  if (input.fail()) {
    printf("could not open file %s\n", filename);
    exit(1);
  }

  // maybe there's some way to do this with initialization values, but not that I could find.
  for(int i = 0; i < 9; ++i) {
    for(int j = 0; j < 9; ++j) {
      potential[i][j][0] = 9;
      for(int k = 1; k < 10; ++k) {
	potential[i][j][k] = 1;
      }
    }
  }

  // read file
  for(int row = 0; row < 9; ++row) {
    // char line[10];
    string line;
    line.reserve(9);
    getline(input, line, '\n');
    int chars = line.length();
    if (chars > 9) {chars = 9;}
    for(int col = 0; col < chars; ++col) {
      if (line[col] == ' ' || line[col] == '0') {
	// do nothing
      } else if (isdigit(line[col])) {
	place_number(row, col, line[col] - '0', false);	
      } else {
	fprintf(stderr, "unexpected character on line %d:%d, \"\"\n", row+1, col+1);
	exit(1);
      }
    }
  }
  if (cells_to_find != -1 && cells_to_find <= remaining) {
    remaining = cells_to_find;
  }

  printf("\E[?25l");
  atexit(onexit);
  print_puzzle();

  while (remaining > 0 && (found != 0 || updated != 0)) {
    // printf("found %d\n", found);
    // printf("updated %d\n", updated);
    //printf("remaining %d\n", remaining);
    updated = 0;

   // integrity_check();
    
    // update potentials
    for (int i = 0; i < found; ++i) {
      uint8_t row = found_row[i];
      uint8_t col = found_col[i];
      uint8_t val = grid[row][col];
      // printf("%d x %d\n", row, col);
    
      for (auto y = 0; y < 9; ++y) {
	// printf("-- %d %d, %d\n", y, col, potential[y][col][val]);
	if (grid[y][col] == 0 && potential[y][col][val] == 1) {
	  // printf("col clearing %d %d, %d, %d\n", y, col, potential[y][col][0], grid[y][col]);
	  potential[y][col][0] -= 1;
	  potential[y][col][val] = 0;
	}
      }

      // clear out row
      for (auto x = 0; x < 9; ++x) {
	if (grid[row][x] == 0 && potential[row][x][val] == 1) {
	  // printf("row clearing %d %d\n", row, x);
	  potential[row][x][0] -= 1;
	  potential[row][x][val] = 0;
	}
      }
	
      // clear out square
      auto square_y = row / 3;
      auto square_x = col / 3;
      // printf("row, col: %d %d |square x y, %d %d\n", row, col, square_x, square_y);
      for (auto y = square_y * 3; y < (square_y + 1) * 3; ++y) {
	for (auto x = square_x * 3; x < (square_x + 1) * 3; ++x) {
	  if (grid[y][x] == 0 && potential[y][x][val] == 1) {
	    // printf("squ clearing %d %d\n", y, x);
	    potential[y][x][0] -= 1;
	    potential[y][x][val] = 0;		      
	  }
	}
      }
    }
    reprint_puzzle();
 
    found = 0;
  
    // find cells that only have one potential value
    for (auto row = 0; row < 9; ++row) {
      for (auto col = 0; col < 9; ++ col) {
	if (grid[row][col] == 0 && potential[row][col][0] == 1) {
	  // printf("(%u) %d, %d ", potential[row][col][0], row, col);          
	  for (uint8_t i = 1; i < 10; ++i) {
	    if (potential[row][col][i] == 1) {
	      place_number(row, col, i, true);
	      // printf("(o) playing %d at %d %d\n", i, row, col);
	      break;
	    }
	  }
	}
      }
    }

    // find only cell in row/column/squares that can have specific value

    // columns
    for (int col = 0; col < 9; ++col) {
      uint8_t in_row[10] = {0};
      for (int row = 0; row < 9; ++row) {
	if (grid[row][col] != 0) {
	  in_row[grid[row][col]] = 1;
	}
      }
      for (int i = 1; i < 10; ++i) {
	uint8_t possible_y = 10;
	if (in_row[i] == 0) {
	  for (int row = 0; row < 9; ++row) {
	    if (grid[row][col] == 0 && potential[row][col][i] == 1) {
	      if (possible_y != 10) {
		possible_y = 10;
		break;
	      } else {
		possible_y = row;
	      }
	    }
	  }
	  if (possible_y != 10) {
	    // printf("(c) playing %d at %d %d\n", i, possible_y, col);
	    place_number(possible_y, col, i, true);
	  }
	}
      }
    }
    
    // rows    
    for (int row = 0; row < 9; ++row) {
      uint8_t in_col[10] = {0};
      for (int col = 0; col < 9; ++col) {
	if (grid[row][col] != 0) {
	  in_col[grid[row][col]] = 1;
	}
      }
      
      for (int i = 1; i < 10; ++i) {
	uint8_t possible_x = 10;
	if (in_col[i] == 0) {
	  for (int col = 0; col < 9; ++col) {
	    if (grid[row][col] == 0 && potential[row][col][i] == 1) {
	      if (possible_x != 10) {
		possible_x = 10;
		break;
	      } else {
		possible_x = col;
	      }
	    }
	  }
	  if (possible_x != 10) {
	    // printf("(r) playing %d at %d %d\n", i, row, possible_x);
	    place_number(row, possible_x, i, true);
	  }
	}
      }
    }

    // squares
    for (int square = 0; square < 9; ++square) {
      uint8_t sq_y = 3 * (square / 3);
      uint8_t sq_x = 3 * (square % 3);

      uint8_t in_squ[10] = {0};
      for (int row = 0; row < 3; ++row){
	for (int col = 0; col < 3; ++col) {
	  // printf(" found %d\n ",grid[sq_y+row][sq_x+col]);
	  if (grid[sq_y+row][sq_x+col] != 0) {
	    // printf("in square found %d\n ",grid[sq_y+row][sq_x+col]);
	    in_squ[grid[sq_y+row][sq_x+col]] = 1;
	  }
	}
      }

      for (int i = 1; i < 10; ++i) {
	uint8_t possible_x = 10;
	uint8_t possible_y = 10;
	if (in_squ[i] == 0) {
	  // printf("checking for %d in sq %d %d\n", i, sq_y, sq_x);
	  for (int row = 0; row < 3; ++row){
	    for (int col = 0; col < 3; ++col) {
	      // printf("  %d %d\n", sq_y+row, sq_x+col);
	      if (grid[sq_y+row][sq_x+col] == 0 && potential[sq_y+row][sq_x+col][i] == 1) {
                // printf("  here\n");
		if (possible_x != 10) {
		  possible_x = 10;
		  possible_y = 11;
		  break;
		} else {
		  possible_y = sq_y+row;
		  possible_x = sq_x+col;
		  // printf("poss %d @ %d %d\n", i, possible_y, possible_x);
		}
	      }
	    }
	    if (possible_y == 11) {
	      break;
	    }
	  }
	  if (possible_x != 10) {
	    // for(int j = 1; j < 10; ++j) {
	    //   printf("%d ", in_squ[j]);
	    // }
	    // printf("\n");	  
	    // printf("(s) playing %d at %d %d\n", i, possible_y, possible_x);
	    place_number(possible_y, possible_x, i, true);
	  }	  
	}
      }    
    }
    
    // inferring cols and rows in squares
    if (found == 0) {
      for (int square = 0; square < 9; ++square) {
	uint8_t sq_y = 3 * (square / 3);
	uint8_t sq_x = 3 * (square % 3);

	uint8_t in_squ[10] = {0};
	for (int row = 0; row < 3; ++row){
	  for (int col = 0; col < 3; ++col) {
	    if (grid[sq_y+row][sq_x+col] != 0) {
	      in_squ[grid[sq_y+row][sq_x+col]] = 1;
	    }
	  }
	}
	// printf("[ ");
	// for (int i = 1; i < 10; ++i) {
	//   printf("%d ", in_squ[i]);
	// }
	// printf("]\n");

	for (int i = 1; i < 10; ++i) {
	  if (in_squ[i] == 0) {
	    uint8_t pos_x[4];
	    uint8_t pos_y[4];
	    uint8_t pos_found = 0;
	    for (int row = 0; row < 3; ++row) {
	      for (int col = 0; col < 3; ++col) {
		if (grid[sq_y+row][sq_x+col] == 0 && potential[sq_y+row][sq_x+col][i] == 1) {
		  pos_x[pos_found] = sq_x+col;
		  pos_y[pos_found] = sq_y+row;
		  ++pos_found;
		}
		if (pos_found > 3) {
		  break;
		}
	      }
	      if (pos_found > 3) {
		break;
	      }
	    }
	    // There must be a better way
	    if (pos_found > 3 || pos_found == 0) {
	      continue;
	    }
	    uint8_t last = pos_x[0];
	    for (int j = 1; j < pos_found && last != 10; ++j) {
	      if (last != pos_x[j]) {last = 10;}
	    }
	    if (last != 10) {
	      // clear out other squares in col
 	      for (int k = 0; k < 3; ++k) {
		if (3*k != sq_y) {
		  for (int m = 0; m < 3; ++m) {
		    if (grid[k*3+m][last] == 0 && potential[k*3 + m][last][i] == 1) {
		      ++updated;
		      potential[k*3 + m][last][i] = 0;
		      --potential[k*3 + m][last][0];
		    }
		  }
		}
	      }
	      continue;
	    }
	    
	    last = pos_y[0];
	    for (int j = 1; j < pos_found && last != 10; ++j) {
	      if (last != pos_y[j]) {last = 10;}
	    }
	    if (last != 10) {
	      // printf("In square %d %d (%d) %d must be in row %d. \n", sq_y, sq_x, square, i, last);
	      // clear out other squares in row
	      for (int k = 0; k < 3; ++k) {
		if (3*k != sq_x) {
		  for (int m = 0; m < 3; ++m) {
		    if (grid[last][k*3 + m] == 0 && potential[last][k*3 + m][i] == 1) {
		      ++updated;
		      potential[last][k*3 + m][i] = 0;
		      --potential[last][k*3 + m][0];
		    }
		  }
		}
	      }
	    }
	  }
	}
     }
    }
  }
}

void reprint_puzzle() {
  if (!plain) {
    printf("\E[13A");
  } else {
    printf("\E[9A");
  }
  
  print_puzzle();
  if (animate) {usleep(sleep_time);}
}

void print_puzzle() {
  if (!plain) {printf("┌───────┬───────┬───────┐\n");}
  for (auto r = 0; r < 9; ++r) {
    char line[9];
    for (int i = 0; i < 9; ++i) {
      if (grid[r][i] == 0) {
	line[i] = ' ';
      } else {
	line[i] = '0' + grid[r][i];
      }
    }
    if (!plain) {
      printf("│ %c %c %c │ %c %c %c │ %c %c %c │\n",	\
	   line[0],line[1],line[2],line[3], \
	   line[4],line[5],line[6],line[7],line[8]);
    } else {
      printf("%c%c%c%c%c%c%c%c%c\n",	\
	     line[0],line[1],line[2],line[3], \
	     line[4],line[5],line[6],line[7],line[8]);      
    }
    if (!plain && (r == 2 || r == 5)) {
      printf("├───────┼───────┼───────┤\n");      
    }
  }
  if (!plain) {printf("└───────┴───────┴───────┘\n");}
}

void place_number(uint8_t y, uint8_t x, uint8_t value, bool reprint) {
  if (remaining == 0) {return;}
  grid[y][x] = value;
  --remaining;
  potential[y][x][0] = 0;
  found_row[found] = y;
  found_col[found] = x;
  ++found;
  if (reprint) {reprint_puzzle();}
}

void integrity_check() {
  for(int row = 0; row < 9; ++row) {
    for(int col = 0; col < 9; ++col) {
      if (grid[row][col] != 0 ) {
	if (potential[row][col][0] != 0) {
  	  printf("ERR: filled cell <%d,%d> has potential of %d\n", row, col, potential[row][col][0]);
	}
	for (int r = 0; r < 9; ++r) {
	  if (row != r && grid[row][col] == grid[r][col]) {
	    printf("Err: cells in same column with same value, <%d,%d> and <%d,%d> are both %d\n", row, col, r, col, grid[row][col]);
	  }
	}
	for (int c = 0; c < 9; ++c) {
	  if (col != c && grid[row][col] == grid[row][c]) {
	    printf("Err: cells in same row with same value, <%d,%d> and <%d,%d> are both %d\n", row, col, row, c, grid[row][col]);
	  }
	}
	int square_y = 3*(row / 3);
	int square_x = 3*(col / 3);
	for (int y = 0; y < 3; ++y) {       
	  for (int x = 0; x < 3; ++x) {            
	    if (square_y+y != row && square_x+x != col) {
	      if (grid[row][col] == grid[square_y+y][square_x+x]) {
		printf("sy %d, sx %d\n",square_y, square_x);
		printf("Err: cells in same square with same value, <%d,%d> and <%d,%d> are both %d\n", row, col, square_y+y, square_x+x, grid[row][col]);
	      }
	    }
	  }
	}
      } else {
	int potentials = 0;
	for (int i = 1; i < 10; ++i) {
	  if (potential[row][col][i] == 1) {
	    ++potentials;
	  } else if (potential[row][col][i] != 0) {
	    printf("ERR: <%d,%d,%d> is %d\n", row, col, i, potential[row][col][i]);
	  }
	}
	if (potential[row][col][0] != potentials) {
	  printf("ERR: recorded potentials (%d), actual potentials (%d)\n", potential[row][col][0], potentials);
	}
      }
    }
  }
}
