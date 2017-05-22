#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <sodium.h>


//gcc `pkg-config --cflags gtk+-3.0 libsodium` -o life life.c `pkg-config --libs gtk+-3.0 libsodium`

/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;

static short int grid[136][136];
static short int buffer[136][136];

void init_grid() {
  int i, j;
  for (i = 0; i < 134; i++) {
    for (j = 0; j < 134; j++) {
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

static int currently_drawing = 0;

void *game_of_life(void *ptr) {
  currently_drawing = 1;
  int i,j;
  double x1, y1, x2, y2; // Just to store the size of surface

  printf("Ejecutamos game_of_life\n" );
  cairo_surface_t *buffer;
  //Buffer surface stuff
  cairo_t *cr;
  cr = cairo_create(surface); //  Copy current state of surface in to another surface
  cairo_clip_extents(cr, &x1, &y1, &x2, &y2); // Get size of surface
  cairo_destroy(cr);
  buffer = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR_ALPHA, x2, y2); // Create our buffer surface
  cr = cairo_create (buffer);
  short int count;
  for (i = 1; i < 135; i++) {
    for (j = 1; j < 135; j++) {
      //printf("%d", grid[i][j]);
      count = grid[i-1][j-1];
      count += grid[i-1][j];
      count += grid[i-1][j+1];
      count += grid[i][j-1];
      // Skip current cell
      count += grid[i][j+1];
      count += grid[i+1][j-1];
      count += grid[i+1][j];
      count += grid[i+1][j+1];
      if (grid[i][j] == 1) {
        if (count == 2 || count == 3) {
          buffer[i][j] = 1;
          cairo_set_source_rgb(cr, 0, 0, 0);
        } else {
          buffer[i][j] = 0;
          cairo_set_source_rgb(cr, 1, 1, 1);
        }
      } else if (grid[i][j] == 0) {
        if (count == 3) {
          buffer[i][j] = 1;
          cairo_set_source_rgb(cr, 0, 0, 0);
        } else {
          buffer[i][j] = 0;
          cairo_set_source_rgb(cr, 1, 1, 1);
        }
      }
      cairo_rectangle (cr, (i*6), (j*6), 6, 6);
      cairo_fill (cr);
    }
  }
  for ( i = 1; i < 135; i++) {
    for ( j = 1; j < 135; j++) {
      grid[i][j] = buffer[i][j];
    }
  }

  // gtk_widget_queue_draw (da);
  cairo_destroy (cr);
  // gdk_threads_leave();
  cairo_surface_destroy(buffer);
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
  gtk_widget_set_size_request (drawing_area, 800, 800);

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
  init_grid ();
  gdk_threads_add_timeout (100, game_of_life, drawing_area);
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
