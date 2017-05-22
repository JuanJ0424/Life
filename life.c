#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <sodium.h>

struct gol_req{
  GtkWidget *drawing_area;
  cairo_surface_t *lines;
};

//gcc `pkg-config --cflags gtk+-3.0 libsodium` -o life life.c `pkg-config --libs gtk+-3.0 libsodium`

/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;
// SURFACE TO STORE THE GRID BORDERS
static cairo_surface_t *lines = NULL;

static short int grid[140][217];
static short int buffer[140][217];
static int r,c;

void init_grid(GtkWidget *da) {
  r = (sizeof(grid)/sizeof(grid[0]))-1;
  c = (sizeof(grid[0])/sizeof(grid[0][0])-1);
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
  for (i = 1; i < r+1; i++) {
    cairo_move_to(cr,0,(i*6));
    cairo_line_to(cr,1302,(i*6));
  }
  for (j = 1; j < c+1; j++) {
    cairo_move_to(cr,(j*6),0);
    cairo_line_to(cr,(j*6),840);
  }
  cairo_stroke(cr);
  cairo_destroy(cr);
  cairo_create(surface);
  cairo_set_source_surface (cr,lines, 0, 0);
  cairo_paint(cr);
  for (i = 0; i < r+1; i++) {
    for (j = 0; j < c+1; j++) {
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

short int do_count(short int i, short int j){
  short int count = 0;
  count = grid[(i-1)%r][(j-1)%c] + grid[(i-1)%r][j] + grid[(i-1)%r][(j+1)%c];
  count += grid[][] + grid[][];
  count += grid[][] + grid[][] + grid [][];

  if (i !=0 && j != 0){ // Center cells
    count = grid[i-1][j-1] + grid[i-1][j] + grid[i-1][j+1];
    count += grid[i][j-1] + grid[i][j+1];
    count += grid[i+1][j-1] + grid[i+1][j] + grid[i+1][j+1];
  } else if ((i == 0||i == r) && !(j==0||j==c)) {
    //printf("Top cell %d %d\n",i,j);
    if (i == 0) { // Top cells
      count = grid[r][j-1] + grid[r][j] + grid[r][j+1];
      count += grid[0][j-1] + grid[0][j+1];
      count += grid[1][j-1] + grid [1][j] + grid[1][j+1];
    } else { //(i==r)  Bottom cells
      //printf("Bottom cell %d %d\n",i,j);
      count = grid[r-1][j-1] + grid[r-1][j] + grid[r-1][j+1];
      count += grid[r][j-1] + grid[r][j+1];
      count += grid[0][j-1] + grid [0][j] + grid[0][j+1];
    }
  }else if ((j == 0||j==c) && !(i==0||i==r)) {
    //printf("FC cell %d %d\n",i,j);
    if (j == 0){  // First column cells
      count = grid[i-1][c] + grid[i-1][0] + grid[i-1][1];
      count += grid[i][c] + grid[i][1];
      count += grid[i+1][c] + grid [i+1][0] + grid[i+1][1];
    } else { // (j == c) Last column cells
      count = grid[i-1][c-1] + grid[i-1][c] + grid[i-1][0];
      count += grid[i][c-1] + grid[i][0];
      count += grid[i+1][c-1] + grid [i+1][c] + grid[i+1][0];
    }
  } else if(i == 0 && j == 0){ // Top left cell
    count = grid[r][c] + grid[r][0] + grid[r][1];
    count += grid[0][c] + grid[0][1];
    count += grid[1][c] + grid [1][0] + grid[1][1];
  } else if(i== 0 && j == c){ // Top right cell
    count = grid[r][c-1] + grid[r][c] + grid[r][0];
    count += grid[0][c-1] + grid[0][0];
    count += grid[1][c-1] + grid [1][c] + grid[1][0];
  } else if (i == r && j== 0) { // Botton left
    count = grid[r-1][c] + grid[r-1][0] + grid[r-1][1];
    count += grid[r][c] + grid[1][1];
    count += grid[0][c] + grid [0][0] + grid[0][1];
  } else if (i == r && j == c) { //Bottom right
    count = grid[r-1][c-1] + grid[r-1][c] + grid[r-1][0];
    count += grid[r][c-1] + grid[r][0];
    count += grid[0][c-1] + grid [0][r] + grid[0][0];
  } else if (i == r) { // Bottom cells
    //printf("Bottom cell %d %d\n",i,j);
    count = grid[r-1][j-1] + grid[r-1][j] + grid[r-1][j+1];
    count += grid[r][j-1] + grid[r][j+1];
    count += grid[0][j-1] + grid [0][j] + grid[0][j+1];
  }

  return count;
}

gboolean game_of_life(GtkWidget *da) {
  // struct gol_req info = *((struct gol_req *)data);
  short int i,j;
  cairo_t *cr;
  short int count;
  short int color;
  cr = cairo_create (surface);
  cairo_set_source_surface(cr, lines, 0, 0);
  cairo_paint(cr);
  cairo_destroy (cr);
  cr = cairo_create (surface);
  cairo_set_source_rgb(cr, 0, 0, 0);
  for (i = 0; i < r+1  ; i++) {
    for (j = 0; j < c+1 ; j++) {
      count = do_count(i,j);
      if (grid[i][j] == 1) {
        if (count == 2 || count == 3) {
          buffer[i][j] = 1;
          color = 1;
        } else {
          buffer[i][j] = 0;
          color = 0;
        }
      } else if (grid[i][j] == 0) {
        if (count == 3) {
          buffer[i][j] = 1;
          color = 1;
        } else {
          buffer[i][j] = 0;
          color = 0;
        }
      }
      if (color == 1) {
        cairo_rectangle (cr, 6*j, 6*i, 6, 6);
      }
    }
  }
  cairo_fill (cr);
  gtk_widget_queue_draw (da);
  for ( i = 0; i < r+1; i++) {
    for ( j = 0; j < c+1; j++) {
      grid[i][j] = buffer[i][j];
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
  printf("Mostre todo\n");
  //-------------------- SET PERIODIC EXECUTION OF GAME OF LIFE --------------------
  // g_time_out makes a periodic call to a specified function
  init_grid (drawing_area);
  struct gol_req info;
  // info.drawing_area = drawing_area;
  // info.lines = lines;
  g_timeout_add_full (G_PRIORITY_HIGH, (guint)26, (GSourceFunc)game_of_life, drawing_area, NULL);
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
