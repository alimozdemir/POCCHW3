//@author   Alim Ozdemir 150140807
//@date     16.12.2016
//@desc

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TRANS 100
struct cell **matrix;
int N = 0;

void print();
void advanceTime();
int findStation(char);
void setCell(int, int, int, int, int);
void setTransToCell(int, int, int, int);
void intDefault(int *, int, int);

int findTransmission(struct cell *);

void collisions(struct cell *);

struct cell
{
    char station; //default(*)
    int transArray[MAX_TRANS];
    int transCount;
};

struct station
{
    char name;
    int x, y, capacity;
};

struct transmission
{
    char src, dest;
    int startTime, capacity, stage;
    int startX, startY, isStarted, isTransmitted;
    int collisionFlag, successFlag;
};

struct station stations[100];
int stationCount = 0;

struct transmission transmissions[100];
int transmissionCount = 0;
int transmittedCount = 0;

int time = 0;

int main(int argc, char *argv[])
{
    if (argc == 2)
    {
        N = atoi(argv[1]);
    }
    
    int i = 0, j = 0;

    matrix = (struct cell **)malloc(N * sizeof(struct cell *));

    for (i = 0; i < N; i++)
        matrix[i] = (struct cell *)malloc(N * sizeof(struct cell));

    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++)
        {
            matrix[i][j].station = '*';
            matrix[i][j].transCount = 0;
            intDefault(matrix[i][j].transArray, MAX_TRANS, -1);
        }

    FILE *stationFile = fopen("stations.txt", "r");

    if (stationFile != NULL)
    {
        struct station temp;
        while (fscanf(stationFile, "%c %d %d %d\n", &temp.name, &temp.x, &temp.y, &temp.capacity) != EOF)
        {
            stations[stationCount].name = temp.name;
            stations[stationCount].x = temp.x;
            stations[stationCount].y = temp.y;
            stations[stationCount].capacity = temp.capacity;
            stationCount++;
        }

        fclose(stationFile);
    }

    for (i = 0; i < stationCount; i++)
    {
        int x = stations[i].x - 1;
        int y = stations[i].y - 1;

        matrix[x][y].station = stations[i].name;
    }

    FILE *transFile = fopen("transmissions.txt", "r");

    if (transFile != NULL)
    {
        struct transmission temp;
        while (fscanf(transFile, "%c %c %d\n", &temp.src, &temp.dest, &temp.startTime) != EOF)
        {
            transmissions[transmissionCount].src = temp.src;
            transmissions[transmissionCount].dest = temp.dest;
            transmissions[transmissionCount].startTime = temp.startTime;
            transmissions[transmissionCount].capacity = 0;
            transmissions[transmissionCount].stage = 0;
            transmissions[transmissionCount].startX = -1;
            transmissions[transmissionCount].startY = -1;
            transmissions[transmissionCount].isStarted = -1;

            transmissionCount++;
        }
        fclose(transFile);
    }

    advanceTime();

    //clear last propagates.
    advanceTime();

    print();

    return 0;
}

void advanceTime()
{
    int i = 0, j = 0, k = 0;

    time++;

    FILE *logFile = fopen("150140807.txt", "a+");

    //propagate signals if there is
    for (i = 0; i < transmissionCount; i++)
    {
        if (transmissions[i].isStarted == 1)
        {
            int x = transmissions[i].startX;
            int y = transmissions[i].startY;

            if (transmissions[i].stage < transmissions[i].capacity)
            {

                //remove old signals from cells
                setCell(x, y, transmissions[i].stage, i, 0);

                transmissions[i].stage++;

                //send new signal through stage
                setCell(x, y, transmissions[i].stage, i, 1);
            }
            else if (transmissions[i].stage == transmissions[i].capacity)
            {
                transmissions[i].isStarted = -1;
                transmittedCount++;

                if (transmissions[i].successFlag == -1 && transmissions[i].collisionFlag == -1)
                {

                    if (logFile != NULL)
                    {
                        fprintf(logFile, "OOR: %c => %c (%d => %d)\n", transmissions[i].src, transmissions[i].dest, transmissions[i].startTime, time - 1);
                    }

                    printf("OOR %c %c time %d %d \n", transmissions[i].src, transmissions[i].dest, transmissions[i].startTime, time - 1);
                }

                //clear the signals
                setCell(x, y, transmissions[i].stage, i, 0);
            }
        }
    }

    //find if there exists any new transmission
    for (i = 0; i < transmissionCount; i++)
    {
        if (transmissions[i].startTime == time)
        {
            int stationIndex = findStation(transmissions[i].src);

            if (stationIndex != -1)
            {
                int x = stations[stationIndex].x - 1;
                int y = stations[stationIndex].y - 1;

                transmissions[i].capacity = stations[stationIndex].capacity;
                transmissions[i].startX = x;
                transmissions[i].startY = y;
                transmissions[i].stage = 1;
                transmissions[i].isStarted = 1;
                transmissions[i].isTransmitted = 0;
                transmissions[i].collisionFlag = -1;
                transmissions[i].successFlag = -1;

                setCell(x, y, transmissions[i].stage, i, 1);
            }
        }
    }

    //check for collisions

    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            struct cell *temp = &matrix[i][j];

            if (temp->station != '*')
            {
                if (temp->transCount == 1) //success
                {
                    int transIndex = findTransmission(temp);
                    if (transIndex != -1)
                    {
                        if (transmissions[transIndex].dest == temp->station)
                        {
                            if (logFile != NULL)
                            {
                                fprintf(logFile, "SUCCESS: %c => %c (%d => %d)\n", transmissions[transIndex].src, transmissions[transIndex].dest, transmissions[transIndex].startTime, time);
                            }

                            transmissions[transIndex].successFlag = 1;
                        }
                    }
                }
                else if (temp->transCount > 0) //collision
                {
                    //her transmission için check edilicek

                    collisions(temp);

                    for (k = 0; k < transmissionCount; k++)
                    {
                        if (transmissions[k].collisionFlag == 1)
                        {
                            if (logFile != NULL)
                            {
                                fprintf(logFile, "COLLISIONS: %c => %c (%d => %d)\n", transmissions[k].src, transmissions[k].dest, transmissions[k].startTime, time);
                            }
                        }
                    }

                    //output("COLLISION OCCURS\n");
                }
                else
                {
                    //do nothing
                }
            }
        }
    }

    if (logFile != NULL)
    {
        fclose(logFile);
    }

    if (transmittedCount < transmissionCount)
    {
        print();
        advanceTime();
    }
}

void collisions(struct cell *temp)
{
    int i = 0, j = 0, count = 0, count2 = 0;
    for (i = 0; i < MAX_TRANS; i++)
    {
        if (count == temp->transCount)
            break;

        if (temp->transArray[i] != -1)
        {
            struct transmission *trans1 = &transmissions[temp->transArray[i]];

            for (j = 0; j < MAX_TRANS; j++)
            {

                if (count2 == temp->transCount)
                    break;

                if (temp->transArray[j] != -1)
                {
                    struct transmission *trans2 = &transmissions[temp->transArray[j]];
                    if (trans1->dest == trans2->dest)
                    {
                        trans1->collisionFlag = 1;
                        trans2->collisionFlag = 1;
                    }

                    count2++;
                }
            }

            count++;
        }
    }
}

void intDefault(int *array, int size, int val)
{
    int i = 0;
    for (i = 0; i < size; i++)
    {
        array[i] = val;
    }
}

int findStation(char name)
{
    int i = 0, result = -1;
    for (i = 0; i < stationCount; i++)
    {
        if (stations[i].name == name)
        {
            result = i;
            break;
        }
    }

    return result;
}

void setCell(int x, int y, int size, int index, int op)
{
    int i = x - size, j = y - size;

    for (i = x - size; i <= x + size; i += 1)
    {
        for (j = y - size; j <= y + size; j += size)
        {
            if (j == y) // for initial case
                continue;

            //x boundries
            if (i >= 0 && i < N && j >= 0 && j < N)
            {
                setTransToCell(i, j, index, op);
            }
        }
    }

    for (i = x - size; i <= x + size; i += size)
    {
        for (j = y - size; j <= y + size; j += 1)
        {
            if (i == x) // for initial case
                continue;

            if (i >= 0 && i < N && j >= 0 && j < N)
            {
                setTransToCell(i, j, index, op);
            }
        }
    }
}

void setTransToCell(int x, int y, int index, int op)
{
    struct cell *temp = &matrix[x][y];

    int count = MAX_TRANS;
    int i = 0;

    for (i = 0; i < count; i++)
    {
        if (op == 0)
        {
            if (temp->transArray[i] == index)
            {
                temp->transArray[i] = -1;
                temp->transCount--;
                break;
            }
        }
        else
        {
            //already added to that cell.
            if (temp->transArray[i] == index)
            {
                break;
            }
            if (temp->transArray[i] == -1)
            {
                temp->transArray[i] = index;
                temp->transCount++;
                break;
            }
        }
    }
}

void print()
{
    int i = 0, j = 0;
    printf("---------\n");
    for (i = 0; i < N; i++)
    {
        for (j = 0; j < N; j++)
        {
            if (matrix[i][j].station == '*')
            {
                printf("%d ", matrix[i][j].transCount);
            }
            else
            {
                printf("%c ", matrix[i][j].station);
            }
        }
        printf("\n");
    }
}

int findTransmission(struct cell *temp)
{
    int i = 0, result = -1;

    for (i = 0; i < MAX_TRANS; i++)
    {
        if (temp->transArray[i] != -1)
        {
            result = temp->transArray[i];
            break;
        }
    }

    return result;
}
