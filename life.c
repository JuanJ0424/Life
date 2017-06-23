#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include <cairo.h>
#include <sodium.h>
#include <limits.h>
#include <inttypes.h>

#define C_SIZE 6

struct gol_req{
    GtkWidget *drawing_area;
    GtkScale *density_scale;
    GtkScale *mem_size_scale;
    GtkScale *delay_scale;
    GtkEntry *bentry;
    GtkEntry *sentry;
    GtkComboBoxText *mem_rule_combo;
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
    unsigned int *id_timeout;
    char *f_e; //know if the rule has been reset or not
    char *t_a; //know if the timeout is already set
    char *rand;
    char *reset;
    short int *delay;
    
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

static void close_window (GtkApplication *app) {
    if (surface)
        cairo_surface_destroy (surface);
    g_application_quit(G_APPLICATION(app));
    
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

void allocate_grid (struct gol_req *info) {
    info->grid = malloc((*info->r)*sizeof(short int*));
    info->buffer = malloc((*info->r)*sizeof(short int*));
    int i, j;
    for (i = 0; i < *info->r; i++) {
        if(NULL ==(info->grid[i] = malloc(*(info->c)*sizeof(short int))))
            printf("Ni el grid se pudo hacer\n");
        
        if(NULL ==(info->buffer[i] = malloc(*(info->c)*sizeof(short int))))
            printf("Ni el buffer se pudo hacer\n");
    }
}

void allocate_life_memory(struct gol_req *info) {
    int i,j;
    info->memory = malloc((*info->mem_size)*sizeof(short int**));
    for(i = 0; i<(*info->mem_size); i++) {
        info->memory[i] = malloc((*info->r)*sizeof(short int*));
        for (j=0; j<*info->r; j++) {
            if (NULL==(info->memory[i][j] = malloc((*info->c)*sizeof(short int))))
                printf("No se le asigno memoria a una fila de la memoria %d \n", i);
        }
    }
}

void free_life_memory(struct gol_req *info) {
//    int i, j, k;
//    for(i=0; i<(*info->mem_size); i++) {
//        for(j=0; j<(*info->r); j++) {
//            for(k=0; k<(*info->c); k++) {
//             //   free(info->memory[i][j][k]);
//            }
//            free(info->memory[i][j]);
//        }
//        free(info->memory[i]);
//    }
    free(info->memory);
}

void random_initial_grid (struct gol_req * info) {
    int i,j;
    for (i = 0; i < *info->r; i++) {
        for (j = 0; j < *info->c; j++) {
            if (randombytes_uniform(101)<(100-*info->initial_density)){
                info->grid[i][j] = 1;
            } else {
                info->grid[i][j] = 0;
            }
        }
    }
}

void fill_memory(struct gol_req *info) {
    int i,j;
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
//    short int **aux = info->buffer;
//    for(i = 0; i<(*info->mem_size); i++) {
//        info->buffer = info->memory[i];
//        game_of_life(info);
//    }
//    info->buffer = aux;
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

static void start_game_of_life(GtkButton *button,
                               gpointer   data){
    struct gol_req *info = (struct gol_req *)data;

    if (*info->f_e == 0x01) { // if its the first execution of the current rule configuration
        printf("------------------\n");
        *info->f_e = 0x00; // now it won't be the first execution
        *info->reset = 0x01; // The memory has to be reset if the reset button is pressed
        *info->t_a = 0x01; // The timeout (periodic call) to game_of_life_with_memory is going to be active now
        // Retrieve values from GUI
        *info->initial_density = (short int)gtk_range_get_value(GTK_RANGE(info->density_scale));
        *info->mem_size = (short int) gtk_range_get_value(GTK_RANGE(info->mem_size_scale));
        info->b = gtk_entry_get_text(info->bentry);
        info->s = gtk_entry_get_text(info->sentry);
        *info->delay = (short int) gtk_range_get_value(GTK_RANGE(info->delay_scale));
        
        if (!strcmp("Minoria", gtk_combo_box_text_get_active_text(info->mem_rule_combo))) {
            printf("d minoria\n");
            *info->mem_rule = -1;
        } else if (!strcmp("Paridad", gtk_combo_box_text_get_active_text(info->mem_rule_combo))) {
            printf("d paridad\n");
            *info->mem_rule = 0;
        } else if (!strcmp("Mayoria", gtk_combo_box_text_get_active_text(info->mem_rule_combo))) {
            printf("d mayoria\n");
            *info->mem_rule = 1;
        } else {
            printf("otro %s",gtk_combo_box_text_get_active_text(info->mem_rule_combo) );
            *info->mem_rule = 2;
        }
        // Logging to know wheter the retrieved values are ok
        printf("b: %s\n", info->b);
        printf("s: %s\n", info->s);
        printf("Tamanio memoria: %d\n", *info->mem_size);
        printf("Densidad: %d\n", *info->initial_density);
        printf("Delay: %d\n", *info->delay);
        
        allocate_life_memory(info);
        if (*info->rand == 0x01) {
         random_initial_grid(info);
        }
        fill_memory(info);
        
        //-------------------- SET PERIODIC EXECUTION OF GAME OF LIFE --------------------
        // g_time_out makes a periodic call to a specified function
        info->id_timeout =  g_timeout_add_full (G_PRIORITY_HIGH, (guint)*info->delay, (GSourceFunc)game_of_life_with_memory, (gpointer)info, NULL);
    } else {
        if(*info->t_a == 0x00) { // Just add the timeout if is not already set
            *info->delay = (short int) gtk_range_get_value(GTK_RANGE(info->delay_scale));
            info->id_timeout =  g_timeout_add_full (G_PRIORITY_HIGH, (guint)*info->delay, (GSourceFunc)game_of_life_with_memory, (gpointer)info, NULL);
            *info->t_a = 0x01; // The timeout has been set
        }
    }
    
}
static void stop_game_of_life(GtkButton *button,
                               gpointer   data){
    struct gol_req *info = (struct gol_req *)data;
    if(*info->t_a == 0x01) { // Just try to remove the timeout if it is active
        g_source_remove(info->id_timeout);
        *info->t_a = 0x00; // The timeout is not active anymore
    }
    
}
static void reset_game_of_life(GtkButton *button,
                              gpointer   data){
    struct gol_req *info = (struct gol_req *)data;
    if(*info->t_a == 0x01) { // Just try to remove the timeout if it is active
        g_source_remove(info->id_timeout);
        *info->t_a = 0x00; // The timeout is not active anymore
    }
    if(*info->reset == 0x01) { // Check if the memory has not been reset yet
        *info->f_e = 0x01; // It will be the first execution of the new configuration
        free_life_memory(info); // Free all the gol memory, because a new mem size could've been selected
        *info->reset = 0x00; // Mark that the memory reset is already done
    }
    paint_grid_lines(info); // Paint an empty grid
    gtk_widget_queue_draw (info->drawing_area); // Request render

}

static void
draw_brush (GtkWidget *widget,
            gdouble    x,
            gdouble    y, struct gol_req *info)
{
    int xreal, yreal;
    cairo_t *cr;
    if (*info->t_a==0x00) {
        cr = cairo_create (surface);
        xreal = (int)x;
        yreal = (int)y;
        xreal = xreal/C_SIZE;
        yreal = yreal/C_SIZE;
        info->grid[x][y] = 1;
        *info->rand = 0x00;
        cairo_rectangle (cr, C_SIZE*xreal, C_SIZE*yreal, C_SIZE, C_SIZE);
        cairo_fill (cr);
        
        cairo_destroy (cr);
        
        /* Now invalidate the affected region of the drawing area. */
        gtk_widget_queue_draw_area (widget, C_SIZE*x, C_SIZE*y, C_SIZE, C_SIZE);
    }
}

static gboolean
button_press_event_cb (GtkWidget      *widget,
                       GdkEventButton *event,
                       gpointer        data)
{
    /* paranoia check, in case we haven't gotten a configure event */
    if (surface == NULL)
        return FALSE;
    struct gol_req *info = (struct gol_req *)data;
    if (event->button == GDK_BUTTON_PRIMARY)
    {
        draw_brush (widget, event->x, event->y, info);
    }
    else if (event->button == GDK_BUTTON_SECONDARY)
    {
        clear_surface ();
        gtk_widget_queue_draw (widget);
    }
    
    /* We've handled the event, stop processing */
    return TRUE;
}

static gboolean
motion_notify_event_cb (GtkWidget      *widget,
                        GdkEventMotion *event,
                        gpointer        data)
{
    /* paranoia check, in case we haven't gotten a configure event */
    if (surface == NULL)
        return FALSE;
    struct gol_req *info = (struct gol_req *)data;
    if (event->state & GDK_BUTTON1_MASK)
        draw_brush (widget, event->x, event->y, info);
    
    /* We've handled it, stop processing */
    return TRUE;
}

static void
activate (GtkApplication *app,
          gpointer        user_data)
{
    GtkWidget *window;
    GtkWidget *frame;
    GtkWidget *drawing_area;
    GtkWidget *pwindow;
    GtkWidget *pframe;
    
    
    
    window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "Game of Life");
    gtk_window_set_resizable(window, FALSE);
    
    pwindow = gtk_application_window_new(app);
    gtk_window_set_resizable(pwindow, FALSE);
    gtk_window_set_title (GTK_WINDOW (pwindow), "Life Control Panel");
    
    
    g_signal_connect (app, "window_removed", G_CALLBACK (close_window), app);
    
//    printf("tipo 1 %s", G_OBJECT_TYPE_NAME(G_APPLICATION(app)));
    
    gtk_container_set_border_width (GTK_CONTAINER (window), 8);
    gtk_container_set_border_width (GTK_CONTAINER (pwindow), 4);
    
    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
    gtk_container_add (GTK_CONTAINER (window), frame);
    
    pframe = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (pframe), GTK_SHADOW_IN);
    gtk_container_add (GTK_CONTAINER (pwindow), pframe);
    
    GtkGrid *pgrid = gtk_grid_new();
    gtk_container_add(pframe, pgrid);
    
    GtkLabel *rule_label = gtk_label_new("Configuración de la Regla");
    gtk_label_set_justify(rule_label, GTK_JUSTIFY_FILL);
    gtk_grid_attach(GTK_GRID(pgrid), rule_label, 0, 0, 5, 1);
    
    GtkLabel *blabel = gtk_label_new("B");
    gtk_grid_attach(GTK_GRID(pgrid), blabel, 0, 1, 2, 1);
    gtk_label_set_justify(blabel, GTK_JUSTIFY_CENTER);
    GtkEntry *bentry = gtk_entry_new_with_buffer(gtk_entry_buffer_new("3", 1));
    gtk_grid_attach(pgrid, bentry, 2, 1, 2, 1);

    GtkLabel *slabel = gtk_label_new("S");
    gtk_grid_attach(GTK_GRID(pgrid), slabel, 0, 2, 2, 1);
    gtk_label_set_justify(slabel, GTK_JUSTIFY_CENTER);
    GtkEntry *sentry = gtk_entry_new_with_buffer(gtk_entry_buffer_new("23", 2));
    gtk_grid_attach(pgrid, sentry, 2, 2, 2, 1);
    
    GtkLabel *density_label = gtk_label_new("Densidad Inicial");
    gtk_grid_attach(GTK_GRID(pgrid), density_label, 0, 3, 2, 1);
    gtk_label_set_justify(density_label, GTK_JUSTIFY_CENTER);
    GtkScale *density_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 100, 1);
    gtk_range_set_value(GTK_RANGE(density_scale), 50);
    gtk_grid_attach(pgrid, density_scale, 2, 3, 2, 1);
    
    GtkLabel *mem_size_label = gtk_label_new("Tamaño memoria");
    gtk_grid_attach(GTK_GRID(pgrid), mem_size_label, 0, 4, 2, 1);
    gtk_label_set_justify(mem_size_label, GTK_JUSTIFY_CENTER);
    GtkScale *mem_size_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 1, 25, 1);
    gtk_grid_attach(pgrid, mem_size_scale, 2, 4, 2, 1);
    
    GtkLabel *mem_rule_label = gtk_label_new("Regla memoria");
    gtk_grid_attach(GTK_GRID(pgrid), mem_rule_label, 0, 5, 2, 1);
    gtk_label_set_justify(mem_rule_label, GTK_JUSTIFY_CENTER);
    GtkComboBoxText *mem_rule_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append(mem_rule_combo, NULL, "Minoria");
    gtk_combo_box_text_append(mem_rule_combo, NULL, "Paridad");
    gtk_combo_box_text_append(mem_rule_combo, NULL, "Mayoria");
    gtk_combo_box_text_append(mem_rule_combo, NULL, "Igualdad");
    gtk_combo_box_set_active(mem_rule_combo, 0);
    gtk_grid_attach(GTK_GRID(pgrid), mem_rule_combo, 2, 5, 2, 1);
    
    GtkLabel *delay_label = gtk_label_new("Delay");
    gtk_grid_attach(GTK_GRID(pgrid), delay_label, 0, 6, 2, 1);
    gtk_label_set_justify(delay_label, GTK_JUSTIFY_CENTER);
    GtkScale *delay_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 25, 200, 1);
    gtk_grid_attach(pgrid, delay_scale, 2, 6, 2, 1);
    
    GtkButton *play = gtk_button_new_with_label("Evaluar");
    GtkButton *stop = gtk_button_new_with_label("Detener");
    GtkButton *reset = gtk_button_new_with_label("Reiniciar");
    gtk_grid_attach(GTK_GRID(pgrid), gtk_separator_new(GTK_ORIENTATION_HORIZONTAL), 0, 7, 5, 2);
    gtk_grid_attach(GTK_GRID(pgrid), play, 1, 8, 1, 1);
    gtk_grid_attach(GTK_GRID(pgrid), stop, 3, 8, 1, 1);
    gtk_grid_attach(GTK_GRID(pgrid), reset,0, 9, 5, 1);

    
    drawing_area = gtk_drawing_area_new ();
    /* set a minimum size */
    gtk_widget_set_size_request (drawing_area, 1300, 840);
    
    gtk_widget_set_size_request (pframe, 200, 200);
    
    gtk_container_add (GTK_CONTAINER (frame), drawing_area);
    
    /* Signals used to handle the backing surface */
    g_signal_connect (drawing_area, "draw",
                      G_CALLBACK (draw_cb), NULL);
    g_signal_connect (drawing_area,"configure-event",
                      G_CALLBACK (configure_event_cb), NULL);
    
    
    /* Ask to receive events the drawing area doesn't normally
     * subscribe to. In particular, we need to ask for the
     * button press and motion notify events that want to handle.
     */
    
    gtk_widget_set_events (drawing_area, gtk_widget_get_events (drawing_area)
                           | GDK_BUTTON_PRESS_MASK
                           | GDK_POINTER_MOTION_MASK);
    gtk_widget_show_all (window);
    gtk_widget_show_all(pwindow);
    printf("tipo 2 %s\n", G_OBJECT_TYPE_NAME(G_APPLICATION(app)));
    
    //-------------------- SET REQUIREMENTS --------------------
    struct gol_req *info = malloc(sizeof(struct gol_req));
    // SIZE OF GRID
    info->r = malloc(sizeof(short int));
    info->c = malloc(sizeof(short int));
    *info->r = 140;
    *info->c = 217;
    
    info->initial_density = malloc(sizeof(short int));

    info->mem_rule = malloc(sizeof(short int));
    info->mem_size = malloc(sizeof(short int));
    
    // printf("rule b[0] %d s[0] %d\n",b[0]-'0', s[0]-'0');
    
    info->drawing_area = drawing_area;
    info->density_scale = density_scale;
    info->mem_size_scale = mem_size_scale;
    info->bentry = bentry;
    info->sentry = sentry;
    info->mem_rule_combo = mem_rule_combo;
    info->id_timeout = malloc(sizeof(unsigned int));
    info->f_e = malloc(sizeof(char));
    *info->f_e = 0x01;
    info->delay_scale = delay_scale;
    info->delay = malloc(sizeof(short int));
    info->reset = malloc(sizeof(char));
    *info->reset = 0x00;
    info->t_a = malloc(sizeof(char));
    *info->t_a = 0x00;
    *info->rand = 0x01;
    
    allocate_grid(info);
    paint_grid_lines (info);
    
    printf("info %p\n",info);
    g_signal_connect (G_OBJECT(play), "clicked", G_CALLBACK (start_game_of_life), (gpointer)&(*info));
    g_signal_connect (G_OBJECT(stop), "clicked", G_CALLBACK (stop_game_of_life), (gpointer)&(*info));
    g_signal_connect (G_OBJECT(reset), "clicked", G_CALLBACK (reset_game_of_life), (gpointer)&(*info));
    g_signal_connect (drawing_area, "motion-notify-event",G_CALLBACK (motion_notify_event_cb), (gpointer)&(*info));
    g_signal_connect (drawing_area, "button-press-event",G_CALLBACK (button_press_event_cb), (gpointer)&(*info));
}

int main (int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new ("juan.life", G_APPLICATION_FLAGS_NONE);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);
    return status;
}
