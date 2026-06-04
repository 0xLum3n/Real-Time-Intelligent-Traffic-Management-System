#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <limits.h>
#include <stdbool.h>
#include <sys/time.h>

/*******************************************************************************

CORE SYSTEM ARCHITECTURE & CONSTANTS
******************************************************************************/
#define NUM_NODES 10
#define MAX_ROUTE_LEN 20
#define INF 999999
#define LANE_CAPACITY 50
#define HISTORY_SIZE 60
#define MAX_EVENTS 100

typedef enum {
CAR,
BUS,
TRUCK,
AMBULANCE
} VehicleType;

typedef enum {
OPEN,
BLOCKED,
UNDER_CONSTRUCTION
} RoadStatus;

typedef enum {
EVENT_ACCIDENT,
EVENT_AMBULANCE,
EVENT_ROADBLOCK,
EVENT_CONGESTION
} EventType;

/*******************************************************************************

DATA STRUCTURES
******************************************************************************/

typedef struct Vehicle {
int id;
int src;
int dest;
VehicleType type;
int priority;
long long spawn_time;
long long wait_start_time;
long long total_wait_time;

int route[MAX_ROUTE_LEN];
int route_len;
int route_idx;

struct Vehicle* next;
} Vehicle;

typedef struct {
Vehicle* front;
Vehicle* rear;
int size;
pthread_mutex_t lock;
} Queue;

typedef struct {
int exists;
RoadStatus status;
int base_cost;
int congestion;
} Edge;

typedef struct {
Queue incoming[NUM_NODES];
int active_lane;
int green_time;
pthread_mutex_t lock;
} Intersection;

typedef struct {
EventType type;
int node_a;
int node_b;
long long timestamp;
int active;
} SystemEvent;

typedef struct {
int total_generated;
int total_processed;
int total_completed;
long long cumulative_wait_time;
long long cumulative_travel_time;
int emergency_handled;
int congestion_events;

int generation_history[HISTORY_SIZE];
int completion_history[HISTORY_SIZE];
int history_idx;

pthread_mutex_t lock;
} Analytics;

/*******************************************************************************

GLOBAL STATE
******************************************************************************/

Edge network[NUM_NODES][NUM_NODES];
pthread_rwlock_t graph_lock;

Intersection nodes[NUM_NODES];

Analytics stats;

SystemEvent event_queue[MAX_EVENTS];
int event_count = 0;
pthread_mutex_t event_lock;

FILE* log_file;
pthread_mutex_t log_lock;

int simulation_running = 1;
int vehicle_id_counter = 1;

/*******************************************************************************

UTILITY FUNCTIONS
******************************************************************************/

long long current_timestamp() {
struct timeval tv;
gettimeofday(&tv, NULL);
return (long long)(tv.tv_sec) * 1000 + (long long)(tv.tv_usec) / 1000;
}

const char* get_vtype_str(VehicleType type) {
switch(type) {
case CAR: return "CAR";
case BUS: return "BUS";
case TRUCK: return "TRUCK";
case AMBULANCE: return "AMBULANCE";
default: return "UNKNOWN";
}
}

const char* get_event_str(EventType type) {
switch(type) {
case EVENT_ACCIDENT: return "ACCIDENT";
case EVENT_AMBULANCE: return "AMBULANCE_DISPATCH";
case EVENT_ROADBLOCK: return "ROADBLOCK";
case EVENT_CONGESTION: return "HEAVY_TRAFFIC";
default: return "UNKNOWN";
}
}

void log_csv(const char* event, int node, int lane, int v_id, const char* details) {
pthread_mutex_lock(&log_lock);
if (log_file) {
fprintf(log_file, "%lld,%s,%d,%d,%d,%s\n",
current_timestamp(), event, node, lane, v_id, details);
fflush(log_file);
}
pthread_mutex_unlock(&log_lock);
}

/*******************************************************************************

QUEUE MANAGEMENT (VEHICLE QUEUES)
******************************************************************************/

void init_queue(Queue* q) {
q->front = NULL;
q->rear = NULL;
q->size = 0;
pthread_mutex_init(&q->lock, NULL);
}

void push_queue(Queue* q, Vehicle* v) {
pthread_mutex_lock(&q->lock);
v->next = NULL;

if (v->type == AMBULANCE) {
    v->next = q->front;
    q->front = v;
    if (q->rear == NULL) {
        q->rear = v;
    }
} else {
    if (q->rear == NULL) {
        q->front = v;
        q->rear = v;
    } else {
        q->rear->next = v;
        q->rear = v;
    }
}
q->size++;
pthread_mutex_unlock(&q->lock);
}

void push_front(Queue* q, Vehicle* v) {
pthread_mutex_lock(&q->lock);
v->next = q->front;
q->front = v;
if (q->rear == NULL) {
q->rear = v;
}
q->size++;
pthread_mutex_unlock(&q->lock);
}

Vehicle* pop_queue(Queue* q) {
pthread_mutex_lock(&q->lock);
if (q->front == NULL) {
pthread_mutex_unlock(&q->lock);
return NULL;
}
Vehicle* v = q->front;
q->front = v->next;
if (q->front == NULL) {
q->rear = NULL;
}
q->size--;
pthread_mutex_unlock(&q->lock);
return v;
}

Vehicle* peek_queue(Queue* q) {
if (q->front == NULL) return NULL;
return q->front;
}

/*******************************************************************************

ADVANCED DIJKSTRA ROUTING
******************************************************************************/

int calculate_route(int src, int dest, Vehicle* v) {
int dist[NUM_NODES];
int prev[NUM_NODES];
int visited[NUM_NODES];

pthread_rwlock_rdlock(&graph_lock);

for (int i = 0; i < NUM_NODES; i++) {
    dist[i] = INF;
    prev[i] = -1;
    visited[i] = 0;
}

dist[src] = 0;

for (int count = 0; count < NUM_NODES - 1; count++) {
    int min = INF, u = -1;
    for (int i = 0; i < NUM_NODES; i++) {
        if (!visited[i] && dist[i] <= min) {
            min = dist[i];
            u = i;
        }
    }
    
    if (u == -1 || u == dest) break;
    visited[u] = 1;
    
    for (int i = 0; i < NUM_NODES; i++) {
        if (!visited[i] && network[u][i].exists && network[u][i].status == OPEN) {
            int cost = network[u][i].base_cost + network[u][i].congestion;
            if (dist[u] != INF && dist[u] + cost < dist[i]) {
                dist[i] = dist[u] + cost;
                prev[i] = u;
            }
        }
    }
}

pthread_rwlock_unlock(&graph_lock);

if (dist[dest] == INF) {
    return 0; 
}

int temp_route[MAX_ROUTE_LEN];
int curr = dest;
int len = 0;

while (curr != -1 && len < MAX_ROUTE_LEN) {
    temp_route[len++] = curr;
    curr = prev[curr];
}

v->route_len = len;
v->route_idx = 0;

for (int i = 0; i < len; i++) {
    v->route[i] = temp_route[len - 1 - i];
}

return 1;
}

/*******************************************************************************

INITIALIZATION
******************************************************************************/

void init_system() {
pthread_rwlock_init(&graph_lock, NULL);
pthread_mutex_init(&log_lock, NULL);
pthread_mutex_init(&stats.lock, NULL);
pthread_mutex_init(&event_lock, NULL);

memset(&stats, 0, sizeof(Analytics));

for (int i = 0; i < NUM_NODES; i++) {
    for (int j = 0; j < NUM_NODES; j++) {
        network[i][j].exists = 0;
        network[i][j].status = OPEN;
        network[i][j].base_cost = 0;
        network[i][j].congestion = 0;
    }
}

int edges[][3] = {
    {0,1,10}, {1,0,10}, {1,2,15}, {2,1,15},
    {0,3,20}, {3,0,20}, {1,4,10}, {4,1,10}, {2,5,25}, {5,2,25},
    {3,4,15}, {4,3,15}, {4,5,10}, {5,4,10},
    {3,6,10}, {6,3,10}, {4,7,20}, {7,4,20}, {5,8,15}, {8,5,15},
    {6,7,10}, {7,6,10}, {7,8,15}, {8,7,15}, {8,9,10}, {9,8,10}
};

int num_edges = sizeof(edges) / sizeof(edges[0]);
for (int i = 0; i < num_edges; i++) {
    int u = edges[i][0];
    int v = edges[i][1];
    int w = edges[i][2];
    network[u][v].exists = 1;
    network[u][v].base_cost = w;
}

for (int i = 0; i < NUM_NODES; i++) {
    pthread_mutex_init(&nodes[i].lock, NULL);
    nodes[i].active_lane = -1;
    nodes[i].green_time = 0;
    for (int j = 0; j < NUM_NODES; j++) {
        init_queue(&nodes[i].incoming[j]);
    }
}

log_file = fopen("traffic_log.csv", "w");
if (log_file) {
    fprintf(log_file, "timestamp,event,node,lane,vehicle_id,details\n");
    fflush(log_file);
}
}

/*******************************************************************************

THREAD: SIMULATION ENGINE (VEHICLE GENERATOR)
******************************************************************************/

void* thread_vehicle_generator(void* arg) {
while (simulation_running) {
int generated_this_tick = 0;
int num_vehicles = (rand() % 3) + 1;

    for (int i = 0; i < num_vehicles; i++) {
        int src = rand() % NUM_NODES;
        int dest = rand() % NUM_NODES;
        while (dest == src) { dest = rand() % NUM_NODES; }
        
        Vehicle* v = (Vehicle*)malloc(sizeof(Vehicle));
        v->id = vehicle_id_counter++;
        v->src = src;
        v->dest = dest;
        v->spawn_time = current_timestamp();
        v->wait_start_time = v->spawn_time;
        v->total_wait_time = 0;
        
        int type_rand = rand() % 100;
        if (type_rand < 5) v->type = AMBULANCE;
        else if (type_rand < 15) v->type = BUS;
        else if (type_rand < 35) v->type = TRUCK;
        else v->type = CAR;
        
        v->priority = (v->type == AMBULANCE) ? 100 : 1;
        
        if (calculate_route(src, dest, v)) {
            push_queue(&nodes[src].incoming[src], v); 
            
            pthread_mutex_lock(&stats.lock);
            stats.total_generated++;
            if (v->type == AMBULANCE) stats.emergency_handled++;
            pthread_mutex_unlock(&stats.lock);
            
            char details[100];
            sprintf(details, "Type:%s Dest:%d RouteLen:%d", get_vtype_str(v->type), dest, v->route_len);
            log_csv("SPAWN", src, src, v->id, details);
            
            generated_this_tick++;
        } else {
            free(v); 
        }
    }
    
    pthread_mutex_lock(&stats.lock);
    stats.generation_history[stats.history_idx] = generated_this_tick;
    pthread_mutex_unlock(&stats.lock);
    
    usleep(800000); 
}
return NULL;
}

/*******************************************************************************

THREAD: SMART SIGNAL SYSTEM
******************************************************************************/

void* thread_signal_controller(void* arg) {
while (simulation_running) {
for (int i = 0; i < NUM_NODES; i++) {
pthread_mutex_lock(&nodes[i].lock);

        if (nodes[i].green_time > 0) {
            nodes[i].green_time--;
            pthread_mutex_unlock(&nodes[i].lock);
            continue;
        }
        
        int best_lane = -1;
        int max_density = -1;
        int emergency_lane = -1;
        
        for (int j = 0; j < NUM_NODES; j++) {
            Queue* q = &nodes[i].incoming[j];
            pthread_mutex_lock(&q->lock);
            
            if (q->size > 0) {
                Vehicle* front = peek_queue(q);
                if (front && front->type == AMBULANCE) {
                    emergency_lane = j;
                }
                if (q->size > max_density) {
                    max_density = q->size;
                    best_lane = j;
                }
            }
            
            pthread_mutex_unlock(&q->lock);
        }
        
        if (emergency_lane != -1) {
            nodes[i].active_lane = emergency_lane;
            nodes[i].green_time = 10; 
            log_csv("SIGNAL_OVERRIDE", i, emergency_lane, -1, "Ambulance Priority");
        } else if (best_lane != -1) {
            nodes[i].active_lane = best_lane;
            
            double density = (double)max_density / LANE_CAPACITY;
            if (density > 0.8) nodes[i].green_time = 8;
            else if (density > 0.4) nodes[i].green_time = 5;
            else nodes[i].green_time = 3;
        } else {
            nodes[i].active_lane = -1;
            nodes[i].green_time = 0;
        }
        
        pthread_mutex_unlock(&nodes[i].lock);
    }
    usleep(1000000); 
}
return NULL;
}

/*******************************************************************************

THREAD: VEHICLE MOVEMENT & ROUTING
******************************************************************************/

void* thread_movement(void* arg) {
while (simulation_running) {
int completed_this_tick = 0;
long long now = current_timestamp();

    for (int i = 0; i < NUM_NODES; i++) {
        pthread_mutex_lock(&nodes[i].lock);
        
        int active = nodes[i].active_lane;
        if (active != -1 && nodes[i].green_time > 0) {
            Queue* q = &nodes[i].incoming[active];
            
            Vehicle* v = pop_queue(q);
            if (v != NULL) {
                v->total_wait_time += (now - v->wait_start_time);
                
                if (v->route_idx + 1 >= v->route_len) {
                    
                    pthread_mutex_lock(&stats.lock);
                    stats.total_completed++;
                    stats.cumulative_wait_time += v->total_wait_time;
                    stats.cumulative_travel_time += (now - v->spawn_time);
                    pthread_mutex_unlock(&stats.lock);
                    
                    log_csv("ARRIVED", i, active, v->id, "Reached Destination");
                    completed_this_tick++;
                    free(v);
                } else {
                    int next_node = v->route[v->route_idx + 1];
                    
                    pthread_rwlock_rdlock(&graph_lock);
                    int is_blocked = (network[i][next_node].status != OPEN);
                    pthread_rwlock_unlock(&graph_lock);
                    
                    if (is_blocked) {
                        
                        log_csv("REROUTE", i, active, v->id, "Path Blocked");
                        if (calculate_route(i, v->dest, v)) {
                            
                            push_front(q, v); 
                        } else {
                            log_csv("DROP", i, active, v->id, "No Alternate Route");
                            free(v); 
                        }
                    } else {
                        
                        Queue* dest_q = &nodes[next_node].incoming[i];
                        if (pthread_mutex_trylock(&dest_q->lock) == 0) {
                            
                            v->route_idx++;
                            v->wait_start_time = current_timestamp();
                            v->next = NULL;
                            
                            if (v->type == AMBULANCE) {
                                v->next = dest_q->front;
                                dest_q->front = v;
                                if (!dest_q->rear) dest_q->rear = v;
                            } else {
                                if (!dest_q->rear) {
                                    dest_q->front = v;
                                    dest_q->rear = v;
                                } else {
                                    dest_q->rear->next = v;
                                    dest_q->rear = v;
                                }
                            }
                            dest_q->size++;
                            pthread_mutex_unlock(&dest_q->lock);
                            
                            log_csv("MOVE", i, next_node, v->id, "Advanced");
                            
                            pthread_mutex_lock(&stats.lock);
                            stats.total_processed++;
                            pthread_mutex_unlock(&stats.lock);
                        } else {
                            
                            push_front(q, v);
                        }
                    }
                }
            }
        }
        pthread_mutex_unlock(&nodes[i].lock);
    }
    
    pthread_mutex_lock(&stats.lock);
    stats.completion_history[stats.history_idx] = completed_this_tick;
    stats.history_idx = (stats.history_idx + 1) % HISTORY_SIZE;
    pthread_mutex_unlock(&stats.lock);
    
    usleep(500000); 
}
return NULL;
}

/*******************************************************************************

THREAD: EVENT ENGINE (ACCIDENTS)
******************************************************************************/

void* thread_event_engine(void* arg) {
while (simulation_running) {
usleep(10000000);

    int u = rand() % NUM_NODES;
    int v = rand() % NUM_NODES;
    
    pthread_rwlock_wrlock(&graph_lock);
    if (network[u][v].exists && network[u][v].status == OPEN) {
        network[u][v].status = BLOCKED;
        
        pthread_mutex_lock(&event_lock);
        event_queue[event_count].type = EVENT_ACCIDENT;
        event_queue[event_count].node_a = u;
        event_queue[event_count].node_b = v;
        event_queue[event_count].timestamp = current_timestamp();
        event_queue[event_count].active = 1;
        event_count++;
        pthread_mutex_unlock(&event_lock);
        
        pthread_mutex_lock(&stats.lock);
        stats.congestion_events++;
        pthread_mutex_unlock(&stats.lock);
        
        char details[50];
        sprintf(details, "Accident blocking %d->%d", u, v);
        log_csv("ACCIDENT", u, v, -1, details);
        
        pthread_rwlock_unlock(&graph_lock);
        
        usleep(15000000);
        
        pthread_rwlock_wrlock(&graph_lock);
        network[u][v].status = OPEN;
        
        log_csv("CLEARED", u, v, -1, "Road Cleared");
        pthread_rwlock_unlock(&graph_lock);
        
    } else {
        pthread_rwlock_unlock(&graph_lock);
    }
}
return NULL;
}

/*******************************************************************************

THREAD: LIVE DASHBOARD
******************************************************************************/

void* thread_dashboard(void* arg) {
while (simulation_running) {
printf("\033[2J\033[H");

    printf("==================================================\n");
    printf("       SMART CITY TRAFFIC CONTROL CENTER\n");
    printf("==================================================\n\n");
    
    pthread_mutex_lock(&stats.lock);
    int gen_tot = stats.total_generated;
    int comp_tot = stats.total_completed;
    double avg_wait = comp_tot ? (double)stats.cumulative_wait_time / comp_tot / 1000.0 : 0.0;
    double avg_travel = comp_tot ? (double)stats.cumulative_travel_time / comp_tot / 1000.0 : 0.0;
    
    int gen_sum = 0, comp_sum = 0;
    for (int i = 0; i < HISTORY_SIZE; i++) {
        gen_sum += stats.generation_history[i];
        comp_sum += stats.completion_history[i];
    }
    double throughput = (double)comp_sum / HISTORY_SIZE;
    double predicted_load = (double)gen_sum / HISTORY_SIZE;
    
    printf("SYSTEM METRICS:\n");
    printf("  Vehicles Generated : %d\n", gen_tot);
    printf("  Vehicles Completed : %d\n", comp_tot);
    printf("  Avg Wait Time      : %.2f sec\n", avg_wait);
    printf("  Avg Travel Time    : %.2f sec\n", avg_travel);
    printf("  Emergencies Handled: %d\n", stats.emergency_handled);
    printf("  Active Accidents   : %d\n", stats.congestion_events);
    printf("  Current Throughput : %.2f veh/tick\n", throughput);
    printf("  Predicted Load     : %.2f veh/tick (Moving Avg)\n", predicted_load);
    pthread_mutex_unlock(&stats.lock);
    
    printf("\nINTERSECTION STATUS (Nodes 0-4):\n");
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&nodes[i].lock);
        int active = nodes[i].active_lane;
        int green = nodes[i].green_time;
        
        printf("  [Node %d] Signal: ", i);
        if (active != -1 && green > 0) {
            printf("GREEN for Lane %d (%ds left) | ", active, green);
        } else {
            printf("ALL RED | ");
        }
        
        int total_q = 0;
        for (int j = 0; j < NUM_NODES; j++) {
            pthread_mutex_lock(&nodes[i].incoming[j].lock);
            total_q += nodes[i].incoming[j].size;
            pthread_mutex_unlock(&nodes[i].incoming[j].lock);
        }
        printf("Total Queued: %d\n", total_q);
        pthread_mutex_unlock(&nodes[i].lock);
    }
    
    printf("\n==================================================\n");
    fflush(stdout);
    usleep(1000000); 
}
return NULL;
}

/*******************************************************************************

MAIN ENTRY POINT
******************************************************************************/

int main() {
srand(time(NULL));
init_system();

pthread_t t_gen, t_sig, t_mov, t_evt, t_dash;

pthread_create(&t_gen, NULL, thread_vehicle_generator, NULL);
pthread_create(&t_sig, NULL, thread_signal_controller, NULL);
pthread_create(&t_mov, NULL, thread_movement, NULL);
pthread_create(&t_evt, NULL, thread_event_engine, NULL);
pthread_create(&t_dash, NULL, thread_dashboard, NULL);

pthread_join(t_dash, NULL); 

simulation_running = 0;

pthread_join(t_gen, NULL);
pthread_join(t_sig, NULL);
pthread_join(t_mov, NULL);
pthread_join(t_evt, NULL);

if (log_file) fclose(log_file);
return 0;
}