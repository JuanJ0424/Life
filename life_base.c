#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>

//gcc `pkg-config --cflags gtk+-3.0` -o life life.c `pkg-config --libs gtk+-3.0`

/* Surface to store current scribbles */
static cairo_surface_t *surface = NULL;

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
    cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
    cairo_destroy(cr);
    clone = 1;
    past = cairo_surface_create_similar(surface, CAIRO_CONTENT_COLOR_ALPHA, x2, y2);
    cr = cairo_create(past);
    cairo_set_source_surface(cr,surface, 0, 0);
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

/* Draw a rectangle on the surface at the given position */
// static void
// draw_brush (GtkWidget *widget,
//             gdouble    x,
//             gdouble    y)
// {
//   cairo_t *cr;
//
//   /* Paint to the surface, where we store our state */
//   cr = cairo_create (surface);
//
//   cairo_rectangle (cr, x - 3, y - 3, 6, 6);
//   cairo_fill (cr);
//
//   cairo_destroy (cr);
//
//   /* Now invalidate the affected region of the drawing area. */
//   gtk_widget_queue_draw_area (widget, x - 3, y - 3, 6, 6);
// }

/* Handle button press events by either drawing a rectangle
 * or clearing the surface, depending on which button was pressed.
 * The ::button-press signal handler receives a GdkEventButton
 * struct which contains this information.
 */
// static gboolean
// button_press_event_cb (GtkWidget      *widget,
//                        GdkEventButton *event,
//                        gpointer        data)
// {
//   /* paranoia check, in case we haven't gotten a configure event */
//   if (surface == NULL)
//     return FALSE;
//
//   if (event->button == GDK_BUTTON_PRIMARY)
//     {
//       draw_brush (widget, event->x, event->y);
//     }
//   else if (event->button == GDK_BUTTON_SECONDARY)
//     {
//       clear_surface ();
//       gtk_widget_queue_draw (widget);
//     }
//
//   /* We've handled the event, stop processing */
//   return TRUE;
// }

/* Handle motion events by continuing to draw if button 1 is
 * still held down. The ::motion-notify signal handler receives
 * a GdkEventMotion struct which contains this information.
 */
// static gboolean
// motion_notify_event_cb (GtkWidget      *widget,
//                         GdkEventMotion *event,
//                         gpointer        data)
// {
//   /* paranoia check, in case we haven't gotten a configure event */
//   if (surface == NULL)
//     return FALSE;
//
//   if (event->state & GDK_BUTTON1_MASK)
//     draw_brush (widget, event->x, event->y);
//
//   /* We've handled it, stop processing */
//   return TRUE;
// }

static void close_window (void) {
  if (surface)
    cairo_surface_destroy (surface);

  gtk_main_quit ();
}

gboolean game_of_life(GtkWidget *da) {
  /* Static int keeps its value between invocations,
  so the value asigned behind it's actually
  just its initial value*/
  static int i = 0;

  printf("Se ejecuta game_of_life");
  cairo_t *cr;
  /* Paint to the surface, where we store our state */
  cr = cairo_create (surface);
  if (i == 20)
    return FALSE;
  cairo_rectangle (cr, (i*6)+3, (i*6)+3, 6, 6);
  cairo_fill (cr);
  gtk_widget_queue_draw (da);
  cairo_destroy (cr);
  i += 1;
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
  g_timeout_add (100, (GSourceFunc)game_of_life, drawing_area);
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
