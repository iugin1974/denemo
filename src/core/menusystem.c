/*
 * menusystem.c
 * 
 * Copyright 2016 Richard Shann <richard@rshann.plus.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */


#include <denemo/denemo.h>
#include <glib/gstdio.h> //for g_access. Use GFile instead?
#include "core/view.h"
#include "core/utils.h"
#include "core/keymapio.h"
#include "core/menusystem.h"
#include "core/keyboard.h"

#include "command/commandfuncs.h"
#include "command/select.h"
#include "command/grace.h"
#include "command/lilydirectives.h"
#include "command/scorelayout.h"

#include "audio/pitchentry.h"
#include "audio/playback.h"
#include "audio/audiointerface.h"
#include "scripting/scheme-callbacks.h"
#include "printview/printview.h"

#include "ui/texteditors.h"


DenemoAction DummyAction; //this dummy parameter is passed to built-in callbacks to distinguish them from callbacks via scripts which have a NULL parameter (for historical reasons).
typedef void (*BuiltInCallback)(gpointer action, gpointer param);
typedef struct MenuEntry {gchar *name; gchar *dummy; gchar *label; gchar *dummy2; gchar *tooltip; gpointer callback;} MenuEntry;


static MenuEntry menu_entries[] = {
#include "entries.h"
};


GHashTable *ActionWidgets, *Actions;




static void
show_type (GtkWidget * widget, gchar * message)
{
  g_message ("%s%s", message, widget ? g_type_name (G_TYPE_FROM_INSTANCE (widget)) : "NULL widget");
}


/**
 *  callback changing the input source (keyboard only/audio/midi)
 *
 */

static void
change_input_type (GtkCheckMenuItem * item, gint val)
{
  DenemoProject *project = Denemo.project;
  if(!gtk_check_menu_item_get_active(item))
    return;
 

  switch (val)
    {
    case INPUTKEYBOARD:
      if (project->input_source == INPUTAUDIO)
        {
          //g_debug("Stopping audio\n");
          stop_pitch_input ();
        }
      if (project->input_source == INPUTMIDI)
        {
          g_print("Stopping midi\n");
          Denemo.keyboard_state = GDK_SHIFT_MASK;
          Denemo.keyboard_state_locked = TRUE;
        }
      project->input_source = INPUTKEYBOARD;
      Denemo.project->last_source = INPUTKEYBOARD;
      g_print ("Input keyboard %d", Denemo.project->last_source);
      infodialog (_("Rhythms will be entered as notes at the cursor height"));
      break;
    case INPUTAUDIO:
      g_print("Starting audio\n");
      if (project->input_source == INPUTMIDI)
        {
          g_print("Stopping midi\n");
          stop_pitch_input ();
        }
      project->input_source = INPUTAUDIO;
      if (setup_pitch_input ())
        {
          project->input_source = INPUTKEYBOARD;
          warningdialog (_("Could not start Audio input"));
          //gtk_radio_action_set_current_value (current, INPUTKEYBOARD);
        }
      else
        start_pitch_input ();
      break;
    case INPUTMIDI:
      if (have_midi ())
        {
            Denemo.keyboard_state = 0;
            Denemo.keyboard_state_locked = FALSE;
            project->input_source = INPUTMIDI;
            infodialog (_("Rhythms will be entered as (brown) notes without pitch"));
        }
      else
        {
            warningdialog (_("No MIDI in device was found on startup- re-start Denemo with the device plugged in, select the device in the MIDI tab of the preferences dialog and re-start Denemo again."));
        }
      g_print ("Input source %d\n", project->input_source);
      break;
    default:
      g_warning ("Bad Value");
      break;
    }

    write_input_status ();
}

static void
placeInPalette (GtkWidget * widget, DenemoAction * action)
{
  gchar *name = (gchar *) denemo_action_get_name (action);
  gint idx = lookup_command_from_name (Denemo.map, name);
  if (idx > 0)
    place_action_in_palette (idx, name);
}



static void
configure_keyboard_idx (GtkWidget * w, gint idx)
{
  command_center_select_idx (NULL, idx);
}

/* return a directory path for a system menu ending in menupath, or NULL if none exists
   checking user's download then the installed menus
   user must free the returned string*/
static gchar *
get_system_menupath (gchar * menupath)
{
  gchar *filepath = g_build_filename (get_user_data_dir (TRUE), "download", COMMANDS_DIR, "menus", menupath, NULL);
  //g_debug("No file %s\n", filepath);
  if (0 != g_access (filepath, 4))
    {
      g_free (filepath);
      filepath = g_build_filename (get_system_data_dir (), COMMANDS_DIR, "menus", menupath, NULL);
    }
  return filepath;
}


static void
appendSchemeText_cb (GtkWidget * widget, gchar * text)
{
  gboolean sensitive = gtk_widget_get_visible (gtk_widget_get_toplevel (Denemo.script_view));
  appendSchemeText (text);
  if (!sensitive)
    toggle_scheme ();//activate_action ("/MainMenu/ViewMenu/ToggleScript");
}

/* save the action (which must be a script),
   setting the script text to the script currently in the script_view
   The save is to the user's menu hierarchy on disk
*/
static void
saveMenuItem (GtkWidget * widget, DenemoAction * action)
{
  gchar *name = (gchar *) denemo_action_get_name (action);
  gint idx = lookup_command_from_name (Denemo.map, name);

  command_row *row = NULL;
  keymap_get_command_row (Denemo.map, &row, idx);
  if (!row) return;
  gchar *tooltip = (gchar *) lookup_tooltip_from_idx (Denemo.map, idx);
  gchar *label = (gchar *) lookup_label_from_idx (Denemo.map, idx);

  gchar *xml_filename = g_strconcat (name, XML_EXT, NULL);
  gchar *xml_path = g_build_filename (get_user_data_dir (TRUE), COMMANDS_DIR, "menus", row->menupath, xml_filename, NULL);
  g_free (xml_filename);

  gchar *scm_filename = g_strconcat (name, SCM_EXT, NULL);
  gchar *scm_path = g_build_filename (get_user_data_dir (TRUE), COMMANDS_DIR, "menus", row->menupath, scm_filename, NULL);
  g_free (scm_filename);

  gchar *scheme = get_script_view_text ();
  if (scheme && *scheme && confirm (_("Save Script"), g_strconcat (_("Over-write previous version of the script for "), name, _(" ?"), NULL)))
    {
      gchar *dirpath = g_path_get_dirname (xml_path);
      g_mkdir_with_parents (dirpath, 0770);
      g_free (dirpath);
      save_command_metadata (xml_filename, name, label, tooltip, row->after);
      save_command_data (scm_path, scheme);
      row->scheme = NULL;
    }
  else
    warningdialog (_("No script saved"));
}
/* write scheme script from Denemo.script_view into file init.scm in the user's local menupath.
*/
static void
put_initialization_script (GtkWidget * widget, gchar * directory)
{
  gchar *filename = g_build_filename (get_user_data_dir (TRUE), COMMANDS_DIR, "menus", directory, INIT_SCM, NULL);
  if ((!g_file_test (filename, G_FILE_TEST_EXISTS)) || confirm (_("There is already an initialization script here"), _("Do you want to replace it?")))
    {
      gchar *scheme = get_script_view_text ();
      if (scheme && *scheme)
        {
          FILE *fp = fopen (filename, "w");
          if (fp)
            {
              fprintf (fp, "%s", scheme);
              fclose (fp);
              if (confirm (_("Wrote init.scm"), _("Shall I execute it now?")))
                call_out_to_guile (scheme);
            }
          else
            {
              warningdialog (_("Could not create init.scm;\n" "you must create your scripted menu item in the menu\n" "before you create the initialization script for it, sorry."));
            }
          g_free (scheme);
        }
    }
}

static void find_directives_created (GtkWidget * widget, gchar *name)
{
  gchar *thescript = g_strdup_printf ("(DirectivesFromCommand \"%s\")", name);
  call_out_to_guile (thescript);
  g_free (thescript);
}


/* replace dangerous characters in command names */
static void
subst_illegals (gchar * myname)
{
  gchar *c;                     // avoid whitespace etc
  for (c = myname; *c; c++)
    if (*c == ' ' || *c == '\t' || *c == '\n' || *c == '/' || *c == '\\')
      *c = '-';
}


/* gets a name label and tooltip from the user, then creates a menuitem in the menu
   given by the path myposition whose callback is the activate on the current scheme script.
*/

static void
insertScript (GtkWidget * widget, gchar * insertion_point)
{
  DenemoProject *project = Denemo.project;
  gchar *myname, *mylabel, *myscheme, *mytooltip, *submenu;
  gchar *myposition = g_path_get_dirname (insertion_point);
  gchar *after = g_path_get_basename (insertion_point);
  gint idx = lookup_command_from_name (Denemo.map, after);
  myname = string_dialog_entry (project, "Create a new menu item", "Give item name (avoid clashes): ", "MyName");
  //FIXME check for name clashes

  if (myname == NULL)
    return;
  subst_illegals (myname);
  mylabel = string_dialog_entry (project, _("Create a new menu item"), _("Give menu label: "), _("My Label"));
  if (mylabel == NULL)
    return;

  mytooltip = string_dialog_entry (project, _("Create a new menu item"), _("Give explanation of what it does: "), _("Prints my special effect"));
  if (mytooltip == NULL)
    return;
  if (confirm (_("Create a new menu item"), _("Do you want the new menu item in a submenu?")))
    {
      submenu = string_dialog_entry (project, _("Create a new menu item"), _("Give a label for the Sub-Menu"), _("Sub Menu Label"));
      if (submenu)
        {
          subst_illegals (submenu);
          myposition = g_strdup_printf ("%s/%s", myposition, submenu);  //FIXME G_DIR_SEPARATOR in myposition???
        }
    }

  myscheme = get_script_view_text ();

  gchar *xml_filename = g_strconcat (myname, XML_EXT, NULL);
  gchar *scm_filename = g_strconcat (myname, SCM_EXT, NULL);
  g_info ("The filename built is %s from %s", xml_filename, myposition);
  gchar *xml_path = g_build_filename (get_user_data_dir (TRUE), COMMANDS_DIR, "menus", myposition, xml_filename, NULL);
  gchar *scm_path = g_build_filename (get_user_data_dir (TRUE), COMMANDS_DIR, "menus", myposition, scm_filename, NULL);
  g_free (xml_filename);
  if ((!g_file_test (xml_path, G_FILE_TEST_EXISTS)) || (g_file_test (xml_path, G_FILE_TEST_EXISTS) && confirm (_("Duplicate Name"), _("A command of this name is already available in your custom menus; Overwrite?"))))
    {
      gchar *dirpath = g_path_get_dirname (xml_path);
      g_mkdir_with_parents (dirpath, 0770);
      g_free (dirpath);
      //g_file_set_contents(xml_path, text, -1, NULL);
      save_command_metadata (xml_path, myname, mylabel, mytooltip, idx < 0 ? NULL : after);
      save_command_data (scm_path, myscheme);
      load_commands_from_xml (xml_path);//g_print ("Loading from %s\n", xml_path);

      if (confirm (_("New Command Added"), _("Do you want to save this with your default commands?")))
        save_accels ();
    }
  else
    warningdialog (_("Operation cancelled"));
  g_free (myposition);
  return;
}

/* get init.scm for the current path into the scheme text editor.
*/
static void
get_initialization_script (GtkWidget * widget, gchar * directory)
{
  GError *error = NULL;
  gchar *script;
  g_debug ("Loading %s/init.scm into Denemo.script_view", directory);

  GList *dirs = NULL;
  dirs = g_list_append (dirs, g_build_filename (get_user_data_dir (TRUE), COMMANDS_DIR, "menus", directory, NULL));
  dirs = g_list_append (dirs, g_build_filename (get_user_data_dir (TRUE), "download", COMMANDS_DIR, "menus", directory, NULL));
  dirs = g_list_append (dirs, g_build_filename (get_system_data_dir (), COMMANDS_DIR, "menus", directory, NULL));
  gchar *filename = find_path_for_file (INIT_SCM, dirs);

  if (!filename)
    {
      g_warning ("Could not find scm initialization file");
      return;
    }

  if (g_file_get_contents (filename, &script, NULL, &error))
    appendSchemeText (script);
  else
    g_warning ("Could not get contents of %s", filename);
  g_free (script);
  g_free (filename);
}

/**
 *  Function to toggle whether rhythm toolbar is visible
 *  (no longer switches keymap to Rhythm.keymaprc when toolbar is on back to standard when off.)
 *
 */
static void
toggle_rhythm_toolbar (DenemoAction * action, gpointer param)
{
  GtkWidget *widget;
  widget = denemo_menusystem_get_widget ("/RhythmToolBar");
  //g_debug("Callback for %s\n", g_type_name(G_TYPE_FROM_INSTANCE(widget)));
  if ((!action) || gtk_widget_get_visible (widget))
    {

      gtk_widget_hide (widget);
    }
  else
    {

      gtk_widget_show (widget);
      /* make sure we are in Insert and Note for rhythm toolbar */
      // activate_action( "/MainMenu/ModeMenu/Note");
      //activate_action( "/MainMenu/ModeMenu/InsertMode");
    }
  if (Denemo.prefs.persistence && (Denemo.project->view == DENEMO_MENU_VIEW))
    Denemo.prefs.rhythm_palette = gtk_widget_get_visible (widget);
  write_status (Denemo.project);
}

/**
 *  Function to toggle whether main toolbar is visible
 *
 *
 */
void
toggle_toolbar (DenemoAction * action, gpointer param)
{
  GtkWidget *widget;
  widget = denemo_menusystem_get_widget ("/ToolBar");
  if ((!action) || gtk_widget_get_visible (widget))
    gtk_widget_hide (widget);
  else
    gtk_widget_show_all (widget);
  if (Denemo.prefs.persistence && (Denemo.project->view == DENEMO_MENU_VIEW))
    Denemo.prefs.toolbar = gtk_widget_get_visible (widget);
  write_status (Denemo.project);
}

/**
 *  Function to toggle whether playback toolbar is visible
 *
 *
 */
void
toggle_playback_controls (DenemoAction * action, gpointer param)
{
  GtkWidget *widget;
  widget = Denemo.playback_control;
  if ((!action) || gtk_widget_get_visible (widget))
    gtk_widget_hide (widget);
  else
    gtk_widget_show (widget);
  if (Denemo.prefs.persistence && (Denemo.project->view == DENEMO_MENU_VIEW))
    Denemo.prefs.playback_controls = gtk_widget_get_visible (widget);
  write_status (Denemo.project);
}

/**
 *  Function to toggle whether playback toolbar is visible
 *
 *
 */
void
toggle_midi_in_controls (DenemoAction * action, gpointer param)
{
  GtkWidget *widget;
  widget = Denemo.midi_in_control;
  if ((!action) || gtk_widget_get_visible (widget))
    gtk_widget_hide (widget);
  else
    gtk_widget_show (widget);
  if (Denemo.prefs.persistence && (Denemo.project->view == DENEMO_MENU_VIEW))
    Denemo.prefs.midi_in_controls = gtk_widget_get_visible (widget);
  write_status (Denemo.project);
}

#if 0
/**
 *  Function to toggle whether keyboard bindings can be set by pressing key over menu item
 *
 *
 */
static void
toggle_quick_edits (DenemoAction * action, gpointer param)
{
 //Denemo.prefs.quickshortcuts = !Denemo.prefs.quickshortcuts;
 
 get_widget (Toggle_Quick.... this is silly - this is not a viewable item.
}
#endif
/**
 *  Function to toggle visibility of print preview pane of current project
 *
 *
 */
static void
toggle_print_view (DenemoAction * action, gpointer param)
{
  if (Denemo.non_interactive)
    return;
#ifndef USE_EVINCE
  g_debug ("This feature requires denemo to be built with evince");
#else
  GtkWidget *w = gtk_widget_get_toplevel (Denemo.printarea);
  if ((!action) || gtk_widget_get_visible (w))
    gtk_widget_hide (w);
  else
    {
      // gtk_widget_show (w);
      //if (GPOINTER_TO_INT (g_object_get_data (G_OBJECT (Denemo.printarea), "printviewupdate")) < Denemo.project->changecount)
      //  refresh_print_view (TRUE);
      implement_show_print_view (TRUE);
    }
#endif
write_status (Denemo.project);
}

/**
 *  Function to toggle visibility of playback view pane of current project
 *
 *
 */
static void
toggle_playback_view (DenemoAction * action, gpointer param)
{
  if (Denemo.non_interactive)
    return;
  GtkWidget *w = gtk_widget_get_toplevel (Denemo.playbackview);
  if (gtk_widget_get_visible (w))
    gtk_widget_hide (w);
  else
    {
	 gtk_widget_show (w);
    }
write_status (Denemo.project);
}

/**
 *  Function to toggle visibility of score layout window of current project
 *
 *
 */
static void
toggle_score_layout (DenemoAction * action, gpointer param)
{

  DenemoProject *project = Denemo.project;
  GtkWidget *w = project->score_layout;
  GList *g = gtk_container_get_children (GTK_CONTAINER (w));
  if (g == NULL)
    {
      create_default_scoreblock ();
    }
  if ((!action) || gtk_widget_get_visible (w))
    gtk_widget_hide (w);
  else
    {
      gtk_widget_show (w);
    }
write_status (Denemo.project);
}


/**
 *  Function to toggle visibility of command manager window
 *
 *
 */
static void
toggle_command_manager (DenemoAction * action, gpointer param)
{
  if (Denemo.command_manager == NULL)
    {
      configure_keyboard_dialog (action, NULL);
      gtk_widget_show (Denemo.command_manager);
    }
  else
    {
      GtkWidget *w = Denemo.command_manager;
      
      if ((!action) || gtk_widget_get_visible (w))
        gtk_widget_hide (w);
      else
        gtk_widget_show (w);
    }
write_status (Denemo.project);
}

/**
 *  show the vbox containing the verses notebook for the current movement.
 *
 *
 */
void
show_verses (void)
{
  GtkWidget *widget = Denemo.project->movement->lyricsbox;
  static gint last_height = 100;
  if (!widget)
    g_warning ("No lyrics");
  else
    {
      //if ((!action) || gtk_widget_get_visible (widget))
        //{
          //GtkWidget *parent = gtk_widget_get_parent (gtk_widget_get_parent (widget));
          //gint height = get_widget_height (parent);
          //last_height = get_widget_height (widget);
          //gtk_paned_set_position (GTK_PANED (parent), height);
          //gtk_widget_hide (widget);
        //}
      //else
        {
          gtk_widget_show (widget);
          GtkWidget *parent = gtk_widget_get_parent (gtk_widget_get_parent (widget));
          gint height = get_widget_height (parent);
          if ((height > last_height))
            gtk_paned_set_position (GTK_PANED (parent), height - last_height);
        }
     // if (Denemo.prefs.persistence && (Denemo.project->view == DENEMO_MENU_VIEW))
     //   Denemo.prefs.lyrics_pane = gtk_widget_get_visible (widget);
    }
}

/**
 *  Function to toggle whether object menubar is visible
 *
 *
 */
static void
toggle_object_menu (DenemoAction * action, gpointer param)
{
  GtkWidget *widget;
  widget = denemo_menusystem_get_widget ("/ObjectMenu");
  if (!widget)
    return;                     // internal error - out of step with menu_entries...
  if (gtk_widget_get_visible (widget))
    {
      gtk_widget_hide (widget);
    }
  else
    {
      gtk_widget_show (widget);
    }
}


/**
 *  Function to toggle the visibility of the LilyPond text window. It refreshes
 *  the text if needed
 */
static void
toggle_lilytext (DenemoAction * action, gpointer param)
{
  DenemoProject *project = Denemo.project;
  //if(!project->textview)
  refresh_lily_cb (action, project);
  if (!gtk_widget_get_visible (Denemo.textwindow))
    gtk_widget_show /*_all*/ (Denemo.textwindow);
  else
    gtk_widget_hide (Denemo.textwindow);
 write_status (Denemo.project);
}


/**
 *  Function to toggle the visibility of the Scheme text window.
 */
void
toggle_scheme (void)
{
  GtkWidget *widget = gtk_widget_get_toplevel (Denemo.script_view);
 // set_toggle (ToggleScript_STRING, !gtk_widget_get_visible (textwindow));
  if (gtk_widget_get_visible (widget))
    {
      gtk_widget_hide (widget);
    }
  else
    {
      gtk_widget_show (widget);
    }
write_status (Denemo.project);
}

//toggle_rhythm_mode

/**
 *  Function to toggle visibility of score area (display) pane of current project
 *
 *
 */
static void
toggle_score_view (DenemoAction * action, gpointer param)
{
  GtkWidget *w = gtk_widget_get_parent (gtk_widget_get_parent (Denemo.scorearea));
  if ((!action) || gtk_widget_get_visible (w))
    gtk_widget_hide (w);
  else
    {
      gtk_widget_show (w);
      gtk_widget_grab_focus (Denemo.scorearea);
    }
}

/**
 *  Function to toggle visibility of titles etc of current project
 *
 *
 */
static void
toggle_scoretitles (void)
{

  GtkWidget *widget = Denemo.project->buttonboxes;
  if (gtk_widget_get_visible (widget))
    gtk_widget_hide (widget);
  else
    gtk_widget_show (widget);
  if (Denemo.prefs.persistence && (Denemo.project->view == DENEMO_MENU_VIEW))
    Denemo.prefs.visible_directive_buttons = gtk_widget_get_visible (widget);

}




typedef struct ToggleMenuEntry {gchar *name; GtkWidget *item; gchar *label; gchar *dummy; gchar *tooltip; gpointer callback; gboolean initial;} ToggleMenuEntry;


/**
 * Toggle entries for the menus
 */
ToggleMenuEntry toggle_menu_entries[] = {
    {TogglePrintView_STRING, NULL, N_("Typeset Music"), NULL, N_("Shows the Print View\nwith the music typeset by the LilyPond Music Typesetter."),
   G_CALLBACK (toggle_print_view), FALSE}
  , 
    


  {ToggleCommandManager_STRING, NULL, N_("Command Center"), NULL, N_("Shows a searchable list of all commands, enables setting of keyboard short-cuts, etc."),
   G_CALLBACK (toggle_command_manager), FALSE}
  ,
  
 

  {ToggleScoreLayout_STRING, NULL, N_("Score Layout"), NULL, N_("Shows an overview of the score where various elements can be rearranged, deleted etc. to form a customized layout"),
   G_CALLBACK (toggle_score_layout), FALSE}
  ,


  
   {ToggleRhythmToolbar_STRING, NULL, N_("Snippets"), NULL, N_("Show/hide a toolbar which allows\nyou to store and enter snippets of music and to enter notes using rhythm pattern of a snippet"),
   G_CALLBACK (toggle_rhythm_toolbar), FALSE}
  ,
      
    
  {ToggleToolbar_STRING, NULL, N_("Tools"), NULL, N_("Show/hide a toolbar for general operations on music files"),
   G_CALLBACK (toggle_toolbar), FALSE}
  ,
  {TogglePlaybackControls_STRING, NULL, N_("Playback Control"), NULL, N_("Show/hide playback controls"),
   G_CALLBACK (toggle_playback_controls), FALSE}
  ,
  {ToggleMidiInControls_STRING, NULL, N_("Midi In Control"), NULL, N_("Show/hide Midi Input controls"),
   G_CALLBACK (toggle_midi_in_controls), FALSE}
  ,
  {TogglePlaybackView_STRING, NULL, N_("Playback View"), NULL, N_("Shows the PlayBack View from which a more sophisticated playback of the music is possible"),
   G_CALLBACK (toggle_playback_view), FALSE}
  ,    

  {ToggleScoreTitles_STRING, NULL, N_("Titles, Buttons etc"), NULL, N_("Shows a bar holding the title etc of the music and buttons for selecting a movement to make currrent."),
   G_CALLBACK (toggle_scoretitles), FALSE}
  ,

  {ToggleObjectMenu_STRING, NULL, N_("Object Menu"), NULL, N_("Show/hide a menu which is arranged by objects\nThe actions available for note objects change with the mode"),
   G_CALLBACK (toggle_object_menu), FALSE}
  ,
  {ToggleLilyText_STRING, NULL, N_("LilyPond"), NULL, N_("Show/hide the LilyPond music typesetting language window. Any errors in typesetting are shown here."),
   G_CALLBACK (toggle_lilytext), FALSE}
  ,
  {ToggleScript_STRING, NULL, N_("Scheme Script"), NULL, N_("Show Scheme script window. Sequences of commands can be recorded here\nand then executed or turned into new commaneds."),
   G_CALLBACK (toggle_scheme), FALSE}
  ,

 


  {ToggleScoreView_STRING, NULL, N_("Score"), NULL, N_("Shows/hides the music in the Denemo Display"),
   G_CALLBACK (toggle_score_view), FALSE}
 
};


typedef struct RadioMenuEntry {gchar *name; GtkWidget *item; gchar *label; gchar *dummy; gchar *tooltip;  input_mode val;} RadioMenuEntry;

static RadioMenuEntry input_menu_entries[] = {
  {"KeyboardOnly", NULL, N_("No External Input"), NULL, N_("Entry of notes via computer keyboard only\nIgnores connected MIDI or microphone devices."),
    INPUTKEYBOARD},
  {"JackMidi", NULL, N_("Midi Input"), NULL, N_("Input from a MIDI source. Set up the source first using Edit → Change Preferences → Audio/Midi\nUse View → MIDI In Control to control what the input does.\n"), 
    INPUTMIDI},
  {"Microphone", NULL, N_("Audio Input"), NULL, N_("Enable pitch entry from microphone"), 
    INPUTAUDIO }
  };

static void
load_command_from_location (GtkWidget * w, gchar * filepath)
{
  gchar *location = g_strdup_printf ("%s%c", filepath, G_DIR_SEPARATOR);
  g_info ("Calling the file loader with %s", location);
  load_keymap_dialog_location (location);
  g_free (location);
}


/*
  menu_click:
  intercepter for the callback when clicking on menu items for the set of Actions the Denemo offers.
  Left click runs default action, after recording the item in a scheme script if recording.
  Right click offers pop-up menu for setting shortcuts etc

*/
static gboolean
menu_click (GtkWidget * widget, GdkEventButton * event, DenemoAction * action)
{
  keymap *the_keymap = Denemo.map;
  const gchar *func_name = denemo_action_get_name (action);

  //g_print("In menu click action is %p name is %s button %d\n",action, func_name, event->button);

  gint idx = lookup_command_from_name (the_keymap, func_name);
  Denemo.LastCommandId = idx;
  command_row *row = NULL;
  keymap_get_command_row (the_keymap, &row, idx);
  if (!row)
    return TRUE;//stop other handlers running
  //g_debug("event button %d, idx %d for %s recording = %d scm = %d\n", event->button, idx, func_name, Denemo.ScriptRecording,g_object_get_data(G_OBJECT(action), "scm") );
  if (event->button != 3)       //Not right click
    {
      if (Denemo.ScriptRecording)
        {
          append_scheme_call ((gchar *) func_name);
        }
      gchar *trunc = g_strdup (lookup_label_from_idx (Denemo.map, Denemo.LastCommandId));  
      truncate_label (NULL, trunc);
      gchar *c = trunc;
      for(;*c;c++) if (*c=='&') *c='+';//ampsersand not allowed in markup
      g_string_printf (Denemo.input_filters, "%s  <span foreground=\"Green\">%s.</span> <span foreground=\"blue\">%s</span>", trunc, _("Help"), _("(use Fn12 to Repeat)"));
      write_input_status();
      g_free (trunc);
     return FALSE;
   }

  gboolean sensitive = gtk_widget_get_visible (gtk_widget_get_toplevel (Denemo.script_view));   //some buttons should be insensitive if the Scheme window is not visible
  GtkWidget *menu = gtk_menu_new ();
  gchar *labeltext = g_strdup_printf ("Help for %s", func_name);
  GtkWidget *item = gtk_menu_item_new_with_label (labeltext);

  g_free (labeltext);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (popup_help_for_action), (gpointer) action);

  /* Place button in palette */
  if (idx != -1)
    {
      item = gtk_menu_item_new_with_label (_("Place Command in a Palette"));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (placeInPalette), action);
    }

  /* "drag" menu item onto button bar */

  //item = gtk_menu_item_new_with_label (_("Place Command on the Title Bar"));
  //gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  //g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (placeOnButtonBar), action);


  if (idx != -1)
    {
      //item = gtk_menu_item_new_with_label (_("Create Mouse Shortcut"));
      //gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      //g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK (createMouseShortcut), action);
      item = gtk_menu_item_new_with_label (_("Open Command Center\non this command"));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (configure_keyboard_idx), GINT_TO_POINTER (idx));
      item = gtk_menu_item_new_with_label (_("Save Command Set"));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (save_default_keymap_file), action);


      item = gtk_separator_menu_item_new ();
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
    }                           //idx!=-1

  // applies if it is a built-in command: FIXME not set for the popup menus though
  gchar *myposition = g_object_get_data (G_OBJECT (widget), "menupath");
  //g_print("position from built in is %s\n", myposition);
  if (row && !myposition)    //menu item runs a script
    myposition = row->menupath;

 //g_print("position is %s\n", myposition); e.g /ObjectMenu/NotesRests
  if (myposition == NULL)
    {
      g_warning("Cannot find the position of this menu item %s in the menu system", func_name);
      return TRUE;
    }
  static gchar *filepath;       // static so that we can free it next time we are here.
  if (filepath)
    g_free (filepath);
  filepath = get_system_menupath (myposition);
  if (0 == g_access (filepath, 4))
    {
      //g_debug("We can look for a menu item in the path %s\n", filepath);
      item = gtk_menu_item_new_with_label ("More Commands");
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (load_command_from_location), (gpointer) filepath);
    }


  static gchar *filepath2;
  if (filepath2)
	g_free (filepath2);
    filepath2 = g_build_filename (get_user_data_dir (TRUE), COMMANDS_DIR, "menus", myposition, NULL); //g_print ("%s", filepath2);
	 if (0 == g_access (filepath2, 4))
		{
		  //g_print("We can look for a menu item in the path %s at %p\n", filepath2, filepath2);
		  item = gtk_menu_item_new_with_label ("My Commands");
		  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (load_command_from_location), (gpointer) filepath2);
		}
	


  if (!is_action_name_builtin ((gchar *) func_name))
    {
      gchar *scheme = get_scheme_from_idx (idx);
      if (!scheme)
        g_warning ("Could not get script for %s", denemo_action_get_name (action));
      else
        {
          item = gtk_menu_item_new_with_label (_("Get Script into Scheme Window"));
          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (appendSchemeText_cb), scheme);
          
          item = gtk_menu_item_new_with_label (_("Find Directives Created by this Command"));
		 // gtk_widget_set_sensitive (item, sensitive);
		  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
		  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (find_directives_created), (gpointer)func_name);
  
        }
      {

        item = gtk_menu_item_new_with_label (_("Save Script from Scheme Window"));
        gtk_widget_set_sensitive (item, sensitive);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
        g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (saveMenuItem), action);
      }
#ifdef EXTRA_WORK
      if (Denemo.project->xbm)
        {
          //item = gtk_menu_item_new_with_label (_("Save Graphic"));
          // GtkSettings* settings = gtk_settings_get_default();
          // gtk_settings_set_long_property  (settings,"gtk-menu-images",(glong)TRUE, "XProperty");
          //item = gtk_image_menu_item_new_from_stock("Save Graphic", gtk_accel_group_new());
          item = gtk_image_menu_item_new_from_stock (_("Save Graphic") /*_("_OK") */ , NULL);

          gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
          g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (saveGraphicItem), action);
        }
#endif
#ifdef UPLOAD_TO_DENEMO_DOT_ORG
      item = gtk_menu_item_new_with_label (_("Upload this Script to denemo.org"));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
      g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (uploadMenuItem), action);
#endif
    }
    

  
  item = gtk_menu_item_new_with_label (_("Save Script as New Menu Item"));
  gtk_widget_set_sensitive (item, sensitive);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  static gchar *insertion_point;
  if (insertion_point)
    g_free (insertion_point);
  insertion_point = g_build_filename (myposition, func_name, NULL);
  //g_debug("using %p %s for %d %s %s\n", insertion_point, insertion_point, idx, myposition, func_name);
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (insertScript), insertion_point);

  /* options for getting/putting init.scm */
  item = gtk_menu_item_new_with_label (_("Get Initialization Script for this Menu"));
  gtk_widget_set_sensitive (item, sensitive);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (get_initialization_script), myposition);

  item = gtk_menu_item_new_with_label (_("Put Script as Initialization Script for this Menu"));
  gtk_widget_set_sensitive (item, sensitive);

  gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);
  g_signal_connect (G_OBJECT (item), "activate", G_CALLBACK (put_initialization_script), myposition);
  

   
  

  gtk_widget_show_all (menu);
  gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
  return TRUE;
}

//static gboolean
//thecallback (GtkWidget * widget, GdkEventButton * event, DenemoAction * action)
//{
  //if (event->button == 1 && !(event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)))
    //return FALSE;
  //g_debug ("going for %d for %d\n", event->button, event->state);
  //event->button = 3;
  //return menu_click (widget, event, action);
//}



static void
attach_right_click_callback (GtkWidget * widget, DenemoAction * action)
{
  gtk_widget_add_events (widget, (GDK_BUTTON_RELEASE_MASK));      //will not work because label are NO_WINDOW
  g_signal_connect (G_OBJECT (widget), "button-release-event", G_CALLBACK (menu_click), action);

}

/**
 * Key snooper function. This function intercepts all key events before they are
 * passed to other functions for further processing. We use do quick shortcut edits.
 */
static gint
dnm_key_snooper (GtkWidget *grab_widget, GdkEventKey * event)
{
  //show_type (grab_widget, "type is ");
  //no special processing for key release events
  if (event->type == GDK_KEY_RELEASE)
    return FALSE;
  //if the grab_widget is a menu, the event could be a quick edit
  if (Denemo.prefs.quickshortcuts && GTK_IS_MENU (grab_widget))
    {
        
     GtkWidget *item = gtk_menu_shell_get_selected_item (GTK_MENU_SHELL(grab_widget));
      DenemoAction *action = (DenemoAction*)g_object_get_data (G_OBJECT(item), "action");  
      return keymap_accel_quick_edit_snooper (grab_widget, event, action);
    }
  //else we let the event be processed by other functions
  return FALSE;
}

static void
attach_key_snooper (GtkWidget * widget, DenemoAction * action)
{
  show_type (widget, "attaching to type ");
  GdkWindow *window = gtk_widget_get_window (widget);
  if (window)
    gdk_window_set_source_events (window, GDK_SOURCE_KEYBOARD, GDK_KEY_PRESS_MASK);
  //else g_warning ("No GdkWindow for menu item");
  gtk_widget_add_events (widget, (GDK_KEY_PRESS_MASK));
  g_signal_connect (G_OBJECT (widget), "key-press-event", G_CALLBACK (dnm_key_snooper), action);
}


/*  
 *
    * set callback for right click on menu items and
    * set the shortcut label
    * (was proxy_connected())
*/
static void
attach_accels_and_callbacks (DenemoAction * action, GtkWidget * proxy)
{
  int command_idx;
  command_row *row;
  attach_right_click_callback (proxy, action);
 // attach_key_snooper (proxy, action);
 //  g_signal_connect (G_OBJECT (proxy), "button-press-event", G_CALLBACK (thecallback), action);
 //show_type (proxy, "the type is");
  const gchar *tooltip = denemo_action_get_tooltip (action);
  const gchar *additional_text;
  
  g_object_set_data (G_OBJECT (proxy), "action", action);
  
  if (tooltip && g_str_has_prefix (tooltip, _("Menu:")))
    additional_text = _("Click here then hover over the menu items to find out what they will do");
  else
    additional_text = _("Left click to execute the command, press a key to assign a keyboard shortcut to the command,\nRight click to get a menu from which you can\nCreate a button for this command, or a two-key keyboard shortcut or more options still");
  gchar *tip = g_strconcat (tooltip, "\n------------------------------------------------------------------\n", additional_text, NULL);

  denemo_widget_set_tooltip_text (proxy, tip);
  g_free (tip);

  if (Denemo.map == NULL)
    return;
  command_idx = lookup_command_from_name (Denemo.map, denemo_action_get_name (action));

  if (command_idx > -1)
    {
      if (keymap_get_command_row (Denemo.map, &row, command_idx))
        {
          update_accel_labels (Denemo.map, command_idx);

          if (row->hidden)
            set_visibility_for_action (action, FALSE);
        }
    }
}

void denemo_menusystem_new (void)
 {
    ActionWidgets = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);
    Actions = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);
    denemo_menusystem_add_menu (NULL, "/MainMenu");
    denemo_menusystem_add_menu (NULL, "/ObjectMenu"); 
    denemo_menusystem_add_menu (NULL, "/RhythmToolBar"); //popup menus,  toolbar etc need seeding too
    denemo_menusystem_add_menu (NULL, "/ToolBar");
 }
 


GtkWidget *denemo_menusystem_get_widget (gchar *path)
{
  GList *list = g_hash_table_lookup (ActionWidgets, path); 
  if (list) 
    return (GtkWidget*)list->data;
  return NULL;
}



static void popup (GtkWidget *menuitem, GtkWidget *menu)
 {
     gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, 0, gtk_get_current_event_time ());
 }

DenemoAction *denemo_action_new (const gchar *name, const gchar *label, const gchar *tooltip)
{
    if (!Actions)
        {
            g_critical ("No Actions in %s", __FILE__);
            return NULL;
        }
    DenemoAction *action = (DenemoAction *)g_malloc (sizeof (DenemoAction));
    action->name = g_strdup(name);
    action->label = g_strdup(label);
    action->tooltip = g_strdup(tooltip);
    g_hash_table_insert (Actions, action->name, action);  
    return action;
}

static gpointer get_callback (gchar *name)
{
    gint i;
    for (i=0;i<G_N_ELEMENTS (menu_entries);i++)
        {
            if (!strcmp (menu_entries[i].name, name))
                return menu_entries[i].callback;
            
        }
   return NULL;
}
static gchar *get_label_for_name (gchar *name)
{
    gint i;
    if (Denemo.prefs.menunavigation)
		{ gchar *ret = NULL;
			ret = 
				(!strcmp(name, "EditMenu"))?("_Edit"):
				(!strcmp(name, "Educational"))?("Ed_ucational"):
				(!strcmp(name, "FileMenu"))?("_File"):
				(!strcmp(name, "HelpMenu"))?("_Help"):
				(!strcmp(name, "InputMenu"))?("_Input"):
				(!strcmp(name, "MoreMenu"))?("M_ore"):
				(!strcmp(name, "NavigationMenu"))?("Navi_gation"):
				(!strcmp(name, "PlaybackMenu"))?("_Playback"):
				(!strcmp(name, "ViewMenu"))?("Vie_w"):
				(!strcmp(name, "ChordMenu"))?("_Chords"):
				(!strcmp(name, "ClefMenu"))?("C_lefs"):
				(!strcmp(name, "Directives"))?("_Directives"):
				(!strcmp(name, "Key"))?("_Keys"):
				(!strcmp(name, "Lyrics"))?("_Lyrics"):
				(!strcmp(name, "MeasureMenu"))?("Measu_res"):
				(!strcmp(name, "MovementMenu"))?("_Movements"):
				(!strcmp(name, "NotesRests"))?("_Notes/Rests"):
				(!strcmp(name, "Score"))?("_Score"):
				(!strcmp(name, "StaffMenu"))?("Staffs/_Voices"):
				(!strcmp(name, "TimeSig"))?("_Time Signature"):
				NULL;
			if (ret)
				return ret;
		}
    for (i=0;i<G_N_ELEMENTS (menu_entries);i++)
        {
            if (!strcmp (menu_entries[i].name, name))
                return gettext(menu_entries[i].label);
            
        }
   return NULL;
}
static gchar *get_palette_name_from_list (GList *g)
{
   g = g_list_last (g);
   GString *gs = g_string_new((gchar*)g->data);
   for (g=g->prev;g;g=g->prev)
        {
            g_string_append_printf (gs, " ◀ %s", g->data);
        }
 return g_string_free (gs, FALSE);   
}
static gchar *get_location_from_list (GList *g)
{
   GString *gs = g_string_new((gchar*)g->data);
   for (g=g->next;g;g=g->next)
        {
            g_string_append_printf (gs, " ▶ %s", g->data);
        }
 return g_string_free (gs, FALSE);   
}
static void create_palette_for_menu (GtkWidget *menu)
{
   GList *g, *children = gtk_container_get_children (GTK_CONTAINER(menu));
   GList *palette_names = (GList*)g_object_get_data (G_OBJECT(menu), "labels");
   gchar *palette_name = get_palette_name_from_list (palette_names); 
    DenemoPalette *pal = get_palette (palette_name);
    if(pal==NULL)
        {
        gboolean palette_is_empty = TRUE;
        pal = set_palette_shape (palette_name, FALSE, 1);
        pal->menu = TRUE;
        for (g=children;g;g=g->next)
            {
            GtkWidget *item = (GtkWidget*)g->data;
            gchar *script;
            DenemoAction *action = (DenemoAction *)g_object_get_data (G_OBJECT(item), "action");
            if (action==NULL)
               continue;//skip submenus
               gint command_idx = lookup_command_from_name (Denemo.map, denemo_action_get_name (action));
               if ((command_idx>-1) && lookup_hidden_from_idx (Denemo.map, command_idx))
                    continue;
            script = g_strdup_printf ("(d-%s)", action->name);
             gchar *escaped = g_markup_escape_text (action->label, -1);
            palette_add_button (pal, escaped, action->tooltip, script);
            palette_is_empty = FALSE;
            g_free (escaped);
            }
        g_list_free (children); 
        if (palette_is_empty)
          {
            delete_palette (pal);
            warningdialog (_("All the commands in this menu are hidden - use the Command Center to un-hide them or More Commands from the Main Menu"));
          }
        }
    else
        gtk_widget_show (gtk_widget_get_parent (pal->box)), gtk_widget_show (pal->box);
}

static GList *clone_list (GList *g)
{
    GList *h;
    for (h=NULL;g;g=g->next)
        {
            h = g_list_append (h, g->data);
        }
    return h;
}

/*
 *  if path is "/MainMenu" or /ObjectMenu creates a menubar
    otherwise creates a menu item that pops up a menu 
 * 
 */
void denemo_menusystem_add_menu (gchar *path, gchar *name)
{
    GtkWidget *w;
    
    if (path==NULL)
        {
            if (!strcmp(name, "/MainMenu"))
               w = gtk_menu_bar_new (), g_object_set_data (G_OBJECT(w), "labels", g_list_append (NULL, _("Main Menu")));
            else
                {
                if ((!strcmp(name, "/ObjectMenu")))
                    w = gtk_menu_bar_new (), g_object_set_data (G_OBJECT(w), "labels", g_list_append (NULL, _("Object Menu")));
                else
                    w = gtk_toolbar_new (), g_object_set_data (G_OBJECT(w), "labels", g_list_append (NULL,_("Tool Bar")));
                }
            path = g_strdup(name);
        }
    else
        {
        GtkWidget *item, *parent = denemo_menusystem_get_widget (path);
        if (!parent)
            {
                g_critical ("No menu in for path %s, name %s in %s", path, name, __FILE__);
                return;
            }        
        gchar *label = get_label_for_name (name);
        
        if (label==NULL) label = name;
        item = Denemo.prefs.menunavigation? gtk_menu_item_new_with_mnemonic (label) : gtk_menu_item_new_with_label (label);
        gtk_widget_show (item);
        gtk_menu_shell_append (GTK_MENU_SHELL (parent), item);
        w = gtk_menu_new ();
        gtk_widget_show (w);
        g_signal_connect (G_OBJECT (w), "key-press-event", G_CALLBACK(dnm_key_snooper), NULL);
        GList *labels = g_object_get_data (G_OBJECT(parent), "labels");
        g_object_set_data (G_OBJECT(w), "labels", g_list_append(clone_list(labels), label));
      
      GList *current = (GList*)g_hash_table_lookup (ActionWidgets, name);
      if(current==NULL)
        {
            current = g_list_append (current, w);//g_print ("created widget for name %s\n", name);
            g_hash_table_insert (ActionWidgets, g_strdup (name), current);
        }
        
     
        gtk_menu_item_set_submenu (GTK_MENU_ITEM(item), w);

        
        //we need to add a menu item that creates a palette (whose name is the menupath) for the items of the menu, and code in the palettes.c to display such palettes in a sub-menu when choosing etc       
        //item = gtk_menu_item_new_with_label ("8>< 8>< 8>< 8>< 8>< 8>< 8>< 8>< 8>< 8>< 8><");
        item = gtk_menu_item_new_with_label ("---------------------------------------------");
        gtk_widget_set_tooltip_text (item, _("Tear off this menu as a palette"));
        gtk_menu_shell_append (GTK_MENU_SHELL (w), item);
        g_signal_connect_swapped (G_OBJECT (item), "activate", G_CALLBACK(create_palette_for_menu), w);
        gtk_widget_show (item);  

        path = g_build_filename (path, name, NULL); //g_print("menu %s\n", path);
        g_object_set_data (G_OBJECT(w), "menupath", path);
        }
    GList *current = (GList*)g_hash_table_lookup (ActionWidgets, path);
    current = g_list_append (current, w);
    g_hash_table_insert (ActionWidgets, path, current); //g_print ("the path %s added to ActionWidgets", path);
}

static gint get_item_position (GtkWidget *menu, gchar *name)
{
        gint position = -1;
        if (name)
            {
            GList *children = gtk_container_get_children (GTK_CONTAINER (menu));
            GList *g;
            gint i=0;
            for (g=children;g;g=g->next, i++)
                {
                    gchar *n = g_object_get_data (G_OBJECT(g->data), "name");
                    if (n && !strcmp (n, name))
                        {
                            position = i;
                            break;
                        }
                }
        g_list_free (children);
        }
    return position;      
}


static void relabel_tear_off (GtkWidget *menu, gint length)
{
   GList *children = gtk_container_get_children (GTK_CONTAINER (menu));
  // g_print ("length %d\n", length);
   if (children)
        {
            GtkMenuItem *tearoff = (GtkMenuItem*)children->data;
            gchar *label =  g_strnfill((gsize)length*2, '-'); //g_print ("label is %s\n", label);
            gchar *scissor = g_strconcat ("✂", label, NULL);
            gtk_menu_item_set_label (tearoff, scissor);
            g_free (scissor);  
            g_list_free (children);
            g_free (label);        
        }
}
/*
 * if name is in entries.h create a menu item that calls the callback on activate signal
 * otherwise create menuitem that calls activate_action
*/
void denemo_menusystem_add_command (gchar *path, gchar *name, gchar *after)
{
	if (get_menupath_for_name (name)) 
		{
			if (!Denemo.old_user_data_dir)
				g_warning("Not upgrading but already have %s\n", name);
			return;
		}
    DenemoAction *action = denemo_menusystem_get_action (name);
    GtkWidget *item, *parent = denemo_menusystem_get_widget (path);
    if (!parent)
        {
            g_critical ("No menu in %s", __FILE__);
            return;
        }    gchar *label = get_label_for_name (name);
    if(label==NULL) 
        label = name;
    gint length = strlen (label);
    gint curw = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (parent), "width"));
    if (length > curw)
        {
            relabel_tear_off (parent, length);
            g_object_set_data (G_OBJECT (parent), "width", GINT_TO_POINTER (length));
        }
    item = gtk_menu_item_new_with_label (label);
    gtk_widget_show (item);
    g_object_set_data (G_OBJECT (item), "menupath", path); //FIXME is this in use???
    g_object_set_data (G_OBJECT (item), "name", name);   

    gint position = get_item_position (parent, after);
  
    if(position<0)
        gtk_menu_shell_append (GTK_MENU_SHELL (parent), item);
    else
        gtk_menu_shell_insert (GTK_MENU_SHELL (parent), item, position + 1);
    gpointer callback = get_callback (name);
    if (callback)
        g_signal_connect (item, "activate", G_CALLBACK (callback), NULL);
    else
        g_signal_connect_swapped (item, "activate", G_CALLBACK (activate_action), name);
     //  g_print ("Storing menu item for %s\n", name);

    GList *current = (GList*)g_hash_table_lookup (ActionWidgets, name);
    current = g_list_append (current, item);
    g_hash_table_insert (ActionWidgets, g_strdup (name), current); 
    attach_accels_and_callbacks (action, item);  
}

//returns the (first) menupath stored for action of name
gchar *get_menupath_for_name (gchar *name)
{
    GList *current = (GList*)g_hash_table_lookup (ActionWidgets, name);
    if (current)
        return (gchar*)g_object_get_data (G_OBJECT (current->data), "menupath");
    return NULL;
}

//returns labels stored for action of name
gchar *get_location_for_name (gchar *name)
{
    GList *current = (GList*)g_hash_table_lookup (ActionWidgets, name);
    if (current)
        {
          GtkWidget *item = (GtkWidget*)current->data;  
          GtkWidget *menu =  gtk_widget_get_parent(item);
          GList *labels = g_object_get_data (G_OBJECT (menu), "labels");
          if (labels) 
            return get_location_from_list (labels);
       }
    
    return NULL;
}


//this is called early on, before the keymap has been set up
void denemo_menusystem_add_actions (void)
{
    gint i;
    for (i=0;i<G_N_ELEMENTS (menu_entries);i++)
        {
          DenemoAction *action = denemo_action_new (menu_entries[i].name, gettext(menu_entries[i].label), gettext(menu_entries[i].tooltip));
          action->type = DENEMO_MENU_ITEM;
          action->callback = menu_entries[i].callback;
        }  
}
static void denemo_action_group_add_toggle_actions (void)
{  
    GtkWidget *item, *parent = denemo_menusystem_get_widget ("/MainMenu/ViewMenu");
    if (!parent)
        {
            g_critical ("No menu in %s", __FILE__);
            return;
        }
    gint i;
    for ( i=0;i<G_N_ELEMENTS (toggle_menu_entries);i++)
        {
        DenemoAction *action = denemo_action_new (toggle_menu_entries[i].name, gettext(toggle_menu_entries[i].label), gettext(toggle_menu_entries[i].tooltip));
        action->type = DENEMO_MENU_ITEM;
        action->callback = toggle_menu_entries[i].callback;
        item = gtk_check_menu_item_new_with_label (gettext(toggle_menu_entries[i].label));
       // toggle_menu_entries[i].item = item;
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(item), toggle_menu_entries[i].initial);
       
        gtk_widget_show (item);
        toggle_menu_entries[i].item = item;
        gtk_menu_shell_insert (GTK_MENU_SHELL (parent), item, i+1); //placed after the tear-off item.
        g_signal_connect (item, "activate", G_CALLBACK (toggle_menu_entries[i].callback), NULL);
        }  
}

void set_toggle (gchar *name, gboolean value)
{
    gint i;
    for (i=0;i<G_N_ELEMENTS (toggle_menu_entries);i++)
        {
            if (!strcmp (name,    toggle_menu_entries[i].name))
                {
                  if(gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(toggle_menu_entries[i].item)) != value)
                    {
                      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(toggle_menu_entries[i].item), value); 
                      g_message ("Set %s to %s\n", name, value? "active":"inactive");
                      }
                  else
                    g_message ("Set %s to %s\n", name, value? "active":"inactive");
                    return;
                }
        }
}
gboolean get_toggle (gchar *name)
{
    gint i;
    for (i=0;i<G_N_ELEMENTS (toggle_menu_entries);i++)
        {
            if (!strcmp (name,    toggle_menu_entries[i].name))
                {
                  return gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM(toggle_menu_entries[i].item));
                }
        }
   return FALSE;
}

void denemo_action_group_add_radio_actions (void)
{
   GtkWidget *item, *parent = denemo_menusystem_get_widget ("/MainMenu/InputMenu");
    if (!parent)
        {
            g_critical ("No menu in %s", __FILE__);
            return;
        }
   gint i;
   GSList *group = NULL;

    for (i=0;i<G_N_ELEMENTS (input_menu_entries);i++)
        {
        DenemoAction *action = denemo_action_new (input_menu_entries[i].name, gettext(input_menu_entries[i].label), gettext(input_menu_entries[i].tooltip));
        action->type = DENEMO_MENU_ITEM;
        
        item = gtk_radio_menu_item_new_with_label (group, gettext(input_menu_entries[i].label));
        
        group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));
        gtk_menu_shell_insert (GTK_MENU_SHELL (parent), item, i+1);//placed after the tear-off item.

        if (i == (have_midi ()? 1 : 0))
           gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item), TRUE);//must do this before connecting signal otherwise the audio will be re-started.
        g_signal_connect (item, "toggled", G_CALLBACK(change_input_type), GINT_TO_POINTER(i));
       }  
}
                                      
                    
DenemoAction *denemo_menusystem_get_action (gchar *name)  
{
  return  (name && Actions)?(DenemoAction*)g_hash_table_lookup (Actions, name):NULL;
 
}   

GList* denemo_action_get_proxies (DenemoAction *action)
{
    
    return (action && ActionWidgets)?(GList*)g_hash_table_lookup (ActionWidgets, action->name):NULL;
}

gchar *denemo_action_get_name (DenemoAction *action) {
    return action?action->name:NULL;
}
gchar *denemo_action_get_tooltip (DenemoAction *action) {
    return action?action->tooltip:NULL;
}



void denemo_action_activate(DenemoAction *action)
{
    if(action) 
        {
        if (action->type && action->callback)
            ((BuiltInCallback)(action->callback))(&DummyAction, NULL);
        else 
            activate_script (action, NULL);
        }
}

static void toolbar_new_callback (void)
    {new_score_cb (NULL, NULL);}
static void  toolbar_open_callback (void)
    {file_open_with_check (NULL, NULL);}
static void  toolbar_save_callback (void)
    {file_savewrapper (NULL, NULL);}
static void  toolbar_print_callback (void)
    {
#ifndef USE_EVINCE
  g_debug ("This feature requires denemo to be built with evince");
#else
    if (!gtk_widget_get_visible ( gtk_widget_get_toplevel (Denemo.printarea)))
        set_toggle (TogglePrintView_STRING, TRUE);
    else
        implement_show_print_view(TRUE);
#endif
    }

static void  toolbar_move_to_start_callback (void)
    {movetostart (NULL, NULL);}    
static void  toolbar_move_to_end_callback (void)
    {movetoend (NULL, NULL);}    
    
    
static void initialize_toggle_settings (void)
    {   

    gint i;
    for ( i=0;i<G_N_ELEMENTS (toggle_menu_entries);i++)
        {
        if (!strcmp(toggle_menu_entries[i].name, ToggleScoreTitles_STRING))
            toggle_menu_entries[i].initial = Denemo.prefs.visible_directive_buttons;
            //FIXME others
        }     
    }  
    
    
static void create_toolbar_items (void)
{
    
  //  denemo_menusystem_add_command ( "/ToolBar", "NewScore");
    GtkWidget *parent = denemo_menusystem_get_widget ( "/ToolBar");
    GtkWidget *item;
    
    item = (GtkWidget*)gtk_tool_button_new (NULL, _("New"));
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON(item), "document-new");
    gtk_widget_set_tooltip_text (item, _("Start a new musical score for a named instrument/voice."));
    gtk_widget_show (item);
    gtk_toolbar_insert (GTK_TOOLBAR(parent), GTK_TOOL_ITEM(item), -1);
    g_signal_connect_swapped (item, "clicked", G_CALLBACK (toolbar_new_callback), NULL);
    
    
    
    item = (GtkWidget*)gtk_tool_button_new (NULL, _("Open"));
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON(item), "document-open");
    gtk_widget_set_tooltip_text (item, _("Open a file containing a music score for editing"));
    gtk_widget_show (item);
    gtk_toolbar_insert (GTK_TOOLBAR(parent), GTK_TOOL_ITEM(item), -1);
    g_signal_connect_swapped (item, "clicked", G_CALLBACK (toolbar_open_callback), NULL);
   
    
    
    item = (GtkWidget*)gtk_tool_button_new (NULL, _("Save"));
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON(item), "document-save");
    gtk_widget_set_tooltip_text (item, _("Save the score. The score is saved to disk in XML format."));
    gtk_widget_show (item);
    gtk_toolbar_insert (GTK_TOOLBAR(parent), GTK_TOOL_ITEM(item), -1);
    g_signal_connect_swapped (item, "clicked", G_CALLBACK (toolbar_save_callback), NULL);
   
    item = (GtkWidget*)gtk_tool_button_new (NULL, _("Print"));
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON(item), "document-print");
    gtk_widget_set_tooltip_text (item, _("Typesets the score using LilyPond and opens a print dialog"));
    gtk_widget_show (item);
    gtk_toolbar_insert (GTK_TOOLBAR(parent), GTK_TOOL_ITEM(item), -1);
    g_signal_connect_swapped (item, "clicked", G_CALLBACK (toolbar_print_callback), NULL);

#ifdef PROBLEM_WITH_PRINT_VIEW_DISPLAY_FIXED
//check item is out of sync/Print View ignore delete signal    
    item = (GtkWidget*)gtk_tool_button_new (NULL, _("PrintView"));
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON(item), GTK_STOCK_PRINT_PREVIEW);
    gtk_widget_set_tooltip_text (item, _("Typesets the score\nIf you have a score layout selected it will use that\notherwise all movements staffs and lyrics are typeset by default.\n"
    "Be patient! It takes time to create a beautifully laid out score.\nOnce complete you can view and then send to your printer or to a file as a .pdf document."));
    gtk_widget_show (item);
    gtk_toolbar_insert (GTK_TOOLBAR(parent), GTK_TOOL_ITEM(item), -1);
    g_signal_connect_swapped (item, "clicked", G_CALLBACK (toolbar_printview_callback), NULL);
#endif
    
    item = (GtkWidget*)gtk_tool_button_new (NULL, _("Move to Staff/Voice Beginning"));
#if ((GTK_MAJOR_VERSION==3)&&(GTK_MINOR_VERSION<10))
    gtk_tool_button_set_label (GTK_TOOL_BUTTON(item), "|◀-");
#else     
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON(item), "go-first");
#endif    
    
    gtk_widget_set_tooltip_text (item, _("Cursor to start of staff/voice, without extending selection if any"));
    gtk_widget_show (item);
    gtk_toolbar_insert (GTK_TOOLBAR(parent), GTK_TOOL_ITEM(item), -1);
    g_signal_connect_swapped (item, "clicked", G_CALLBACK (toolbar_move_to_start_callback), NULL);
      
    item = (GtkWidget*)gtk_tool_button_new (NULL, _("Move to Staff/Voice End"));
    
#if ((GTK_MAJOR_VERSION==3)&&(GTK_MINOR_VERSION<10))
    gtk_tool_button_set_label (GTK_TOOL_BUTTON(item), "-▶|");
#else    
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON(item), "go-last");
#endif

    gtk_widget_set_tooltip_text (item, _("Cursor to end of staff/voice, without extending selection if any"));
    gtk_widget_show (item);
    gtk_toolbar_insert (GTK_TOOLBAR(parent), GTK_TOOL_ITEM(item), -1);
    g_signal_connect_swapped (item, "clicked", G_CALLBACK (toolbar_move_to_end_callback), NULL);
}

void finalize_menusystem(void)
  {
      initialize_toggle_settings ();
      
      denemo_action_group_add_toggle_actions ();
      denemo_action_group_add_radio_actions ();
      
      GtkWidget *button = (GtkWidget*)gtk_tool_button_new (NULL, _("Create Snippet"));
      
      gtk_widget_set_tooltip_text (button, _("Copy selection as music snippet or rhythm pattern for notes to follow as they are entered"));
      g_signal_connect (G_OBJECT(button), "clicked", G_CALLBACK(create_rhythm_cb), NULL);
      GtkWidget *rhythm_toolbar = denemo_menusystem_get_widget ("/RhythmToolBar");
      gtk_container_add (GTK_CONTAINER (rhythm_toolbar), button);
      
      gtk_widget_show_all (denemo_menusystem_get_widget ("/MainMenu/InputMenu"));
      
      gtk_widget_show_all (rhythm_toolbar);
      gtk_widget_show_all (gtk_widget_get_toplevel (Denemo.script_view));
      gtk_widget_hide (gtk_widget_get_toplevel (Denemo.script_view));
      
      gtk_widget_hide (rhythm_toolbar);//So that preference starts with off
     //gtk_widget_show (denemo_menusystem_get_widget (ToggleScoreTitles_STRING));
     if(Denemo.prefs.visible_directive_buttons)
       gtk_widget_show (Denemo.project->buttonboxes);
    else
       gtk_widget_hide (Denemo.project->buttonboxes);
 
      gtk_widget_show (Denemo.menubar);
      //gtk_widget_show (denemo_menusystem_get_widget ("/ObjectMenu"));
      gtk_widget_hide (denemo_menusystem_get_widget ("/ObjectMenu"));
      create_toolbar_items ();
  }
      
/* add ui elements for menupath if missing 
else create menu and a menuitem that will popup the menu when activated and in all cases associate the name with the menu widget.
*/

void
instantiate_menus (gchar * menupath)
{
  //g_print("Instantiate menus for %s\n", menupath);

  gchar *up1 = g_path_get_dirname (menupath);
  gchar *name = g_path_get_basename (menupath);
  GtkWidget *widget = denemo_menusystem_get_widget (up1);
  if (!strcmp (up1, "/"))
    {
      g_critical ("bad menu path");
      return;
    }
  if (!strcmp (up1, "."))
    {
      g_critical ("bad menu path");
      return;
    }
  if (widget == NULL)
    instantiate_menus (up1);
  if (NULL == denemo_menusystem_get_action (name))
    {//g_print("Menu %s has no action\n", name);
      gchar *tooltip = g_strconcat (_("Menu:\nnamed \""), name, _("\" located at "), menupath, _(" in the menu system"), NULL);
      DenemoAction *action = denemo_action_new (name, name, tooltip);
      g_free (tooltip);
      //denemo_action_group_add_action (action);

      gint idx = lookup_command_from_name (Denemo.map, name);
      command_row* row = NULL;
      keymap_get_command_row(Denemo.map, &row, idx);//g_print ("For %s get idx %d and row %p\n", name, idx, row);
      if(row)
        row->menupath = up1;
    }
  denemo_menusystem_add_menu (up1, name);
}
