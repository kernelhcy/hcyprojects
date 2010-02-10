/* 俄罗斯方块*/
/* Pentris - five-square Tetris */

/* Written by David A. Madore <david.madore@ens.fr> */
/* Initially written: 2005-01-18 */
/* This version: 2005-01-18 */

/* This program is in the Public Domain.  However, I would appreciate
 * being credited if significant parts of it are being used
 * somewhere. */
/* This program is provided without any warranty of any kind. */

/* Compile with something like this:
    gcc -Wall -O6 -std=c99 -pedantic -W -Wwrite-strings -Wcast-qual -Wpointer-arith -Wstrict-prototypes -Wno-unused pentris.c -o pentris `pkg-config --cflags gtk+-2.0` `pkg-config --libs gtk+-2.0`
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

/* === Various game parameters that can be changed at compile time === */

/* Pit area size */
#ifndef NBLINES
#define NBLINES 20
#endif
#ifndef NBCOLUMNS
#define NBCOLUMNS 10
#endif

/* Whether to display the next piece that will fall (note that this
 * means the evaluation functions must be suitably modified!) */
#ifndef ADVERTIZE_NEXT
#define ADVERTIZE_NEXT 1
#endif

/* Time in millisecond that a piece takes to fall by one line */
#ifndef FALL_INTERVAL
#define FALL_INTERVAL 500
#endif

/* Square size in pixels */
#ifndef SQUAREPIXELS_X
#define SQUAREPIXELS_X 16
#endif
#ifndef SQUAREPIXELS_Y
#define SQUAREPIXELS_Y 16
#endif

/* === No user-serviceable parts below :-) === */

/* A Tetris board (lines are numbered from BOTTOM UP) */
typedef char board_t[NBLINES][NBCOLUMNS];

/* Now define all the shapes */
#define SHAPELINES 5
#define SHAPECOLUMNS 5
#define SHAPEORIENTS 4
#define NBSHAPES 18

struct shape_s {
  int useful_orients;
  char nickname;
  char s[SHAPEORIENTS][SHAPELINES][SHAPECOLUMNS];
} shapes[NBSHAPES] = {
  { 4, 'P', {{{0,0,0,0,0},{0,0,1,0,0},{0,0,1,1,0},{0,0,1,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,0,0,0},{0,1,1,1,0},{0,1,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,1,0,0},{0,1,1,0,0},{0,0,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,1,0},{0,1,1,1,0},{0,0,0,0,0},{0,0,0,0,0}}} },
  { 4, 'Q', {{{0,0,0,0,0},{0,0,1,0,0},{0,1,1,0,0},{0,1,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,1,0,0},{0,1,1,1,0},{0,0,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,1,0},{0,0,1,1,0},{0,0,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,0,0,0},{0,1,1,1,0},{0,0,1,1,0},{0,0,0,0,0}}} },
  { 2, 'I', {{{0,0,0,0,0},{0,0,0,0,0},{1,1,1,1,1},{0,0,0,0,0},{0,0,0,0,0}},
             {{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0}},
             {{0,0,0,0,0},{0,0,0,0,0},{1,1,1,1,1},{0,0,0,0,0},{0,0,0,0,0}},
             {{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0}}} },
  { 1, 'X', {{{0,0,0,0,0},{0,0,1,0,0},{0,1,1,1,0},{0,0,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,0,0},{0,1,1,1,0},{0,0,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,0,0},{0,1,1,1,0},{0,0,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,0,0},{0,1,1,1,0},{0,0,1,0,0},{0,0,0,0,0}}} },
  { 4, 'C', {{{0,0,0,0,0},{0,0,0,0,0},{0,1,0,1,0},{0,1,1,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,1,0,0},{0,1,0,0,0},{0,1,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,1,1,0},{0,1,0,1,0},{0,0,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,1,0},{0,0,0,1,0},{0,0,1,1,0},{0,0,0,0,0}}} },
  { 4, 'W', {{{0,0,0,0,0},{0,1,1,0,0},{0,0,1,1,0},{0,0,0,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,0,1,0},{0,0,1,1,0},{0,1,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,0,0,0},{0,1,1,0,0},{0,0,1,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,1,0},{0,1,1,0,0},{0,1,0,0,0},{0,0,0,0,0}}} },
  { 4, 'T', {{{0,0,0,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,1,1,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,0,0,0},{0,1,1,1,0},{0,1,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,1,1,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,0,1,0},{0,1,1,1,0},{0,0,0,1,0},{0,0,0,0,0}}} },
  { 2, 'S', {{{0,0,0,0,0},{0,0,0,1,0},{0,1,1,1,0},{0,1,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,1,0,0},{0,0,1,0,0},{0,0,1,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,0,1,0},{0,1,1,1,0},{0,1,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,1,0,0},{0,0,1,0,0},{0,0,1,1,0},{0,0,0,0,0}}} },
  { 2, 'Z', {{{0,0,0,0,0},{0,1,0,0,0},{0,1,1,1,0},{0,0,0,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,1,0},{0,0,1,0,0},{0,1,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,0,0,0},{0,1,1,1,0},{0,0,0,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,1,0},{0,0,1,0,0},{0,1,1,0,0},{0,0,0,0,0}}} },
  { 4, 'V', {{{0,0,0,0,0},{0,1,0,0,0},{0,1,0,0,0},{0,1,1,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,1,1,0},{0,1,0,0,0},{0,1,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,1,1,0},{0,0,0,1,0},{0,0,0,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,0,1,0},{0,0,0,1,0},{0,1,1,1,0},{0,0,0,0,0}}} },
  { 4, 'L', {{{0,0,0,0,0},{0,1,0,0,0},{0,1,1,1,1},{0,0,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,1,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0}},
             {{0,0,0,0,0},{0,0,0,0,0},{1,1,1,1,0},{0,0,0,1,0},{0,0,0,0,0}},
             {{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,1,1,0,0},{0,0,0,0,0}}} },
  { 4, 'l', {{{0,0,0,0,0},{0,0,1,0,0},{0,1,1,1,1},{0,0,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,0,0},{0,0,1,1,0},{0,0,1,0,0},{0,0,1,0,0}},
             {{0,0,0,0,0},{0,0,0,0,0},{1,1,1,1,0},{0,0,1,0,0},{0,0,0,0,0}},
             {{0,0,1,0,0},{0,0,1,0,0},{0,1,1,0,0},{0,0,1,0,0},{0,0,0,0,0}}} },
  { 4, 'J', {{{0,0,0,0,0},{0,0,0,1,0},{1,1,1,1,0},{0,0,0,0,0},{0,0,0,0,0}},
             {{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,0,0,0},{0,1,1,1,1},{0,1,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,1,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,0,1,0,0}}} },
  { 4, 'j', {{{0,0,0,0,0},{0,0,1,0,0},{1,1,1,1,0},{0,0,0,0,0},{0,0,0,0,0}},
             {{0,0,1,0,0},{0,0,1,0,0},{0,0,1,1,0},{0,0,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,0,0,0},{0,1,1,1,1},{0,0,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,0,0},{0,1,1,0,0},{0,0,1,0,0},{0,0,1,0,0}}} },
  { 4, 'K', {{{0,0,0,0,0},{0,0,1,0,0},{0,1,1,1,0},{0,1,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,1,0,0},{0,0,1,1,0},{0,0,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,0,1,0},{0,1,1,1,0},{0,0,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,0,0},{0,1,1,0,0},{0,0,1,1,0},{0,0,0,0,0}}} },
  { 4, 'k', {{{0,0,0,0,0},{0,0,1,0,0},{0,1,1,1,0},{0,0,0,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,0,0},{0,0,1,1,0},{0,1,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,0,0,0},{0,1,1,1,0},{0,0,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,1,0},{0,1,1,0,0},{0,0,1,0,0},{0,0,0,0,0}}} },
#if 1
  { 4, 's', {{{0,0,0,0,0},{0,1,1,0,0},{0,0,1,1,1},{0,0,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,0,1,0},{0,0,1,1,0},{0,0,1,0,0},{0,0,1,0,0}},
             {{0,0,0,0,0},{0,0,0,0,0},{1,1,1,0,0},{0,0,1,1,0},{0,0,0,0,0}},
             {{0,0,1,0,0},{0,0,1,0,0},{0,1,1,0,0},{0,1,0,0,0},{0,0,0,0,0}}} },
  { 4, 'z', {{{0,0,0,0,0},{0,0,1,1,0},{1,1,1,0,0},{0,0,0,0,0},{0,0,0,0,0}},
             {{0,0,1,0,0},{0,0,1,0,0},{0,0,1,1,0},{0,0,0,1,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,0,0,0},{0,0,1,1,1},{0,1,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,0,0,0},{0,1,1,0,0},{0,0,1,0,0},{0,0,1,0,0}}} },
#else
  { 4, 's', {{{0,0,0,0,0},{0,1,1,0,0},{0,0,1,1,1},{0,0,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,0,0},{0,1,1,0,0},{0,1,0,0,0},{0,1,0,0,0}},
             {{0,0,0,0,0},{0,0,0,0,0},{0,1,1,1,0},{0,0,0,1,1},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,0,1,0,0},{0,0,1,0,0},{0,1,1,0,0},{0,1,0,0,0}}} },
  { 4, 'z', {{{0,0,0,0,0},{0,0,0,1,1},{0,1,1,1,0},{0,0,0,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,0,0,0},{0,1,0,0,0},{0,1,1,0,0},{0,0,1,0,0}},
             {{0,0,0,0,0},{0,0,0,0,0},{0,0,1,1,1},{0,1,1,0,0},{0,0,0,0,0}},
             {{0,0,0,0,0},{0,1,0,0,0},{0,1,1,0,0},{0,0,1,0,0},{0,0,1,0,0}}} },
#endif
};

/* Statistics on the shapes */
typedef int stats_t[NBSHAPES];

int
crunch_lines (board_t b)
     /* Remove full lines from board (editing it in place!), return
      * number of lines removed. */
{
  int i, i0, j;
  char fl;
  int crunched;

  crunched = 0;
  i0 = 0;
  for ( i=0 ; i<NBLINES ; i++ )
    {
      do
        {
          if ( i0 < NBLINES )
            {
              fl = 1;
              for ( j=0 ; j<NBCOLUMNS ; j++ )
                if ( ! b[i0][j] )
                  {
                    fl = 0;
                    break;
                  }
            }
          else
            fl = 0;
          if ( fl )
            {
              i0++;
              crunched++;
            }
        }
      while ( fl );
      if ( i0 >= NBLINES )
        memset (b[i], 0, sizeof(b[i]));
      else {
        if ( i0 > i )
          memcpy (b[i], b[i0], sizeof(b[i]));
        i0++;
      }
    }
  return crunched;
}

char
can_place (board_t b, int sh, int or, int ln, int cl)
     /* Decide whether the given shape can be inserted in the given
      * board at the given position.  Note that ln or cl might be
      * negative (e.g., if the shape has no square in the first line
      * or column). */
{
  int i, j;

  for ( i=0 ; i<SHAPELINES ; i++ )
    for ( j=0 ; j<SHAPECOLUMNS ; j++ )
      if ( shapes[sh].s[or][i][j] )
        if ( ln+i<0 || cl+j<0 || ln+i>=NBLINES || cl+j>=NBCOLUMNS
             || b[ln+i][cl+j] )
          return 0;
  return 1;
}

char
can_place_notop (board_t b, int sh, int or, int ln, int cl)
     /* Does almost the same thing as can_place(), except that the
      * well is considered infinite on top. */
{
  int i, j;

  for ( i=0 ; i<SHAPELINES ; i++ )
    for ( j=0 ; j<SHAPECOLUMNS ; j++ )
      if ( shapes[sh].s[or][i][j] )
        if ( ln+i<0 || cl+j<0 || cl+j>=NBCOLUMNS
             || ( ln+i<NBLINES && b[ln+i][cl+j] ) )
          return 0;
  return 1;
}

void
do_place (board_t b, int sh, int or, int ln, int cl)
     /* Insert the given shape in the given board at the given
      * position.  Essentially meaningful only after a call to
      * can_place(). */
{
  int i, j;

  for ( i=0 ; i<SHAPELINES ; i++ )
    for ( j=0 ; j<SHAPECOLUMNS ; j++ )
      if ( shapes[sh].s[or][i][j] )
        if ( ln+i>=0 && cl+j>=0 && ln+i<NBLINES && cl+j<NBCOLUMNS )
          b[ln+i][cl+j] = 1;
}

void
print_board (board_t b)
     /* Convenience function to print the current board. */
{
  int i, j;

  for ( i=NBLINES-1 ; i>=0 ; i-- )
    {
      printf ("|");
      for ( j=0 ; j<NBCOLUMNS ; j++ )
        printf ("%c", (b[i][j]?'*':' '));
      printf ("|");
      printf ("\n");
    }
  printf ("+");
  for ( j=0 ; j<NBCOLUMNS ; j++ )
    printf ("-");
  printf ("+");
  printf ("\n");
}

#define AREA_X (NBCOLUMNS*SQUAREPIXELS_X)
#define AREA_Y (NBLINES*SQUAREPIXELS_Y)
#if ADVERTIZE_NEXT
#define NEXT_X (SHAPECOLUMNS*SQUAREPIXELS_X)
#define NEXT_Y (SHAPELINES*SQUAREPIXELS_Y)
#endif

board_t main_board;                     /* The board on which we play */
stats_t shape_stats;                    /* Number of times was given */
int shape_falling;                      /* Falling shape */
int orient, line, column;               /* Position of falling shape */
#if ADVERTIZE_NEXT
int shape_next, orient_next;            /* Next shape */
#endif
int score;                              /* Player's score */
GRand *rnd;                             /* Random number generator */
GtkDrawingArea *game_area;              /* Game area drawable */
#if ADVERTIZE_NEXT
GtkDrawingArea *next_area;              /* Next shape area drawable */
#endif
char paused;                            /* Is the game paused? */
guint tick;                             /* Timer ID */

void
compute_shape_next (void)
     /* Essential function to compute the next shape to promise.  This
      * is very similar to my_eval (SEARCH_DEPTH, main_board,
      * shape_stats, shape_falling, INT_MAX).  Result is stored in
      * shape_next (or shape_falling, if we not ADVERTIZE_NEXT). */
{
  int sh;

  sh = g_rand_int_range (rnd, 0, NBSHAPES);
#if ! ADVERTIZE_NEXT
  orient = 0;
  shape_falling = sh;
#else
  orient_next = 0;
  shape_next = sh;
#endif
  shape_stats[sh]++;
}

char
finalize_shape (void)
     /* Add the fallen shape to the board (freeze it). */
{
  if ( can_place (main_board, shape_falling, orient, line, column) )
    {
      do_place (main_board, shape_falling, orient, line, column);
      score += crunch_lines (main_board);
      printf ("score: %d\n", score);
      return 1;
    }
  else
    {
      /* can_place() is false, whereas can_place_notop() is true =>
       * shape overflows the well. */
      int sh;

      printf ("you die!\n");
      printf ("Shape statistics:");
      for ( sh=0 ; sh<NBSHAPES ; sh++ )
        printf ("\t%c: %d", shapes[sh].nickname, shape_stats[sh]);
      printf ("\n");
      return 0;
    }
}

void
initiate_shape (void)
     /* Start a new falling shape.  We do some non-Tetris-standard
      * things here (choose a random orientation and a random column,
      * and, perhaps more unexpectedly, make the shape fall from the
      * highest possible line - which might not be the highest line of
      * the board). */
{
#if ADVERTIZE_NEXT
  shape_falling = shape_next;
  orient = orient_next;
#endif
  compute_shape_next ();
  column = (NBCOLUMNS-SHAPECOLUMNS)/2;
  for ( line = NBLINES-SHAPELINES+1 ; ; line++ ) /* Very ad hoc... */
    if ( can_place_notop (main_board, shape_falling,
                          orient, line, column) )
      break;
}

void
force_redisplay (void)
     /* Tell Gtk+ that things must be redrawn. */
{
#if ADVERTIZE_NEXT
  gtk_widget_queue_draw_area (GTK_WIDGET (next_area), 0, 0, NEXT_X, NEXT_Y);
#endif
  gtk_widget_queue_draw_area (GTK_WIDGET (game_area), 0, 0, AREA_X, AREA_Y);
}

gboolean
fall_callback (gpointer data)
     /* Make the shape fall by one line: this is called by a timer. */
{
  if ( can_place_notop (main_board, shape_falling, orient, line-1, column) )
    line--;
  else
    {
      if ( ! finalize_shape () )
        gtk_main_quit ();
      else
        initiate_shape ();
    }
  force_redisplay ();
  return TRUE;
}

gboolean
fall_callback_justonce (gpointer data)
     /* Work around brain-deadness in glib (there is no way to add a
      * one-shot call to the source list, the function must return
      * FALSE to remove itself - duh!). */
{
  fall_callback (data);
  return FALSE;
}

void
unpause (void)
{
  if ( paused )
    {
      paused = 0;
      /* Note: we wish to force an immediate fall (to avoid pause
       * being used to reset the fall timer and be able to move
       * endlessly a piece without falling), but we cannot simply call
       * fall_callback(), because it might initiate a different shape,
       * and all sorts of things would behave strangely.  So we must
       * ask glib to queue the calling of fall_callback() just once.
       * Unfortunately, glib's brain-deadness does not permit this!
       * So we use g_idle_add() instead.  Duh... */
      g_idle_add (fall_callback_justonce, NULL);
      tick = g_timeout_add (FALL_INTERVAL, fall_callback, NULL);
    }
}

gboolean
key_press_callback (GtkWidget *widget, GdkEventKey *event, gpointer data)
     /* Handle user keypress. */
{
  char redraw;

  redraw = 0;
  if ( event->keyval == GDK_Left )
    {
      unpause ();
      if ( can_place_notop (main_board, shape_falling,
                            orient, line, column-1) )
        {
          column--;
          redraw = 1;
        }
    }
  else if ( event->keyval == GDK_Right )
    {
      unpause ();
      if ( can_place_notop (main_board, shape_falling,
                            orient, line, column+1) )
        {
          column++;
          redraw = 1;
        }
    }
  else if ( event->keyval == GDK_space )
    {
      unpause ();
      while ( can_place_notop (main_board, shape_falling,
                               orient, line-1, column) )
        {
          line--;
          redraw = 1;
        }
      if ( redraw )
        {
          /* Restart timer! */
          g_source_remove (tick);
          tick = g_timeout_add (FALL_INTERVAL, fall_callback, NULL);
        }
    }
  else if ( event->keyval == GDK_Up )
    {
      unpause ();
      if ( can_place_notop (main_board, shape_falling,
                            (orient+1)%SHAPEORIENTS, line, column) )
        {
          orient = (orient+1)%SHAPEORIENTS;
          redraw = 1;
        }
    }
  else if ( event->keyval == GDK_Return )
    {
      if ( paused )
        unpause ();
      else
        {
          paused = 1;
          g_source_remove (tick);
        }
    }
  else if ( event->keyval == GDK_q )
    gtk_main_quit ();
  if ( redraw )
    force_redisplay ();
  return TRUE;
}

gboolean
expose_callback (GtkWidget *area, GdkEventExpose *event, gpointer data)
     /* Redraw either the game area or the next shape area. */
{
  GdkDrawable *drawable;
  static char did_init = 0;
  static GdkGC *gc;
  static GdkColor color0;
  static GdkColor color1;
  static GdkColor color2;
  int i, j;

  /* What is your DRAWABLE? */
  drawable = GDK_DRAWABLE (area->window);
  if ( ! did_init )
    {
      /* What is your GRAPHIC CONTEXT? */
      gc = gdk_gc_new (drawable);
      /* What is your FAVORITE COLOR? */
      gdk_color_parse ("blue", &color0);
      gdk_colormap_alloc_color (gdk_colormap_get_system (), &color0,
                                FALSE, TRUE);
      gdk_color_parse ("darkslategray", &color1);
      gdk_colormap_alloc_color (gdk_colormap_get_system (), &color1,
                                FALSE, TRUE);
      gdk_color_parse ("red", &color2);
      gdk_colormap_alloc_color (gdk_colormap_get_system (), &color2,
                                FALSE, TRUE);
      did_init = 1;
    }
  /* Redraw game area... */
  if ( area == GTK_WIDGET(game_area) )
    {
      for ( i=0 ; i<NBLINES ; i++ )
        for ( j=0 ; j<NBCOLUMNS ; j++ )
          {
            if ( main_board[i][j] )
              {
                gdk_gc_set_foreground (gc, &color1);
                gdk_draw_rectangle (drawable, gc, TRUE,
                                    j*SQUAREPIXELS_X, (NBLINES-1-i)
*SQUAREPIXELS_Y,
                                    SQUAREPIXELS_X, SQUAREPIXELS_Y);
              }
            else if ( i-line>=0 && j-column>=0
                      && i-line<SHAPELINES && j-column<SHAPECOLUMNS
                      && shapes[shape_falling].s[orient][i-line][j-column] )
              {
                gdk_gc_set_foreground (gc, &color2);
                gdk_draw_rectangle (drawable, gc, TRUE,
                                    j*SQUAREPIXELS_X, (NBLINES-1-i)
*SQUAREPIXELS_Y,
                                    SQUAREPIXELS_X, SQUAREPIXELS_Y);
              }
            else
              {
                gdk_gc_set_foreground (gc, &color0);
                gdk_draw_rectangle (drawable, gc, FALSE,
                                    j*SQUAREPIXELS_X, (NBLINES-1-i)
*SQUAREPIXELS_Y,
                                    SQUAREPIXELS_X-1, SQUAREPIXELS_Y-1);
              }
          }
    }
#if ADVERTIZE_NEXT
  /* Redraw next shape area... */
  else if ( area == GTK_WIDGET(next_area) )
    {
      for ( i=0 ; i<SHAPELINES ; i++ )
        for ( j=0 ; j<SHAPECOLUMNS ; j++ )
          {
            if ( shapes[shape_next].s[orient_next][i][j] )
              {
                gdk_gc_set_foreground (gc, &color2);
                gdk_draw_rectangle (drawable, gc, TRUE,
                                    j*SQUAREPIXELS_X, (SHAPELINES-1-i)
*SQUAREPIXELS_Y,
                                    SQUAREPIXELS_X, SQUAREPIXELS_Y);
              }
            else
              {
                gdk_gc_set_foreground (gc, &color0);
                gdk_draw_rectangle (drawable, gc, FALSE,
                                    j*SQUAREPIXELS_X, (SHAPELINES-1-i)
*SQUAREPIXELS_Y,
                                    SQUAREPIXELS_X-1, SQUAREPIXELS_Y-1);
              }
          }
    }
#endif
  return TRUE;
}

int
main (int argc, char *argv[])
     /* Idiotic setup and whatnot */
{
  GtkWindow *window;
  GtkHBox *box;

  gtk_init (&argc, &argv);

  rnd = g_rand_new ();
#if ADVERTIZE_NEXT
  shape_next = g_rand_int_range (rnd, 0, NBSHAPES);
  orient_next = 0;
#endif
  initiate_shape ();

  window = GTK_WINDOW (gtk_window_new (GTK_WINDOW_TOPLEVEL));
  gtk_container_set_border_width (GTK_CONTAINER (window), 10);
  box = GTK_HBOX (gtk_hbox_new (FALSE, 10));
  gtk_container_add (GTK_CONTAINER (window), GTK_WIDGET (box));

  game_area = GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_drawing_area_size (game_area, AREA_X, AREA_Y);
#if ADVERTIZE_NEXT
  next_area = GTK_DRAWING_AREA (gtk_drawing_area_new ());
  gtk_drawing_area_size (next_area, NEXT_X, NEXT_Y);
#endif

  gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (game_area));
#if ADVERTIZE_NEXT
  gtk_container_add (GTK_CONTAINER (box), GTK_WIDGET (next_area));
#endif

  g_signal_connect (G_OBJECT (game_area), "expose_event",
                    G_CALLBACK (expose_callback), NULL);
#if ADVERTIZE_NEXT
  g_signal_connect (G_OBJECT (next_area), "expose_event",
                    G_CALLBACK (expose_callback), NULL);
#endif
  g_signal_connect (G_OBJECT (window), "key_press_event",
                    G_CALLBACK (key_press_callback), NULL);
  tick = g_timeout_add (FALL_INTERVAL, fall_callback, NULL);

  gtk_widget_show (GTK_WIDGET (game_area));
#if ADVERTIZE_NEXT
  gtk_widget_show (GTK_WIDGET (next_area));
#endif
  gtk_widget_show (GTK_WIDGET (box));
  gtk_widget_show (GTK_WIDGET (window));

  gtk_main ();

  return 0;
}

