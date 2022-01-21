#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <math.h>
#include <string.h>
#include <stdio.h>  //Used only if we want to print something
#include <sys/time.h>


#define SCREEN_WIDTH 2000
#define SCREEN_HEIGHT 1800
#define DEFAULT_NUM_CLUSTERS 100              //Number of Clusters
#define DEFAULT_NUM_POINTS 250000
#define RADIUS 1               //Radius of a single point
#define SQUARE_DIMENSIONS 10    //Dimmension of a single centroid
#define FRAMES_PER_SECONDS 14   //Numbers of updates modified in a single seconds
#define APPLICATION_TITLE "Serial K Cluster"
#define TRUE 1
#define FALSE 0
typedef struct
{
    short r,g,b;
}Color;

//###POINT STRUCT###
typedef struct
{
    int x,
        y;
    int clusterId;
    Color color;
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
}Cluster;

int initClustersAndPoints(Point* points, Cluster* clusters);    //Initializes the Cluster Array and the Points Array
int drawClusters(Cluster* clusters);                            //Displays the p_clusters (Centroids) (with filled squares)
int drawPoints(Point* points);                                  //Displays the Points on screen (with filled circles)
Color updatePoint(Point *point);     //Assigns to a point the cluster(color & clusterId)
float distanceBetweenPoints(int x0,int y0, int x1,int y1);      //Calculates distance between two p_points
bool updateClusters(Cluster *clusters);                         //Updates all p_clusters positions
void updatePoints();            //Updates all p_points colors

void printTime(struct timeval *pTimerStart, struct timeval *pTimerEnd);
struct timeval t1,t2;
Cluster *p_clusters;
Point *p_points;
int main()
{
    srand48(1234);
    p_clusters = malloc(sizeof(Cluster) * DEFAULT_NUM_CLUSTERS);
    p_points = malloc(sizeof(Point) * DEFAULT_NUM_POINTS);
    bool done = false;
    int iterationsCounter = 1;               //Used to print the iterations

    initClustersAndPoints(p_points, p_clusters);

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

    ALLEGRO_DISPLAY  *display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);

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
            //Displaying Points and p_clusters
            drawPoints(p_points);
            drawClusters(p_clusters);
            al_flip_display();      //Display all the drew objectes
            //Print the iteration number
            printf( "Doing the %d iteration\n", iterationsCounter);
            fflush(stdout);
            //Updates position of p_clusters and the p_points color
            gettimeofday(&t1,NULL);
            done = updateClusters(p_clusters);
            if(!done)
            {
                printf("Clusters are balanced!\n");
                fflush(stdout);
                al_destroy_timer(frameTimer);
            }
            else
                updatePoints();
            gettimeofday(&t2,NULL);
            printTime(&t1,&t2);
            iterationsCounter++;
            redraw = false;
        }
    }

    al_destroy_display(display);
    al_destroy_timer(frameTimer);
    al_destroy_event_queue(queue);

    return 0;
}

void printTime(struct timeval *pTimeval0, struct timeval *pTimeval1)
{

    printf("\tDone in %ld microseconds\n", (pTimeval1->tv_sec - pTimeval0->tv_sec)*1000000 + (pTimeval1->tv_usec - pTimeval0->tv_usec));
    fflush(stdout);
}

void updatePoints()
{
    for (int i = 0; i < DEFAULT_NUM_POINTS; ++i)
        p_points[i].color = updatePoint(&p_points[i]);

}

bool updateClusters(Cluster *clusters)
{
    bool modified = false;
    for (int i = 0; i < DEFAULT_NUM_CLUSTERS; ++i)
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
    return modified;
}

int initClustersAndPoints(Point* points, Cluster* clusters)
{
    //Create p_clusters Points
    printf("Creating Random Clusters\n");
    fflush(stdout);
    gettimeofday(&t1,NULL);

    for (int i = 0; i < DEFAULT_NUM_CLUSTERS; ++i)
    {
        clusters[i].id = (int)i;
        clusters[i].y = (int)i % SCREEN_HEIGHT;
        clusters[i].x = (int)i;
        clusters[i].color.r = (short)(lrand48() % 256);
        clusters[i].color.g = (short)(lrand48() % 256);
        clusters[i].color.b = (short)(lrand48() % 256);
        clusters[i].numElements = 0;
        clusters[i].allX = 0;
        clusters[i].allY = 0;
    }
    gettimeofday(&t2,NULL);
    printTime(&t1,&t2);

    printf("Creating Random Points\n");
    fflush(stdout);
    gettimeofday(&t1,NULL);
    for (int i = 0; i < DEFAULT_NUM_POINTS; ++i)
    {
        points[i].x = (int)(lrand48() % SCREEN_WIDTH);
        points[i].y = (int)(lrand48() % SCREEN_HEIGHT);
        points[i].color = updatePoint(&points[i]);
    }
    gettimeofday(&t2,NULL);
    printTime(&t1,&t2);
    return 0;
}
Color updatePoint(Point *point)
{
    int idMin = 0;
    float minDistance = distanceBetweenPoints(point->x, point->y, p_clusters[0].x, p_clusters[0].y);
    float tempDistance = 0;
    for (int i = 1; i < DEFAULT_NUM_CLUSTERS; ++i)
    {
        tempDistance = distanceBetweenPoints(point->x, point->y, p_clusters[i].x, p_clusters[i].y);
        if (tempDistance < minDistance)
        {
            minDistance = tempDistance;
            idMin = i;
        }
    }
    p_clusters[idMin].allX += point->x;
    p_clusters[idMin].allY += point->y;
    p_clusters[idMin].numElements++;
    return p_clusters[idMin].color;
}
float distanceBetweenPoints(int x0,int y0, int x1,int y1)
{
    return (float)sqrt(pow(x0-x1,2) + pow(y0-y1,2));

}
int drawPoints(Point* points)
{
    for (int i = 0; i < DEFAULT_NUM_POINTS; ++i)
    {
        //al_draw_pixel((float)p_points[i].x,(float)p_points[i].y, p_points[i].color);
        al_draw_filled_circle((float)points[i].x,(float)points[i].y,RADIUS, al_map_rgb(points[i].color.r,points[i].color.g,points[i].color.b));
    }
    return 0;
}

int drawClusters(Cluster* clusters)
{
    for (int i = 0; i < DEFAULT_NUM_CLUSTERS; ++i)
    {
        al_draw_filled_rectangle((float)clusters[i].x - SQUARE_DIMENSIONS, (float)clusters[i].y - SQUARE_DIMENSIONS , (float)clusters[i].x + SQUARE_DIMENSIONS, (float)clusters[i].y + SQUARE_DIMENSIONS,
                                 al_map_rgb(clusters[i].color.r,clusters[i].color.g,clusters[i].color.b));
    }
    return 0;
}


