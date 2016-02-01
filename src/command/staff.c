/**
 * staff.cpp
 * functions dealing with whole staffs
 *
 * for Denemo, a gtk+ frontend to GNU Lilypond
 * (c) 1999-2005 Matthew Hiller 
 */

#include "command/chord.h"
#include "command/contexts.h"
#include <denemo/denemo.h>
#include "ui/dialogs.h"
#include "command/measure.h"
#include "ui/moveviewport.h"
#include "command/object.h"
#include "command/processstaffname.h"
#include "command/staff.h"
#include <stdlib.h>
#include <string.h>
#include "display/calculatepositions.h"
#include "command/commandfuncs.h"
#include "command/lilydirectives.h"
#include "display/displayanimation.h"
#include "command/select.h"
#include "core/utils.h"
#include "command/lyric.h"

gboolean
signal_structural_change (DenemoProject * project)
{
  project->layout_sync = project->changecount;
  return TRUE;
}


/**
 * Return the first measure node of the given staffops
 * @param thestaff a staffnode
 * @return the first measure node of the staff
 */
measurenode *
staff_first_measure_node (staffnode * thestaff)
{
  return ((DenemoStaff *) thestaff->data)->measures;
}

/**
 * Return the nth measure of the given staff
 * @param thestaff a staffnode
 * @param n the number of the measure to return
 * @return the nth measure of the staff
 */
measurenode *
staff_nth_measure_node (staffnode * thestaff, gint n)
{
  return g_list_nth (((DenemoStaff *) thestaff->data)->measures, n);
}

/**
 * Reset movement->currentprimarystaff based on current value of
 * movement->currentstaff 
 * @param movement a scoreinfo structure
 * @return none
 */
void
staff_set_current_primary (DenemoMovement * movement)
{
  for (movement->currentprimarystaff = movement->currentstaff; movement->currentprimarystaff && (((DenemoStaff *) movement->currentprimarystaff->data)->voicecontrol & DENEMO_SECONDARY); movement->currentprimarystaff = movement->currentprimarystaff->prev)
   ;
}

/**
 * Copies the staff data from a source staff to destination staff
 * @param src the source staff
 * @param dest the destination staff
 * @return none
 */
static void
staff_copy_bits (DenemoStaff * src, DenemoStaff * dest)
{
  dest->clef.type = src->clef.type;     //other fields - take care if dynamic
  dest->keysig.number = src->keysig.number;
  dest->keysig.isminor = src->keysig.isminor;
  memcpy (dest->keysig.accs, src->keysig.accs, SEVENGINTS);
  dest->timesig.time1 = src->timesig.time1;
  dest->timesig.time2 = src->timesig.time2;
  dest->volume = 127;
  dest->no_of_lines = 5;
  dest->transposition = 0;
  dest->hide_lyrics = src->hide_lyrics;
  dest->space_above = 0;
  dest->space_below = 0;
  dest->context = DENEMO_NONE;
}

/**
 * Copies a staffs parameters from source to destination FIXME: only for a new voice - does not really copy
 * @param src the source staff
 * @param dest the destination staff
 * @return none
 */
static void
staff_copy_properties (DenemoStaff * src, DenemoStaff * dest)
{
  dest->midi_instrument = g_string_new (src->midi_instrument->str);
  dest->space_above = src->space_above;
  dest->space_shorten = src->space_shorten;
  dest->space_below = src->space_below;
  dest->no_of_lines = src->no_of_lines;
  dest->transposition = src->transposition;
  dest->hide_lyrics = src->hide_lyrics;

  dest->volume = src->volume;
  dest->voicecontrol = DENEMO_SECONDARY;
  staff_beams_and_stems_dirs (dest);
}



/* copies a staff without its music data to another staff */
void
staff_copy (DenemoStaff * src, DenemoStaff * dest)
{
  dest->denemo_name = g_string_new (src->denemo_name->str);
  dest->lily_name = g_string_new (src->lily_name->str);
  dest->midi_instrument = g_string_new (src->midi_instrument->str);
  dest->midi_channel = src->midi_channel;
  dest->midi_prognum = src->midi_prognum;
  dest->hide_lyrics = src->hide_lyrics;
  
  dest->no_of_lines = src->no_of_lines;
  dest->transposition = src->transposition;
  dest->space_above = src->space_above; /**< space above the staff used in the denemo gui */
  dest->space_shorten = src->space_shorten; /**< space by the staff is shorter in height because of few staff lines */
  dest->space_below = src->space_below; /**< space below the staff used in the denemo gui */
  dest->range = src->range;/**<TRUE if range_hi,lo should be observed. */
  dest->range_hi = src->range_hi;/**< highest note playable by instrument, mid_c_offset */
  dest->range_lo = src->range_lo;/**< lowest note playable by instrument, mid_c_offset */
  dest->volume = src->volume;
  dest->voicecontrol = src->voicecontrol;
  dest->clef.type = src->clef.type;
  dest->leftmost_clefcontext = &dest->clef;

  dest->staff_directives = clone_directives (src->staff_directives);
  dest->voice_directives = clone_directives (src->voice_directives);
  
  dest->clef.directives = clone_directives (src->clef.directives);
  dest->keysig.directives = clone_directives (src->keysig.directives);
  dest->timesig.directives = clone_directives (src->timesig.directives);


  dest->keysig.number = src->keysig.number;
  dest->keysig.isminor = src->keysig.isminor;
  memcpy (dest->keysig.accs, src->keysig.accs, SEVENGINTS);
  dest->leftmost_keysig = &dest->keysig;

  dest->timesig.time1 = src->timesig.time1;
  dest->timesig.time2 = src->timesig.time2;

  dest->transposition = src->transposition;

  dest->space_above = 0;
  dest->space_below = 0;
  dest->context = src->context;
}

/**
 * Insert a new staff into the score
 * @param movement the scoreinfo structure
 * @param staff the staff to insert
 * @param action where to insert the new staff
 * @param addat the position to insert at
 * @return none
 */
static void
staff_insert (DenemoMovement * movement, DenemoStaff * staff, enum newstaffcallbackaction action, gint addat)
{
  movement->thescore = g_list_insert (movement->thescore, staff, addat - 1);
  if (action != BEFORE)
    if (action != AFTER)
      {
        movement->currentstaff = g_list_nth (movement->thescore, addat - 1);
        movement->currentstaffnum = addat;
        staff_set_current_primary (movement);
        find_leftmost_staffcontext (staff, movement);
      }
  set_staff_transition (20);
}

/**
 * Create and insert a new staff into the score
 * @param movement the scoreinfo structure
 * @param action the staffs type / where to insert it
 * @param context the staffs contexts
 * @return the newly created staff
 */
DenemoStaff*
staff_new (DenemoProject * project, enum newstaffcallbackaction action, DenemoContext context)
{
  DenemoMovement *movement = project->movement;
  if (movement == NULL) return NULL;
  take_snapshot ();
  DenemoStaff *staff = (DenemoStaff *) g_malloc (sizeof (DenemoStaff));

  if(!Denemo.non_interactive){
    staff->staffmenu = (GtkMenu *) gtk_menu_new ();
    staff->voicemenu = (GtkMenu *) gtk_menu_new ();
  }

  measurenode *themeasures = NULL;      /* Initial set of measures in staff */
  gint numstaffs = g_list_length (movement->thescore);
  gint i, addat = 1;
  //g_debug ("newstaff: Num staffs %d", numstaffs);
  if (numstaffs == 0)
    {
      action = INITIAL;

      staff->clef.type = DENEMO_TREBLE_CLEF;
      staff->keysig.number = 0;
      staff->keysig.isminor = FALSE;
      memset (staff->keysig.accs, 0, SEVENGINTS);
      staff->timesig.time1 = 4;
      staff->timesig.time2 = 4;
      staff->volume = 127;
      staff->no_of_lines = 5;
      staff->transposition = 0;

      staff->space_above = 20;
      staff->space_below = 0;
      staff->nummeasures = 1;
      staff->midi_channel = 0;
#if 0
      movement->measurewidths = g_list_append (movement->measurewidths, GINT_TO_POINTER (movement->measurewidth));
#else
      movement->measurewidths = g_list_append (NULL, GINT_TO_POINTER (movement->measurewidth));     //FIXME free old measurewidths
#endif
    }
  else
    {
      /* how did this work before? a new staff must have the same number of measures as the present one(s) */
      staff->nummeasures = g_list_length (staff_first_measure_node (movement->thescore));
      staff_copy_bits ((DenemoStaff *) movement->currentstaff->data, staff);
      staff->midi_channel = (numstaffs < 9 ? numstaffs : numstaffs + 1) & 0xF;
    }

  if (action == NEWVOICE)
    {
      staff->voicecontrol = DENEMO_SECONDARY;
      staff->nummeasures = g_list_length (staff_first_measure_node (movement->currentstaff));        //FIXME redundant
    }
  else
    {
      staff->voicecontrol = DENEMO_PRIMARY;
    };

  for (i = 0; i < staff->nummeasures; i++)
    {
      themeasures = g_list_append (themeasures, NULL);
    };

  if (action == INITIAL || action == ADDFROMLOAD)
    {
      movement->currentmeasure = themeasures;
    }

#if 0
  // always go home for new staffs.
  else
    {
      movement->currentmeasure = g_list_nth (themeasures, movement->currentmeasurenum - 1);

    }
#endif

  /* Now fix the stuff that shouldn't be directly copied from
   * the current staff, if this staff was non-initial and that
   * was done to begin with */

  staff->measures = themeasures;
  staff->denemo_name = g_string_new ("");
  staff->lily_name = g_string_new ("");

  //staff->staff_prolog = NULL;
  // staff->lyrics_prolog = NULL;
  //staff->figures_prolog = NULL;
  //staff->fakechords_prolog = NULL;
  staff->context = context;
  if (action == INITIAL)
    g_string_sprintf (staff->denemo_name, _("Part 1"));
  else
    g_string_sprintf (staff->denemo_name, _("Part %d"), numstaffs + 1);
  set_lily_name (staff->denemo_name, staff->lily_name);
  staff->midi_instrument = g_string_new ("");
  staff->device_port = g_string_new ("NONE");
  staff->leftmost_timesig = &staff->timesig;

  /* In what position should the scrollbar be added?  */
  switch (action)
    {
    case INITIAL:
      addat = 1;
      break;
    case LAST:
    case ADDFROMLOAD:
      addat = numstaffs + 1;
      break;
    case BEFORE:
      addat = movement->currentstaffnum;
      break;
    case AFTER:
    case NEWVOICE:
    case LYRICSTAFF:
    case FIGURESTAFF:
      addat = movement->currentstaffnum + 1;
      break;
    case CHORDSTAFF:
      addat = movement->currentstaffnum + 1;
      break;
    case FIRST:
      addat = 1;
      break;
    default:
      break;
    }

  if (action != INITIAL && action != ADDFROMLOAD)
    {
      if (action == NEWVOICE)
        {
          staff_copy_properties ((DenemoStaff *) movement->currentstaff->data, staff);
          set_lily_name (staff->denemo_name, staff->lily_name);       //this should be re-done if the denemo_name is reset.
          staff_insert (movement, staff, action, addat);
        }
      else
        {
          //      ret = staff_properties_change (&itp);
          //      if (ret)
          {
            /* 
               If staff_properties_change returns false,
               then the staff should probably be removed
               Fixed 09042005 Adam Tee 
             */
            staff_insert (movement, staff, action, addat);
            movement->currentmeasurenum = 1;
            /*
               Reset leftmeasure num to 1 to be at the start of 
               the next staff. 
             */
            movement->leftmeasurenum = 1;
          }
          //    else
          //      {
          /*
           *  Free the staff struct as it has not been inserted 
           *  into the score
           */
          //     g_free (staff);
          //  }
        }
    }
  else                          // is INITIAL or ADDFROMLOAD
    {
      staff_insert (movement, staff, action, addat);
      movement->leftmeasurenum = 1;
    }
  if ( addat==1)
       staff->space_above = 20;

  return staff;
}

/**
 * Remove the project->movement->currentstaff from the piece project and reset movement->currentstaff
 * if only one staff, inserts a new empty one
 * if interactive checks for custom_scoreblock
 * if a staff is deleted, updates the changecount
 * @param project the DenemoProject structure
 * @return nothing
 */
void
staff_delete (DenemoProject * project, gboolean interactive)
{
  DenemoMovement *movement = project->movement;
  DenemoStaff *curstaffstruct = movement->currentstaff->data;
  gboolean has_next = (movement->currentstaff->next != NULL);
  (void) signal_structural_change (project);
  if (movement->currentstaff == NULL)
    return;

  gboolean give_info = FALSE;   //give info about removing matching context
  if (interactive && (curstaffstruct->context != DENEMO_NONE) && (!confirm (_("A context is set on this staff"), _("You will need to alter/delete the matching staff; Proceed?"))))
    return;
  if (interactive && (curstaffstruct->context != DENEMO_NONE))
    give_info = TRUE;
  take_snapshot ();
  gboolean isprimary = (gboolean) (curstaffstruct->voicecontrol & DENEMO_PRIMARY);
  //FIXME free_staff()

  free_directives (curstaffstruct->staff_directives);
  free_directives (curstaffstruct->timesig.directives);
  free_directives (curstaffstruct->keysig.directives);

  if(!Denemo.non_interactive){
    gtk_widget_destroy ((GtkWidget *) (curstaffstruct->staffmenu));
    gtk_widget_destroy ((GtkWidget *) (curstaffstruct->voicemenu));
  }

  g_list_foreach (curstaffstruct->measures, freeobjlist, NULL);
  g_list_free (curstaffstruct->measures);
  g_string_free (curstaffstruct->denemo_name, FALSE);   //FIXME these should all be TRUE??
  g_string_free (curstaffstruct->lily_name, FALSE);
  g_string_free (curstaffstruct->midi_instrument, FALSE);
  // g_list_foreach (curstaffstruct->verse_views, (GFunc)destroy_parent, NULL);//FIXME it is enough to destroy the notebook, here we are only destroying the GtkTextViews
  if (curstaffstruct->verse_views)
    gtk_widget_destroy (gtk_widget_get_parent (gtk_widget_get_parent (curstaffstruct->verse_views->data)));

  g_free (curstaffstruct);


  if (movement->currentstaff == g_list_last (movement->thescore))
    movement->currentstaffnum--;      //deleting the last, so the currentstaffnum must decrease
  else
    set_staff_transition (20);
  movement->thescore = g_list_delete_link (movement->thescore, movement->currentstaff);
  if (movement->thescore == NULL)
    {
      staff_new (project, INITIAL, DENEMO_NONE);
    }
  movement->currentstaff = g_list_nth (movement->thescore, movement->currentstaffnum - 1);


  if (isprimary && has_next)                // we deleted the primary, so the next one (which is present) must become the primary
    {
      ((DenemoStaff *) movement->currentstaff->data)->voicecontrol = DENEMO_PRIMARY;
      movement->currentprimarystaff = movement->currentstaff;
    }
  else
    {
      staff_set_current_primary (movement);
    }
  setcurrents (movement);
  if (movement->markstaffnum)
    calcmarkboundaries (movement);
  if (project->movement->currentstaffnum < project->movement->top_staff)
    project->movement->top_staff = project->movement->currentstaffnum;
  show_lyrics ();

  if(!Denemo.non_interactive){
    update_vscrollbar (project);
    displayhelper (project);
    score_status (project, TRUE);
  }
  
  if (give_info)
    infodialog (_("The staff deleted had a start/end context; if you still have the staff with the matching end/start context\n then you should remove it (or its context) now.\nSee Staff->properties->context\nYou will not be able to print with miss-matched contexts."));
  return;
}

/**
 * Sets the beams and stem directions across the given staff
 * @param thestaff a staff structure
 * @return none
 */
void
staff_beams_and_stems_dirs (DenemoStaff * thestaff)
{
  measurenode *curmeasure;
  gint nclef, time1, time2, stem_directive;

  nclef = thestaff->clef.type;
  time1 = thestaff->timesig.time1;
  time2 = thestaff->timesig.time2;
  stem_directive = DENEMO_STEMBOTH;

  for (curmeasure = thestaff->measures; curmeasure; curmeasure = curmeasure->next)
    {
      calculatebeamsandstemdirs ((objnode *) curmeasure->data, &nclef, &time1, &time2, &stem_directive);
    }
}

/**
 * Sets which accidentals to show across a staff on a key sig change
 * @param thestaff a staff stucture
 * @return none
 */
void
staff_show_which_accidentals (DenemoStaff * thestaff)
{
  gint feed[7];
  gint feednum;
  measurenode *curmeasure;

  memcpy (feed, thestaff->keysig.accs, SEVENGINTS);
  feednum = thestaff->keysig.number;
  for (curmeasure = thestaff->measures; curmeasure; curmeasure = curmeasure->next)
    feednum = showwhichaccidentals ((objnode *) curmeasure->data, feednum, feed);
}

/**
 * Function to set the note positions on the given staff when there is a clef change
 * @param thestaff a staff structure
 * @return none 
 */
void
staff_fix_note_heights (DenemoStaff * thestaff)
{
  gint nclef = thestaff->clef.type;
  //gint time1 = thestaff->stime1;//USELESS
  //gint time2 = thestaff->stime2;//USELESS
  //gint initialclef;//USELESS
  measurenode *curmeasure;
  objnode *curobj;
  DenemoObject *theobj;

  for (curmeasure = thestaff->measures; curmeasure; curmeasure = curmeasure->next)
    {
      //initialclef = nclef;
      for (curobj = (objnode *) curmeasure->data; curobj; curobj = curobj->next)
        {
          theobj = (DenemoObject *) curobj->data;
          switch (theobj->type)
            {
            case CHORD:
              newclefify (theobj, nclef);
              break;
            case TIMESIG:      //USELESS
              //time1 = ((timesig *) theobj->object)->time1;
              // time2 = ((timesig *) theobj->object)->time2;
              break;
            case CLEF:
              nclef = ((clef *) theobj->object)->type;
              break;
            default:
              break;
            }
        }                       /* End for */
    }                           /* End for */
  staff_beams_and_stems_dirs (thestaff);
}

/**
 * Callback function to insert a staff in the initial position
 * @param action a Gtk Action
 * @param project the DenemoProject structure
 * @return none
 */
void
staff_new_initial (GtkAction * action, DenemoScriptParam * param)
{
  DenemoProject *project = Denemo.project;
  while (project->movement->currentstaff && project->movement->currentstaff->prev)
    movetostaffup (NULL, NULL);
  staff_new_before (action, NULL);
}

/**
 Callback function to insert a staff before the current staff
 * @param action a Gtk Action
 * @param project the DenemoProject structure
 * @return none
 */
void
staff_new_before (GtkAction * action, DenemoScriptParam * param)
{
  DenemoProject *project = Denemo.project;
  (void) signal_structural_change (project);

  movetostart (NULL, NULL);
  staff_new (project, BEFORE, DENEMO_NONE);
  if (project->movement->currentstaffnum >= project->movement->top_staff)
    project->movement->top_staff++;
  project->movement->currentstaffnum++;
  project->movement->bottom_staff++;
  set_bottom_staff (project);
  move_viewport_down (project);

  movetostaffup (NULL, NULL);
  displayhelper (project);

}

/**
 * Callback function to insert a staff after the current staff
 * @param action a Gtk Action
 * @param project the DenemoProject structure
 * @return none
 */
void
staff_new_after (GtkAction * action, DenemoScriptParam * param)
{
  DenemoProject *project = Denemo.project;
  if (!signal_structural_change (project))
    return;
  movetostart (NULL, NULL);
  staff_new (project, AFTER, DENEMO_NONE);
  if(!Denemo.non_interactive){
    set_bottom_staff (project);
    update_vscrollbar (project);
  }
  movetostaffdown (NULL, NULL);
  displayhelper (project);
}

/**
 * Callback function to insert a staff at the bottom of the score
 * @param action a Gtk Action
 * @param project the DenemoProject structure
 * @return none
 */
void
staff_new_last (GtkAction * action, DenemoScriptParam * param)
{
  DenemoProject *project = Denemo.project;
  while (project->movement->currentstaff && project->movement->currentstaff->next)
    movetostaffdown (NULL, NULL);
  staff_new_after (action, param);
}

/**
 * Callback function to add a new voice to the current staff
 * @param action a Gtk Action
 * @param project the DenemoProject structure
 * @return none
 */
void
staff_new_voice (GtkAction * action, DenemoScriptParam * param)
{
  DenemoProject *project = Denemo.project;
  staff_new (project, NEWVOICE, DENEMO_NONE);
  set_bottom_staff (project);
  update_vscrollbar (project);
  setcurrents (project->movement);
  if (project->movement->markstaffnum)
    calcmarkboundaries (project->movement);
  displayhelper (project);
}
