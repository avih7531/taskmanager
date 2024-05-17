/*
 *   Logic for visual mode
 *   - handle start row
 *   - handle middle rows (toggle all x)
 *   - on the current y fill 0<=current_x
 *
 *   Logic for blinking
 *   - after the switch statement call a function called blink
 *   - in blink, make a new x_positions called x_positions_mirror
 *   - replace all selected boxes with highlight color in x_positions_mirror
 *   - switch between printing x_positions and x_positions_mirror every 250ms
 */

#include <sqlite3.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <linenoise.h>

// ANSI escape codes for color
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_YELLOW "\x1b[33m"
#define ANSI_COLOR_CYAN "\x1b[36m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_RESET "\x1b[0m"

// Global variable to store X positions
int x_positions[6][24] = {0}; // Adjusted to 24 rows

// Color of the block character
enum BlockColor { NONE, RED, GREEN, BLUE, YELLOW, CYAN, MAGENTA };

// Current color of the block character
enum BlockColor current_color = RED;

// Global variable to track whether visual mode is active
bool visual_mode = false;
int start_x = 0;
int start_y = 0;

// Function to clear screen
void clear_screen() { printf("\033[2J\033[H"); }

// Function to move cursor
void move_cursor(int x, int y) { printf("\033[%d;%dH", y, x); }

void draw_grid(int cursor_x, int cursor_y, const char *current_date) {
  clear_screen();

  // Column titles
  printf("  ");
  for (int i = 0; i < 6; i++) {
    printf("%3d", i);
  }
  printf("\n");

  // Define block character
  const char *block_char = "[\u2588]";

  // Check if it's a leap day and update block character
  if (strlen(current_date) == 10 && current_date[0] == '0' &&
      current_date[1] == '2' && current_date[3] == '2' &&
      current_date[4] == '9') {
    block_char = "[󰤇]";
  }

  if (strcmp(current_date, "06_28_2004") == 0) {
    block_char = "[󰃫]";
  }

  if (strcmp(current_date, "06_29_1980") == 0) {
    block_char = "[󰪰]";
  }

  if (strcmp(current_date, "03_28_2005") == 0) {
    block_char = "[󰄛]";
  }

  // Print grid
  for (int i = 0; i < 24; i++) {
    printf("%.2d ", (i < 21) ? i + 4 : i - 20); // Adjusted for labels
    for (int j = 0; j < 6; j++) {
      switch (x_positions[j][i]) {
      case NONE:
        printf("[ ]");
        break;
      case RED:
        printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET,
               block_char); // Block character in red
        break;
      case GREEN:
        printf(ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET,
               block_char); // Block character in green
        break;
      case BLUE:
        printf(ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET,
               block_char); // Block character in blue
        break;
      case YELLOW:
        printf(ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET,
               block_char); // Block character in yellow
        break;
      case CYAN:
        printf(ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET,
               block_char); // Block character in cyan
        break;
      case MAGENTA:
        printf(ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET,
               block_char); // Block character in magenta
        break;
      }
    }
    printf("\n");
  }

  // Print current color indicator
  printf("\nColor indicator: ");
  switch (current_color) {
  case NONE:
  case RED:
    printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET, block_char);
    break;
  case GREEN:
    printf(ANSI_COLOR_GREEN "%s" ANSI_COLOR_RESET, block_char);
    break;
  case BLUE:
    printf(ANSI_COLOR_BLUE "%s" ANSI_COLOR_RESET, block_char);
    break;
  case YELLOW:
    printf(ANSI_COLOR_YELLOW "%s" ANSI_COLOR_RESET, block_char);
    break;
  case CYAN:
    printf(ANSI_COLOR_CYAN "%s" ANSI_COLOR_RESET, block_char);
    break;
  case MAGENTA:
    printf(ANSI_COLOR_MAGENTA "%s" ANSI_COLOR_RESET, block_char);
    break;
  }
  printf("\n");

  // Calculate time worked
  int time_worked = 0;
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 24; j++) {
      if (x_positions[i][j] != NONE) {
        time_worked += 10; // 10 minutes per non-zero element
      }
    }
  }

  // Convert time worked to hours and minutes
  int hours = time_worked / 60;
  int minutes = time_worked % 60;

  // Print time worked
  printf("Time Worked: %d hr %d min\n", hours, minutes);

  // Print debugging info
  printf("\nDebugging:\nCoordinates (%d, %d)\nCursor Position (%d, "
         "%d)\nFilename %s\nMode %s",
         cursor_x, cursor_y, ((cursor_x - 2) / 3) - 1, cursor_y - 2,
         current_date, (visual_mode ? "visual " : "insert "));

  move_cursor(cursor_x, cursor_y);
  fflush(stdout);
}

// Function to enable raw mode for terminal
void enable_raw_mode() {
  struct termios raw;
  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= ~(ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to disable raw mode for terminal
void disable_raw_mode() {
  struct termios raw;
  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag |= (ECHO | ICANON);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Function to initialize x_positions array with random X positions
void initialize_x_positions() {
  srand(time(NULL));
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 24; j++) {
      x_positions[i][j] = rand() % 1; // Randomly assign 0-3
    }
  }
}

// Function to place a block or empty space at the current cursor position
void toggle_block_at_cursor(int cursor_x, int cursor_y) {
  // Calculate the corresponding indices in x_positions array
  int grid_x = ((cursor_x - 2) / 3) - 1;
  int grid_y = cursor_y - 2; // Adjusted for labels

  // Check boundaries and toggle x_positions array value
  if (grid_x >= 0 && grid_x < 6 && grid_y >= 0 && grid_y < 24) {
    if (x_positions[grid_x][grid_y] != NONE) {
      x_positions[grid_x][grid_y] = NONE;
    } else {
      x_positions[grid_x][grid_y] = current_color;
    }
  }
}

// SQLite callback function for query results
int callback(int argc, char **argv, char **azColName) {
  for (int i = 0; i < argc; i++) {
    printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
  }
  printf("\n");
  return 0;
}

// Function to save the array to the SQLite database labeled with the entered
// date
void save_array_to_db(const char *date_str, sqlite3 *db) {
  char formatted_date_str[11];
  strcpy(formatted_date_str, date_str);
  for (int i = 0; formatted_date_str[i]; i++) {
    if (formatted_date_str[i] == '/') {
      formatted_date_str[i] = '_';
    }
  }

  char insert_sql[200];
  snprintf(insert_sql, sizeof(insert_sql),
           "INSERT OR REPLACE INTO Days (Date, Data) VALUES ('%s', ?)",
           formatted_date_str);

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, insert_sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return;
  }

  sqlite3_bind_blob(stmt, 1, x_positions, sizeof(x_positions), SQLITE_STATIC);
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE) {
    fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
    return;
  }

  sqlite3_finalize(stmt);
}

// Function to load the array from the SQLite database labeled with the entered
// date
void load_array_from_db(const char *date_str, sqlite3 *db) {
  char formatted_date_str[11];
  strcpy(formatted_date_str, date_str);
  for (int i = 0; formatted_date_str[i]; i++) {
    if (formatted_date_str[i] == '/') {
      formatted_date_str[i] = '_';
    }
  }

  char select_sql[100];
  snprintf(select_sql, sizeof(select_sql),
           "SELECT Data FROM Days WHERE Date = '%s'", formatted_date_str);

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW) {
    const void *data = sqlite3_column_blob(stmt, 0);
    int bytes = sqlite3_column_bytes(stmt, 0);
    memcpy(x_positions, data, bytes);
  } else {
    printf("No data found for the date: %s\n", date_str);
  }

  sqlite3_finalize(stmt);
}

// Function to validate and format the date string
char *getDateFormatted(char *date_str) {
  int month, day, year;
  char *text_date = malloc(13); // MM_DD_YYYY.txt plus null terminator
  if (text_date == NULL) {
    return NULL; // Memory allocation failed
  }

  if (sscanf(date_str, "%d/%d/%d", &month, &day, &year) != 3) {
    free(text_date);
    return NULL; // Incorrect format
  }

  if (month < 1 || month > 12 || day < 1 || day > 31) {
    free(text_date);
    return NULL; // Invalid month or day
  }

  if ((month == 4 || month == 6 || month == 9 || month == 11) && day > 30) {
    free(text_date);
    return NULL; // April, June, September, November have only 30 days
  }

  if (month == 2) {
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
      if (day > 29) {
        free(text_date);
        return NULL; // February in a leap year has at most 29 days
      }
    } else {
      if (day > 28) {
        free(text_date);
        return NULL; // February in a non-leap year has at most 28 days
      }
    }
  }

  sprintf(text_date, "%.2d_%.2d_%.4d", month, day, year);

  return text_date;
}

// Function to print all dates stored in the database
void print_all_dates_from_db(sqlite3 *db) {
  printf("All dates stored in the database:\n");

  char *select_sql = "SELECT Date FROM Days";

  sqlite3_stmt *stmt;
  int rc = sqlite3_prepare_v2(db, select_sql, -1, &stmt, 0);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "Failed to prepare statement: %s\n", sqlite3_errmsg(db));
    return;
  }

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
    const unsigned char *date = sqlite3_column_text(stmt, 0);
    printf("%s\n", date);
  }

  if (rc != SQLITE_DONE) {
    fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
  }

  sqlite3_finalize(stmt);
}

/**
 * Fix for backwards selection by Martin Angell : www.martinangell.com for more
 * info
 */
void gods_chosen_function(int *start_x, int *start_y, int *end_x, int *end_y) {
  if (*start_y > *end_y) {
    *start_y ^= *end_y;
    *end_y ^= *start_y;
    *start_y ^= *end_y;
    *start_x ^= *end_x;
    *end_x ^= *start_x;
    *start_x ^= *end_x;
  }
  if (*start_y == *end_y) {
    if (*start_x > *end_x) {
      *start_x ^= *end_x;
      *end_x ^= *start_x;
      *start_x ^= *end_x;
    }
  }
}

void toggle_or_clear_multiple_boxes(int start_x, int start_y, int end_x,
                                    int end_y, int visual_mode) {
  gods_chosen_function(&start_x, &start_y, &end_x, &end_y);

  if (start_y == end_y) {
    if (start_x <= end_x) {
      for (int x = start_x; x <= end_x; x++) {
        if (visual_mode) {
          toggle_block_at_cursor(x, start_y);
        } else {
          if (x >= 0 && x < 6 && start_y >= 2 && start_y < 26) {
            x_positions[x - 1][start_y - 2] = NONE;
          }
        }
      }
    } else {
      for (int x = start_x; x >= end_x; x--) {
        if (visual_mode) {
          toggle_block_at_cursor(x, start_y);
        } else {
          if (x >= 0 && x < 6 && start_y >= 2 && start_y < 26) {
            x_positions[x][start_y - 2] = NONE;
          }
        }
      }
    }
  } else {
    // Handle other cases here
  }
}

int main() {
  clear_screen();
  enable_raw_mode();

  char *date_str = linenoise("Enter the date (MM/DD/YYYY): ");
  while (getDateFormatted(date_str) == NULL) {
    clear_screen();
    printf("Invalid date... \n");
    date_str = linenoise("Enter the date (MM/DD/YYYY): ");
  }

  sqlite3 *db;
  int rc = sqlite3_open(".calendar.db", &db);
  if (rc) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return 1;
  }

  char *errmsg;
  rc = sqlite3_exec(
      db, "CREATE TABLE IF NOT EXISTS Days (Date TEXT PRIMARY KEY, Data BLOB)",
      NULL, 0, &errmsg);
  if (rc != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", errmsg);
    sqlite3_free(errmsg);
  }

  // Format the date string
  char *formatted_date_str = getDateFormatted(date_str);
  free(date_str); // Freeing the original date string
  if (formatted_date_str == NULL) {
    disable_raw_mode();
    sqlite3_close(db);
    return 1;
  }

  load_array_from_db(formatted_date_str, db);

  int cursor_x = 5; // Column 2 (second column)
  int cursor_y = 2; // Row 4 (skip column titles and additional rows)

  draw_grid(cursor_x, cursor_y, formatted_date_str);

  char c;
  while ((c = getchar()) != EOF && c != 'q') {
    if (!visual_mode) {
      switch (c) {
      case 'k':
        if (cursor_y >
            2) { // Skip the row for the column titles and additional rows
          cursor_y--;
        }
        break;
      case 'j':
        if (cursor_y < 25) { // Limit the maximum row to the bottom of the grid
          cursor_y++;
        }
        break;
      case 'h':
        if ((cursor_x - 2) / 3 == 1 && cursor_y >= 3) {
          cursor_x = 23; // Move to the first bracket box
          cursor_y--;    // Move to the next line
        }
        if (cursor_x > 6) {
          cursor_x -= 3;
        }
        break;
      case 'l':
        // Check if 'l' is pressed in the last bracket box of a row
        if ((cursor_x) / 4 == 5 && cursor_y >= 2 && cursor_y < 25) {
          cursor_x = 2; // Move to the first bracket box
          cursor_y++;   // Move to the next line
        }
        if (cursor_x < 20) {
          cursor_x += 3;
        }
        break;
      case '\n':
        toggle_block_at_cursor(cursor_x, cursor_y);
        break;
      case 'x':
        if (x_positions[((cursor_x - 2) / 3) - 1][(cursor_y - 2)] != NONE)
          toggle_block_at_cursor(cursor_x, cursor_y);
        break;
      case 'v':
        visual_mode = true;
        start_x = cursor_x;
        start_y = cursor_y;
        break;
      case '[':
        cursor_x = 5;
        cursor_y = 2;
        break;
      case ']':
        cursor_x = 20;
        cursor_y = 25;
        break;
      case '1':
        current_color = RED;
        break;
      case '2':
        current_color = GREEN;
        break;
      case '3':
        current_color = BLUE;
        break;
      case '4':
        current_color = YELLOW;
        break;
      case '5':
        current_color = CYAN;
        break;
      case '6':
        current_color = MAGENTA;
        break;
      }
    } else {
      // In visual mode
      switch (c) {
      case 'k':
        if (cursor_y >
            2) { // Skip the row for the column titles and additional rows
          cursor_y--;
        }
        break;
      case 'j':
        if (cursor_y < 25) { // Limit the maximum row to the bottom of the grid
          cursor_y++;
        }
        break;
      case 'h':
        if ((cursor_x - 2) / 3 == 1 && cursor_y >= 3) {
          cursor_x = 23; // Move to the first bracket box
          cursor_y--;    // Move to the next line
        }
        if (cursor_x > 6) {
          cursor_x -= 3;
        }
        break;
      case 'l':
        // Check if 'l' is pressed in the last bracket box of a row
        if ((cursor_x) / 4 == 5 && cursor_y >= 2 && cursor_y < 25) {
          cursor_x = 2; // Move to the first bracket box
          cursor_y++;   // Move to the next line
        }
        if (cursor_x < 20) {
          cursor_x += 3;
        }
        break;
      case '[':
        cursor_x = 5;
        cursor_y = 2;
        break;
      case ']':
        cursor_x = 20;
        cursor_y = 25;
        break;
      case 27:
        visual_mode = false;
        break;
      case '\n':
        toggle_or_clear_multiple_boxes(start_x, start_y, cursor_x, cursor_y, 1);
        visual_mode = false; // Exiting visual mode after selection
        break;
      case 'x':
        toggle_or_clear_multiple_boxes(start_x, start_y, cursor_x, cursor_y, 0);
        visual_mode = false; // Exiting visual mode after selection
        break;
      case '1':
        current_color = RED;
        break;
      case '2':
        current_color = GREEN;
        break;
      case '3':
        current_color = BLUE;
        break;
      case '4':
        current_color = YELLOW;
        break;
      case '5':
        current_color = CYAN;
        break;
      case '6':
        current_color = MAGENTA;
        break;
      }
    }
    draw_grid(cursor_x, cursor_y, formatted_date_str);
  }

  disable_raw_mode();

  // Clear the screen
  clear_screen();

  for (int i = 0; i < 24; i++) {
    for (int j = 0; j < 6; j++) {
      printf("[%d]", x_positions[j][i]);
    }
    printf("\n");
  }

  // Save the array to the database labeled with the entered date
  save_array_to_db(formatted_date_str, db);
  sqlite3_close(db);

  return 0;
}
