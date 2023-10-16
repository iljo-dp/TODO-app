#include <ncurses.h>
#include <json-c/json.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

// Define the maximum number of days in a month
#define MAX_DAYS_IN_MONTH 31

// Structure for appointments
struct Appointment {
    char date[12];
    char description[50];
    char start_hour[6];
    char end_hour[6];
};

// Structure for a dynamic array of appointments
struct DynamicArray {
    struct Appointment* data;
    size_t size;
    size_t capacity;
};

struct DynamicArray appointments; // Declare the appointments array

// Array to track days with appointments
int has_appointment[MAX_DAYS_IN_MONTH];

// Function to initialize the dynamic array
void initialize_dynamic_array() {
    appointments.data = NULL;
    appointments.size = 0;
    appointments.capacity = 0;
    for (int i = 0; i < MAX_DAYS_IN_MONTH; i++) {
        has_appointment[i] = 0; // Initialize to no appointments
    }
}
const char* month_name(int month);
int days_in_month(int year, int month);

const char* month_name(int month) {
    const char* months[] = {
        "January", "February", "March", "April", "May", "June",
        "July", "August", "September", "October", "November", "December"
    };
    return months[month];
}

int days_in_month(int year, int month) {
    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month == 1 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))) {
        return 29;
    }

    return days_in_month[month];
}

void resize_dynamic_array() {
    size_t new_capacity = appointments.capacity == 0 ? 1 : appointments.capacity * 2;
    struct Appointment* new_data = (struct Appointment*)malloc(new_capacity * sizeof(struct Appointment));
    if (new_data) {
        if (appointments.data) {
            memcpy(new_data, appointments.data, appointments.size * sizeof(struct Appointment));
            free(appointments.data);
        }
        appointments.data = new_data;
        appointments.capacity = new_capacity;
    }
}

void add_appointment(const struct Appointment* appointment) {
    if (appointments.size == appointments.capacity) {
        resize_dynamic_array();
    }
    appointments.data[appointments.size++] = *appointment;
}

void remove_appointment(int index) {
    if (index >= 0 && index < appointments.size) {
        for (int i = index; i < appointments.size - 1; i++) {
            appointments.data[i] = appointments.data[i + 1];
        }
        appointments.size--;
    }
}

void save_appointments() {
    json_object* root = json_object_new_array();

    for (int i = 0; i < appointments.size; i++) {
        json_object* appointment_obj = json_object_new_object();
        json_object_object_add(appointment_obj, "date", json_object_new_string(appointments.data[i].date));
        json_object_object_add(appointment_obj, "description", json_object_new_string(appointments.data[i].description));
        json_object_object_add(appointment_obj, "start_hour", json_object_new_string(appointments.data[i].start_hour));
        json_object_object_add(appointment_obj, "end_hour", json_object_new_string(appointments.data[i].end_hour));
        json_object_array_add(root, appointment_obj);
    }

    FILE* file = fopen("appointments.json", "w");
    if (file) {
        fprintf(file, "%s", json_object_to_json_string_ext(root, JSON_C_TO_STRING_SPACED));
        fclose(file);
    }

    json_object_put(root);
}

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
            if (count > appointments.capacity) {
                // Resize the dynamic array if needed
                appointments.capacity = count;
                appointments.data = (struct Appointment*)realloc(appointments.data, appointments.capacity * sizeof(struct Appointment));
            }
            appointments.size = count; // Set the size to match the loaded data

            for (int i = 0; i < count; i++) {
                json_object* appointment_obj = json_object_array_get_idx(root, i);

                json_object* date_obj = json_object_object_get(appointment_obj, "date");
                json_object* description_obj = json_object_object_get(appointment_obj, "description");
                json_object* start_hour_obj = json_object_object_get(appointment_obj, "start_hour");
                json_object* end_hour_obj = json_object_object_get(appointment_obj, "end_hour");

                if (date_obj != NULL && json_object_get_type(date_obj) == json_type_string &&
                    description_obj != NULL && json_object_get_type(description_obj) == json_type_string) {
                    const char* date = json_object_get_string(date_obj);
                    const char* description = json_object_get_string(description_obj);
                    const char* start_hour = json_object_get_string(start_hour_obj);
                    const char* end_hour = json_object_get_string(end_hour_obj);

                    strncpy(appointments.data[i].date, date, sizeof(appointments.data[i].date));
                    strncpy(appointments.data[i].description, description, sizeof(appointments.data[i].description));
                    strncpy(appointments.data[i].start_hour, start_hour, sizeof(appointments.data[i].start_hour));
                    strncpy(appointments.data[i].end_hour, end_hour, sizeof(appointments.data[i].end_hour));

                    // Extract the day of the year and mark it as having an appointment
                    int day = atoi(strtok(strdup(date), "/"));
                    has_appointment[day - 1] = 1;
                }
            }
        }

        json_object_put(root);
    }
}

void draw_calendar(int year, int month, int day, int selected_day, int* has_appointment) {
    attron(COLOR_PAIR(2));
    mvprintw(1, 1, "Calendar for %s %d", month_name(month), year);
    mvprintw(3, 1, "Sun Mon Tue Wed Thu Fri Sat");

    int num_days = days_in_month(year, month);

    int current_row = 4;
    int current_col = 1;

    for (int i = 1; i <= num_days; i++) {
        if (i == day) {
            attron(COLOR_PAIR(1));
        }
        if (i == selected_day) {
            attron(A_REVERSE);
        }

        // Check if this day has an appointment and set text color accordingly
        if (has_appointment[i - 1]) {
            attron(COLOR_PAIR(3));
        }

        mvprintw(current_row, current_col, "%2d", i);

        if (i == day) {
            attroff(COLOR_PAIR(1));
        }
        if (i == selected_day) {
            attroff(A_REVERSE);
        }

        // Turn off the red color
        if (has_appointment[i - 1]) {
            attroff(COLOR_PAIR(3));
        }

        current_col += 4;

        if (current_col > 27) {
            current_col = 1;
            current_row++;
        }
    }
}

void draw_appointments(int row, int col, int max_height, int* has_appointment) {
    mvprintw(row - 2, col, "Appointments:");
    int max_appointments = appointments.size > 10 ? 10 : appointments.size;

    for (int i = 0; i < max_appointments; i++) {
        attron(COLOR_PAIR(4));
        mvprintw(row + i, col, "%s (%s --> %s): %s", appointments.data[i].date, appointments.data[i].start_hour, appointments.data[i].end_hour, appointments.data[i].description);
        attroff(COLOR_PAIR(4));
    }
}

void show_help() {
    mvprintw(LINES - 3, 0, "Keybindings:");
    mvprintw(LINES - 2, 0, "Arrows: Navigate | Enter: Select a day | A: Add appointment | Q: Quit");
    refresh();
}

int main() {
    initscr();
    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);
    init_pair(2, COLOR_WHITE, COLOR_BLACK);
    init_pair(3, COLOR_RED, COLOR_BLACK); // Red for days with appointments
    init_pair(4, COLOR_CYAN, COLOR_BLACK);

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

    initialize_dynamic_array(); // Initialize the dynamic array

    load_appointments(); // Load appointments from the file

    while (1) {
        clear();
        draw_calendar(year, month, day, selected_day, has_appointment);
        draw_appointments(12, 1, LINES - 15, has_appointment);
        show_help();
        refresh();

        int ch = getch();

        if (ch == 'q' || ch == 'Q') {
            save_appointments(); // Save the appointments to the file
            break;
        } else if (ch == KEY_RIGHT) {
            if (selected_day < 31) {
                selected_day++;
            }
        } else if (ch == KEY_LEFT) {
            if (selected_day > 1) {
                selected_day--;
            }
        } else if (ch == KEY_DOWN) {
            if (selected_day <= 24) {
                selected_day += 7;
            }
        } else if (ch == KEY_UP) {
            if (selected_day > 7) {
                selected_day -= 7;
            }
        } else if (ch == 'A' || ch == 'a') {
            // Create a temporary window for input
            WINDOW* input_win = newwin(8, COLS - 2, LINES / 2 - 4, 1);
            box(input_win, 0, 0);
            mvwprintw(input_win, 1, 2, "Enter appointment details:");
            mvwprintw(input_win, 2, 2, "Date (MM/DD/YYYY): ");
            wrefresh(input_win);

            // Enable input echoing
            echo();

            // Gather input for the new appointment
            char input_date[12];
            char input_description[50];
            char input_start_hour[6];
            char input_end_hour[6];

            mvwgetstr(input_win, 2, 21, input_date);

            mvwprintw(input_win, 3, 2, "Description: ");
            wrefresh(input_win);
            mvwgetstr(input_win, 3, 15, input_description);

            mvwprintw(input_win, 4, 2, "Start hour (HH:MM): ");
            wrefresh(input_win);
            mvwgetstr(input_win, 4, 23, input_start_hour);

            mvwprintw(input_win, 5, 2, "End hour (HH:MM): ");
            wrefresh(input_win);
            mvwgetstr(input_win, 5, 20, input_end_hour);

            // Disable input echoing
            noecho();

            // Close the input window
            delwin(input_win);

            // Extract the day of the year
            int input_day = atoi(strtok(input_date, "/"));

            // Create a new appointment with the gathered information
            struct Appointment new_appointment;
            strncpy(new_appointment.date, input_date, sizeof(new_appointment.date) - 1);
            strncpy(new_appointment.description, input_description, sizeof(new_appointment.description) - 1);
            strncpy(new_appointment.start_hour, input_start_hour, sizeof(new_appointment.start_hour) - 1);
            strncpy(new_appointment.end_hour, input_end_hour, sizeof(new_appointment.end_hour) - 1);

            // Mark the day with an appointment
            has_appointment[input_day - 1] = 1;

            // Add the new appointment
            add_appointment(&new_appointment);
            save_appointments(); // Save the updated appointments
        }
    }

    save_appointments(); // Save the appointments to the file
    endwin();
    return 0;
}

