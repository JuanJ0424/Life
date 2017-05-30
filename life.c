#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <sodium.h>

#define C_SIZE 6

struct gol_req{
  GtkWidget *drawing_area;
  short int *r;
  short int *c;
  short int **grid;
  short int **buffer;
  short int ***memory;
  short int *mem_size;
  short int *mem_rule;
  short int *initial_density;
  char *b;
  char *s;
};

void fill_memory(struct gol_req *info);
void game_of_life(struct gol_req *info);

//gcc `pkg-config --cflags gtk+-3.0 libsodium` -o life life.c -g `pkg-config --libs gtk+-3.0 libsodium`

/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;
static cairo_surface_t *lines = NULL;


// MODULO FUNCTION (NOT REMAINDER) THAT RETURNS ONLY POSITVE INTEGERS
int mod(int a, int b){
    int r = a % b;
    return r < 0 ? r + b : r;
}

void paint_grid_lines(struct gol_req *info) {
  double x1, y1, x2, y2;
  cairo_t *cr;
  cr = cairo_create(surface); //  Copy current state of surface in to another surface
  cairo_clip_extents(cr, &x1, &y1, &x2, &y2); // Get size of surface
  cairo_destroy(cr);
  lines = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR, x2, y2); // Create our buffer surface
  cr = cairo_create (lines);
  cairo_set_source_rgb(cr, 1, 1, 1);
  cairo_paint(cr);
  cairo_set_source_rgb(cr, 0, 0, 0);
  cairo_set_line_width (cr, 0.1);
  int i, j;
  for (i = 1; i < *info->r; i++) {
    cairo_move_to(cr,0,(i*C_SIZE));
    cairo_line_to(cr,C_SIZE*(*info->c),(i*C_SIZE));
  }
  for (j = 1; j < *info->c; j++) {
    cairo_move_to(cr,(j*C_SIZE),0);
    cairo_line_to(cr,(j*C_SIZE),C_SIZE*(*info->r));
  }
  cairo_stroke(cr);
  cairo_destroy(cr);
  cairo_create(surface);
  cairo_set_source_surface (cr,lines, 0, 0);
  cairo_paint(cr);
}

static void clear_surface (void) {
  cairo_t *cr;

  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1, 1, 1);
  cairo_paint (cr);

  cairo_destroy (cr);
}

/* Create a new surface of the appropriate size to store our scribbles */
static gboolean
configure_event_cb (GtkWidget         *widget,
                    GdkEventConfigure *event,
                    gpointer           data)
{
  cairo_surface_t *past;
  int clone = 0;
  cairo_t *cr;
  double x1, y1, x2, y2;
  if (surface) { // If surface is not null, then clone it to resize and restore everything
    cr = cairo_create(surface); //  Copy current state of surface in to another surfa
    cairo_clip_extents(cr, &x1, &y1, &x2, &y2); // Get size of surface
    cairo_destroy(cr);
    clone = 1;
    past = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR_ALPHA, x2, y2); // Create our buffer surface
    cr = cairo_create(past);
    cairo_set_source_surface(cr,surface, 0, 0); // Copy from past surface
    cairo_paint(cr);
    cairo_destroy(cr);
    cairo_surface_destroy (surface);
  }
  surface = gdk_window_create_similar_surface (gtk_widget_get_window (widget),
                                               CAIRO_CONTENT_COLOR,
                                               gtk_widget_get_allocated_width (widget),
                                               gtk_widget_get_allocated_height (widget));
 clear_surface();
 if (clone) {
   cr = cairo_create (surface);
   cairo_set_source_surface (cr, past, 0, 0);
   cairo_paint (cr);
   cairo_destroy(cr);
 }

  /* We've handled the configure event, no need for further processing. */
  return TRUE;
}

/* Redraw the screen from the surface. Note that the ::draw
 * signal receives a ready-to-be-used cairo_t that is already
 * clipped to only draw the exposed areas of the widget
 */
static gboolean
draw_cb (GtkWidget *widget,
         cairo_t   *cr,
         gpointer   data)
{
  cairo_set_source_surface (cr, surface, 0, 0);
  cairo_paint (cr);

  return FALSE;
}

static void close_window (void) {
  if (surface)
    cairo_surface_destroy (surface);

  gtk_main_quit ();
}

short int do_count(short int i, short int j, short int **grid, int r, int c){
  short int count = 0;
  count = grid[mod(i-1,r)][mod(j-1,c)] + grid[mod(i-1,r)][j] + grid[mod(i-1,r)][mod(j+1,c)];
  count += grid[i][mod(j-1,c)] + grid[i][mod(j+1,c)];
  count += grid[mod(i+1,r)][mod(j-1,c)] + grid[mod(i+1,r)][j] + grid [mod(i+1,r)][mod(j+1,c)];
  return count;
}

short int evaluate_rule(char *b, char *s, short int *count, short int *value) {
  char *semi_rule;
  if (*value==1)
    semi_rule = s;
  else
    semi_rule = b;
  while(*semi_rule!='\0'){
    if(*count == (*semi_rule-'0')) {
      return 1;
    }
    semi_rule++;
  }
  return 0;
}

void game_of_life(struct gol_req *info) {
  short int i,j;
  cairo_t *cr;
  short int count;
  cr = cairo_create (surface);
  cairo_set_source_surface(cr, lines, 0, 0);
  cairo_paint(cr);
  cairo_destroy (cr);
  cr = cairo_create (surface);
  cairo_set_source_rgb(cr, 0, 0, 0);
  for (i = 0; i < *info->r  ; i++) {
    for (j = 0; j < *info->c ; j++) {
      count = do_count(i,j,info->grid, *info->r, *info->c);
      info->buffer[i][j] = evaluate_rule(info->b, info->s, &count, &info->grid[i][j]);
      if (info->buffer[i][j] == 1) {
        cairo_rectangle (cr, C_SIZE*j, C_SIZE*i, C_SIZE, C_SIZE);
      }
    }
  }
  cairo_fill (cr);
  gtk_widget_queue_draw (info->drawing_area);
  for ( i = 0; i < *info->r; i++) {
    for ( j = 0; j < *info->c; j++) {
      info->grid[i][j] = info->buffer[i][j];
    }
  }
  cairo_destroy (cr);

}

void fill_memory(struct gol_req *info) {
  int i,j;
  for (i = 0; i < *info->r; i++) {
    if(NULL ==(info->grid[i] = malloc(*(info->c)*sizeof(short int))))
      printf("Ni el grid se pudo hacer\n");

    if(NULL ==(info->buffer[i] = malloc(*(info->c)*sizeof(short int))))
      printf("Ni el buffer se pudo hacer\n");
  }
  for (i = 0; i < *info->r; i++) {
    for (j = 0; j < *info->c; j++) {
      if (randombytes_uniform(101)>(100-*info->initial_density)){
          info->grid[i][j] = 1;
      } else {
        info->grid[i][j] = 0;
      }
    }
  }
  for(i = 0; i<(*info->mem_size); i++) {
    game_of_life(info);
    info->memory[i] = info->buffer;
    info->buffer = NULL;
    if (NULL==(info->buffer = malloc((*info->r)*sizeof(short int*))))
      printf("No se le pudo asignar memoria al bufer en fill_memory %d\n", i);
    for (j=0; j<*info->r; j++) {
      if (NULL==(info->buffer[j] = malloc((*info->c)*sizeof(short int))))
        printf("No se le asigno memoria a una fila de buffer en %d \n", i);
    }
  }
}

void memory_rule(struct gol_req *info){
  //printf("Entramos a memory_rule\n");
  int i,j,k;
  short int count = 0;
  short int min, may;
  for(int i = 0; i<*info->r; i++) {
    for (int j = 0; j<*info->c; j++) {
      count = 0;
      for (k = 0; k<*info->mem_size; k++) {
        count += info->memory[k][i][j];
      }
      // MINORITY OF 1's RULE
      if(*info->mem_rule == -1) {
        min = (*info->mem_size%2)==0?(*info->mem_size/2)-1:*info->mem_size/2;
        if(count<=min) // NORMAL CONDITIONS
          info->grid[i][j] = 1;
        else
          info->grid[i][j] = 0;
        // BREAK EQUALITY SITUATIONS BY USING THE MOST RECENT VALUE
        if((*info->mem_size%2)==0 && count == min+1) {
            info->grid[i][j] = info->memory[(*info->mem_size)-1][i][j];
        }
        // PARITY OF ANY NUMBER
      } else if(*info->mem_rule == 0){
        if ((*info->mem_size%2)==0) { // MEMORY SIZE IS EVEN
          if(count==*info->mem_size/2) //EQUAL NUMBER OF 1'S AND 0'S, THE LAST VALUE SURVIVES
            info->grid[i][j] = info->memory[(*info->mem_size)-1][i][j];
          else if (count%2==0)
            info->grid[i][j] = 1;
          else
            info->grid[i][j] = 0;
        } else { // MEMORY SIZE IS ODD, THE NUMBER THAT APPEARS EVEN NUMBER OF TIMES, SURVIVES
          if(count%2==0)
            info->grid[i][j] = 1;
          else
            info->grid[i][j] = 0;
        }
      } else if(*info->mem_rule == 1) {
        may = (*info->mem_size/2)+1;
        if(count>=may)
          info->grid[i][j] = 1;
        else
          info->grid[i][j] = 0;
        if((*info->mem_size%2)==0 && count == may-1) { // BREAK EQUALITY SITUATIONS BY USING THE MOST RECENT VALUE
            info->grid[i][j] = info->memory[(*info->mem_size)-1][i][j];
        }
      } else if(*info->mem_rule == 2) { // EQUALITY RULE
        if(count==*info->mem_size/2)
          info->grid[i][j] = 1;
        else
          info->grid[i][j] = 0;
      }
    }
  }

}

void update_memory(struct gol_req *info) {
  short int i;
  short int **aux = NULL;
  aux = info->memory[0];
  for (i=0;i<(*info->mem_size)-1; i++) {
    info->memory[i] = NULL;
    info->memory[i] = info->memory[i+1];
  }
  info->memory[(*info->mem_size)-1] = NULL;
  info->memory[(*info->mem_size)-1] = info->buffer;
  info->buffer = NULL;
  info->buffer = aux;
}

gboolean game_of_life_with_memory(gpointer data) {
  struct gol_req *info = (struct gol_req *)data;
  memory_rule(info);
  game_of_life(info);
  update_memory(info);
  return TRUE;
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
  GtkWidget *window;
  GtkWidget *frame;
  GtkWidget *drawing_area;


  window = gtk_application_window_new (app);
  gtk_window_set_title (GTK_WINDOW (window), "Game of Life");

  g_signal_connect (window, "destroy", G_CALLBACK (close_window), NULL);

  gtk_container_set_border_width (GTK_CONTAINER (window), 8);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (window), frame);

  drawing_area = gtk_drawing_area_new ();
  /* set a minimum size */
  gtk_widget_set_size_request (drawing_area, 1300, 840);

  gtk_container_add (GTK_CONTAINER (frame), drawing_area);

  /* Signals used to handle the backing surface */
  g_signal_connect (drawing_area, "draw",
                    G_CALLBACK (draw_cb), NULL);
  g_signal_connect (drawing_area,"configure-event",
                    G_CALLBACK (configure_event_cb), NULL);

  /* Event signals */
  // g_signal_connect (drawing_area, "motion-notify-event",
  //                   G_CALLBACK (motion_notify_event_cb), NULL);
  // g_signal_connect (drawing_area, "button-press-event",
  //                   G_CALLBACK (button_press_event_cb), NULL);

  /* Ask to receive events the drawing area doesn't normally
   * subscribe to. In particular, we need to ask for the
   * button press and motion notify events that want to handle.
   */

  gtk_widget_set_events (drawing_area, gtk_widget_get_events (drawing_area)
                                     | GDK_BUTTON_PRESS_MASK
                                     | GDK_POINTER_MOTION_MASK);
  gtk_widget_show_all (window);


  //-------------------- SET REQUIREMENTS --------------------
  // SIZE OF GRID
  short int *r = malloc(sizeof(short int));
  short int *c = malloc(sizeof(short int));
  short int *initial_density = malloc(sizeof(short int));
  *r = 140;
  *c = 217;
  // RULE
  char *b = "3";
  char *s = "23";
  *initial_density = 20;
  // MEMORY RULE -1 IF MINORY OF 1'S, 0 IF PARITY, 1 IF MAYORITY
  short int *mem_rule = malloc(sizeof(short int));
  *mem_rule = 0;
  short int *mem_size = malloc(sizeof(short int));
  *mem_size = 12;
  // ARRAYS
  short int **grid = malloc((*r)*sizeof(short int*));
  short int **buffer = malloc((*r)*sizeof(short int*));
  short int ***memory = malloc((*mem_size)*sizeof(short int**));
  // printf("rule b[0] %d s[0] %d\n",b[0]-'0', s[0]-'0');
  struct gol_req *info = malloc(sizeof(struct gol_req));
  info->drawing_area = drawing_area;
  info->r = r;
  info->c = c;
  info->grid = grid;
  info->buffer = buffer;
  info->memory = memory;
  info->b = b;
  info->s = s;
  info->initial_density = initial_density;
  info->mem_rule = mem_rule;
  info->mem_size = mem_size;
  paint_grid_lines (info);
  fill_memory(info);
  // DENSIDAD, TAMAÑO, MEMORIA, NUMERO
  //-------------------- SET PERIODIC EXECUTION OF GAME OF LIFE --------------------
  // g_time_out makes a periodic call to a specified function
  g_timeout_add_full (G_PRIORITY_HIGH, (guint)2000, (GSourceFunc)game_of_life_with_memory, (gpointer)info, NULL);
}

int main (int argc, char **argv) {
  GtkApplication *app;
  int status;

  app = gtk_application_new ("org.gtk.example", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  return status;
}
