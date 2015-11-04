/* mousing.c
 * callback functions for handling mouse clicks, drags, etc.
 *
 *  for Denemo, a gtk+ frontend to GNU Lilypond
 *  (c) 2000-2005 Matthew Hiller
 */

#include "command/commandfuncs.h"
#include "core/kbd-custom.h"
#include "command/staff.h"
#include "core/utils.h"
#include "command/object.h"
#include "command/select.h"
#include "command/lilydirectives.h"
#include "command/lyric.h"
#include "ui/moveviewport.h"
#include "ui/mousing.h"
#include "audio/fluid.h"
#include "display/draw.h"
#include "core/view.h"
#include "audio/audiointerface.h"

static gboolean lh_down;
static gdouble last_event_x;
static gdouble last_event_y;
static DenemoDirective *last_directive;
typedef enum DragDirection { DRAG_DIRECTION_NONE = 0, DRAG_DIRECTION_UP, DRAG_DIRECTION_DOWN, DRAG_DIRECTION_LEFT, DRAG_DIRECTION_RIGHT} DragDirection;
static enum DragDirection dragging_outside = DRAG_DIRECTION_NONE; //dragging to left or right outside window.

/**
 * Get the mid_c_offset of an object or click from its height relative
 * to the top of the staff.  
 */
gint
offset_from_height (gdouble height, enum clefs clef)
{
  /* Offset from the top of the staff, in half-tones.  */
  gint half_tone_offset = ((gint) (height / HALF_LINE_SPACE + ((height > 0) ? 0.5 : -0.5)));

#define R(x) return x - half_tone_offset

  switch (clef)
    {
    case DENEMO_TREBLE_CLEF:
      R (10);
      break;
    case DENEMO_BASS_CLEF:
      R (-2);
      break;
    case DENEMO_ALTO_CLEF:
      R (4);
      break;
    case DENEMO_G_8_CLEF:
      R (3);
      break;
    case DENEMO_F_8_CLEF:
      R (-9);
      break;
    case DENEMO_TENOR_CLEF:
      R (2);
      break;
    case DENEMO_SOPRANO_CLEF:
      R (8);
      break;
    case DENEMO_FRENCH_CLEF:
      R (12);
      break;
      //when adding clefs get utils.c calculateheight() function correct first, then do this
    default:
      R (0);
      break;
    }
#undef R
  return 0;
}


static gdouble
get_click_height (DenemoProject * gui, gdouble y)
{
  gdouble click_height;
  gint staffs_from_top;
  staffs_from_top = 0;
  GList *curstaff;
  DenemoStaff *staff;
  gint extra_space = 0;
  gint space_below = 0;
  curstaff = g_list_nth (gui->movement->thescore, gui->movement->top_staff - 1);

  if (!(((DenemoStaff *) (gui->movement->currentstaff->data))->voicecontrol & DENEMO_PRIMARY))
    staffs_from_top--;

  for (curstaff = g_list_nth (gui->movement->thescore, gui->movement->top_staff - 1); curstaff; curstaff = curstaff->next)
    {
      //g_debug("before extra space %d\n", extra_space);
      staff = (DenemoStaff *) curstaff->data;
      if (staff->hidden) continue;
      if (staff->voicecontrol & DENEMO_PRIMARY)
        extra_space += (staff->space_above) + space_below;
      if (curstaff == gui->movement->currentstaff)
        break;

      if (staff->voicecontrol & DENEMO_PRIMARY)
        {

          space_below = 0;
          staffs_from_top++;
        }
      space_below = MAX (space_below, ((staff->space_below) + (staff->verse_views ? LYRICS_HEIGHT : 0)));
      //g_debug("after extra space %d space_below %d\n", extra_space, space_below);
    }

  click_height = y - (gui->movement->staffspace * staffs_from_top + gui->movement->staffspace / 4 + extra_space);
  //g_debug("top staff is %d total %d staffs from top is %d click %f\n", gui->movement->top_staff, extra_space, staffs_from_top, click_height);

  return click_height;



}

/**
 * Set the cursor's y position from a mouse click
 *
 */
void
set_cursor_y_from_click (DenemoProject * gui, gdouble y)
{
  /* Click height relative to the top of the staff.  */
  gdouble click_height = get_click_height (gui, y);
  gui->movement->cursor_y = offset_from_height (click_height, (enum clefs) gui->movement->cursorclef);
  gui->movement->staffletter_y = offsettonumber (gui->movement->cursor_y);
}

struct placement_info
{
  gint staff_number, measure_number, cursor_x;
  staffnode *the_staff;
  measurenode *the_measure;
  objnode *the_obj;
  gboolean nextmeasure;
  gboolean offend;              //TRUE when the user has clicked beyond the last note, taking you into appending
  gboolean at_edge; //TRUE when user clicked close (CURSOR_AT_EDGE) to right hand edge of scorearea.
};
#define CURSOR_AT_EDGE (60)
/* find the primary staff of the current staff, return its staffnum */
static gint
primary_staff (DenemoMovement * si)
{
  GList *curstaff;
  for (curstaff = si->currentstaff; curstaff && !(((DenemoStaff *) curstaff->data)->voicecontrol & DENEMO_PRIMARY); curstaff = curstaff->prev)
    ;                           //do nothing
  //g_debug("The position is %d\n", 1+g_list_position(si->thescore, curstaff));
  return 1 + g_list_position (si->thescore, curstaff);
}


/* find which staff in si the height y lies in, return the staff number (not counting non-primary staffs ie voices) */

static gint
staff_at (gint y, DenemoMovement * si)
{
  GList *curstaff;
  gint space = 0;
  gint count;
  gint ret;
  for (curstaff = g_list_nth (si->thescore, si->top_staff - 1), count = 0; curstaff && y > space; curstaff = curstaff->next)
    {
      DenemoStaff *staff = (DenemoStaff *) curstaff->data;

      count++;
      if ((!staff->hidden) && (staff->voicecontrol & DENEMO_PRIMARY))
        space += staff->space_above + staff->space_below + si->staffspace + (staff->verse_views ? LYRICS_HEIGHT : 0);
      //g_debug("y %d and space %d count = %d\n",y,space, count);
    }

  if (y <= 1)
    ret = 1;
  ret = count + si->top_staff - 1;
  if (ret == primary_staff (si))
    ret = si->currentstaffnum;
  return ret > 0 ? ret : 1;
}

/**
 * Gets the position from the clicked position
 *
 */
static void
get_placement_from_coordinates (struct placement_info *pi, gdouble x, gdouble y, gint leftmeasurenum, gint rightmeasurenum, gint scale)
{
  DenemoProject *gui = Denemo.project;
  DenemoMovement *si = gui->movement;
  GList *mwidthiterator = g_list_nth (si->measurewidths,
                                      leftmeasurenum - 1);
  objnode *obj_iterator;
  gint x_to_explain = (gint) (x);
  pi->offend = FALSE;
  pi->the_obj = NULL;
  if (mwidthiterator == NULL)
    {
      g_critical ("Array of measurewidths too small for leftmeasure %d\n", leftmeasurenum);
      return;
    }
  pi->at_edge = (get_widget_width (Denemo.scorearea) -x) < CURSOR_AT_EDGE;
  pi->staff_number = staff_at ((gint) y, si);
  //g_debug("L/R %d %d got staff number %d\n", leftmeasurenum, rightmeasurenum, pi->staff_number); 
  pi->measure_number = leftmeasurenum;
  if (scale)
    x_to_explain = (x_to_explain * scale) / 100;
  x_to_explain -= ((gui->leftmargin+35) + si->maxkeywidth + SPACE_FOR_TIME);

  //g_debug("Explaining %d\n", x_to_explain);
  while (x_to_explain > GPOINTER_TO_INT (mwidthiterator->data) && pi->measure_number < rightmeasurenum)
    {
      x_to_explain -= (GPOINTER_TO_INT (mwidthiterator->data) + SPACE_FOR_BARLINE);
      mwidthiterator = mwidthiterator->next;
      pi->measure_number++;
    }
  //g_debug("got to measure %d\n", pi->measure_number);
  pi->nextmeasure = ((si->system_height > 0.5 || x_to_explain > GPOINTER_TO_INT (mwidthiterator->data)) && pi->measure_number >= rightmeasurenum);

  pi->the_staff = g_list_nth (si->thescore, pi->staff_number - 1);
  pi->the_measure = staff_nth_measure_node (pi->the_staff, pi->measure_number - 1);
  if (pi->the_measure != NULL)
    {                           /*check to make sure user did not click on empty space */
      obj_iterator = (objnode *) pi->the_measure->data;
      pi->cursor_x = 0;
      pi->the_obj = NULL;
      if (obj_iterator)
        {
          DenemoObject *current, *next;

          for (; obj_iterator->next; obj_iterator = obj_iterator->next, pi->cursor_x++)
            {
              current = (DenemoObject *) obj_iterator->data;
              next = (DenemoObject *) obj_iterator->next->data;
              /* This comparison neatly takes care of two possibilities:

                 1) That the click was to the left of current, or

                 2) That the click was between current and next, but
                 closer to current.

                 Do the math - it really does work out.  */

              //???modify current->x by gx where graphic_override is set????
              if (x_to_explain - (current->x + current->minpixelsalloted) < next->x - x_to_explain)
                {
                  pi->the_obj = obj_iterator;
                  break;
                }
            }
          if (!obj_iterator->next)
            /* That is, we exited the loop normally, not through a break.  */
            {
              DenemoObject *current = (DenemoObject *) obj_iterator->data;
              pi->the_obj = obj_iterator;//g_print("x_to_explain %d, compare current->x=%d and minpix %d\n",x_to_explain,current->x,current->minpixelsalloted);
              /* The below makes clicking to get the object at the end of
                 a measure (instead of appending after it) require
                 precision.  This may be bad; tweak me if necessary.  */
              if ((x_to_explain > current->x + current->minpixelsalloted) || (x_to_explain > GPOINTER_TO_INT (mwidthiterator->data) - current->minpixelsalloted / 3))   //if closer to barline than object center
                pi->offend = TRUE, pi->cursor_x++;
            }
        }
      //g_debug("got to cursor x %d\n", pi->cursor_x);
    }
}


void
assign_cursor (guint state, guint cursor_num)
{
  guint *cursor_state = g_new (guint, 1);
  *cursor_state = state;
  //g_print("Storing cursor %d for state 0x%x in hash table %p\n", cursor_num, state, Denemo.map->cursors );  
  GdkCursor *cursor = gdk_cursor_new (cursor_num);
  if (cursor)
    g_hash_table_insert (Denemo.map->cursors, cursor_state, cursor);
}

void
set_cursor_for (guint state)
{
  gint the_state = state;
  GdkCursor *cursor = g_hash_table_lookup (Denemo.map->cursors, &the_state);
  //g_print("looked up %x in %p got cursor %p\n", state, Denemo.map->cursors,  cursor);
  if (cursor)
    gdk_window_set_cursor (gtk_widget_get_window (Denemo.window), cursor);
  else
    gdk_window_set_cursor (gtk_widget_get_window (Denemo.window), gdk_cursor_new (GDK_LEFT_PTR));       //FIXME? does this take time/hog memory
}


/* appends the name(s) for modifier mod to ret->str */

void
append_modifier_name (GString * ret, gint mod)
{
  gint i;
  static const gchar *names[] = {
    "Shift",
    "CapsLock",
    "Control",
    "Alt",
    "NumLock",
    "MOD3",
    "Penguin",
    "AltGr"
  };
  for (i = 0; i < DENEMO_NUMBER_MODIFIERS; i++)
    if ((1 << i) & mod)
      g_string_append_printf (ret, "%s%s", "-", names[i]);
  g_string_append_printf (ret, "%s", mod ? "" : "");
}

/* returns a newly allocated GString containing a shortcut name */
GString *
mouse_shortcut_name (gint mod, mouse_gesture gesture, gboolean left)
{

  GString *ret = g_string_new ((gesture == GESTURE_PRESS) ? (left ? "PrsL" : "PrsR") : ((gesture == GESTURE_RELEASE) ? (left ? "RlsL" : "RlsR") : (left ? "MveL" : "MveR")));

  append_modifier_name (ret, mod);
  //g_debug("Returning %s for mod %d\n", ret->str, mod);
  return ret;

}


/* perform an action for mouse-click stored with shortcuts */
static void
perform_command (gint modnum, mouse_gesture press, gboolean left)
{
  GString *modname = mouse_shortcut_name (modnum, press, left);
  gint command_idx = lookup_command_for_keybinding_name (Denemo.map, modname->str);
  if (press != GESTURE_MOVE)
    {
      if (!Denemo.prefs.strictshortcuts)
        {
          if (command_idx < 0)
            {
              g_string_free (modname, TRUE);
              modname = mouse_shortcut_name (modnum & (~GDK_LOCK_MASK /*CapsLock */ ), press, left);
              command_idx = lookup_command_for_keybinding_name (Denemo.map, modname->str);
            }
          if (command_idx < 0)
            {
              g_string_free (modname, TRUE);
              modname = mouse_shortcut_name (modnum & (~GDK_MOD2_MASK /*NumLock */ ), press, left);
              command_idx = lookup_command_for_keybinding_name (Denemo.map, modname->str);
            }
          if (command_idx < 0)
            {
              g_string_free (modname, TRUE);
              modname = mouse_shortcut_name (modnum & (~(GDK_LOCK_MASK | GDK_MOD2_MASK)), press, left);
              command_idx = lookup_command_for_keybinding_name (Denemo.map, modname->str);
            }
        }
    }


  if (command_idx >= 0)
    {

      if (Denemo.prefs.learning)
        KeyPlusMouseGestureShow(modname->str, command_idx);
      
      execute_callback_from_idx (Denemo.map, command_idx);
      displayhelper (Denemo.project);
    }
  g_string_free (modname, TRUE);
}

static gboolean selecting = FALSE;
static gboolean dragging_separator = FALSE;
static gboolean dragging_audio = FALSE;
static gboolean dragging_tempo = FALSE;


static gboolean
change_staff (DenemoMovement * si, gint num, GList * staff)
{
  if (si->currentstaffnum == num)
    return FALSE;
  hide_lyrics ();
  si->currentstaffnum = num;
  si->currentstaff = staff;
  show_lyrics ();
  return TRUE;
}

static void
transform_coords (double *x, double *y)
{
  DenemoProject *gui = Denemo.project;

  gint application_height = get_widget_height (Denemo.scorearea);
  gint line_height = application_height * gui->movement->system_height;
  gint line_num = ((int) *y) / line_height;
  *y -= line_num * line_height;
  *x /= gui->movement->zoom;
  *y /= gui->movement->zoom;
  // *x += ((double)line_num * gui->movement->widthtoworkwith / ((int)(1/gui->movement->system_height))) - 1.0* (line_num?(double)gui->leftmargin:0.0);
}

static void extend_selection (DragDirection direction)
{
   switch (direction) {
       case DRAG_DIRECTION_RIGHT:
        cursorright (NULL, NULL);
        break;
      case DRAG_DIRECTION_LEFT:
        cursorleft (NULL, NULL);
        break;
      case DRAG_DIRECTION_UP:
        staffup (NULL, NULL);
        move_viewport_up (Denemo.project);
        break;
      case DRAG_DIRECTION_DOWN:
        staffdown (NULL, NULL);
        move_viewport_down (Denemo.project);
        break;
    }
        
    gtk_widget_queue_draw(Denemo.scorearea);
}
gint
scorearea_leave_event (GtkWidget * widget, GdkEventCrossing * event)
{
    gint allocated_height = get_widget_height (Denemo.scorearea);
    gint allocated_width = get_widget_width (Denemo.scorearea);
  if (event->state & GDK_BUTTON1_MASK)
    {
       dragging_outside = (event->x>=allocated_width)?DRAG_DIRECTION_RIGHT:(event->x < 0)?DRAG_DIRECTION_LEFT:(event->y < 0)? DRAG_DIRECTION_UP : DRAG_DIRECTION_DOWN;
       last_event_x = event->x_root;
       last_event_y = event->y_root;
    }
  gdk_window_set_cursor (gtk_widget_get_window (Denemo.window), gdk_cursor_new (GDK_LEFT_PTR)); //FIXME? does this take time/hog memory
  return FALSE;                 //allow other handlers (specifically the pitch entry one)
}

gint
scorearea_enter_event (GtkWidget * widget, GdkEventCrossing * event)
{
  dragging_outside = DRAG_DIRECTION_NONE;
  if(Denemo.keyboard_state_locked) return FALSE;
//g_debug("start the enter with ks = %x and state %x\n", Denemo.keyboard_state, event->state);
  if (event->state & GDK_CONTROL_MASK)
    Denemo.keyboard_state |= GDK_CONTROL_MASK;
  else
    Denemo.keyboard_state &= ~GDK_CONTROL_MASK;

  if (event->state & GDK_SHIFT_MASK)
    Denemo.keyboard_state |= GDK_SHIFT_MASK;
  else
    Denemo.keyboard_state &= ~GDK_SHIFT_MASK;
#if 0
//perhaps it would be better to clear Denemo.keyboard_state on focus out event???
  if (event->state & GDK_MOD1_MASK)
    Denemo.keyboard_state |= GDK_MOD1_MASK;
  else
    Denemo.keyboard_state &= ~(CHORD_MASK | GDK_MOD1_MASK);
#endif
//      g_debug("end the enter with ks %x (values  %x %x)\n", event->state, ~GDK_CONTROL_MASK, Denemo.keyboard_state & (~GDK_CONTROL_MASK) );
  set_midi_in_status ();
  return FALSE;                 //allow other handlers 
}

/**
 * Mouse motion callback 
 *
 */
gint
scorearea_motion_notify (GtkWidget * widget, GdkEventButton * event)
{
  DenemoProject *gui = Denemo.project;
  if (gui == NULL || gui->movement == NULL)
    return FALSE;
  if (Denemo.scorearea == NULL)
    return FALSE;
  gint allocated_height = get_widget_height (Denemo.scorearea);
  gint line_height = allocated_height * gui->movement->system_height;
  if(dragging_outside)
    {
          gint incrx, incry;
          incrx=incry=0;
          if(((gint)((last_event_x - event->x_root)/gui->movement->zoom)) != 0)
            {
                incrx = -(last_event_x - event->x_root)/gui->movement->zoom;
                last_event_x = event->x_root;
            }
          if( ((gint)((last_event_y - event->y_root)/gui->movement->zoom)) != 0)
            {
                incry = -(last_event_y - event->y_root)/gui->movement->zoom;
                last_event_y = event->y_root;
            }    
        if((dragging_outside==DRAG_DIRECTION_RIGHT) && (incrx > 1)
            || ((dragging_outside==DRAG_DIRECTION_LEFT) && (incrx < -1))
            || ((dragging_outside==DRAG_DIRECTION_UP) && (incry < 0))
            || ((dragging_outside==DRAG_DIRECTION_DOWN) && (incry > 0)))
            extend_selection(dragging_outside);
    return TRUE;
    }
  if (event->y < 0)
    event->y = 0.0;
  gint line_num = ((int) event->y) / line_height;
  
  
   if (last_directive && (GDK_SHIFT_MASK & event->state) && (GDK_CONTROL_MASK & event->state))
      {
          gint incrx, incry;
          incrx=incry=0;
          if(((gint)((last_event_x - event->x_root)/gui->movement->zoom)) != 0)
            {
                incrx = (last_event_x - event->x_root)/gui->movement->zoom;
                last_event_x = event->x_root;
            }
          if( ((gint)((last_event_y - event->y_root)/gui->movement->zoom)) != 0)
            {
                incry = (last_event_y - event->y_root)/gui->movement->zoom;
                last_event_y = event->y_root;
            }           
            
        if(last_directive->graphic)
            {
                last_directive->gx -= incrx; 
                last_directive->gy -= incry;
            } 
        else 
            {
                last_directive->tx -= incrx;
                last_directive->ty -= incry;
            }
        draw_score_area();
        
        return TRUE;
      }
  
  

  if(gui->movement->recording && dragging_audio)
    {   
        if(gui->movement->recording->type == DENEMO_RECORDING_MIDI)
        {
            #if 0
            //This is moving only the NoteOn, so it could be moved later than the note off, and indeed later than a later note in the stream 
            //- quite a bit more work needed to drag MIDI to correct the timing.
            smf_event_t *midievent;
            GList *marked_onset = gui->movement->marked_onset;
            if(marked_onset)
                {
                midievent = ((DenemoRecordedNote *)marked_onset->data)->event;
                gint shift =  2500*(event->x_root - last_event_x)/gui->movement->zoom;
                g_debug (" %f (%f %f)",shift/(double)gui->movement->recording->samplerate, 
                    midievent->time_seconds,
                    ((DenemoRecordedNote *)marked_onset->data)->timing/(double)gui->movement->recording->samplerate) ;

                ((DenemoRecordedNote *)marked_onset->data)->timing += shift;
                
                midievent->time_seconds += shift/(double)gui->movement->recording->samplerate;
                }
            #endif
            g_warning("No drag for MIDI yet");
            return TRUE;
        }

        gui->movement->recording->leadin -= 500*(event->x_root - last_event_x)/gui->movement->zoom;//g_debug("%d %d => %d\n", (int)(10*last_event_x), (int)(10*event->x_root), (int)(10*last_event_x) - (int)(10*event->x_root));
        last_event_x = event->x_root;
        update_leadin_widget ( gui->movement->recording->leadin/(double)gui->movement->recording->samplerate);
        gtk_widget_queue_draw(Denemo.scorearea);
        return TRUE; 
    }
  if(gui->movement->recording && dragging_tempo)
    {       
        gdouble change = (event->x_root - last_event_x)/gui->movement->zoom;
        last_event_x = event->x_root;
        struct placement_info pi;
        get_placement_from_coordinates (&pi, event->x, 0, gui->lefts[line_num], gui->rights[line_num], gui->scales[line_num]);
        change /= pi.measure_number;
        update_tempo_widget ( change);
        set_tempo (); exportmidi (NULL, gui->movement, 0, 0);  
        gtk_widget_queue_draw(Denemo.scorearea);
        return TRUE; 
    }
#define DENEMO_MINIMUM_SYSTEM_HEIGHT (0.01)


  if (dragging_separator)
    {
      gui->movement->system_height = event->y / get_widget_height (Denemo.scorearea);
      if (gui->movement->system_height < DENEMO_MINIMUM_SYSTEM_HEIGHT)
        gui->movement->system_height = DENEMO_MINIMUM_SYSTEM_HEIGHT;
      if (gui->movement->system_height > 1.0)
        gui->movement->system_height = 1.0;
      scorearea_configure_event (Denemo.scorearea, NULL);
      draw_score_area();
      return TRUE;
    }

  if (line_height - ((int) event->y - 8) % line_height < 12)
    gdk_window_set_cursor (gtk_widget_get_window (Denemo.window), gdk_cursor_new (GDK_SB_V_DOUBLE_ARROW));
  else
    gdk_window_set_cursor (gtk_widget_get_window (Denemo.window), gdk_cursor_new (GDK_LEFT_PTR));       //FIXME? does this take time/hog memory

  transform_coords (&event->x, &event->y);
  //g_debug("Marked %d\n", gui->movement->markstaffnum);


  if (gui->lefts[line_num] == 0)
    return TRUE;




  if (lh_down || (selecting && gui->movement->markstaffnum))
    {
      struct placement_info pi;
      pi.the_staff = NULL;
      if (event->y < 0)
        get_placement_from_coordinates (&pi, event->x, 0, gui->lefts[line_num], gui->rights[line_num], gui->scales[line_num]);
      else
        get_placement_from_coordinates (&pi, event->x, event->y, gui->lefts[line_num], gui->rights[line_num], gui->scales[line_num]);
      if (pi.the_staff == NULL)
        return TRUE;            //could not place the cursor
      if (pi.the_measure != NULL)
        {                       /*don't place cursor in a place that is not there */
          change_staff (gui->movement, pi.staff_number, pi.the_staff);
          gui->movement->currentmeasurenum = pi.measure_number;
          gui->movement->currentmeasure = pi.the_measure;
          gui->movement->currentobject = pi.the_obj;
          gui->movement->cursor_x = pi.cursor_x;
          gui->movement->cursor_appending = (gui->movement->cursor_x == (gint) (g_list_length ((objnode *) gui->movement->currentmeasure->data)));

          set_cursor_y_from_click (gui, event->y);
          if (lh_down & !selecting)
            {
              if (gui->movement->markstaffnum)
                set_point (NULL, NULL);
              else
                set_mark (NULL, NULL);
              selecting = TRUE;
            }
          calcmarkboundaries (gui->movement);
          if (event->state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK))
            perform_command (event->state, GESTURE_MOVE, event->state & GDK_BUTTON1_MASK);

          /* redraw to show new cursor position  */
          draw_score_area();
        }
    }

  if (Denemo.project->midi_destination & MIDICONDUCT)
    {
      advance_time (0.01);
      return TRUE;
    }
  return TRUE;
}


/**
 * Mouse button press callback 
 *
 */
gint
scorearea_button_press (GtkWidget * widget, GdkEventButton * event)
{
  DenemoProject *gui = Denemo.project;
  if (gui == NULL || gui->movement == NULL)
    return FALSE;
  gboolean left = (event->button != 3);
  //if the cursor is at a system separator start dragging it
  gint allocated_height = get_widget_height (Denemo.scorearea);
  gint line_height = allocated_height * gui->movement->system_height;
  gint line_num = ((int) event->y) / line_height;
  last_event_x = event->x_root;
  last_event_y = event->y_root;
  //g_debug("diff %d\n", line_height - ((int)event->y)%line_height);

  if (dragging_separator == FALSE)
    if (line_height - ((int) event->y - 8) % line_height < 12)
      {
        if (Denemo.prefs.learning)
          MouseGestureShow(_("Dragging line separator."), _("This will allow the display to show more music, split into lines. The typeset score is not affected."),
            MouseGesture);
        dragging_separator = TRUE;
        return TRUE;
      }
  dragging_separator = FALSE;
  
  if(gui->movement->recording)
    {
     //g_debug("audio %f %f\n", event->x, event->y);


      if(event->y < 20*gui->movement->zoom /* see draw.c for this value, the note onsets are drawn in the top 20 pixels */)
        {
            if (event->type==GDK_2BUTTON_PRESS) 
                {   
                    gui->movement->marked_onset_position = (gint)event->x/gui->movement->zoom;
                    if(gui->movement->marked_onset_position < (gui->leftmargin+35) + SPACE_FOR_TIME + gui->movement->maxkeywidth) {
                         if (Denemo.prefs.learning)
                            MouseGestureShow(_("Double Click Note Onset"), _("This represents detected note onsets which occur\nbefore the start of the score.\nIf they are just noise,\nor if you are working on just a portion of the audio that is ok.\nOtherwise drag with left mouse button to synchronize\nwith the start of the score."),
          MouseGesture);
                        
                    }
                    gtk_widget_queue_draw(Denemo.scorearea);
                    return TRUE;
                } else 
                {
                    gdk_window_set_cursor (gtk_widget_get_window (Denemo.window), gdk_cursor_new (left?GDK_SB_H_DOUBLE_ARROW:GDK_X_CURSOR));
                    left? (dragging_audio = TRUE) : (dragging_tempo = TRUE);
                     if (Denemo.prefs.learning)
                     left? MouseGestureShow(_("Left Drag Note Onset"), _("This moves the audio to synchronize the start with the score.\nYou can use the Leadin button for this too."),
          MouseGesture) :
                        MouseGestureShow(_("Right Drag Note Onset"), _("This changes the tempo of the score.\nUse this to synchronize the beat after setting the start"),
          MouseGesture);
                    gtk_widget_queue_draw(Denemo.scorearea);
                    return TRUE;
                }
        }
      
    }
  
  
  //g_debug("before %f %f\n", event->x, event->y);
  transform_coords (&event->x, &event->y);
  //g_debug("after %f %f\n", event->x, event->y);

  
  gtk_widget_grab_focus (widget);
  gint key = gui->movement->maxkeywidth;
  gint cmajor = key ? 0 : 5;    //allow some area for keysig in C-major

  if (gui->lefts[line_num] == 0)
    return TRUE;                //On an empty system at the bottom where there is not enough room to draw another staff.

  struct placement_info pi;
  pi.the_staff = NULL;
  if (event->y < 0)
    get_placement_from_coordinates (&pi, event->x, 0, gui->lefts[line_num], gui->rights[line_num], gui->scales[line_num]);
  else
    get_placement_from_coordinates (&pi, event->x, event->y, gui->lefts[line_num], gui->rights[line_num], gui->scales[line_num]);
  if (pi.the_staff == NULL)
    return TRUE;                //could not place the cursor
  change_staff (gui->movement, pi.staff_number, pi.the_staff);


  if (left && (gui->movement->leftmeasurenum > 1) && (event->x < (gui->leftmargin+35) + SPACE_FOR_TIME + key) && (event->x > gui->leftmargin))
    {
      if (Denemo.prefs.learning)
        MouseGestureShow(_("Press Left."), _("This moved the cursor to the measure offscreen left. The display is shifted to place that measure on screen."),
          MouseGesture);
      moveto_currentmeasurenum (gui, gui->movement->leftmeasurenum - 1);
      write_status (gui);
      draw_score_area();
      return TRUE;
    }
  else if (pi.nextmeasure)
    {
      if ((pi.at_edge) && ((pi.the_obj==NULL) || ((pi.the_obj->next == NULL) && (pi.offend))))//crashed here with the_obj 0x131 !!!
        {
          if ((gui->movement->currentmeasurenum != gui->movement->rightmeasurenum) &&
                (!moveto_currentmeasurenum (gui, gui->movement->rightmeasurenum + 1)))
              moveto_currentmeasurenum (gui, gui->movement->rightmeasurenum);
          else if ((gui->movement->cursor_appending) &&
                (!moveto_currentmeasurenum (gui, gui->movement->rightmeasurenum + 1)))
              moveto_currentmeasurenum (gui, gui->movement->rightmeasurenum);


          

          if (gui->movement->currentmeasurenum != gui->movement->rightmeasurenum) {
            if (Denemo.prefs.learning)
              MouseGestureShow(_("Press Left."), _("This moved the cursor to the measure off-screen right. The display is shifted to move the cursor to the middle."),
                MouseGesture);
          write_status (gui);
          return TRUE;
        }
        }
    }


  if (pi.the_measure != NULL)
    {                           /*don't place cursor in a place that is not there */
      //gui->movement->currentstaffnum = pi.staff_number;
      //gui->movement->currentstaff = pi.the_staff;
      gui->movement->currentmeasurenum = pi.measure_number;
      gui->movement->currentmeasure = pi.the_measure;
      gui->movement->currentobject = pi.the_obj;
      gui->movement->cursor_x = pi.cursor_x;
      gui->movement->cursor_appending = (gui->movement->cursor_x == (gint) (g_list_length ((objnode *) gui->movement->currentmeasure->data)));
      set_cursor_y_from_click (gui, event->y);
      if (event->type==GDK_2BUTTON_PRESS) 
                {
                    if(gui->movement->recording &&  !g_strcmp0 (((DenemoStaff *) gui->movement->currentstaff->data)->denemo_name->str, DENEMO_CLICK_TRACK_NAME))
                        {
                            gui->movement->marked_onset_position = (gint)event->x/gui->movement->zoom;
                            if (Denemo.prefs.learning)
                                MouseGestureShow(_("Double Click on Click Track"), _("This will mark the MIDI note onset."), MouseGesture);
                            return TRUE;
                 
                        }
                        
                    else
                        {
                          if (Denemo.prefs.learning)
                            MouseGestureShow(_("Double Click."), _("This gives information about the object at the cursor. Click on a notehead for information about a note in a chord."),
                              MouseGesture);
                                    display_current_object();
                                    return TRUE;
                        }
                }
            else 
                {
                  if (Denemo.prefs.learning)
                    MouseGestureShow(_("Press Left."), _("This moved the cursor to the object position clicked. The cursor height becomes the clicked point."),
                      MouseGesture);
                            write_status (gui);
               }
            }

  gint offset = (gint) get_click_height (gui, event->y);
  
   if ((((DenemoStaff *) gui->movement->currentstaff->data)->voicecontrol != DENEMO_PRIMARY) && (gui->movement->leftmeasurenum == 1) && (event->x > gui->leftmargin))
    {
          MouseGestureShow(_("Left on Voice"), _("The clef shown here affects the display only (as this voice is displayed on the staff above. You can change the display clef using the clef menu. Warning! you will get confused if you set the key signature or time signature of a voice different to the staff it is typeset on. Run the Staff/Voice property editor to adjust any inconsistencies."),
                  MouseGesture);
  
    }
  if ((((DenemoStaff *) gui->movement->currentstaff->data)->voicecontrol == DENEMO_PRIMARY) && (gui->movement->leftmeasurenum == 1) && (event->x > gui->leftmargin))
    {
      if (event->x < (gui->leftmargin+35) - cmajor)
        {
        if (offset<-10)
            {  
                if (Denemo.prefs.learning)
                MouseGestureShow(_("Left on Staff name."), _("This pops up the built-in staff properties. For other properties of the current staff see the staff menu or the tools icon before the clef."),
                  MouseGesture);
                staff_properties_change_cb (NULL, NULL);
            }
        else
            {
              if (Denemo.prefs.learning)
                MouseGestureShow(_("Left on initial Clef."), _("This pops up the initial clef menu."),
                  MouseGesture);
              popup_menu ("/InitialClefEditPopup");
            }
          return TRUE;
        }
      else if (event->x < (gui->leftmargin+35) + key + cmajor)
        {
          if (left)
            {
              if (offset > 0 && (offset < STAFF_HEIGHT / 2))
                {
                  if (Denemo.prefs.learning)
                    MouseGestureShow(_("Left Click on blue."), _("This adds one sharp."),
                      MouseGesture);
                if ((gui->movement->currentmeasure->next==NULL)  || confirm (_("Initial Key Signature Change"), _("Sharpen Keysignature?")))
                  call_out_to_guile ("(d-SharpenInitialKeysigs)");
                }
              else if (offset > 0 && (offset < STAFF_HEIGHT))
                {
                  if (Denemo.prefs.learning)
                    MouseGestureShow(_("Left Click on red."), _("This adds one flat."),
                      MouseGesture);
                  if ((gui->movement->currentmeasure->next==NULL) || confirm (_("Initial Key Signature Change"), _("Flatten Keysignature?")))
                    call_out_to_guile ("(d-FlattenInitialKeysigs)");
                }
            }
          else
            {
              if (Denemo.prefs.learning)
                MouseGestureShow(_("Right Click on key."), _("This pops up the key signature menu."),
                    MouseGesture);  
              popup_menu ("/InitialKeyEditPopup");
            }
          return TRUE;
        }
      else if (event->x < (gui->leftmargin+35) + SPACE_FOR_TIME + key)
        {
          if (Denemo.prefs.learning)
            MouseGestureShow(_("Click on Time."), _("This pops up the time signature menu."),
                    MouseGesture); 
          popup_menu ("/InitialTimeEditPopup");
          return TRUE;
        }
    }

  if (event->x < gui->leftmargin)
    {
       if (gui->braces)
        {
                gint width = BRACEWIDTH * g_list_length (gui->braces);
                //gint count = (gui->leftmargin - event->x)/BRACEWIDTH;
                if ((gui->leftmargin - event->x) < width)
                {
                    gint count = 1 + (width - gui->leftmargin + event->x)/BRACEWIDTH;
                    
                    
                    if ((count>0) && (count <= g_list_length (gui->braces)))
                        {
                            DenemoBrace *brace = (DenemoBrace*)g_list_nth_data (gui->braces, count-1);
                            gint choice = choose_option (_("Editing Staff Groups (Braces)"), _("Edit Start Brace"), _("Edit End Brace"));
                            gint staffnum = choice?brace->startstaff:brace->endstaff;
                            //g_print ("Count is %d for start at %d\n", count, staffnum);
                            GtkWidget *menuitem = gtk_ui_manager_get_widget (Denemo.ui_manager, "/ObjectMenu/StaffMenu/StaffGroupings");
                            goto_movement_staff_obj (NULL, -1, staffnum, 1, 0);
                            if (menuitem)
                                if (choice)
                                    gtk_menu_popup (GTK_MENU (gtk_menu_item_get_submenu (GTK_MENU_ITEM (menuitem))), NULL, NULL, NULL, NULL, 0, GDK_CURRENT_TIME);
                                else 
                                {
                                    if (staff_directive_get_tag ("BraceEnd"))
                                        call_out_to_guile ("(d-BraceEnd)");
                                    else
                                        warningdialog (_( "This staff grouping has no End Brace so it finishes on the lowest staff. Use the Staffs/Voices->Staff Groupings menu to place an End Brace on the desired staff"));
                                }
                            //note the popup returns as soon as the menu is popped up, so we can't go back to the original position.
                            
                        }
                    
                    return TRUE;
                }
                
        }
        
      if (pi.staff_number == gui->movement->currentstaffnum)
        {
          gint offset = (gint) get_click_height (gui, event->y);
          if (offset < STAFF_HEIGHT / 2)
            {
              if (((DenemoStaff *) gui->movement->currentstaff->data)->staff_directives)
                {
                  if (Denemo.prefs.learning)
                    MouseGestureShow(_("Click on Staff Directives."), _("This pops up the staff directives menu for editing"),
                      MouseGesture);                  
                  edit_staff_properties ();//gtk_menu_popup (((DenemoStaff *) gui->movement->currentstaff->data)->staffmenu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
                }
              return TRUE;
            }
          else if (((DenemoStaff *) gui->movement->currentstaff->data)->voice_directives)
            {
              if (Denemo.prefs.learning)
                MouseGestureShow(_("Click on Voice Directives."), _("This pops up the voice directives menu for editing"),
                    MouseGesture);  
              edit_voice_properties ();//gtk_menu_popup (((DenemoStaff *) gui->movement->currentstaff->data)->voicemenu, NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
              return TRUE;
            }
        }
    }

  if (left)
    {
      if (!(GDK_SHIFT_MASK & event->state))
        gui->movement->markstaffnum = 0;
      lh_down = TRUE;
    }
  else
    {
      if (gui->movement->cursor_appending && (event->state==0))
        {
          if (Denemo.prefs.learning)
           { 
              
                    MouseGestureShow(_("Right Click Appending."), _("This pops up the append menu"),
                        MouseGesture);
            
           }
          popup_menu ("/NoteAppendPopup");
          return TRUE;
        }
        
        if ((GDK_CONTROL_MASK & event->state) == GDK_CONTROL_MASK) {     
            if (Denemo.prefs.learning)  
                    MouseGestureShow(_("Control-Right Click."), _("This pops up menu for inserting barlines and many other sorts of objects"),
                        MouseGesture);
            gtk_menu_popup (GTK_MENU (gtk_widget_get_parent(gtk_ui_manager_get_widget (Denemo.ui_manager, "/ObjectMenu/Directives/Markings"))),
                            NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());                       
            return TRUE;
        }
      if ((GDK_SHIFT_MASK & event->state) == GDK_SHIFT_MASK) {     
            if (Denemo.prefs.learning)  
                    MouseGestureShow(_("Shift-Right Click."), _("This allows editing the directives/attributes of the object at the cursor"),
                        MouseGesture);       
            call_out_to_guile ("(d-EditSimilar 'once)");
            return TRUE;
        }
    }
    
    
      
  if (left && (GDK_SHIFT_MASK & event->state) && (GDK_CONTROL_MASK & event->state))
  {
     // if current object is directive, start dragging its graphic, dragging_display=TRUE
      
      DenemoObject *obj;
      if (Denemo.prefs.learning)  
                    MouseGestureShow(_("Control-Shift-Drag."), _("This allows dragging objects in the display.\nAll sorts of directives such as staccato dots, ornaments, repeat marks etc can be dragged if the display is too cluttered.\nThe typeset score is unaffected.\nClick on a notehead to drag things attached to the notehead,\nor off the noteheads for things attached to the whole chord."),
                        MouseGesture);       

    last_directive = get_next_directive_at_cursor ();
    if(last_directive)
        {
            score_status (Denemo.project, TRUE);
            return TRUE;
        }
    infodialog (_("Control-Shift-Drag is used to tidy up the Denemo display. Useful if Denemo has created a clutter with your input music.\nIf you have several things attached to one object you can move them in turn by dragging them in turn.\nNotes, Slurs and Ties are fixed but most other things can be moved to make the input music clear. Does not affect the typeset score!\nNB! if you have dragged something to one side of a note you have to control-shift-click on the note itself to drag it back - it is where the cursor is that counts."));
    return TRUE;
 }
  
  
  
    
    
    
  set_cursor_for (event->state | (left ? GDK_BUTTON1_MASK : GDK_BUTTON3_MASK));



  
  //displayhelper(Denemo.project);
  draw_score(NULL);//this is needed to refresh cached values such as the prevailing time signature, before the command is invoked

  perform_command (event->state | (left ? GDK_BUTTON1_MASK : GDK_BUTTON3_MASK), GESTURE_PRESS, left);

  return TRUE;
}


/**
 * Mouse button release callback 
 *
 */
gint
scorearea_button_release (GtkWidget * widget, GdkEventButton * event)
{
  DenemoProject *gui = Denemo.project;
  if (gui == NULL || gui->movement == NULL)
    return FALSE;
  last_directive = NULL;
  gboolean left = (event->button != 3);
  if(gui->movement->recording && (dragging_tempo || dragging_audio))
    {       
            dragging_tempo = dragging_audio = FALSE;
            gdk_window_set_cursor (gtk_widget_get_window (Denemo.window), gdk_cursor_new (GDK_LEFT_PTR));       //FIXME? does this take time/hog memory
            return TRUE; 
    }
  if (dragging_separator)
    {
      if (Denemo.prefs.learning)
        MouseGestureShow(_("Dragged line separator."), _("This allows the display to show more music, split into lines. The typeset score is not affected."),
          MouseGesture);
      dragging_separator = FALSE;
      return TRUE;
    }

  //g_signal_handlers_block_by_func(Denemo.scorearea, G_CALLBACK (scorearea_motion_notify), gui); 
  if (left)
    lh_down = FALSE;
  selecting = FALSE;
  set_cursor_for (event->state & DENEMO_MODIFIER_MASK);
  transform_coords (&event->x, &event->y);
  set_cursor_y_from_click (gui, event->y);
  gtk_widget_queue_draw(Denemo.scorearea);
  perform_command (event->state, GESTURE_RELEASE, left);

  return TRUE;
}

gint
scorearea_scroll_event (GtkWidget * widget, GdkEventScroll * event)
{
  DenemoProject *gui = Denemo.project;
  if (gui == NULL || gui->movement == NULL)
    return FALSE;
  switch (event->direction)
    {
      DenemoScriptParam param;
    case GDK_SCROLL_UP:
      if (event->state & GDK_CONTROL_MASK)
        {
          if (Denemo.prefs.learning) {
            gint command_idx = lookup_command_from_name(Denemo.map, "ZoomIn");
            KeyStrokeShow (_("Ctrl + Mouse Wheel Up"), command_idx, TRUE);
          }
          Denemo.project->movement->zoom *= 1.1;
          scorearea_configure_event (Denemo.scorearea, NULL);
        }
      else if (event->state & GDK_SHIFT_MASK)
        {
          gint command_idx = lookup_command_from_name(Denemo.map, "MoveToMeasureLeft");
          if (Denemo.prefs.learning) {
            KeyStrokeShow (_("Shift + Mouse Wheel Up"), command_idx, TRUE);
          }
          execute_callback_from_idx(Denemo.map, command_idx);//scroll_left ();

        }
      else
        {
          if (Denemo.prefs.learning) {
            gint command_idx = lookup_command_from_name(Denemo.map, "MoveToStaffUp");
            KeyStrokeShow (_("Unshifted + Mouse Wheel Up"), command_idx, TRUE);
          }
          movetostaffup (NULL, &param);
          if (!param.status) {
            DenemoStaff *thestaff = (DenemoStaff*)(Denemo.project->movement->currentstaff->data);
            if(thestaff->space_above < MAXEXTRASPACE)
              {
                thestaff->space_above++;
                g_debug ("Increasing the height of the top staff");
              } 
          }
        }
      break;
    case GDK_SCROLL_DOWN:
      if (event->state & GDK_CONTROL_MASK)
        {
          if (Denemo.prefs.learning) {
            gint command_idx = lookup_command_from_name(Denemo.map, "ZoomOut");
            KeyStrokeShow (_("Ctrl + Mouse Wheel Down"), command_idx, TRUE);
          }
          Denemo.project->movement->zoom /= 1.1;
          if (Denemo.project->movement->zoom < 0.01)
            Denemo.project->movement->zoom = 0.01;
          scorearea_configure_event (Denemo.scorearea, NULL);
          //displayhelper(gui);
        }
      else if (event->state & GDK_SHIFT_MASK)
        {
          gint command_idx = lookup_command_from_name(Denemo.map, "MoveToMeasureRight");
          if (Denemo.prefs.learning) {
            KeyStrokeShow (_("Shift + Mouse Wheel Down"), command_idx, TRUE);
          }
          execute_callback_from_idx(Denemo.map, command_idx);//scroll_right ();
        }
      else
        {
          if (Denemo.prefs.learning) {
            gint command_idx = lookup_command_from_name(Denemo.map, "MoveToStaffDown");
            KeyStrokeShow (_("Unshifted + Mouse Wheel Down"), command_idx, TRUE);
          }
          movetostaffdown (NULL, &param);
          if (!param.status) {
            warningmessage ("This is the bottom staff");
           // DenemoStaff *thestaff = (DenemoStaff*)(Denemo.project->movement->currentstaff->data);
           // thestaff->space_below++; //This doesn't help, because the viewport does not change.
           // warningmessage ("Increasing the space below the bottom staff");
           //move_viewport_down(Denemo.project);
          }
        }
      break;
    case GDK_SCROLL_LEFT:
      movetomeasureleft (NULL, &param);
      if (!param.status)
        warningmessage ("This is the first measure");
      break;
    case GDK_SCROLL_RIGHT:
      movetomeasureright (NULL, &param);
      if (!param.status)
        warningmessage ("This is the last measure");
      break;

    default:
      break;
    }
  displayhelper (Denemo.project);
  return FALSE;
}
