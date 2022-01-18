#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <math.h>
#include <string.h>
#include <stdio.h>  //Used only if we want to print something
#include <sys/time.h>
#include <pthread.h>



#define SCREENWIDTH 3000
#define SCREENHEIGHT 1800
#define NUM_CLUSTERS 10000      //Number of clusters(Centroids) 100000
#define NUM_POINTS 25000
#define RADIUS 1               //Radius of a single point
#define SQUARE_DIMENSIONS 10    //Dimmension of a single centroid
#define FRAMES_PER_SECONDS 14   //Numbers of updates modified in a single seconds
#define APPLICATION_TITLE "K Cluster"
#define NUM_THREADS 8

typedef struct
{
    short r,g,b
}Color;

//###POINT STRUCT###
typedef struct
{
    int x,
        y;
    int clusterId;
    Color color
}Point;

//###CLUSTER STRUCT###
typedef struct
{
    int id;
    Color color;
    int x,
        y;
    int allX;
    int allY;
    int numElements;
    pthread_mutex_t mutex;
}Cluster;


void* initCluster(void* thread);
void * initPoints(void *thread);    //Initializes the Cluster Array and the Points Array
int drawClusters(Cluster* clusters);                            //Displays the clusters (Centroids) (with filled squares)
int drawPoints(Point* points);                                  //Displays the Points on screen (with filled circles)
void * updatePoint(Point* point);     //Assigns to a point the cluster(color & clusterId)
float distanceBetweenPoints(int x0,int y0, int x1,int y1);      //Calculates distance between two points
void* updateClusters(void* thread);                         //Updates all clusters positions
void * updatePoints(void *thread);            //Updates all points colors
void printTime(struct timeval *pTimerStart, struct timeval *pTimerEnd);
void printTimeAndText(char text0[],char text1[],struct timeval *pTimerStart, struct timeval *pTimerEnd);

Cluster *clusters;
Point *points;
struct timeval t1,t2;
bool modified = false;
int main()
{

    clusters = malloc(sizeof(Cluster) * NUM_CLUSTERS);
    points = malloc(sizeof(Point) * NUM_POINTS);


    int iterationsCounter = 1;               //Used to print the iterations

    long thread;
    pthread_t * threadHandles;
    int threadCount = NUM_THREADS;
    threadHandles = malloc(threadCount * sizeof (pthread_t));
    printf("Creating Random Clusters\n");
    fflush(stdout);

    gettimeofday(&t1,NULL);
    for (thread = 0;  thread < threadCount ; thread++)
        pthread_create(&threadHandles[thread], NULL, initCluster, (void *) thread);
    for (thread = 0;  thread <threadCount ; thread++)
        pthread_join(threadHandles[thread],NULL);
    gettimeofday(&t2,NULL);
    printTime(&t1,&t2);

    for(thread = 0; thread < threadCount; thread++)
        pthread_create(&threadHandles[thread],NULL,initPoints,(void *) thread);
    for (thread = 0;  thread <threadCount ; thread++)
        pthread_join(threadHandles[thread],NULL);


    if (!al_init())
    {
        al_show_native_message_box(NULL,NULL,NULL,"Could not inizialize Allegro5",NULL,NULL);
        return -1;
    }
    if( !al_install_keyboard())
    {
        al_show_native_message_box(NULL,NULL,NULL,"Could not inizialize keyboard",NULL,NULL);
        return -1;
    }

    ALLEGRO_TIMER* frameTimer = al_create_timer(1.0 / FRAMES_PER_SECONDS);
    ALLEGRO_EVENT_QUEUE* queue = al_create_event_queue();

    ALLEGRO_DISPLAY  *display = al_create_display(SCREENWIDTH,SCREENHEIGHT);

    if(!display)
    {
        al_show_native_message_box(display,"Sample Title", "Display Settings","Display Window was not created successfully", NULL, ALLEGRO_MESSAGEBOX_ERROR);
        return -1;
    }

    al_register_event_source(queue, al_get_keyboard_event_source());
    al_register_event_source(queue, al_get_display_event_source(display));
    al_register_event_source(queue, al_get_timer_event_source(frameTimer));



    al_set_window_title(display,APPLICATION_TITLE);
    al_init_primitives_addon();

    bool redraw = true;
    ALLEGRO_EVENT event;

    al_start_timer(frameTimer);
    while(1)
    {
        al_wait_for_event(queue, &event);
        if(event.type == ALLEGRO_EVENT_TIMER)
            redraw = true;
        else if((event.type == ALLEGRO_EVENT_KEY_DOWN) || (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE))
            break;

        if(redraw)
        {
            //Reset Display
            al_clear_to_color(al_map_rgb(0, 0, 0));
            //Displaying Points and clusters
            drawPoints(points);
            drawClusters(clusters);
            al_flip_display();      //Display all the drew objectes
            //Print the iteration number
            printf( "Doing the %d iteration\n", iterationsCounter);
            fflush(stdout);
            printf("\tDoing the updateCluster Function\n");
            fflush(stdout);
            //Updates position of clusters and the points color
            gettimeofday(&t1,NULL);
            for (thread = 0;  thread < threadCount ; thread++)
                pthread_create(&threadHandles[thread], NULL, updateClusters, (void *) thread);
            for (thread = 0;  thread < threadCount ; thread++)
                pthread_join(threadHandles[thread],NULL);
            gettimeofday(&t2,NULL);

            printTimeAndText("\t\tDone in "," microseconds\n",&t1,&t2);


            if(!modified)
            {
                printf("Clusters are balanced!\n");
                fflush(stdout);
                al_destroy_timer(frameTimer);
            }
            else
            {
                printf("\tDoing the updatePoints Function\n");
                gettimeofday(&t1,NULL);
                for (thread = 0;  thread < threadCount ; thread++)
                    pthread_create(&threadHandles[thread], NULL, updatePoints, (void *) thread);
                for (thread = 0;  thread < threadCount ; thread++)
                    pthread_join(threadHandles[thread],NULL);
                gettimeofday(&t2,NULL);
                printTimeAndText("\t\tDone in "," microseconds\n",&t1,&t2);
                modified = false;
            }

            iterationsCounter++;
            redraw = false;
        }
    }

    al_destroy_display(display);
    al_destroy_timer(frameTimer);
    al_destroy_event_queue(queue);

    return 0;
}

void printTime(struct timeval *pTimerStart, struct timeval *pTimerEnd)
{

    printf("\tDone in %ld microseconds\n", (pTimerEnd->tv_sec - pTimerStart->tv_sec) * 1000000 + (pTimerEnd->tv_usec - pTimerStart->tv_usec));
    fflush(stdout);
}

void printTimeAndText(char text0[],char text1[],struct timeval *pTimerStart, struct timeval *pTimerEnd)
{

    printf("%s%ld%s", text0,(pTimerEnd->tv_sec - pTimerStart->tv_sec) * 1000000 + (pTimerEnd->tv_usec - pTimerStart->tv_usec),text1);
    fflush(stdout);
}
void * updatePoints(void *thread)
{
    long threadID = (long) thread;
    for (long i = NUM_POINTS/NUM_THREADS * threadID; i < (NUM_POINTS/NUM_THREADS * (threadID + 1) - 1); i++)
    {
        updatePoint(&points[i]);
    }
}

void* updateClusters(void* thread)
{
    long threadID = (long) thread;
    for (long i = NUM_CLUSTERS/NUM_THREADS * threadID; i < (NUM_CLUSTERS/NUM_THREADS * (threadID + 1) - 1); i++)
    {
        if(clusters[i].numElements == 0)
            continue;

        int tempAverageX = clusters[i].allX / clusters[i].numElements;
        int tempAverageY = clusters[i].allY / clusters[i].numElements;
        if (tempAverageX != clusters[i].x)
        {
            clusters[i].x = tempAverageX;
            modified = true;
        }
        if(tempAverageY != clusters[i].y)
        {
            clusters[i].y = tempAverageY;
            modified = true;
        }

        clusters[i].numElements = 0;
        clusters[i].allX = 0;
        clusters[i].allY = 0;
    }
    pthread_exit(NULL);
}

void* initCluster(void* thread)
{

    //Create clusters Points
    long threadID = (long)thread;
    for (long i = NUM_CLUSTERS/NUM_THREADS * threadID; i < (NUM_CLUSTERS/NUM_THREADS * (threadID + 1) - 1); i++)
    {
        clusters[i].id = (int)i;
        clusters[i].y = (int)i %SCREENHEIGHT;
        clusters[i].x = (int)i;
        clusters[i].color.r = (short)(lrand48() % 256);
        clusters[i].color.g = (short)(lrand48() % 256);
        clusters[i].color.b = (short)(lrand48() % 256);
        clusters[i].numElements = 0;
        clusters[i].allX = 0;
        clusters[i].allY = 0;
    }
    pthread_exit(NULL);
}
void* initPoints(void *thread)
{
    long threadID = (long)thread;
    for (long i = NUM_POINTS/NUM_THREADS * threadID; i < (NUM_POINTS/NUM_THREADS * (threadID + 1) - 1); i++)
    {
        points[i].x = (int)(lrand48() % SCREENWIDTH);
        points[i].y = (int)(lrand48() % SCREENHEIGHT);
        updatePoint(&points[i]);
    }
    pthread_exit(NULL);
}
void* updatePoint(Point* point)
{
    int idMin = 0;
    float minDistance = distanceBetweenPoints(point->x, point->y, clusters[0].x, clusters[0].y);
    float tempDistance;
    for (int i = 1; i < NUM_CLUSTERS; ++i)
    {
        tempDistance = distanceBetweenPoints(point->x, point->y, clusters[i].x, clusters[i].y);
        if (tempDistance < minDistance)
        {
            minDistance = tempDistance;
            idMin = i;
        }
    }
    pthread_mutex_lock(&clusters[idMin].mutex);
    clusters[idMin].allX += point->x;
    clusters[idMin].allY += point->y;
    clusters[idMin].numElements++;
    pthread_mutex_unlock(&clusters[idMin].mutex);
    point->color = clusters[idMin].color;
}

float distanceBetweenPoints(int x0,int y0, int x1,int y1)
{
    return (float)sqrt(pow(x0-x1,2) + pow(y0-y1,2));
}
int drawPoints(Point* points)
{
    for (int i = 0; i < NUM_POINTS; ++i)
    {
        //al_draw_pixel((float)points[i].x,(float)points[i].y, points[i].color);
        al_draw_filled_circle((float)points[i].x,(float)points[i].y,RADIUS, al_map_rgb(points[i].color.r,points[i].color.g,points[i].color.b));
    }
    return 0;
}

int drawClusters(Cluster* clusters)
{
    for (int i = 0; i < NUM_CLUSTERS; ++i)
    {
        al_draw_filled_rectangle((float)clusters[i].x - SQUARE_DIMENSIONS, (float)clusters[i].y - SQUARE_DIMENSIONS , (float)clusters[i].x + SQUARE_DIMENSIONS, (float)clusters[i].y + SQUARE_DIMENSIONS,
                                 al_map_rgb(clusters[i].color.r,clusters[i].color.g,clusters[i].color.b));
    }
    return 0;
}


