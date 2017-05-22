#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <sodium.h>

#define C_SIZE 6

struct gol_req{
  GtkWidget *drawing_area;
  cairo_surface_t *lines;
  short int **grid;
  short int **buffer;
};

//gcc `pkg-config --cflags gtk+-3.0 libsodium` -o life life.c `pkg-config --libs gtk+-3.0 libsodium`

/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;



// MODULO FUNCTION (NOT REMAINDER) THAT RETURNS ONLY POSITVE INTEGERS
int mod(int a, int b){
    int r = a % b;
    return r < 0 ? r + b : r;
}

void init_grid(GtkWidget *da, cairo_surface_t *lines, short int **grid, short int **buffer) {
  int r = 140;
  int c = 217;
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
  for (i = 1; i < r; i++) {
    cairo_move_to(cr,0,(i*C_SIZE));
    cairo_line_to(cr,C_SIZE*c,(i*C_SIZE));
  }
  for (j = 1; j < c; j++) {
    cairo_move_to(cr,(j*C_SIZE),0);
    cairo_line_to(cr,(j*C_SIZE),C_SIZE*r);
  }
  cairo_stroke(cr);
  cairo_destroy(cr);
  cairo_create(surface);
  cairo_set_source_surface (cr,lines, 0, 0);
  cairo_paint(cr);
  for (i = 0; i < r; i++) {
    grid[i] = malloc(217*sizeof(short int));
    buffer[i] = malloc(217*sizeof(short int));
  }
  for (i = 0; i < r; i++) {
    for (j = 0; j < c; j++) {
      grid[i][j] = randombytes_uniform(2);
    }
  }
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

gboolean game_of_life(gpointer data) {
  int r = 140;
  int c = 217;
  struct gol_req *info = (struct gol_req *)data;
  short int i,j;
  cairo_t *cr;
  short int count;
  short int color;
  cr = cairo_create (surface);
  cairo_set_source_surface(cr, info->lines, 0, 0);
  cairo_paint(cr);
  cairo_destroy (cr);
  cr = cairo_create (surface);
  cairo_set_source_rgb(cr, 0, 0, 0);
  for (i = 0; i < r  ; i++) {
    for (j = 0; j < c ; j++) {
      count = do_count(i,j,info->grid, r, c);
      if (info->grid[i][j] == 1) {
        if (count == 2 || count == 3) {
          info->buffer[i][j] = 1;
          color = 1;
        } else {
          info->buffer[i][j] = 0;
          color = 0;
        }
      } else if (info->grid[i][j] == 0) {
        if (count == 3) {
          info->buffer[i][j] = 1;
          color = 1;
        } else {
          info->buffer[i][j] = 0;
          color = 0;
        }
      }
      if (color == 1) {
        cairo_rectangle (cr, C_SIZE*j, C_SIZE*i, C_SIZE, C_SIZE);
      }
    }
  }
  cairo_fill (cr);
  gtk_widget_queue_draw (info->drawing_area);
  for ( i = 0; i < r; i++) {
    for ( j = 0; j < c; j++) {
      info->grid[i][j] = info->buffer[i][j];
    }
  }
  cairo_destroy (cr);
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
  short int **grid = malloc(140*sizeof(short int*));
  short int **buffer = malloc(140*sizeof(short int*));

  //-------------------- SET PERIODIC EXECUTION OF GAME OF LIFE --------------------
  // SURFACE TO STORE THE GRID BORDERS
  cairo_surface_t *lines = NULL;
  init_grid (drawing_area, lines, grid, buffer);
  struct gol_req *info = malloc(sizeof(struct gol_req));
  info->drawing_area = drawing_area;
  info->lines = lines;
  info->grid = grid;
  info->buffer = buffer;
  // g_time_out makes a periodic call to a specified function
  g_timeout_add_full (G_PRIORITY_HIGH, (guint)26, (GSourceFunc)game_of_life, (gpointer)info, NULL);
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
