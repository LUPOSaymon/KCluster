#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <math.h>
#include <string.h>
#include <stdio.h>  //Used only if we want to print something
#include <sys/time.h>
#include <pthread.h>
#include <argp.h>
#include <omp.h>


#define APPLICATION_TITLE "Parallel K Cluster "   //Title of the Application
#define SCREEN_WIDTH 2000
#define SCREEN_HEIGHT 1800
#define DEFAULT_NUM_CLUSTERS 2000                   //Number of Clusters
#define DEFAULT_NUM_POINTS 200000                   //Number of Points
#define DEFAULT_NUM_THREADS 1
#define DEFAULT_INTERATIVE false
#define DEFAULT_SHOWITERATIONS false
#define RADIUS 1                        //Radius of a single point
#define SQUARE_DIMENSIONS 10            //Dimension of a single centroid

#define FRAMES_PER_SECONDS 14           //Numbers of updates modified in a single seconds
#define VERSION_STRING "K-Cluster 0.01"
#define DOCUMENTATION_STRING
#define DEFAULT_OPENMP false
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


struct Arguments                        //Used by main to communicate with parse_opt
{
    int clusters, points, threads;
    bool interactive;
    bool showIterations;
    bool openmp;
};

void* initClusters(void* thread);                               //Initializes the Clusters Array
void* initPoints(void *thread);                                 //Initializes the Points Array
void* updateClusters(void* thread);                             //Updates all pClusters positions
void* updatePoints(void *thread);                               //Updates all pPoints colors
void* updatePoint(Point* point);                                //Assigns to a single point the relative Cluster
void drawClusters(Cluster* clusters);                           //Displays the Clusters (Centroids)
void drawPoints(Point* points);                                 //Displays the Points on screen
float distanceBetweenPoints(int x0,int y0, int x1,int y1);      //Calculates distance between two pPoints
void printTime(struct timeval *pTimerStart, struct timeval *pTimerEnd);
void printTimeAndText(char text0[],char text1[],struct timeval *pTimerStart, struct timeval *pTimerEnd);

static error_t parse_opt (int key, char *arg, struct argp_state *state)
{
    /* Get the input argument from argp_parse, which we
       know is a pointer to our Arguments structure. */
    struct Arguments *arguments = state->input;

    switch (key)
    {
        case 'i':
            arguments->interactive = true;
            break;
        case 'I':
            arguments->showIterations = true;
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
        case 'o':
            arguments->openmp = true;
            break;
        case ARGP_KEY_ARG:
            if (state->arg_num != ARGP_NO_ARGS)
                /* Too many Arguments. */
                argp_usage (state);
            break;

        case ARGP_KEY_END:
            /* If not enough Arguments. */
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

Cluster *pClusters;
Point   *pPoints;
int     numClusters,
        numPoints,
        numThreads;
bool    modified = false;
bool useOpenmp = DEFAULT_OPENMP;

//###ARGP VARIABLES###
//Documentation that appears every time with --help
static char doc[] = "Simple K Cluster using pthread and openMP. Points and Clusters are positioned randomly and, by default, "
                    "we have numCluster = 2000 and numPoints = 200000";
const char *argpProgramVersion = VERSION_STRING;    //Version of the program
static char argsDoc[] = "";                         //A description of the Arguments we accept.

static struct argp_option options[] =               //The set of arguments that we accept
        {
                {"interactive",     'i',            0,      0,  "Run program in interactive mode" },
                {"omp",             'o',            0   ,   0,  "Run program using OpenMP insthread of pThread"},
                {"clusters",        'c',    "INTEGER",      0,  "Set the number of pClusters " },
                {"points",          'p',    "INTEGER",      0,  "Set the number of pPoints" },
                {"threads",         't',    "INTEGER",      0,  "Set number of threads" },
                {"show_iterations", 'I',            0,      0,  "Show iterations time" },
                {"openmp",          'o',            0,      0,  "Use the openmp library" },
                {0}
        };

static struct argp argp = {options, parse_opt, argsDoc, doc };  //The Argp parser


#pragma clang diagnostic push
#pragma ide diagnostic ignored "openmp-use-default-none"
int main(int argc,char** argv)
{
    //####MAIN SETUP####
    int iterationsCounter = 0;               //Used to print the iterations
    bool showIterations;
    struct  timeval t1,t2;
    bool    interactive = DEFAULT_INTERATIVE;
    struct timeval start,end;
    gettimeofday(&start,NULL);
    srand48(1234);      //Set the seed for the random function
    long totalTimePerIteration = 0;

    //###ARGP SETUP###
    struct Arguments arguments;
    arguments.interactive = DEFAULT_INTERATIVE;
    arguments.clusters = DEFAULT_NUM_CLUSTERS;
    arguments.points = DEFAULT_NUM_POINTS;
    arguments.threads = DEFAULT_NUM_THREADS;
    arguments.showIterations = DEFAULT_SHOWITERATIONS;
    arguments.openmp = DEFAULT_OPENMP;
    argp_parse (&argp, argc, argv, 0, 0, &arguments);
    interactive = arguments.interactive;
    numClusters = arguments.clusters;
    numPoints = arguments.points;
    numThreads = arguments.threads;
    showIterations = arguments.showIterations;
    useOpenmp = arguments.openmp;

    //####ALLEGRO SETUP####
    ALLEGRO_TIMER *frameTimer = NULL;
    ALLEGRO_EVENT_QUEUE *queue = NULL;
    ALLEGRO_DISPLAY *display = NULL;
    ALLEGRO_EVENT event;



    //####THREAD SETUP####
    long thread;
    pthread_t *threadHandles;
    int threadCount = numThreads;
    threadHandles = malloc(threadCount * sizeof (pthread_t));

    //####STARTING####
    pClusters = malloc(sizeof(Cluster) * numClusters);
    pPoints = malloc(sizeof(Point) * numPoints);
    if(showIterations)
    {
        fprintf(stdout, "Creating Random Clusters\n");
        gettimeofday(&t1, NULL);
    }
    if(useOpenmp)
    {
        #pragma omp parallel num_threads(threadCount)
        {
            int my_rank=omp_get_thread_num();
            initClusters((void *) my_rank);
        }
    }
    else
    {
        for (thread = 0;  thread < numThreads ; thread++)
            pthread_create(&threadHandles[thread], NULL, initClusters, (void *) thread);
        for (thread = 0;  thread < numThreads ; thread++)
            pthread_join(threadHandles[thread],NULL);
    }
    if(showIterations)
    {
        gettimeofday(&t2,NULL);
        printTime(&t1,&t2);
    }


    if(showIterations)
    {
        fprintf(stdout,"Creating Random Points\n");
        gettimeofday(&t1,NULL);
    }
    if(useOpenmp)
    {
        #pragma omp parallel num_threads(threadCount)
        {
            int my_rank=omp_get_thread_num();
            initPoints((void *) my_rank);
        }
    }
    else
    {
        for(thread = 0; thread < numThreads; thread++)
            pthread_create(&threadHandles[thread],NULL,initPoints,(void *) thread);
        for (thread = 0;  thread < numThreads ; thread++)
            pthread_join(threadHandles[thread],NULL);
    }
    if(showIterations)
    {
        gettimeofday(&t2,NULL);
        printTime(&t1,&t2);
    }

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
        al_start_timer(frameTimer);
    }
    bool keep_going = true;
    while(keep_going)
    {
        iterationsCounter++;
        if(interactive)
        {
            al_wait_for_event(queue, &event);
            if (event.type == ALLEGRO_EVENT_TIMER)
            {
                //Reset Display
                al_clear_to_color(al_map_rgb(0, 0, 0));
                //Displaying Points and pClusters
                drawPoints(pPoints);
                drawClusters(pClusters);
                al_flip_display();      //Display all the drew objectes
            }
            else if ((event.type == ALLEGRO_EVENT_KEY_DOWN) || (event.type == ALLEGRO_EVENT_DISPLAY_CLOSE))
            {
                al_destroy_timer(frameTimer);
                exit(0);
                break;
            }
        }

        if(showIterations)
        {
            //Print the iteration number
            fprintf( stdout,"Doing the %d iteration\n", iterationsCounter);
            fprintf(stdout,"\tDoing the updateCluster Function\n");
            //Updates position of pClusters and the pPoints color

        }
        gettimeofday(&t1,NULL);
        if(useOpenmp)
        {
            #pragma omp parallel num_threads(threadCount)
            {
                int my_rank=omp_get_thread_num();
                updateClusters((void *) my_rank);
            }
        }
        else
        {
            for (thread = 0;  thread < threadCount ; thread++)
                pthread_create(&threadHandles[thread], NULL, updateClusters, (void *) thread);
            for (thread = 0;  thread < threadCount ; thread++)
                pthread_join(threadHandles[thread],NULL);
        }
        gettimeofday(&t2,NULL);
        if(showIterations)
        {
            printTimeAndText("\t\tDone in "," microseconds\n",&t1,&t2);
        }
        totalTimePerIteration += (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);

        if(!modified)
        {
            fprintf(stdout,"Clusters are balanced!\n");
            if (interactive)
                al_destroy_timer(frameTimer);
            keep_going = false;
            gettimeofday(&end,NULL);
            printf("================================================\n");
            !useOpenmp ? printf("|| USING 'pthread'") : printf("USING 'openmp'");
            printf(" WITH %d THREADS\n",numThreads);
            printf("|| WITH %d CLUSTERS AND %d POINTS\n",numClusters,numPoints);
            printf("|| *********************************************\n");
            printTimeAndText("|| DONE IN  ", " microseconds\n", &start, &end);
            printf("|| WITH %d ITERATIONS\n",iterationsCounter);
            printf("|| AVERAGE TIME PER ITERATION: %ld microseconds\n", totalTimePerIteration/iterationsCounter);
            printf("================================================\n");
        }
        else
        {
            if(showIterations)
            {
                fprintf(stdout,"\tDoing the updatePoints Function\n");

            }
            gettimeofday(&t1,NULL);
            if(useOpenmp)
            {
                #pragma omp parallel num_threads(threadCount)
                {
                    int my_rank=omp_get_thread_num();
                    updatePoints((void *) my_rank);
                }
            }
            else
            {
                for (thread = 0;  thread < threadCount ; thread++)
                    pthread_create(&threadHandles[thread], NULL, updatePoints, (void *) thread);
                for (thread = 0;  thread < threadCount ; thread++)
                    pthread_join(threadHandles[thread],NULL);
            }
            gettimeofday(&t2,NULL);
            if(showIterations)
            {

                printTimeAndText("\t\tDone in "," microseconds\n",&t1,&t2);
            }
            totalTimePerIteration += (t2.tv_sec - t1.tv_sec) * 1000000 + (t2.tv_usec - t1.tv_usec);
            modified = false;
        }

    }

    if(interactive)
    {
        al_wait_for_event(queue,&event);
        al_destroy_display(display);
        al_destroy_event_queue(queue);

    }
    return 0;
}

void printTime(struct timeval *pTimerStart, struct timeval *pTimerEnd)
{

    fprintf(stdout,"\tDone in %ld microseconds\n", (pTimerEnd->tv_sec - pTimerStart->tv_sec) * 1000000 + (pTimerEnd->tv_usec - pTimerStart->tv_usec));
}

void printTimeAndText(char* text0,char* text1,struct timeval *pTimerStart, struct timeval *pTimerEnd)
{
    fprintf(stdout,"%s%ld%s", text0,(pTimerEnd->tv_sec - pTimerStart->tv_sec) * 1000000 + (pTimerEnd->tv_usec - pTimerStart->tv_usec),text1);
}
void * updatePoints(void *thread)
{
    long threadID = (long) thread;
    for (long i = numPoints / numThreads * threadID; i < (numPoints / numThreads * (threadID + 1)); i++)
        updatePoint(&pPoints[i]);
}
void* initClusters(void* thread)
{
    //Create pClusters Points
    long threadID = (long)thread;
    for (long i = numClusters / numThreads * threadID; i < (numClusters / numThreads * (threadID + 1)); i++)
    {
        pClusters[i].id = (int)i;
        pClusters[i].y = (int)(lrand48() % SCREEN_HEIGHT);
        pClusters[i].x = (int)(lrand48() % SCREEN_WIDTH) ;
        pClusters[i].color.r = (short)(lrand48() % 256);
        pClusters[i].color.g = (short)(lrand48() % 256);
        pClusters[i].color.b = (short)(lrand48() % 256);
        pClusters[i].numElements = 0;
        pClusters[i].allX = 0;
        pClusters[i].allY = 0;
        pthread_mutex_init( &pClusters[i].mutex, NULL);
    }
}
void* initPoints(void *thread)
{
    long threadID = (long)thread;
    for (long i = numPoints / numThreads * threadID; i < (numPoints / numThreads * (threadID + 1)); i++)
    {
        pPoints[i].x = (int)(lrand48() % SCREEN_WIDTH);
        pPoints[i].y = (int)(lrand48() % SCREEN_HEIGHT);
        updatePoint(&pPoints[i]);
    }
}

void* updateClusters(void* thread)
{
    long threadID = (long) thread;
    for (long i = numClusters / numThreads * threadID; i < (numClusters / numThreads * (threadID + 1)); i++)
    {
        if(pClusters[i].numElements == 0)
            continue;
        int tempAverageX = pClusters[i].allX / pClusters[i].numElements;
        int tempAverageY = pClusters[i].allY / pClusters[i].numElements;
        if (tempAverageX != pClusters[i].x)
        {
            pClusters[i].x = tempAverageX;
            modified = true;
        }
        if(tempAverageY != pClusters[i].y)
        {
            pClusters[i].y = tempAverageY;
            modified = true;
        }
        pClusters[i].numElements = 0;
        pClusters[i].allX = 0;
        pClusters[i].allY = 0;
    }
}
void* updatePoint(Point* point)
{
    int idMin = 0;
    float minDistance = distanceBetweenPoints(point->x, point->y, pClusters[0].x, pClusters[0].y);
    float tempDistance;
    for (int i = 1; i < numClusters; ++i)
    {
        tempDistance = distanceBetweenPoints(point->x, point->y, pClusters[i].x, pClusters[i].y);
        if (tempDistance < minDistance)
        {
            minDistance = tempDistance;
            idMin = i;
        }
    }

    pthread_mutex_lock(&pClusters[idMin].mutex);
    pClusters[idMin].allX += point->x;
    pClusters[idMin].allY += point->y;
    pClusters[idMin].numElements++;
    pthread_mutex_unlock(&pClusters[idMin].mutex);
    point->color = pClusters[idMin].color;
}

float distanceBetweenPoints(int x0,int y0, int x1,int y1)
{
    return (float)sqrt(pow(x0-x1,2) + pow(y0-y1,2));
}

void drawClusters(Cluster* clusters)
{
    for (int i = 0; i < numClusters; ++i)
    {
        al_draw_filled_rectangle((float)clusters[i].x - SQUARE_DIMENSIONS, (float)clusters[i].y - SQUARE_DIMENSIONS , (float)clusters[i].x + SQUARE_DIMENSIONS, (float)clusters[i].y + SQUARE_DIMENSIONS,
                                 al_map_rgb(clusters[i].color.r,clusters[i].color.g,clusters[i].color.b));
    }
}
void drawPoints(Point* points)
{
    for (int i = 0; i < numPoints; ++i)
        al_draw_filled_circle((float)points[i].x,(float)points[i].y,RADIUS, al_map_rgb(points[i].color.r,points[i].color.g,points[i].color.b));

}



