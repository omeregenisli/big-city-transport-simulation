#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>


#ifdef _WIN32
#define CLEAR "cls"
#else
#define CLEAR "clear"
#endif

#define MAX_BUSES 12
#define MAX_QUEUE 15
#define SELECTED_BUS_ID "D2"
#define SELECTED_STOP_ID 'A'
#define MAX_PASSENGERS 50
#define STOP_COUNT 16
#define MAP_ROWS 21
#define MAP_COLS 57
char selectedStopID = 'A';  // Başlangıçta seçili durak
char selectedBusID[4] = "D2";  // Varsayılan seçili otobüs
typedef struct {
    int id;
    char startStop;
    char endStop;
    int luggageCount;
    int luggageIDs[2];
} Passenger;

typedef struct {
    Passenger queue[MAX_QUEUE];
    int count;
} PassengerQueue;

typedef struct {
    char name;
    Passenger queue[MAX_PASSENGERS];
    int front;
    int rear;
} BusStop;

typedef struct {
    char lineName;
    int busID;
    char currentStop;
    int passengerCount;
    Passenger passengers[8];
    int row, col;         // Haritadaki pozisyon
    int targetIndex;      // Hedef durağın indexi

    int luggageCount;
    int direction;
} Bus;

typedef struct {
    char grid[MAP_ROWS][MAP_COLS + 1];
} CityMap;

typedef struct {
    char lineName;
    int stopCount;
    char stops[10];
} BusLine;

char busStops[] = { 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P' };
int globalPassengerID = 1;
int globalLuggageID = 1;
int currentTime = 0;
int running = 0;

BusLine busLines[6] = {
    {'A', 6, {'A', 'E', 'I', 'J', 'K', 'L'}},
    {'B', 7, {'B', 'F', 'E', 'I', 'J', 'N', 'M'}},
    {'C', 7, {'C', 'G', 'K', 'J', 'F', 'E', 'A'}},
    {'D', 7, {'D', 'C', 'G', 'K', 'J', 'N', 'M'}},
    {'L', 8, {'L', 'P', 'O', 'K', 'G', 'C', 'D', 'H'}},
    {'M', 8, {'M', 'N', 'J', 'F', 'G', 'K', 'O', 'P'}}
};

void initializeCity(CityMap* map) {
    for (int i = 0; i < MAP_ROWS; i++) {
        for (int j = 0; j < MAP_COLS; j++) {
            map->grid[i][j] = '#';
        }
        map->grid[i][MAP_COLS] = '\0';
    }
    for (int i = 2; i <= 18; i += 2)
        for (int j = 1; j < MAP_COLS - 1; j++) map->grid[i][j] = ' ';
    for (int j = 5; j <= 51; j += 10)
        for (int i = 1; i < MAP_ROWS - 1; i++) map->grid[i][j] = ' ';

    map->grid[2][1] = 'A'; map->grid[2][11] = 'B'; map->grid[2][21] = 'C'; map->grid[2][41] = 'D';
    map->grid[8][1] = 'E'; map->grid[8][11] = 'F'; map->grid[8][21] = 'G'; map->grid[8][41] = 'H';
    map->grid[14][1] = 'I'; map->grid[14][11] = 'J'; map->grid[14][21] = 'K'; map->grid[14][41] = 'L';
    map->grid[18][1] = 'M'; map->grid[18][11] = 'N'; map->grid[18][31] = 'O'; map->grid[18][51] = 'P';
}

char getRandomStop() {
    return busStops[rand() % 16];
}

void generatePassenger(BusStop stops[], PassengerQueue* pq) {
    Passenger p;
    p.id = globalPassengerID++;
    p.startStop = getRandomStop();
    do { p.endStop = getRandomStop(); } while (p.endStop == p.startStop);
    p.luggageCount = rand() % 3;
    for (int i = 0; i < p.luggageCount; i++)
        p.luggageIDs[i] = globalLuggageID++;

    int index = p.startStop - 'A';
    if (stops[index].rear < MAX_PASSENGERS - 1) {
        stops[index].rear++;
        stops[index].queue[stops[index].rear] = p;
        if (pq->count < MAX_QUEUE)
            pq->queue[pq->count++] = p;
    }
}

void initializeBus(Bus* bus, BusLine* line, int busNumber) {
    bus->lineName = line->lineName;
    bus->busID = busNumber;
    bus->passengerCount = 0;
    bus->luggageCount = 0;
    bus->direction = (busNumber == 1) ? 1 : -1;
    bus->currentStop = (busNumber == 1) ? line->stops[0] : line->stops[line->stopCount - 1];
}

void unloadPassengers(Bus* bus) {
    for (int i = 0; i < bus->passengerCount; )
        if (bus->passengers[i].endStop == bus->currentStop) {
            for (int j = i; j < bus->passengerCount - 1; j++)
                bus->passengers[j] = bus->passengers[j + 1];
            bus->passengerCount--;
        }
        else i++;
}

void loadPassengers(Bus* bus, BusStop stops[]) {
    int stopIndex = bus->currentStop - 'A';
    BusStop* stop = &stops[stopIndex];
    int maxPassengers = 8 - bus->passengerCount;
    int maxLuggage = 8 - bus->luggageCount;

    while (stop->front <= stop->rear && maxPassengers > 0) {
        Passenger p = stop->queue[stop->front];
        if (p.luggageCount <= maxLuggage) {
            bus->passengers[bus->passengerCount++] = p;
            bus->luggageCount += p.luggageCount;
            maxPassengers--;
            maxLuggage -= p.luggageCount;
            stop->front++;
        }
        else break;
    }
}

void moveBus(Bus* bus, BusLine* line, BusStop stops[]) {
    int currentIndex = -1;
    for (int i = 0; i < line->stopCount; i++)
        if (line->stops[i] == bus->currentStop) { currentIndex = i; break; }
    if (currentIndex == -1) return;
    int newIndex = currentIndex + bus->direction;
    if (newIndex < 0 || newIndex >= line->stopCount) {
        bus->direction *= -1;
        newIndex = currentIndex + bus->direction;
    }
    bus->currentStop = line->stops[newIndex];
    unloadPassengers(bus);
    loadPassengers(bus, stops);
}
int calculateWaitingPassengers(BusStop stops[]) {
    int total = 0;
    for (int i = 0; i < STOP_COUNT; i++)
        total += (stops[i].rear - stops[i].front + 1);
    return total;
}

int calculateTravellingPassengers(Bus buses[]) {
    int total = 0;
    for (int i = 0; i < MAX_BUSES; i++)
        total += buses[i].passengerCount;
    return total;
}

float calculateBusOccupancy(Bus buses[]) {
    int totalPassengers = 0;
    for (int i = 0; i < MAX_BUSES; i++)
        totalPassengers += buses[i].passengerCount;
    return (totalPassengers / (float)(MAX_BUSES * 8)) * 100.0f;
}

void drawCityWithBuses(CityMap* map, BusStop stops[], Bus buses[]) {
    char copy[MAP_ROWS][MAP_COLS + 1];
    for (int i = 0; i < MAP_ROWS; i++) {
        strcpy(copy[i], map->grid[i]);
    }

    // Durakların yanına bekleyen yolcu sayisi yaz
    for (int i = 0; i < STOP_COUNT; i++) {
        char stop = 'A' + i;
        int row = -1, col = -1;
        for (int r = 0; r < MAP_ROWS; r++) {
            for (int c = 0; c < MAP_COLS; c++) {
                if (copy[r][c] == stop) {
                    row = r;
                    col = c;
                    break;
                }
            }
            if (row != -1) break;
        }

        // Toplam yolcu sayısı (bekleyen)
        int waiting = stops[i].rear - stops[i].front + 1;
        if (waiting < 0) waiting = 0;

        // Otobüs etiketi belirle
        char busLabels[9] = ""; // Maks 2 otobüs için yeterli (örnek: A1B2)
        int len = 0;
        for (int b = 0; b < MAX_BUSES; b++) {
            if (buses[b].currentStop == stop) {
                len += snprintf(busLabels + len, sizeof(busLabels) - len, "%c%d", buses[b].lineName, buses[b].busID);
            }
        }

        // Durak yazısı: C0 (0 bekleyen), C1 (1 bekleyen), C1A2 (otobüs etiketi)
        char label[10];
        if (strlen(busLabels) > 0)
            snprintf(label, sizeof(label), "%c%d%s", stop, waiting, busLabels);
        else
            snprintf(label, sizeof(label), "%c%d", stop, waiting);

        // Etiketi haritaya yaz
        int maxLen = strlen(label);
        if (row != -1 && col + maxLen < MAP_COLS)
            if (row != -1 && col != -1) {
                for (int b = 0; b < MAX_BUSES; b++) {
                    if (buses[b].currentStop == stop) {
                        char busChar = buses[b].passengerCount + '0';
                        if (busChar > '9') busChar = '*'; // 9'dan fazlaysa yıldız koy
                        copy[row][col + 1] = busChar;
                    }
                }
            }
        if (row + 1 < MAP_ROWS) {
            char waitStr[3];
            snprintf(waitStr, sizeof(waitStr), "%d", waiting);
            copy[row + 1][col + 1] = waitStr[0];
            if (waiting >= 10 && strlen(waitStr) > 1)
                copy[row + 1][col + 2] = waitStr[1];
        }
    }
    // Otobüsleri bulunduğu durağın üstüne yolcu sayısıyla yaz
    for (int b = 0; b < MAX_BUSES; b++) {
        char stop = buses[b].currentStop;

        int row = -1, col = -1;
        for (int r = 0; r < MAP_ROWS; r++) {
            for (int c = 0; c < MAP_COLS; c++) {
                if (copy[r][c] == stop) {
                    row = r;
                    col = c;
                    break;
                }
            }
            if (row != -1) break;
        }

        if (row > 0 && col >= 0 && col < MAP_COLS) {
            char countChar = buses[b].passengerCount + '0';
            copy[row - 1][col] = countChar;
        }
    }

    for (int i = 0; i < MAX_BUSES; i++) {
        char currentStop = buses[i].currentStop;
        int row = -1, col = -1;

        for (int r = 0; r < MAP_ROWS; r++) {
            for (int c = 0; c < MAP_COLS; c++) {
                if (copy[r][c] == currentStop) {
                    row = r;
                    col = c;
                    break;
                }
            }
            if (row != -1) break;
        }

        if (row != -1 && col != -1) {
            // Yolda ise çiz
            if (map->grid[row][col] == ' ') {
                char ch = '0' + buses[i].passengerCount;
                if (strcmp(selectedBusID, "") != 0 &&
                    buses[i].lineName == selectedBusID[0] &&
                    buses[i].busID == (selectedBusID[1] - '0')) {
                    ch = 'S'; // Seçili otobüs için farklı bir karakter (örneğin 'S')
                }
                copy[row][col] = ch;
            }
        }
    }
    // Otobüsleri haritaya yolcu sayısıyla çiz
    for (int b = 0; b < MAX_BUSES; b++) {
        char stop = buses[b].currentStop;
        int row = -1, col = -1;
        for (int r = 0; r < MAP_ROWS; r++) {
            for (int c = 0; c < MAP_COLS; c++) {
                if (map->grid[r][c] == stop) {
                    row = r;
                    col = c;
                    break;
                }
            }
            if (row != -1) break;
        }

        // Yolcu sayısını otobüs olarak göster
        if (row != -1 && col != -1 && map->grid[row][col] == ' ') {
            int count = buses[b].passengerCount;
            if (count < 10)
                copy[row][col] = '0' + count;
            else
                copy[row][col] = '*';  // 10 veya daha fazlaysa yıldızla göster
        }
    }

    for (int i = 0; i < MAP_ROWS; i++) {
        printf("%s\n", copy[i]);
    }
}
void drawSelectedStopDetails(BusStop stops[]) {
    int idx = selectedStopID - 'A';
    BusStop stop = stops[idx];

    printf("\n%-60s\n", "Selected Stop Details");
    printf("%-60s\n", "-----------------------");

    if (stop.front > stop.rear) {
        printf("%-60s\n", "No passengers at this stop.");
        return;
    }

    printf("%-60s\n", "ID   From -> To   Luggage");

    for (int i = stop.front; i <= stop.rear; i++) {
        Passenger p = stop.queue[i];
        printf("%60d:   %c -> %c   ", p.id, p.startStop, p.endStop);
        if (p.luggageCount == 0) printf("-\n");
        else {
            for (int l = 0; l < p.luggageCount; l++) {
                printf("%d", p.luggageIDs[l]);
                if (l < p.luggageCount - 1) printf(",");
            }
            printf("\n");
        }
    }
}
void drawSelectedStopSummary(BusStop stops[]) {
    int idx = selectedStopID - 'A';
    BusStop stop = stops[idx];

    int count = stop.rear - stop.front + 1;
    if (count < 0) count = 0;

    int totalLuggage = 0;
    for (int i = stop.front; i <= stop.rear; i++) {
        totalLuggage += stop.queue[i].luggageCount;
    }

    printf("[%c: %d passengers, %d luggage]\n\n", selectedStopID, count, totalLuggage);
}


// Yeni drawScreen fonksiyonu
void drawScreen(CityMap* city, BusStop stops[], Bus buses[], PassengerQueue* pq) {
    system(CLEAR);
    drawCityWithBuses(city, stops, buses);
    drawSelectedStopSummary(stops);
    printf("123456789012345678901234567890123456789012345678901234567 Time: %d (%s)\n\n",
        currentTime, running ? "running" : "paused");

    // New Passengers
    printf("%*sNew Passengers\n%*s---------------\n   > ", 60, "", 60, "");
    for (int j = 0; j < pq->count; j++) printf("%c", pq->queue[j].startStop);
    printf(" >\n%*s---------------\n", 60, "");

    // General statistics
    printf("%*sWaiting     : %d\n", 60, "", calculateWaitingPassengers(stops));
    printf("%*sTravelling  : %d\n", 60, "", calculateTravellingPassengers(buses));
    printf("%*sBus fullness: %.2f%%\n", 60, "", calculateBusOccupancy(buses));

    // Specific bus display
    printf("%*sBus [%s] Passengers:\n", 60, "", selectedBusID);
    for (int b = 0; b < MAX_BUSES; b++) {
        char temp[4];
        sprintf(temp, "%c%d", buses[b].lineName, buses[b].busID);
        if (strcmp(temp, selectedBusID) == 0) {
            for (int p = 0; p < buses[b].passengerCount; p++) {
                Passenger pa = buses[b].passengers[p];
                printf("%*s%d: %c-%c (L:", 60, "", pa.id, pa.startStop, pa.endStop);
                if (pa.luggageCount == 0) printf("-)");
                else {
                    for (int l = 0; l < pa.luggageCount; l++) {
                        printf("%d", pa.luggageIDs[l]);
                        if (l < pa.luggageCount - 1) printf(",");
                    }
                    printf(")");
                }
                printf("\n");
            }

            // Luggage list
            printf("%*sBus [%s] Luggage:\n%*s| ", 60, "", SELECTED_BUS_ID, 60, "");
            int yazildi = 0;
            for (int p = 0; p < buses[b].passengerCount; p++) {
                for (int l = 0; l < buses[b].passengers[p].luggageCount; l++) {
                    printf("%d | ", buses[b].passengers[p].luggageIDs[l]);
                    yazildi = 1;
                }
            }
            if (!yazildi) printf("- |");
            printf("\n%*s-----\n", 60, "");
        }
    }
    drawSelectedStopDetails(stops);
    printf("\nKeys:\n[R] Run continuously\n[SPACE] Step by step\n[Q] Quit\n");
    printf("[W] Select previous stop  [E] Select next stop  [Current: %c]\n", selectedStopID);

}

void simulateStep(CityMap* city, BusStop stops[], Bus buses[], PassengerQueue* pq) {
    for (int i = 0; i < MAX_BUSES; i++)
        moveBus(&buses[i], &busLines[i / 2], stops);

    // Her adımda 2 yeni yolcu üret
    generatePassenger(stops, pq);
    generatePassenger(stops, pq);

    currentTime++;
    drawScreen(city, stops, buses, pq);
}


int main() {
    srand(time(NULL));

    CityMap city;
    BusStop stops[STOP_COUNT];
    Bus buses[MAX_BUSES];
    PassengerQueue pq = { .count = 0 };

    for (int i = 0; i < STOP_COUNT; i++) {
        stops[i].name = 'A' + i;
        stops[i].front = 0;
        stops[i].rear = -1;
    }

    initializeCity(&city);

    for (int i = 0; i < 50; i++)
        generatePassenger(stops, &pq);

    int busIndex = 0;
    for (int i = 0; i < 6; i++) {
        initializeBus(&buses[busIndex], &busLines[i], 1); busIndex++;
        initializeBus(&buses[busIndex], &busLines[i], 2); busIndex++;
    }

    drawScreen(&city, stops, buses, &pq);

    char input;
    while (1) {
        if (running) {
            simulateStep(&city, stops, buses, &pq);
            Sleep(1300);
        }
        else {
            input = getchar();
            if (input == 'q' || input == 'Q') break;
            else if (input == 'r' || input == 'R') running = 1;
            else if (input == ' ') simulateStep(&city, stops, buses, &pq);
            else if (input == 'w' || input == 'W') {
                if (selectedStopID > 'A') selectedStopID--;
            }
            else if (input == 'e' || input == 'E') {
                if (selectedStopID < 'P') selectedStopID++;
            }
            else if (input == 's' || input == 'S') {
                printf("Enter bus ID (e.g., A1, B2): ");
                scanf("%3s", selectedBusID);
                
                while (getchar() != '\n');  // Enter'ı temizle
            }

            

            

           
        }
    }

    return 0;
}
