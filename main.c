#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include <math.h>

#define SCREENWIDTH 2000
#define SCREENHEIGHT 1800
#define NUM_KLUSTERS 15
#define NUM_POINTS 10000

#define RADIUS 5
#define SQUARE_DIMENSIONS 10
#define FRAMES_PER_SECONDS 14

//###Point Struct###
typedef struct
{
    int x,
        y;
    int klusterId;
    ALLEGRO_COLOR color;

}Point;
//###Cluster Struct###
typedef struct
{
    int id;
    ALLEGRO_COLOR color;
    int x,
        y;
    int allX;
    int allY;
    int numElements;
}Kluster;


int initKlustersAndPoints(Point* points, Kluster* klusters);    //Initialize the Kluster Array and the Points Array
int displayKlusters(Kluster* klusters);                         //Display the Klusters (Centroids) (with filled squares)
int displayPoints(Point* points);                               //Display the Points on screen (with filled circles)
ALLEGRO_COLOR updatePoint(Point* point, Kluster* klusters);    //assign to a point the kluster(color & klusterId)
float distanceBetweenPoints(int x0,int y0, int x1,int y1);

void updateKlusters(Kluster *klusters);

void updatePoints(Point *points, Kluster *klusters);

int main()
{
    srand(time(NULL));
    Kluster klusters[NUM_KLUSTERS];
    Point points[NUM_POINTS];
    initKlustersAndPoints(points, klusters);
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



    al_set_window_title(display,"K Cluster");
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
            al_clear_to_color(al_map_rgb(0, 0, 0));             //Reset Display

            displayPoints(points);
            displayKlusters(klusters);
            al_flip_display();
            updateKlusters(klusters);
            updatePoints(points,klusters);
            redraw = false;
        }


    }

    al_destroy_display(display);
    al_destroy_timer(frameTimer);
    al_destroy_event_queue(queue);

    return 0;
}

void updatePoints(Point *points, Kluster *klusters)
{
    for (int i = 0; i < NUM_POINTS; ++i)
    {
        points[i].color = updatePoint(&points[i],klusters);
    }
}

void updateKlusters(Kluster *klusters)
{

    for (int i = 0; i < NUM_KLUSTERS; ++i)
    {
        klusters[i].x = klusters[i].allX / klusters[i].numElements;
        klusters[i].y = klusters[i].allY / klusters[i].numElements;
        klusters[i].numElements = 0;
        klusters[i].allX = 0;
        klusters[i].allY = 0;
    }

}

int initKlustersAndPoints(Point* points, Kluster* klusters)
{
    //Create Klusters Points
    for (int i = 0; i < NUM_KLUSTERS; ++i)
    {
        klusters[i].id = i;
        klusters[i].x = rand() % SCREENWIDTH;
        klusters[i].y = rand() % SCREENHEIGHT;
        klusters[i].color = al_map_rgb_f(rand() % 256 / (float)256, rand() % 256 / (float)256, rand() % 256 / (float)256);
        klusters[i].numElements = 0;
        klusters[i].allX = 0;
        klusters[i].allY = 0;
    }

    for (int i = 0; i < NUM_POINTS; ++i)
    {
        points[i].x = rand() % SCREENWIDTH;
        points[i].y = rand() % SCREENHEIGHT;
        points[i].color = updatePoint(&points[i],klusters);
    }
    return 0;
}
ALLEGRO_COLOR updatePoint(Point* point, Kluster* klusters)
{
    int idMin = 0;
    float minDistance = distanceBetweenPoints(point->x,point->y,klusters[0].x, klusters[0].y);
    float tempDistance = 0;
    for (int i = 1; i < NUM_KLUSTERS; ++i)
    {
        tempDistance = distanceBetweenPoints(point->x,point->y,klusters[i].x, klusters[i].y);
        if (tempDistance < minDistance)
        {
            minDistance = tempDistance;
            idMin = i;
        }
    }
    klusters[idMin].allX += point->x;
    klusters[idMin].allY += point->y;
    klusters[idMin].numElements++;
    return klusters[idMin].color;
}
float distanceBetweenPoints(int x0,int y0, int x1,int y1)
{
    return (float)sqrt(pow(x0-x1,2) + pow(y0-y1,2));

}
int displayPoints(Point* points)
{
    for (int i = 0; i < NUM_POINTS; ++i)
    {
        al_draw_filled_circle((float)points[i].x,(float)points[i].y,RADIUS, points[i].color);
    }
    return 0;
}

int displayKlusters(Kluster* klusters)
{
    for (int i = 0; i < NUM_KLUSTERS; ++i)
    {
        al_draw_filled_rectangle((float)klusters[i].x - SQUARE_DIMENSIONS,(float)klusters[i].y - SQUARE_DIMENSIONS ,(float)klusters[i].x + SQUARE_DIMENSIONS,(float)klusters[i].y + SQUARE_DIMENSIONS,klusters[i].color);
    }
    return 0;
}


