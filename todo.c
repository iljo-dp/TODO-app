//gcc -o todo todo.c -ljansson
//./todo
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

#define MAX_TASKS 100
#define MAX_TASK_LENGTH 100
#define MAX_DATE_LENGTH 20

typedef struct {
    char beschrijving[MAX_TASK_LENGTH];
    int prioriteit;
    int voltooid;
    char datum[MAX_DATE_LENGTH];
} Taak;

Taak taaklijst[MAX_TASKS];
int taaktelling = 0;

const char* JSON_BESTANDSNAAM = "taken.json";
void takenOpslaan();

void taakToevoegen(const char* beschrijving, int prioriteit, const char* datum) {
    if (taaktelling < MAX_TASKS) {
        strncpy(taaklijst[taaktelling].beschrijving, beschrijving, MAX_TASK_LENGTH);
        taaklijst[taaktelling].prioriteit = prioriteit;
        taaklijst[taaktelling].voltooid = 0;
        strncpy(taaklijst[taaktelling].datum, datum, MAX_DATE_LENGTH);
        taaktelling++;
        printf("Taak toegevoegd: %s\n", beschrijving);
        takenOpslaan();
    } else {
        printf("De takenlijst is vol. Kan geen nieuwe taken toevoegen.\n");
    }
}

void takenTonen() {
    printf("Takenlijst:\n");
    for (int i = 0; i < taaktelling; i++) {
        printf("%d. Prioriteit: %d, Beschrijving: %s, Datum: %s [%s]\n", i + 1, taaklijst[i].prioriteit, taaklijst[i].beschrijving, taaklijst[i].datum, taaklijst[i].voltooid ? "Klaar" : "Niet Klaar");
    }
}

void taakVoltooid(int taaknummer) {
    if (taaknummer >= 1 && taaknummer <= taaktelling) {
        taaklijst[taaknummer - 1].voltooid = 1;
        printf("Taak gemarkeerd als voltooid: %s\n", taaklijst[taaknummer - 1].beschrijving);
        takenOpslaan();
    } else {
        printf("Ongeldig taaknummer. Voer een geldig taaknummer in om te markeren als voltooid.\n");
    }
}

void taakVerwijderen(int taaknummer) {
    if (taaknummer >= 1 && taaknummer <= taaktelling) {
        printf("Taak verwijderd: %s\n", taaklijst[taaknummer - 1].beschrijving);
        for (int i = taaknummer - 1; i < taaktelling - 1; i++) {
            taaklijst[i] = taaklijst[i + 1];
        }
        taaktelling--;
        takenOpslaan();
    } else {
        printf("Ongeldig taaknummer. Voer een geldig taaknummer in om te verwijderen.\n");
    }
}

void createJsonFileIfNotExists() {
    // Controleren of het bestand bestaat
    FILE* bestand = fopen(JSON_BESTANDSNAAM, "r");
    if (bestand == NULL) {
        // Het bestand bestaat niet, dus maak een lege JSON-array aan en sla deze op
        json_t* root = json_array();
        json_dump_file(root, JSON_BESTANDSNAAM, JSON_INDENT(4));
        json_decref(root);
    } else {
        fclose(bestand);
    }
}

void takenLadenVanJson() {
    // JSON-bestand maken als het niet bestaat
    createJsonFileIfNotExists();
    
    json_t* root;
    json_error_t fout;

    root = json_load_file(JSON_BESTANDSNAAM, 0, &fout);
    if (!root) {
        printf("Fout bij het laden van taken uit JSON: %s\n", fout.text);
        return;
    }

    taaktelling = json_array_size(root);
    for (int i = 0; i < taaktelling; i++) {
        json_t* taakObject = json_array_get(root, i);
        strncpy(taaklijst[i].beschrijving, json_string_value(json_object_get(taakObject, "beschrijving")), MAX_TASK_LENGTH);
        taaklijst[i].prioriteit = json_integer_value(json_object_get(taakObject, "prioriteit"));
        taaklijst[i].voltooid = json_boolean_value(json_object_get(taakObject, "voltooid"));
        strncpy(taaklijst[i].datum, json_string_value(json_object_get(taakObject, "datum")), MAX_DATE_LENGTH);
    }

    json_decref(root);
}

void takenOpslaan() {
    json_t* root = json_array();
    for (int i = 0; i < taaktelling; i++) {
        json_t* taakObject = json_object();
        json_object_set(taakObject, "beschrijving", json_string(taaklijst[i].beschrijving));
        json_object_set(taakObject, "prioriteit", json_integer(taaklijst[i].prioriteit));
        json_object_set(taakObject, "voltooid", json_boolean(taaklijst[i].voltooid));
        json_object_set(taakObject, "datum", json_string(taaklijst[i].datum));
        json_array_append(root, taakObject);
    }

    json_dump_file(root, JSON_BESTANDSNAAM, JSON_INDENT(4));
    json_decref(root);
}

int main() {
    // JSON-bestand maken als het niet bestaat
    createJsonFileIfNotExists();
    
    // Taken laden uit JSON
    takenLadenVanJson();

    int keuze;
    char beschrijving[MAX_TASK_LENGTH];
    int prioriteit;
    char datum[MAX_DATE_LENGTH];

    while (1) {
        printf("\nTakenlijst Beheerder\n");
        printf("1. Taak Toevoegen\n");
        printf("2. Taken Tonen\n");
        printf("3. Taak Voltooien\n");
        printf("4. Taak Verwijderen\n");
        printf("5. Afsluiten\n");
        printf("Voer uw keuze in: ");
        scanf("%d", &keuze);
        getchar(); // Verwijder de newline-karakter

        switch (keuze) {
            case 1:
                printf("Voer taak beschrijving in: ");
                fgets(beschrijving, MAX_TASK_LENGTH, stdin);
                printf("Voer taak prioriteit in (1 voor hoog, 2 voor medium, 3 voor laag): ");
                scanf("%d", &prioriteit);
                printf("Voer de taakdatum in (DD-MM-YYYY): ");
                scanf("%s", datum);
                taakToevoegen(beschrijving, prioriteit, datum);
                break;
            case 2:
                takenTonen();
                break;
            case 3:
                int taakNummer;
                takenTonen();
                printf("Voer het taaknummer in om te markeren als voltooid: ");
                scanf("%d", &taakNummer);
                taakVoltooid(taakNummer);
                break;
            case 4:
                takenTonen();
                printf("Voer het taaknummer in om te verwijderen: ");
                scanf("%d", &taakNummer);
                taakVerwijderen(taakNummer);
                break;
            case 5:
                printf("Tot ziens!\n");
                exit(0);
            default:
                printf("Ongeldige keuze. Kies een geldige optie.\n");
        }
    }

    return 0;
}
