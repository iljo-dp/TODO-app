#include <ncurses.h>
#include <json-c/json.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>

#define MAX_NOTES_LINES 20
struct Appointment {
    char date[12];
    char description[50];
    char start_hour[6];
    char end_hour[6];
};

struct Appointment appointments[10];
int appointment_count = 0;

char notes[20][60];  // An array to store notes (20 lines of 60 characters each)
int notes_line = 0;  // Keep track of the current line for adding notes
int notes_row = 12;
int notes_col;
int current_char = 0;
// Variable to track the currently focused area (0 for calendar, 1 for notes)
int focused_area = 0;
_Bool adding_note = 0; // Flag to indicate if the user is adding a note

void draw_calendar(int year, int month, int day);
void draw_appointments(int row, int col, int max_height);
void draw_notes(int row, int col, int max_height, char notes[20][60]);
void add_appointment();
void edit_appointment(int index);
void remove_appointment(int index);
void save_appointments();
void save_notes();
void load_appointments();
void load_notes();
void prompt_for_note();

// Define color pairs
enum Colors {
    COLOR_SELECTED_DAY = 1,
    COLOR_NORMAL_DAY,
    COLOR_TODAY,
    COLOR_APPOINTMENT,
    COLOR_NOTES,  // Added a new color pair for notes
};

int main() {
    initscr();
    start_color();
    init_pair(COLOR_SELECTED_DAY, COLOR_BLACK, COLOR_WHITE);
    init_pair(COLOR_NORMAL_DAY, COLOR_WHITE, COLOR_BLACK);
    init_pair(COLOR_TODAY, COLOR_RED, COLOR_BLACK);
    init_pair(COLOR_APPOINTMENT, COLOR_CYAN, COLOR_BLACK);
    init_pair(COLOR_NOTES, COLOR_WHITE, COLOR_BLACK);

    keypad(stdscr, TRUE);
    curs_set(1);
    noecho();

    int year, month, day;
    time_t current_time;
    struct tm* time_info;

    time(&current_time);
    time_info = localtime(&current_time);
    year = time_info->tm_year + 1900;
    month = time_info->tm_mon;
    day = time_info->tm_mday;

    int selected_day = day;

    notes_col = COLS - 62; // Initialize notes_col here

    load_appointments();
    load_notes();

    int current_line = 0;
    _Bool editing = 0;
    char* mode_label = "Calendar Mode (Tab to Edit Notes)";

    while (1) {
        clear();

        // Draw both calendar and notes
        draw_calendar(year, month, selected_day);
        draw_appointments(12, 1, LINES - 15);
	draw_notes(12, COLS - 60, LINES - 15, notes);
        // Display the current mode label
        mvprintw(LINES - 2, 1, "Navigate: Arrows | Select a day: Enter | Quit: Q");
        mvprintw(LINES - 1, 1, "Add appointment: A | Save: S");
        mvprintw(LINES - 1, COLS - 20, mode_label);

        refresh();

        int ch = getch();

        if (ch == 'q' || ch == 'Q') {
            save_appointments();
            save_notes();
            break;
        } else if (ch == KEY_RIGHT) {
            if (!editing) {
                if (selected_day < 31) {
                    selected_day++;
                }
            }
        } else if (ch == KEY_LEFT) {
            if (!editing) {
                if (selected_day > 1) {
                    selected_day--;
                }
            }
        } else if (ch == KEY_DOWN) {
            if (!editing) {
                if (selected_day <= 24) {
                    selected_day += 7;
                }
            }
        } else if (ch == KEY_UP) {
            if (!editing) {
                if (selected_day > 7) {
                    selected_day -= 7;
                }
            }
        } else if (ch == 10) { // Enter key
            if (!editing) {
                // Handle calendar selection
                // ...
            } else {
                // Handle 'Enter' while editing notes to create a new line
                if (current_line < MAX_NOTES_LINES - 1) {
                    current_line++;
                }
            }
        }  if (ch == 9) { // Tab key
            // Toggle editing mode for notes
            editing = !editing;
            adding_note = 0; // Disable note addition
            if (editing) {
                mode_label = "Editing Notes (Tab to Exit)";
            } else {
                mode_label = "Calendar Mode (Tab to Edit Notes)";
            }
	} else if (editing && (ch == 'I' || ch == 'i')) {
            // Allow adding notes only in edit mode
            adding_note = 1;
            prompt_for_note();
        } else if (!editing && (ch == 'A' || ch == 'a')) {
            add_appointment();
            save_appointments();
        } else if (!editing && (ch == 'S' || ch == 's')) {
            save_notes();
        } else if (editing && isprint(ch) && current_line < MAX_NOTES_LINES) {
            notes[current_line][current_char] = ch;
            if (current_char < 60) {
                current_char++;
            }
        }
    
}

    endwin();
    return 0;
}
// Function to get the name of the month
//
const char* month_name(int month) {
    const char* months[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    return months[month];
}

// Function to get the number of days in a given month
int days_in_month(int year, int month) {
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    // Check for a leap year (February has 29 days)
    if (month == 1 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) {
        return 29;
    }

    return days_in_month[month];
}

// Function to get the day of the week for a given date
int day_of_week(int year, int month, int day) {
    if (month < 3) {
        month += 12;
        year--;
    }
    int h = (day + 2 * month + 3 * (month + 1) / 5 + year + year / 4 - year / 100 + year / 400) % 7;
    return h;
}

void draw_calendar(int year, int month, int day) {
    // Clear the screen and add new code for calendar visualization
    // Define colors for selected day, normal day, today, and appointments
    attron(COLOR_PAIR(COLOR_NORMAL_DAY));

    mvprintw(1, 1, "Calendar for %s %d", month_name(month), year);
    mvprintw(3, 1, "Sun Mon Tue Wed Thu Fri Sat");

    int num_days = days_in_month(year, month);
    int start_day = day_of_week(year, month, 1);

    int current_row = 4;
    int current_col = 1; // Initialize the column position

    for (int i = 1; i <= num_days; i++) {
        if (i == day) {
            attron(COLOR_PAIR(COLOR_SELECTED_DAY));
        }

        // Print the day
        mvprintw(current_row, current_col, "%2d", i);

        if (i == day) {
            attroff(COLOR_PAIR(COLOR_SELECTED_DAY));
        }

        current_col += 4; // Move to the next column

        if (current_col > 27) {
            current_col = 1; // Reset the column position
            current_row++;
        }
    }
}

void draw_appointments(int row, int col, int max_height) {
    mvprintw(row - 2, col, "Appointments:");
    int max_appointments = appointment_count > 10 ? 10 : appointment_count;

    for (int i = 0; i < max_appointments; i++) {
        attron(COLOR_PAIR(COLOR_APPOINTMENT));

        mvprintw(row + i, col, "%s %s -> %s: %s", appointments[i].date, appointments[i].start_hour, appointments[i].end_hour, appointments[i].description);

        attroff(COLOR_PAIR(COLOR_APPOINTMENT));
    }
}
void draw_notes(int row, int col, int max_height, char notes[20][60]) {
    // Draw the "NOTES" section on the right side
    attron(COLOR_PAIR(COLOR_NOTES));
    mvprintw(row, col - 2, "NOTES");
    mvprintw(row, col - 2, "------------------------------------------------------------");

    // Draw the notes text area
    for (int i = 0; i < MAX_NOTES_LINES; i++) {
        mvprintw(row + i + 1, col - 2, "| %-57s |", notes[i]);
    }

    mvprintw(row + max_height - row, col, "-----------------------------------------------------------");
    attroff(COLOR_PAIR(COLOR_NOTES));
}
void remove_appointment(int index) {
    if (index >= 0 && index < appointment_count) {
        for (int i = index; i < appointment_count - 1; i++) {
            appointments[i] = appointments[i + 1];
        }
        appointment_count--;
    }
}
void prompt_for_note() {
    clear();
    mvprintw(LINES - 2, 1, "Press 'I' to add a note. Enter your note below:");

    attron(COLOR_PAIR(COLOR_NOTES));
    mvprintw(12, COLS - 60, "------------------------------------------------------------");
    mvprintw(13, COLS - 60, "| %-57s |", "");
    mvprintw(14, COLS - 60, "------------------------------------------------------------");
    attroff(COLOR_PAIR(COLOR_NOTES));
    mvprintw(13, COLS - 58, "> ");

    int x = COLS - 57;
    int y = 13;
    int current_note_char = 0;
    int ch;

    while (adding_note) {
        ch = getch();
        if (ch == 'i' || ch == 'I') {
            adding_note = 0; // Exit note input mode if the 'I' key is pressed
        } else if (ch == KEY_ENTER || ch == 10) {
            adding_note = 0; // Exit note input mode on Enter key
        } else if (isprint(ch) && current_note_char < 57) {
            mvaddch(y, x + current_note_char, ch);
            notes[notes_line][current_note_char] = ch;
            current_note_char++;
        } else if (ch == KEY_BACKSPACE || ch == 8 || ch == 127) {
            if (current_note_char > 0) {
                mvaddch(y, x + current_note_char - 1, ' ');
                notes[notes_line][current_note_char - 1] = '\0';
                current_note_char--;
            }
        }

        refresh();
    }
}
// Function to save appointments to a JSON file
void save_appointments() {
    json_object* root = json_object_new_array();

    for (int i = 0; i < appointment_count; i++) {
        json_object* appointment_obj = json_object_new_object();
        json_object_object_add(appointment_obj, "date", json_object_new_string(appointments[i].date));
        json_object_object_add(appointment_obj, "description", json_object_new_string(appointments[i].description));
        json_object_array_add(root, appointment_obj);
    }

    FILE* file = fopen("appointments.json", "w");
    if (file) {
        fprintf(file, "%s", json_object_to_json_string_ext(root, JSON_C_TO_STRING_SPACED));
        fclose(file);
    }

    json_object_put(root);
}

void save_notes() {
    FILE* notes_file = fopen("notes.txt", "w");
    if (notes_file) {
        for (int i = 0; i < 20; i++) {
            fprintf(notes_file, "%s\n", notes[i]);
        }
        fclose(notes_file);
    }
}

void load_notes() {
    FILE* notes_file = fopen("notes.txt", "r");
    if (notes_file) {
        for (int i = 0; i < 20; i++) {
            if (fgets(notes[i], sizeof(notes[i]), notes_file) == NULL) {
                break;
            }
            // Remove the newline character
            notes[i][strlen(notes[i]) - 1] = '\0';
        }
        fclose(notes_file);
    }
}

// Function to load appointments from a JSON file
void load_appointments() {
    FILE* file = fopen("appointments.json", "r");
    if (file) {
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        char* file_contents = (char*)malloc(file_size + 1);
        fread(file_contents, 1, file_size, file);
        file_contents[file_size] = '\0';
        fclose(file);

        json_object* root = json_tokener_parse(file_contents);
        free(file_contents);

        if (json_object_get_type(root) == json_type_array) {
            int count = json_object_array_length(root);
            appointment_count = 0;

            for (int i = 0; i < count && i < 10; i++) {
                json_object* appointment_obj = json_object_array_get_idx(root, i);
                const char* date = json_object_get_string(json_object_object_get(appointment_obj, "date"));
                const char* description = json_object_get_string(json_object_object_get(appointment_obj, "description"));

                strncpy(appointments[i].date, date, sizeof(appointments[i].date));
                strncpy(appointments[i].description, description, sizeof(appointments[i].description));
                appointment_count++;
            }
        }

        json_object_put(root);
    }
}void add_appointment() {
    if (appointment_count < 10) {
        struct Appointment appointment;

        // Clear the bottom row
        mvprintw(LINES - 1, 0, "                                                          ");

        mvprintw(LINES - 1, 1, "Enter date (MM-DD-YYYY): ");
        refresh();
        echo();
        getnstr(appointment.date, sizeof(appointment.date));

        // Clear the bottom row again
        mvprintw(LINES - 1, 0, "                                                          ");

        mvprintw(LINES - 1, 1, "Enter start hour (HH:MM): ");
        refresh();
        echo();
        getnstr(appointment.start_hour, sizeof(appointment.start_hour));
        
        // Clear the bottom row again
        mvprintw(LINES - 1, 0, "                                                          ");

        mvprintw(LINES - 1, 1, "Enter end hour (HH:MM): ");
        refresh();
        echo();
        getnstr(appointment.end_hour, sizeof(appointment.end_hour));

        // Clear the bottom row again
        mvprintw(LINES - 1, 0, "                                                          ");

        mvprintw(LINES - 1, 1, "Enter description: ");
        refresh();
        getnstr(appointment.description, sizeof(appointment.description));

        mvprintw(LINES - 1, 1, "Appointment added.");
        refresh();
        napms(1000); // Display the "Appointment added." message for 1 second

        // Clear the bottom row again
        mvprintw(LINES - 1, 0, "                                                          ");

        noecho();
        appointments[appointment_count++] = appointment;
        // Save appointments after adding
        save_appointments();
    }
}
