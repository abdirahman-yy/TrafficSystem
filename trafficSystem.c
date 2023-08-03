#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

pthread_mutex_t northboundMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t southboundMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t bridgeMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t northboundCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t southboundCond = PTHREAD_COND_INITIALIZER;
pthread_cond_t bridgeCond = PTHREAD_COND_INITIALIZER;

typedef enum {
    NORTHBOUND,
    SOUTHBOUND
} Direction;

typedef enum {
    CAR,
    VAN
} VehicleType;

typedef struct {
    int vehicleId;
    VehicleType vehicleType;
    Direction vehicleDirection;
} Vehicle;

const int MAX_QUEUE_SIZE = 100;
Vehicle northboundQueue[MAX_QUEUE_SIZE];
Vehicle southboundQueue[MAX_QUEUE_SIZE];
int northboundQueueSize = 0;
int southboundQueueSize = 0;
int totalWeightOnBridge = 0;
bool isNorthboundTraffic = false;
bool isSouthboundTraffic = false;
bool isBothLanesUsed = false;


void Arrive(int vehicleId, VehicleType vehicleType, Direction vehicleDirection);
void Cross(int vehicleId, VehicleType vehicleType, Direction vehicleDirection);
void Leave(int vehicleId, VehicleType vehicleType, Direction vehicleDirection);
void PrintTrafficFlow();
Direction getRandomDirection(double probabilityNorthbound);
VehicleType getRandomVehicleType();
void* VehicleRoutine(void* args);
void TrafficControlPolicy();
void ReadScheduleFromUser(int* numberOfGroups, int** vehiclesPerGroup, double** probabilityNorthbound, int** delayTimes);
void PrintQueueStatus();
void GenerateVehicleSchedules();


void Arrive(int vehicleId, VehicleType vehicleType, Direction vehicleDirection) {
    if (vehicleDirection == NORTHBOUND) {
        pthread_mutex_lock(&northboundMutex);
        while (isSouthboundTraffic || totalWeightOnBridge + (vehicleType == CAR ? 200 : 300) > 1200 || northboundQueueSize == MAX_QUEUE_SIZE) {
            pthread_cond_wait(&northboundCond, &northboundMutex);
        }
        northboundQueue[northboundQueueSize].vehicleId = vehicleId;
        northboundQueue[northboundQueueSize].vehicleType = vehicleType;
        northboundQueue[northboundQueueSize].vehicleDirection = vehicleDirection;
        northboundQueueSize++;
        pthread_mutex_unlock(&northboundMutex);
    } else {
        pthread_mutex_lock(&southboundMutex);
        while (isNorthboundTraffic || totalWeightOnBridge + (vehicleType == CAR ? 200 : 300) > 1200 || southboundQueueSize == MAX_QUEUE_SIZE) {
            pthread_cond_wait(&southboundCond, &southboundMutex);
        }
        southboundQueue[southboundQueueSize].vehicleId = vehicleId;
        southboundQueue[southboundQueueSize].vehicleType = vehicleType;
        southboundQueue[southboundQueueSize].vehicleDirection = vehicleDirection;
        southboundQueueSize++;
        pthread_mutex_unlock(&southboundMutex);
    }
}


void Cross(int vehicleId, VehicleType vehicleType, Direction vehicleDirection) {
    pthread_mutex_lock(&bridgeMutex);
    if (vehicleDirection == NORTHBOUND) {
        northboundQueueSize--;
    } else {
        southboundQueueSize--;
    }
    totalWeightOnBridge += (vehicleType == CAR ? 200 : 300);
    printf("Vehicle #%d is now crossing the bridge.\n", vehicleId);
    pthread_mutex_unlock(&bridgeMutex);
    sleep(3); 
}


void Leave(int vehicleId, VehicleType vehicleType, Direction vehicleDirection) {
    pthread_mutex_lock(&bridgeMutex);
    totalWeightOnBridge -= (vehicleType == CAR ? 200 : 300);
    printf("Vehicle #%d exited the bridge.\n", vehicleId);
    if (vehicleDirection == NORTHBOUND) {
        if (northboundQueueSize > 0) {
            pthread_cond_signal(&northboundCond);
        } else {
            isNorthboundTraffic = false;
            if (!isSouthboundTraffic) {
                pthread_cond_signal(&bridgeCond);
            }
        }
    } else {
        if (southboundQueueSize > 0) {
            pthread_cond_signal(&southboundCond);
        } else {
            isSouthboundTraffic = false;
            if (!isNorthboundTraffic) {
                pthread_cond_signal(&bridgeCond);
            }
        }
    }
    pthread_mutex_unlock(&bridgeMutex);
}


void TrafficControlPolicy() {
    pthread_mutex_lock(&bridgeMutex);

    if (!isBothLanesUsed && northboundQueueSize > 0 && southboundQueueSize > 0) {
        isBothLanesUsed = true;
        isNorthboundTraffic = false;
        isSouthboundTraffic = false;
    }

    if (isBothLanesUsed && (northboundQueueSize == 0 || southboundQueueSize == 0)) {
        isBothLanesUsed = false;
        if (northboundQueueSize > 0) {
            isNorthboundTraffic = true;
        } else {
            isSouthboundTraffic = true;
        }
    }

    pthread_cond_broadcast(&northboundCond);
    pthread_cond_broadcast(&southboundCond);
    pthread_mutex_unlock(&bridgeMutex);
}


void PrintTrafficFlow() {
    printf("\nTraffic Flow:\n");
    printf("---------------\n");

    printf("Northbound Queue:\n");
    if (northboundQueueSize > 0) {
        for (int i = 0; i < northboundQueueSize; i++) {
            printf("Car #%d (%s) arrived.\n", northboundQueue[i].vehicleId, northboundQueue[i].vehicleDirection == NORTHBOUND ? "northbound" : "southbound");
        }
    } else {
        printf("No vehicles in the Northbound queue.\n");
    }

    printf("---------------\n");
    printf("Southbound Queue:\n");
    if (southboundQueueSize > 0) {
        for (int i = 0; i < southboundQueueSize; i++) {
            printf("Car #%d (%s) arrived.\n", southboundQueue[i].vehicleId, southboundQueue[i].vehicleDirection == NORTHBOUND ? "northbound" : "southbound");
        }
    } else {
        printf("No vehicles in the Southbound queue.\n");
    }
}


Direction getRandomDirection(double probabilityNorthbound) {
    double random = ((double)rand() / (double)RAND_MAX);
    if (random < probabilityNorthbound) {
        return NORTHBOUND;
    } else {
        return SOUTHBOUND;
    }
}


VehicleType getRandomVehicleType() {
    double random = ((double)rand() / (double)RAND_MAX);
    if (random < 0.5) {
        return CAR;
    } else {
        return VAN;
    }
}


void* VehicleRoutine(void* args) {
    int vehicleId = *(int*)args;
    double directionProbability = 0.7;
    int delayTime = rand() % 5;

    if (delayTime > 0) {
        sleep(delayTime);
    }

    Direction vehicleDirection = getRandomDirection(directionProbability);
    VehicleType vehicleType = getRandomVehicleType();
    Arrive(vehicleId, vehicleType, vehicleDirection);

    pthread_mutex_lock(&bridgeMutex);
    PrintQueueStatus();
    pthread_mutex_unlock(&bridgeMutex);

    Cross(vehicleId, vehicleType, vehicleDirection);

    pthread_mutex_lock(&bridgeMutex);
    PrintQueueStatus();
    pthread_mutex_unlock(&bridgeMutex);

    Leave(vehicleId, vehicleType, vehicleDirection);

    pthread_mutex_lock(&bridgeMutex);
    PrintQueueStatus();
    pthread_mutex_unlock(&bridgeMutex);

    free(args);
    return NULL;
}


void ReadScheduleFromUser(int* numberOfGroups, int** vehiclesPerGroup, double** probabilityNorthbound, int** delayTimes) {
    printf("Enter the number of groups: ");
    scanf("%d", numberOfGroups);


    *vehiclesPerGroup = (int*)malloc(*numberOfGroups * sizeof(int));
    *probabilityNorthbound = (double*)malloc(*numberOfGroups * sizeof(double));
    *delayTimes = (int*)malloc(*numberOfGroups * sizeof(int));


    for (int i = 0; i < *numberOfGroups; i++) {
        printf("Enter the number of vehicles in group %d: ", i + 1);
        scanf("%d", &((*vehiclesPerGroup)[i]));

        printf("Enter the probability of vehicles going northbound in group %d (0.0 to 1.0): ", i + 1);
        scanf("%lf", &((*probabilityNorthbound)[i]));

        printf("Enter the delay time for group %d in seconds: ", i + 1);
        scanf("%d", &((*delayTimes)[i]));
    }
}


void PrintQueueStatus() {
    printf("\nCurrent Queue Status:\n");
    printf("---------------\n");

    printf("Northbound Queue Size: %d\n", northboundQueueSize);
    printf("Southbound Queue Size: %d\n", southboundQueueSize);

    printf("---------------\n");
}


int main() {
    srand((unsigned int)time(NULL));


    int numberOfGroups;
    int* vehiclesPerGroup;
    double* probabilityNorthbound;
    int* delayTimes;

    ReadScheduleFromUser(&numberOfGroups, &vehiclesPerGroup, &probabilityNorthbound, &delayTimes);


    pthread_t threads[numberOfGroups][100]; 
    int vehicleId = 1;

    for (int group = 0; group < numberOfGroups; group++) {
        for (int vehicle = 0; vehicle < vehiclesPerGroup[group]; vehicle++) {
            int* vehicleIdArg = (int*)malloc(sizeof(int));
            *vehicleIdArg = vehicleId++;
            pthread_create(&threads[group][vehicle], NULL, VehicleRoutine, vehicleIdArg);
        }
        sleep(delayTimes[group]);
    }


    for (int group = 0; group < numberOfGroups; group++) {
        for (int vehicle = 0; vehicle < vehiclesPerGroup[group]; vehicle++) {
            pthread_join(threads[group][vehicle], NULL);
        }
    }

    PrintTrafficFlow();

    free(vehiclesPerGroup);
    free(probabilityNorthbound);
    free(delayTimes);

    return 0;
}


