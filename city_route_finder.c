#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define MAX_CITIES 50
#define MAX_ROADS 200
#define NAME_LEN 32
#define MAX_ROAD_KM 9999
#define INF INT_MAX

typedef struct Road {
    int a, b;
    int km;
    int closed;
} Road;

typedef struct Link {
    int to;
    Road *road;
    struct Link *next;
} Link;

typedef struct {
    char name[NAME_LEN];
    Link *links;
} City;

typedef struct { int items[MAX_CITIES]; int top; } IntStack;
typedef struct { int items[MAX_CITIES]; int head, count; } IntQueue;
typedef struct { int dist, city; } HeapItem;
typedef struct { HeapItem *items; int size, cap; } MinHeap;
typedef struct { int city; Link *next; } Frame;

static City cities[MAX_CITIES];
static Road *roads[MAX_ROADS];
static int cityCount = 0;
static int roadCount = 0;

static void stackPush(IntStack *s, int v) { s->items[s->top++] = v; }
static int stackPop(IntStack *s) { return s->items[--s->top]; }

static void queuePush(IntQueue *q, int v) {
    q->items[(q->head + q->count) % MAX_CITIES] = v;
    q->count++;
}

static int queuePop(IntQueue *q) {
    int v = q->items[q->head];
    q->head = (q->head + 1) % MAX_CITIES;
    q->count--;
    return v;
}

static int itemLess(HeapItem a, HeapItem b) {
    return a.dist < b.dist || (a.dist == b.dist && a.city < b.city);
}

static int heapPush(MinHeap *h, int dist, int city) {
    if (h->size == h->cap) {
        int newCap = h->cap ? h->cap * 2 : 16;
        HeapItem *p = realloc(h->items, (size_t)newCap * sizeof *p);
        if (!p) return 0;
        h->items = p;
        h->cap = newCap;
    }
    HeapItem item = { dist, city };
    int i = h->size++;
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (!itemLess(item, h->items[parent])) break;
        h->items[i] = h->items[parent];
        i = parent;
    }
    h->items[i] = item;
    return 1;
}

static HeapItem heapPop(MinHeap *h) {
    HeapItem top = h->items[0];
    HeapItem last = h->items[--h->size];
    int i = 0;
    for (;;) {
        int left = 2 * i + 1;
        if (left >= h->size) break;
        int right = left + 1;
        int child = (right < h->size && itemLess(h->items[right], h->items[left])) ? right : left;
        if (!itemLess(h->items[child], last)) break;
        h->items[i] = h->items[child];
        i = child;
    }
    h->items[i] = last;
    return top;
}

static int sameName(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++;
        b++;
    }
    return *a == *b;
}

static int findCity(const char *name) {
    for (int i = 0; i < cityCount; i++)
        if (sameName(cities[i].name, name)) return i;
    return -1;
}

static Road *findRoad(int a, int b) {
    for (Link *l = cities[a].links; l; l = l->next)
        if (l->to == b) return l->road;
    return NULL;
}

static int cleanName(char *dst, const char *src) {
    size_t n = 0;
    int pendingSpace = 0;
    while (*src == ' ') src++;
    if (!isalpha((unsigned char)*src)) return 0;
    for (; *src; src++) {
        if (*src == ' ') {
            pendingSpace = 1;
            continue;
        }
        if (!isalnum((unsigned char)*src) && *src != '-') return 0;
        if (pendingSpace) {
            if (n + 1 >= NAME_LEN) return 0;
            dst[n++] = ' ';
            pendingSpace = 0;
        }
        if (n + 1 >= NAME_LEN) return 0;
        dst[n++] = *src;
    }
    dst[n] = '\0';
    return n >= 2;
}

static void appendLink(int city, Link *link) {
    link->next = NULL;
    if (!cities[city].links) {
        cities[city].links = link;
        return;
    }
    Link *tail = cities[city].links;
    while (tail->next) tail = tail->next;
    tail->next = link;
}

static int addCity(const char *name) {
    if (cityCount >= MAX_CITIES) {
        puts("The map is full.");
        return -1;
    }
    int existing = findCity(name);
    if (existing >= 0) {
        printf("%s is already on the map.\n", cities[existing].name);
        return -1;
    }
    strcpy(cities[cityCount].name, name);
    cities[cityCount].links = NULL;
    return cityCount++;
}

static int addRoad(int a, int b, int km) {
    if (a == b) {
        puts("A road must join two different cities.");
        return 0;
    }
    if (findRoad(a, b)) {
        printf("A road between %s and %s already exists.\n", cities[a].name, cities[b].name);
        return 0;
    }
    if (roadCount >= MAX_ROADS) {
        puts("The map has no room for more roads.");
        return 0;
    }
    Road *road = malloc(sizeof *road);
    Link *forward = malloc(sizeof *forward);
    Link *backward = malloc(sizeof *backward);
    if (!road || !forward || !backward) {
        free(road);
        free(forward);
        free(backward);
        puts("Error: out of memory.");
        return 0;
    }
    road->a = a;
    road->b = b;
    road->km = km;
    road->closed = 0;
    forward->to = b;
    forward->road = road;
    backward->to = a;
    backward->road = road;
    appendLink(a, forward);
    appendLink(b, backward);
    roads[roadCount++] = road;
    return 1;
}

static void freeMap(void) {
    for (int i = 0; i < cityCount; i++) {
        Link *l = cities[i].links;
        while (l) {
            Link *next = l->next;
            free(l);
            l = next;
        }
        cities[i].links = NULL;
    }
    for (int i = 0; i < roadCount; i++) free(roads[i]);
    roadCount = 0;
    cityCount = 0;
}

static int readLine(char *buf, int size) {
    if (!fgets(buf, size, stdin)) return 0;
    char *nl = strchr(buf, '\n');
    if (nl) {
        *nl = '\0';
        return 1;
    }
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) { }
    return -1;
}

static int readNumber(void) {
    char line[40], *end;
    int r = readLine(line, sizeof line);
    if (r == 0) return -2;
    if (r < 0 || line[0] == '\0') return -1;
    long n = strtol(line, &end, 10);
    while (*end == ' ') end++;
    if (*end != '\0' || n < 0 || n > 1000000) return -1;
    return (int)n;
}

static int readCityName(const char *prompt, char *name) {
    char line[100];
    printf("%s", prompt);
    if (readLine(line, sizeof line) != 1 || !cleanName(name, line)) {
        printf("Invalid city name (2 to %d letters, digits, spaces or hyphens; must start with a letter).\n",
               NAME_LEN - 1);
        return 0;
    }
    return 1;
}

static int selectCity(const char *prompt) {
    char name[NAME_LEN];
    if (!readCityName(prompt, name)) return -1;
    int c = findCity(name);
    if (c < 0) printf("City not found: %s\n", name);
    return c;
}

static void loadSampleMap(void) {
    static const char *names[] = {
        "Janakpuri", "Dwarka", "Rajouri Garden", "Karol Bagh", "Connaught Place",
        "Rohini", "Saket", "Nehru Place", "Lajpat Nagar", "Noida", "Gurugram",
        "Ghaziabad", "Faridabad"
    };
    static const int road[][3] = {
        {0, 1, 8}, {0, 2, 6}, {0, 7, 18}, {1, 10, 21}, {1, 2, 14}, {2, 3, 9},
        {2, 5, 15}, {3, 4, 4}, {3, 5, 17}, {4, 8, 9}, {4, 6, 14}, {4, 11, 25},
        {8, 7, 6}, {8, 9, 13}, {6, 7, 6}, {6, 10, 19}, {7, 9, 14}, {9, 11, 17},
        {12, 7, 20}
    };
    for (size_t i = 0; i < sizeof names / sizeof *names; i++) addCity(names[i]);
    for (size_t i = 0; i < sizeof road / sizeof *road; i++)
        addRoad(road[i][0], road[i][1], road[i][2]);
}

static int buildPath(int dest, const int parent[], int path[]) {
    IntStack s = { {0}, 0 };
    for (int v = dest; v != -1; v = parent[v]) stackPush(&s, v);
    int n = 0;
    while (s.top > 0) path[n++] = stackPop(&s);
    return n;
}

static void printRoute(const char *title, const int path[], int n) {
    printf("%s: ", title);
    for (int i = 0; i < n; i++)
        printf("%s%s", cities[path[i]].name, i + 1 < n ? " -> " : "\n");
    int total = 0;
    for (int i = 0; i + 1 < n; i++) {
        int km = findRoad(path[i], path[i + 1])->km;
        total += km;
        printf("  %s -> %s : %d km\n", cities[path[i]].name, cities[path[i + 1]].name, km);
    }
    printf("Total distance: %d km | Roads used: %d\n", total, n - 1);
}

static int dijkstra(int src, int dist[], int parent[]) {
    MinHeap heap = { NULL, 0, 0 };
    for (int i = 0; i < cityCount; i++) {
        dist[i] = INF;
        parent[i] = -1;
    }
    dist[src] = 0;
    if (!heapPush(&heap, 0, src)) return 0;
    while (heap.size > 0) {
        HeapItem cur = heapPop(&heap);
        if (cur.dist > dist[cur.city]) continue;
        for (Link *l = cities[cur.city].links; l; l = l->next) {
            if (l->road->closed) continue;
            int nd = cur.dist + l->road->km;
            if (nd < dist[l->to]) {
                dist[l->to] = nd;
                parent[l->to] = cur.city;
                if (!heapPush(&heap, nd, l->to)) {
                    free(heap.items);
                    return 0;
                }
            }
        }
    }
    free(heap.items);
    return 1;
}

static void bfs(int src, int hops[], int parent[]) {
    IntQueue q = { {0}, 0, 0 };
    for (int i = 0; i < cityCount; i++) {
        hops[i] = -1;
        parent[i] = -1;
    }
    hops[src] = 0;
    queuePush(&q, src);
    while (q.count > 0) {
        int v = queuePop(&q);
        for (Link *l = cities[v].links; l; l = l->next) {
            if (l->road->closed || hops[l->to] != -1) continue;
            hops[l->to] = hops[v] + 1;
            parent[l->to] = v;
            queuePush(&q, l->to);
        }
    }
}

static void showMap(void) {
    if (cityCount == 0) {
        puts("The map is empty.");
        return;
    }
    int closed = 0;
    for (int i = 0; i < roadCount; i++) closed += roads[i]->closed;
    printf("%d cities, %d roads (%d closed)\n", cityCount, roadCount, closed);
    for (int i = 0; i < cityCount; i++) {
        printf("%2d. %s:", i + 1, cities[i].name);
        if (!cities[i].links) printf(" no roads");
        for (Link *l = cities[i].links; l; l = l->next)
            printf("%s %s (%d km%s)", l == cities[i].links ? "" : ",",
                   cities[l->to].name, l->road->km, l->road->closed ? ", closed" : "");
        putchar('\n');
    }
}

static void doAddCity(void) {
    char name[NAME_LEN];
    if (!readCityName("Enter new city name: ", name)) return;
    if (addCity(name) >= 0) printf("%s added to the map.\n", name);
}

static void doAddRoad(void) {
    int a = selectCity("Enter first city: ");
    if (a < 0) return;
    int b = selectCity("Enter second city: ");
    if (b < 0) return;
    printf("Enter distance in km (1-%d): ", MAX_ROAD_KM);
    int km = readNumber();
    if (km < 1 || km > MAX_ROAD_KM) {
        puts("Invalid distance.");
        return;
    }
    if (addRoad(a, b, km))
        printf("Road added: %s - %s (%d km).\n", cities[a].name, cities[b].name, km);
}

static void doToggleRoad(void) {
    int a = selectCity("Enter first city: ");
    if (a < 0) return;
    int b = selectCity("Enter second city: ");
    if (b < 0) return;
    Road *r = findRoad(a, b);
    if (!r) {
        printf("There is no road between %s and %s.\n", cities[a].name, cities[b].name);
        return;
    }
    r->closed = !r->closed;
    printf("Road %s - %s is now %s.\n", cities[a].name, cities[b].name, r->closed ? "closed" : "open");
}

static void doShortest(void) {
    int src = selectCity("Start city: ");
    if (src < 0) return;
    int dst = selectCity("Destination city: ");
    if (dst < 0) return;
    if (src == dst) {
        puts("Start and destination are the same city: 0 km.");
        return;
    }
    int dist[MAX_CITIES], parent[MAX_CITIES], path[MAX_CITIES];
    if (!dijkstra(src, dist, parent)) {
        puts("Error: out of memory.");
        return;
    }
    if (dist[dst] == INF) {
        printf("No route from %s to %s with the roads currently open.\n",
               cities[src].name, cities[dst].name);
        return;
    }
    printRoute("Shortest route", path, buildPath(dst, parent, path));
}

static void doFewestRoads(void) {
    int src = selectCity("Start city: ");
    if (src < 0) return;
    int dst = selectCity("Destination city: ");
    if (dst < 0) return;
    if (src == dst) {
        puts("Start and destination are the same city: 0 km.");
        return;
    }
    int hops[MAX_CITIES], parent[MAX_CITIES], path[MAX_CITIES];
    bfs(src, hops, parent);
    if (hops[dst] == -1) {
        printf("No route from %s to %s with the roads currently open.\n",
               cities[src].name, cities[dst].name);
        return;
    }
    printRoute("Route with fewest roads", path, buildPath(dst, parent, path));
}

static void doReachable(void) {
    int src = selectCity("Start city: ");
    if (src < 0) return;
    int visited[MAX_CITIES] = { 0 }, order[MAX_CITIES], count = 0, top = 0;
    Frame stack[MAX_CITIES];
    visited[src] = 1;
    order[count++] = src;
    stack[top].city = src;
    stack[top++].next = cities[src].links;
    while (top > 0) {
        Frame *f = &stack[top - 1];
        if (!f->next) {
            top--;
            continue;
        }
        Link *l = f->next;
        f->next = l->next;
        if (l->road->closed || visited[l->to]) continue;
        visited[l->to] = 1;
        order[count++] = l->to;
        stack[top].city = l->to;
        stack[top++].next = cities[l->to].links;
    }
    printf("Cities reachable from %s (depth-first order):\n", cities[src].name);
    for (int i = 0; i < count; i++) printf("  %d. %s\n", i + 1, cities[order[i]].name);
    printf("%d of %d cities reachable.\n", count, cityCount);
}

int main(void) {
    int choice;

    puts("===== CITY ROUTE FINDER =====");
    do {
        puts("1. Load the sample Delhi-NCR map\n2. Start with an empty map");
        printf("Enter choice: ");
        choice = readNumber();
        if (choice == -2) return 0;
    } while (choice != 1 && choice != 2);
    if (choice == 1) loadSampleMap();

    do {
        puts("\n1. Show map\n2. Add city\n3. Add road\n4. Close or reopen a road"
             "\n5. Shortest route (by distance)\n6. Route with fewest roads"
             "\n7. Cities reachable from a city\n0. Quit");
        printf("Enter choice: ");
        choice = readNumber();
        switch (choice) {
            case 1: showMap();        break;
            case 2: doAddCity();      break;
            case 3: doAddRoad();      break;
            case 4: doToggleRoad();   break;
            case 5: doShortest();     break;
            case 6: doFewestRoads();  break;
            case 7: doReachable();    break;
            case 0: puts("Thank you for using City Route Finder."); break;
            case -2: choice = 0;      break;
            default: puts("Invalid choice. Try again.");
        }
    } while (choice != 0);

    freeMap();
    return 0;
}
