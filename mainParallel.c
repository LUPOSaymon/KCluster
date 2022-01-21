#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <math.h>
#include <string.h>
#include <stdio.h>  //Used only if we want to print something
#include <sys/time.h>
#include <pthread.h>
#include <argp.h>

#define APPLICATION_TITLE "Parallel K Cluster "   //Title of the Application
#define SCREEN_WIDTH 2000
#define SCREEN_HEIGHT 1800
#define DEFAULT_NUM_CLUSTERS 15             //Number of Clusters
#define DEFAULT_NUM_POINTS 100000                //Number of Points
#define DEFAULT_NUM_THREADS 1
#define DEFAULT_INTERATIVE false
#define RADIUS 1                        //Radius of a single point
#define SQUARE_DIMENSIONS 10            //Dimmension of a single centroid
#define FRAMES_PER_SECONDS 14           //Numbers of updates modified in a single seconds

#define VERSION "K-Cluster 0.01"



//###COLOR STRUCT###
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
    pthread_mutex_t mutex;
}Cluster;


/* Used by main to communicate with parse_opt. */
struct arguments
{
    int clusters, points, threads;
    bool interactive;
};

void* initClusters(void* thread);                               //Initializes the Clusters Array
void* initPoints(void *thread);                                 //Initializes the Points Array
void* updateClusters(void* thread);                             //Updates all p_clusters positions
void* updatePoints(void *thread);                               //Updates all p_points colors
void* updatePoint(Point* point);                                //Assigns to a single point the relative Cluster
void drawClusters(Cluster* clusters);                           //Displays the Clusters (Centroids)
void drawPoints(Point* points);                                 //Displays the Points on screen
float distanceBetweenPoints(int x0,int y0, int x1,int y1);      //Calculates distance between two p_points
void printTime(struct timeval *pTimerStart, struct timeval *pTimerEnd);
void printTimeAndText(char text0[],char text1[],struct timeval *pTimerStart, struct timeval *pTimerEnd);

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
       know is a pointer to our arguments structure. */
    struct arguments *arguments = state->input;

    switch (key)
    {
        case 'i':
            arguments->interactive = true;
            break;
        case 'c':
            arguments->clusters = (int) strtol(arg, NULL, 10);
            break;
        case 'p':
            arguments->points = (int) strtol(arg, NULL, 10);
            break;
        case 't':
            arguments->threads = (int) strtol(arg, NULL, 10);
            break;

        case ARGP_KEY_ARG:
            if (state->arg_num != ARGP_NO_ARGS)
                /* Too many arguments. */
                argp_usage (state);
            break;

        case ARGP_KEY_END:
            /* If not enough arguments. */
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

Cluster *p_clusters;
int     n_clusters,
        n_points,
        n_threads;
Point   *p_points;
struct  timeval t1,t2;
bool    modified = false,
        interactive = DEFAULT_INTERATIVE;

//Argp Variables
/* Program documentation. */
static char doc[] =
        "Simple K Cluster using multi-threading";
const char *argp_program_version = VERSION;
/* A description of the arguments we accept. */
static char args_doc[] = "";
/* The options we understand. */
static struct argp_option options[] =
        {
                {"interactive",  'i', 0,      0,  "Run program in interactive mode" },
                {"clusters",    'c', "INTEGER",      0,  "Set the number of p_clusters " },
                {"points",   'p', "INTEGER",      0,"Set the number of p_points" },
                {"threads",   't', "INTEGER", 0,"Set number of threads" },
                {0}
        };
/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc,char** argv)
{
    struct arguments arguments;
    srand48(1234);      //Set the seed for the random function
    ALLEGRO_TIMER *frameTimer = NULL;
    ALLEGRO_EVENT_QUEUE *queue = NULL;
    ALLEGRO_EVENT_QUEUE *keyboard_exit_queue = NULL;
    ALLEGRO_DISPLAY *display = NULL;
    ALLEGRO_EVENT event;
    bool redraw = false;
    arguments.interactive = DEFAULT_INTERATIVE;
    arguments.clusters = DEFAULT_NUM_CLUSTERS;
    arguments.points = DEFAULT_NUM_POINTS;
    arguments.threads = DEFAULT_NUM_THREADS;

    argp_parse (&argp, argc, argv, 0, 0, &arguments);

    interactive = arguments.interactive;
    n_clusters = arguments.clusters;
    n_points = arguments.points;
    n_threads = arguments.threads;



    p_clusters = malloc(sizeof(Cluster) * n_clusters);
    p_points = malloc(sizeof(Point) * n_points);


    int iterationsCounter = 1;               //Used to print the iterations

    long thread;
    pthread_t * threadHandles;
    int threadCount = n_threads;
    threadHandles = malloc(threadCount * sizeof (pthread_t));

    printf("Creating Random Clusters\n");
    fflush(stdout);
    gettimeofday(&t1,NULL);
    for (thread = 0;  thread < threadCount ; thread++)
        pthread_create(&threadHandles[thread], NULL, initClusters, (void *) thread);
    for (thread = 0;  thread <threadCount ; thread++)
        pthread_join(threadHandles[thread],NULL);
    gettimeofday(&t2,NULL);
    printTime(&t1,&t2);

    printf("Creating Random Points\n");
    fflush(stdout);
    gettimeofday(&t1,NULL);
    for(thread = 0; thread < threadCount; thread++)
        pthread_create(&threadHandles[thread],NULL,initPoints,(void *) thread);
    for (thread = 0;  thread <threadCount ; thread++)
        pthread_join(threadHandles[thread],NULL);
    gettimeofday(&t2,NULL);
    printTime(&t1,&t2);

    if(interactive)
    {
        if (!al_init()) {
            al_show_native_message_box(NULL, NULL, NULL, "Could not inizialize Allegro5", NULL, NULL);
            return -1;
        }
        if (!al_install_keyboard()) {
            al_show_native_message_box(NULL, NULL, NULL, "Could not inizialize keyboard", NULL, NULL);
            return -1;
        }
        frameTimer = al_create_timer(1.0 / FRAMES_PER_SECONDS);
        queue = al_create_event_queue();
        keyboard_exit_queue = al_create_event_queue();
        display = al_create_display(SCREEN_WIDTH, SCREEN_HEIGHT);

        if (!display)
        {
            al_show_native_message_box(display, "Sample Title", "Display Settings",
                                       "Display Window was not created successfully", NULL, ALLEGRO_MESSAGEBOX_ERROR);
            return -1;
        }
        al_register_event_source(queue, al_get_keyboard_event_source());
        al_register_event_source(queue, al_get_display_event_source(display));
        al_register_event_source(queue, al_get_timer_event_source(frameTimer));
        al_set_window_title(display, APPLICATION_TITLE);
        al_init_primitives_addon();
        redraw = true;
        al_start_timer(frameTimer);
    }
    bool keep_going = true;
    while(keep_going)
    {
        if(interactive)
        {
            al_wait_for_event(queue, &event);
            if (event.type == ALLEGRO_EVENT_TIMER)
            {
                redraw = true;
                //Reset Display
                al_clear_to_color(al_map_rgb(0, 0, 0));
                //Displaying Points and p_clusters
                drawPoints(p_points);
                drawClusters(p_clusters);
                al_flip_display();      //Display all the drew objectes
                redraw = false;
            }
            else if ((event.type == ALLEGRO_EVENT_KEY_DOWN) || (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE))
            {
                al_destroy_timer(frameTimer);
                exit(0);
                break;
            }
        }

        //Print the iteration number
        printf( "Doing the %d iteration\n", iterationsCounter);
        fflush(stdout);
        printf("\tDoing the updateCluster Function\n");
        fflush(stdout);
        //Updates position of p_clusters and the p_points color
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
            if (interactive)
                al_destroy_timer(frameTimer);
            keep_going = false;
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
    }

    if(interactive)
    {
        al_destroy_display(display);
        al_destroy_event_queue(queue);
    }
    return 0;
}

void printTime(struct timeval *pTimerStart, struct timeval *pTimerEnd)
{

    printf("\tDone in %ld microseconds\n", (pTimerEnd->tv_sec - pTimerStart->tv_sec) * 1000000 + (pTimerEnd->tv_usec - pTimerStart->tv_usec));
    fflush(stdout);
}

void printTimeAndText(char* text0,char* text1,struct timeval *pTimerStart, struct timeval *pTimerEnd)
{
    printf("%s%ld%s", text0,(pTimerEnd->tv_sec - pTimerStart->tv_sec) * 1000000 + (pTimerEnd->tv_usec - pTimerStart->tv_usec),text1);
    fflush(stdout);
}
void * updatePoints(void *thread)
{
    long threadID = (long) thread;
    for (long i = n_points / n_threads * threadID; i < (n_points / n_threads * (threadID + 1)); i++)
        updatePoint(&p_points[i]);
}
void* initClusters(void* thread)
{

    //Create p_clusters Points
    long threadID = (long)thread;
    for (long i = n_clusters / n_threads * threadID; i < (n_clusters / n_threads * (threadID + 1)); i++)
    {
        p_clusters[i].id = (int)i;
        p_clusters[i].y = (int)i % SCREEN_HEIGHT + 100;
        p_clusters[i].x = (int)i +100;
        p_clusters[i].color.r = (short)(lrand48() % 256);
        p_clusters[i].color.g = (short)(lrand48() % 256);
        p_clusters[i].color.b = (short)(lrand48() % 256);
        p_clusters[i].numElements = 0;
        p_clusters[i].allX = 0;
        p_clusters[i].allY = 0;
    }
    pthread_exit(NULL);
}
void* initPoints(void *thread)
{
    long threadID = (long)thread;
    for (long i = n_points / n_threads * threadID; i < (n_points / n_threads * (threadID + 1)); i++)
    {
        p_points[i].x = (int)(lrand48() % SCREEN_WIDTH);
        p_points[i].y = (int)(lrand48() % SCREEN_HEIGHT);
        updatePoint(&p_points[i]);
    }
    pthread_exit(NULL);
}
void* updateClusters(void* thread)
{
    long threadID = (long) thread;
    for (long i = n_clusters / n_threads* threadID; i < (n_clusters / n_threads * (threadID + 1)); i++)
    {
        if(p_clusters[i].numElements == 0)
            continue;

        int tempAverageX = p_clusters[i].allX / p_clusters[i].numElements;
        int tempAverageY = p_clusters[i].allY / p_clusters[i].numElements;
        if (tempAverageX != p_clusters[i].x)
        {
            p_clusters[i].x = tempAverageX;
            modified = true;
        }
        if(tempAverageY != p_clusters[i].y)
        {
            p_clusters[i].y = tempAverageY;
            modified = true;
        }

        p_clusters[i].numElements = 0;
        p_clusters[i].allX = 0;
        p_clusters[i].allY = 0;
    }
    pthread_exit(NULL);
}
void* updatePoint(Point* point)
{
    int idMin = 0;
    float minDistance = distanceBetweenPoints(point->x, point->y, p_clusters[0].x, p_clusters[0].y);
    float tempDistance;
    for (int i = 1; i < n_clusters; ++i)
    {
        tempDistance = distanceBetweenPoints(point->x, point->y, p_clusters[i].x, p_clusters[i].y);
        if (tempDistance < minDistance)
        {
            minDistance = tempDistance;
            idMin = i;
        }
    }

    pthread_mutex_lock(&p_clusters[idMin].mutex);
    p_clusters[idMin].allX += point->x;
    p_clusters[idMin].allY += point->y;
    p_clusters[idMin].numElements++;
    pthread_mutex_unlock(&p_clusters[idMin].mutex);
    point->color = p_clusters[idMin].color;
}

float distanceBetweenPoints(int x0,int y0, int x1,int y1)
{
    return (float)sqrt(pow(x0-x1,2) + pow(y0-y1,2));
}

void drawClusters(Cluster* clusters)
{
    for (int i = 0; i < n_clusters; ++i)
    {
        al_draw_filled_rectangle((float)clusters[i].x - SQUARE_DIMENSIONS, (float)clusters[i].y - SQUARE_DIMENSIONS , (float)clusters[i].x + SQUARE_DIMENSIONS, (float)clusters[i].y + SQUARE_DIMENSIONS,
                                 al_map_rgb(clusters[i].color.r,clusters[i].color.g,clusters[i].color.b));
    }
}
void drawPoints(Point* points)
{
    for (int i = 0; i < n_points; ++i)
    {
        //al_draw_pixel((float)p_points[i].x,(float)p_points[i].y, p_points[i].color);
        al_draw_filled_circle((float)points[i].x,(float)points[i].y,RADIUS, al_map_rgb(points[i].color.r,points[i].color.g,points[i].color.b));
    }
}



