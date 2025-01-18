/* exportmusicxml.h
 * Header file for exporting Denemo as MusicXML files
 *
 * for Denemo, a gtk+ frontend to GNU Lilypond
 * (c) 2020 Richard Shann
 */

#ifndef EXPORTMUSICXML_H
#define EXPORTMUSICXML_H

#include <denemo/denemo.h>
#include "export/xmldefs.h"

#include <glib.h>

/* libxml includes: for libxml2 this should be <libxml/tree.h> */
#include <libxml/tree.h>

/*
 * Export the given score as musicXML file format to the given
 * filename.
 */
void split_and_process_lyrics(GList **list_sillabe, GList **list_sillabe_type, gchar *text);
gint exportmusicXML (gchar * thefilename, DenemoProject * gui);


#endif
