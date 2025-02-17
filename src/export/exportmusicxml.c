/* exportmusicxml.c
 * Functions for exporting a score to MusicXml
 *
 *
 * for Denemo, a gtk+ frontend to GNU Lilypond
 * (c) 2020 Richard Shann
 */

#include "config.h"
#include <denemo/denemo.h>
#include <libxml/xmlsave.h>
#include "core/exportxml.h"
#include "source/source.h"
#include "core/utils.h"
#include "command/lyric.h"
#include "command/lilydirectives.h"
#include "ui/texteditors.h"
#include "export/xmldefs.h"
#include "command/scorelayout.h"
#include "audio/pitchentry.h"
#include <stdlib.h>
#include <string.h>

/* libxml includes: for libxml2 this should be <libxml/tree.h> */
#include <libxml/tree.h>

typedef struct Syllable {
  GString* syllable;
  char* type;
} Syllable;


static gint OttavaVal = 0;
#define XML_COMPRESSION_RATIO 3
/**
 * return a child node of parent, holding the passed name and integer.
 */
  static xmlNodePtr
newXMLIntChild(xmlNodePtr parent, xmlNsPtr ns, const xmlChar *name, gint content)
{
  gchar *integer = g_strdup_printf("%d", content);
  xmlNodePtr child = xmlNewChild(parent, ns, name, (xmlChar *)integer);
  g_free(integer);
  return child;
}

  static void
determineDuration(gint duration, gchar **durationName)
{
  switch (duration)
  {
    case 0:
      *durationName = "whole";
      break;
    case 1:
      *durationName = "half";
      break;
    case 2:
      *durationName = "quarter";
      break;
    case 3:
      *durationName = "eighth";
      break;
    case 4:
      *durationName = "16th";
      break;
    case 5:
      *durationName = "32nd";
      break;
    case 6:
      *durationName = "64th";
      break;
    case 7:
      *durationName = "128th";
      break;
    default:
      if (duration < -7)
        *durationName = "whole";
      else if (duration < 0)
      {
        return determineDuration(-duration, durationName);
      }
      break;
  }
}

static void get_clef_sign(clefs theclef, gchar **sign, gint *line, gint *octave)
{
  switch (theclef)
  {
    case DENEMO_TREBLE_CLEF:
      *sign = "G";
      *line = 2;
      return;
    case DENEMO_BASS_CLEF:
      *sign = "F";
      *line = 3;
      return;
    case DENEMO_ALTO_CLEF:
      *sign = "C";
      *line = 3;
      return;

    case DENEMO_G_8_CLEF:
      *sign = "G";
      *line = 2;
      *octave = -1;
      return;

    case DENEMO_TENOR_CLEF:
      *sign = "C";
      *line = 4;
      return;

    case DENEMO_SOPRANO_CLEF:
      *sign = "C";
      *line = 1;
      return;

    case DENEMO_F_8_CLEF:
      *sign = "F";
      *line = 2;
      *octave = -1;
      return;

    case DENEMO_FRENCH_CLEF:
      *sign = "G";
      *line = 1;
      return;

    case DENEMO_BARITONE_CLEF:
      *sign = "C";
      *line = 5;
      return;
  }
}

// number of beams thechord requires, 0 if it is a rest
static gint beams(chord *thechord)
{
  int duration = thechord->notes ? thechord->baseduration : 0;
  if (duration < 3)
    return 0;
  return (duration - 2);
}

static gchar *extract_field(gchar *str, gchar *signature)
{
  gchar *title = g_strrstr(str, signature);
  if (title)
  {
    title = g_strdup(title + strlen(signature));
    gchar *c = title;
    do
    {
      if ((*c == '\"') || (*c == '\\'))
        *c = 0;
    } while (*c++);

    // while ((*c++!='\"'));
    //*--c=0;
  }
  return title;
}

static void get_ottava(DenemoDirective *dir, gint *amount)
{
  *amount = 0;
  if (dir->postfix)
    sscanf(dir->postfix->str, "\\ottava #%d", amount);
}

static void split_and_process_lyrics(GList **list_syllables, GList **list_syllable_types, gchar *text)
{
  //FIXME -> lascia le newline nelle sillabe
  // Rimuove i newline dal testo e li sostituisce con spazi
  gchar *normalized_text = g_strdelimit(g_strdup(text), "\n\r", ' ');

  // Suddivide il testo in token usando spazio come delimitatore
  gchar **tokens = g_strsplit(text, " ", 0);

  for (int i = 0; tokens[i] != NULL; i++)
  {

    gchar *current = g_strstrip(tokens[i]); // Elimina spazi inutili


    printf ("Sillaba %s\n", current);
    // Ignora token "--"
    if (g_strcmp0(current, "--") == 0)
      continue;

    // Ignora token "_" o "__", inserisce NULL per le syllabe e i tipi
    if (g_strcmp0(current, "_") == 0 || g_strcmp0(current, "__") == 0)
    {
      *list_syllables = g_list_append(*list_syllables, NULL);
      *list_syllable_types = g_list_append(*list_syllable_types, NULL);
      continue;
    }

    // Controlla se il token precedente e successivo sono "--"
    if ((i > 0 && g_strcmp0(tokens[i - 1], "--") == 0) &&
        (tokens[i + 1] != NULL && g_strcmp0(tokens[i + 1], "--") == 0))
    {
      *list_syllables = g_list_append(*list_syllables, g_strdup(current));
      *list_syllable_types = g_list_append(*list_syllable_types, g_strdup("middle"));
      continue;
    }

    // Controlla se solo il token successivo è "--"
    if (tokens[i + 1] != NULL && g_strcmp0(tokens[i + 1], "--") == 0)
    {
      *list_syllables = g_list_append(*list_syllables, g_strdup(current));
      *list_syllable_types = g_list_append(*list_syllable_types, g_strdup("begin"));
      continue;
    }

    // Controlla se solo il token precedente è "--"
    if (i > 0 && g_strcmp0(tokens[i - 1], "--") == 0)
    {
      *list_syllables = g_list_append(*list_syllables, g_strdup(current));
      *list_syllable_types = g_list_append(*list_syllable_types, g_strdup("end"));
      continue;
    }

    // Caso generico: single sillaba senza "--" intorno
    *list_syllables = g_list_append(*list_syllables, g_strdup(current));
    *list_syllable_types = g_list_append(*list_syllable_types, g_strdup("single"));
  }

  // Libera l'array di token
  g_strfreev(tokens);
}

/**
 * splitString - Divide una stringa di testo in token basati sullo spazio e li memorizza in una lista.
 *
 * @text:       La stringa di input da suddividere in token.
 * @token_list: Puntatore a una lista `GList` che verrà popolata con i token sotto forma di `GString*`.
 *
 * Questa funzione utilizza `g_strsplit` per separare la stringa in base agli spazi.
 * Ogni token non vuoto viene convertito in un `GString` e aggiunto alla lista `token_list`.
 * La memoria allocata per i token viene gestita automaticamente tramite `g_list_append`.
 *
 * Esempio di input/output:
 *     Input:  "ciao mondo -- esempio"
 *     Output: ["ciao", "mondo", "--", "esempio"] (memorizzati in una GList)
 *
 * Nota: La memoria di `token_list` deve essere liberata successivamente con `g_list_free_full`.
 */


void splitString(gchar *text, GList **token_list) {
  gchar** tokens = g_strsplit(text, " ", -1);
  for (gint i = 0; tokens[i]; i++) {
    if (tokens[i] == NULL || *tokens[i] == '\0') continue;
    *token_list = g_list_append(*token_list, g_string_new(tokens[i]));
  }
  g_strfreev(tokens);
}

/**
 * assign_syllable_types - Analizza una lista di token e assegna a ciascuno il tipo di sillaba corrispondente.
 *
 * @token_list:     Lista `GList` contenente i token (`GString*`) ottenuti dalla suddivisione del testo.
 * @syllable_list:  Puntatore a una lista `GList` che verrà popolata con strutture `Syllable*`,
 *                  contenenti la sillaba e il suo tipo.
 *
 * Questa funzione analizza ciascun token e determina il tipo di sillaba in base ai seguenti criteri:
 * - `"--"` viene ignorato in quanto separatore.
 * - `"__"` viene ignorato (non considerato come sillaba).
 * - `"_"` viene memorizzato come una sillaba NULL con tipo NULL.
 * - Se una sillaba è l'unica presente, viene classificata come `"single"`.
 * - Se una sillaba è preceduta o seguita da `"--"`, viene classificata come `"begin"`, `"end"` o `"middle"`.
 * - Altrimenti, viene classificata come `"single"`.
 *
 * Esempio di input/output:
 *     Input:  ["Hal", "--", "lo", "--", "chen"]
 *     Output: [("Hal", "begin"), ("lo", "end"), ("chen", "single")]
 *
 * Nota: La memoria allocata per `syllable_list` deve essere liberata successivamente con `g_list_free_full`.
 */


// TODO -> __ produce il tag extend. Quindi bisogna gestirlo a parte
void assign_syllable_types(GList* const token_list, GList **syllable_list) {
  GList* iter = NULL;
  for (iter = token_list; iter != NULL; iter = iter->next) {
    GString* next_token = NULL;
    GString* previous_token = NULL;
    GString* token = (GString*)iter->data;

    if (iter->prev) previous_token = (GString*)(iter->prev)->data;
    if (iter->next) next_token = (GString*)(iter->next)->data;

    if (g_strcmp0(token->str, "--") == 0) continue;
    if (g_strcmp0(token->str, "__") == 0) continue;
    Syllable* s = g_new(Syllable, 1);

    if (g_strcmp0(token->str, "_") == 0) {
      s->syllable = NULL;
      s->type = NULL;
      *syllable_list = g_list_append(*syllable_list, s);
      continue;
    }

    s->syllable = g_string_new(token->str);

    if (!previous_token) {
      if (!next_token) s->type = g_strdup("single");
      else if (g_strcmp0(next_token->str, "--") == 0) s->type = g_strdup("begin");
      else s->type = g_strdup("single");
    }
    else if (!next_token) {
      if (g_strcmp0(previous_token->str, "--") == 0) s->type = g_strdup("end");
      else s->type = g_strdup("single");
    }
    else {
      if (g_strcmp0(previous_token->str, "--") == 0 && g_strcmp0(next_token->str, "--") == 0) s->type = g_strdup("middle");
      else if (g_strcmp0(previous_token->str, "--") == 0 && g_strcmp0(next_token->str, "--") != 0 ) s->type = g_strdup("end");
      else if (g_strcmp0(previous_token->str, "--") != 0 && g_strcmp0(next_token->str, "--") == 0 ) s->type = g_strdup("begin");
      else s->type = g_strdup("single");
    }

    *syllable_list = g_list_append(*syllable_list, s);
  }
}

void destroy_syllable(Syllable* s) {
  if (s) {
    if (s->syllable) g_string_free(s->syllable, TRUE);  // Libera GString
    if (s->type) g_free(s->type);  // Libera il tipo
    g_free(s);  // Libera la struttura Syllable
  }
}

/**
 * Export the given score as a MusicXML file thefilname
 returns 0 on success
 */
gint exportmusicXML(gchar *thefilename, DenemoProject *gui)
{
  gint ret = 0;
  gint i, j, k;
  GString *filename = g_string_new(thefilename);
  xmlDocPtr doc;
  xmlNodePtr scoreElem, mvmntElem, stavesElem, voicesElem, voiceElem;
  xmlNodePtr measuresElem, measureElem;

  xmlNodePtr curElem;
  xmlNsPtr ns;
  staffnode *curStaff;
  DenemoStaff *curStaffStruct;
  gchar *staffXMLID = 0, *voiceXMLID;
  GList* syllabe_iterator = NULL;
  Syllable* sillaba = NULL;
  measurenode *curMeasure;

  static gchar *version_string;
  if (version_string == NULL)
    version_string = g_strdup_printf("%d", CURRENT_XML_VERSION);

  gboolean single_movement = (1 == g_list_length(gui->movements));
  /* Initialize score-wide variables. */

  /* Create the XML document and output the root element. */

  doc = xmlNewDoc((xmlChar *)"1.0");
  xmlSetDocCompressMode(doc, Denemo.prefs.compression);
  doc->xmlRootNode = scoreElem = xmlNewDocNode(doc, NULL, (xmlChar *)"score-partwise", NULL);
  ns = NULL;

  //<work>
  //<work-number>D. 839</work-number>
  //<work-title>Ave Maria (Ellen's Gesang III) - Page 1</work-title>
  //</work>

  //<identification>
  //<creator type="composer">Franz Schubert</creator>

  xmlNodePtr workElem = xmlNewChild(scoreElem, ns, (xmlChar *)"work", NULL);

  xmlNodePtr identificationElem = xmlNewChild(scoreElem, ns, (xmlChar *)"identification", NULL);

  DenemoDirective *dir = get_header_directive("MovementTitles");
  if (dir)
  {
    GString *data = (GString *)(dir->data);
    if (data)
    {
      gchar *field = extract_field(data->str, "(cons 'title \"");
      if (field && *field)
      {
        xmlNewChild(scoreElem, ns, (xmlChar *)"movement-title", field);
        g_free(field);
      }
      field = extract_field(data->str, "(cons 'composer \"");
      if (field && *field)
      {
        xmlNodePtr creatorElem = xmlNewChild(identificationElem, ns, (xmlChar *)"creator", (xmlChar *)field);
        xmlSetProp(creatorElem, (xmlChar *)"type", (xmlChar *)"composer");
        g_free(field);
      }
    }
  }

  dir = get_scoreheader_directive("ScoreTitles");
  if (dir)
  {
    GString *data = (GString *)(dir->data);
    if (data)
    {
      gchar *field = extract_field(data->str, "(cons 'title \"");
      if (field && *field)
      {
        if (single_movement)
          xmlNewChild(scoreElem, ns, (xmlChar *)"movement-title", field);
        else
          xmlNewChild(workElem, ns, (xmlChar *)"work-title", field);
        g_free(field);
      }
      field = extract_field(data->str, "(cons 'composer \"");
      if (field && *field)
      {
        xmlNodePtr creatorElem = xmlNewChild(identificationElem, ns, (xmlChar *)"creator", (xmlChar *)field);
        xmlSetProp(creatorElem, (xmlChar *)"type", (xmlChar *)"composer");
        g_free(field);
      }
    }
  }
  dir = get_scoreheader_directive("BookTitle");
  if (dir)
  {
    GString *data = (GString *)(dir->display);
    if (data && data->len)
    {

      if (single_movement)
        xmlNewChild(scoreElem, ns, (xmlChar *)"movement-title", data->str);
      else
        xmlNewChild(workElem, ns, (xmlChar *)"work-title", data->str);
    }
  }
  dir = get_scoreheader_directive("BookComposer");
  if (dir)
  {
    GString *data = (GString *)(dir->display);
    if (data && data->len)
    {
      xmlNodePtr creatorElem = xmlNewChild(identificationElem, ns, (xmlChar *)"creator", (xmlChar *)data->str);
      xmlSetProp(creatorElem, (xmlChar *)"type", (xmlChar *)"composer");
    }
  }
  dir = get_movementcontrol_directive("TitledPiece");
  if (dir)
  {
    GString *data = (GString *)(dir->data);
    if (data && data->len)
    {
      xmlNewChild(scoreElem, ns, (xmlChar *)"movement-title", data->str);
    }
  }

  xmlNodePtr encodingElem = xmlNewChild(identificationElem, ns, (xmlChar *)"encoding", NULL);
  xmlNodePtr suppElem = xmlNewChild(encodingElem, ns, (xmlChar *)"supports", NULL);
  xmlSetProp(suppElem, (xmlChar *)"element", (xmlChar *)"beam");
  xmlSetProp(suppElem, (xmlChar *)"type", (xmlChar *)"yes");
  suppElem = xmlNewChild(encodingElem, ns, (xmlChar *)"supports", NULL);
  xmlSetProp(suppElem, (xmlChar *)"element", (xmlChar *)"accidental");
  xmlSetProp(suppElem, (xmlChar *)"type", (xmlChar *)"yes");

  xmlNodePtr partListElem = xmlNewChild(scoreElem, ns, (xmlChar *)"part-list", NULL);
  DenemoMovement *si = gui->movement; // FIXME: loop for movements
                                      // give part ids and attributes for the staffs the ids
                                      //   <score-part id="P1">
  for (i = 0, curStaff = si->thescore; curStaff != NULL; i++, curStaff = curStaff->next)
  {
    xmlNodePtr scorePartElem = xmlNewChild(partListElem, ns, (xmlChar *)"score-part", NULL);
    gchar *val = g_strdup_printf("P%d", i + 1);
    xmlSetProp(scorePartElem, (xmlChar *)"id", val);
    g_free(val);
    xmlNewChild(scorePartElem, ns, (xmlChar *)"part-name", "Voice"); // FIXME use Denemo part name... do instrument name as well?
  }

  for (i = 0, curStaff = si->thescore; curStaff != NULL; i++, curStaff = curStaff->next)
  {
    gint nthTime = 1; // which nth time repeat bar been started. Used by EndVolta which does not carry the information
    xmlNodePtr partElem;
    curStaffStruct = (DenemoStaff *)curStaff->data;
    gboolean tuplet_start = FALSE, in_tuplet = FALSE;
    gboolean in_tie = FALSE;
    gboolean in_slur = FALSE;
    OttavaVal = 0;
    // output the i'th staff
    partElem = xmlNewChild(scoreElem, ns, (xmlChar *)"part", NULL);
    xmlSetProp(partElem, (xmlChar *)"id", (xmlChar *)g_strdup_printf("P%d", i + 1));
    // output measures

    // aggiunge lyrics se presenti (codice mio)
    // i è lo staff corrente
    bool has_lyrics = true;
    GList *lyrics = NULL; // le lyrics (intere, non in syllabe) di un solo sistema
                          // GList *all_lyrics = NULL;
    GList *list_syllabe = NULL; // una lista con tutte le syllabe
    GList *list_syllabe_type = NULL;
    GList *l = NULL;

    if (!curStaffStruct || !curStaffStruct->verse_views)
    {
      printf("Staff %d non ha verse_views\n", i);
      // all_lyrics = g_list_append(all_lyrics, NULL); // la lista delle lyric ha NULL se non ve ne sono da inserire
      has_lyrics = false;
    }

    if (has_lyrics)
    {

      printf("Numero di verse_views: %d\n", g_list_length(curStaffStruct->verse_views));
      printf("Estraggo le lyric dallo Staff %d\n", i);
      // estrae le lyric
      for (l = curStaffStruct->verse_views; l; l = l->next)
      {
        gchar *lyrics = get_text_from_view(l->data);
        if (lyrics)
        {
          GList *token_list = NULL;
          splitString(lyrics, &token_list);

          GList *syllable_list = NULL;
          assign_syllable_types(token_list, &syllable_list);
          syllabe_iterator = syllable_list;
        }
      }

    }

    // Fine mio codice

    for (j = 0, curMeasure = curStaffStruct->themeasures; curMeasure != NULL; j++, curMeasure = curMeasure->next)
    {
      DenemoMeasure *themeasure = (DenemoMeasure *)curMeasure->data;
      measureElem = xmlNewChild(partElem, ns, (xmlChar *)"measure", NULL);
      xmlSetProp(measureElem, (xmlChar *)"number", (xmlChar *)g_strdup_printf("%d", j + 1));

      // output initial clef, time key in first measure

      if (j == 0)
      {
        xmlNodePtr attrElem = NULL;
        xmlNodePtr keyElem, clefElem, timeElem;
        attrElem = xmlNewChild(measureElem, ns, (xmlChar *)"attributes", NULL);

        // xmlNewChild (attrElem, ns, (xmlChar *) "divisions", "48");
        newXMLIntChild(attrElem, ns, (xmlChar *)"divisions", 384); // PPQN remarkably this is not a system-wide define
        keyElem = xmlNewChild(attrElem, ns, (xmlChar *)"key", NULL);
        timeElem = xmlNewChild(attrElem, ns, (xmlChar *)"time", NULL);
        clefElem = xmlNewChild(attrElem, ns, (xmlChar *)"clef", NULL);
        gchar *sign;
        gint line;
        gint octave = 0;
        newXMLIntChild(keyElem, ns, (xmlChar *)"fifths", curStaffStruct->keysig.number);
        xmlNewTextChild(keyElem, ns, (xmlChar *)"mode", (xmlChar *)(curStaffStruct->keysig.isminor ? "minor" : "major"));
        newXMLIntChild(timeElem, ns, (xmlChar *)"beats", curStaffStruct->timesig.time1);
        newXMLIntChild(timeElem, ns, (xmlChar *)"beat-type", curStaffStruct->timesig.time2);
        get_clef_sign(curStaffStruct->clef.type, &sign, &line, &octave);
        xmlNewTextChild(clefElem, ns, (xmlChar *)"sign", (xmlChar *)sign);
        newXMLIntChild(clefElem, ns, (xmlChar *)"line", line);
        if (octave)
          newXMLIntChild(clefElem, ns, (xmlChar *)"clef-octave-change", octave);
      }
      // output objects
      objnode *curObjNode = (objnode *)((DenemoMeasure *)curMeasure->data)->objects;
      gint actual_notes, normal_notes;
      gint num_beams = 0, next_beams = 0, incoming_beams = 0;

      gchar *pending_dynamic = NULL;
      for (; curObjNode != NULL; curObjNode = curObjNode->next)
      {
        DenemoObject *curObj = (DenemoObject *)curObjNode->data;
        DenemoObject *nextObj = (curObjNode->next ? curObjNode->next->data : NULL);
        DenemoObject *prevObj = (curObjNode->prev ? curObjNode->prev->data : NULL);
        xmlNodePtr directionElem = NULL;
        xmlNodePtr directionTypeElem = NULL;

        switch (curObj->type)
        {
          case CHORD:
            {

              xmlNodePtr pitchElem;
              xmlNodePtr noteElem;
              xmlNodePtr notationsElem = NULL;
              chord *thechord = (chord *)curObj->object;
              if (pending_dynamic)
              {
                if (directionElem == NULL)
                {
                  directionElem = xmlNewChild(measureElem, ns, (xmlChar *)"direction", NULL);
                  directionTypeElem = xmlNewChild(directionElem, ns, (xmlChar *)"direction-type", NULL);
                }
                xmlNodePtr dynamicsElem = xmlNewChild(directionTypeElem, ns, (xmlChar *)"dynamics", NULL);
                xmlNewChild(dynamicsElem, ns, (xmlChar *)pending_dynamic, NULL);
                pending_dynamic = NULL;
              }

              if (thechord->crescendo_begin_p)
              {
                if (directionElem == NULL)
                {
                  directionElem = xmlNewChild(measureElem, ns, (xmlChar *)"direction", NULL);
                  directionTypeElem = xmlNewChild(directionElem, ns, (xmlChar *)"direction-type", NULL);
                }
                xmlNodePtr wedgeElem = xmlNewChild(directionTypeElem, ns, (xmlChar *)"wedge", NULL);
                xmlSetProp(wedgeElem, (xmlChar *)"type", (xmlChar *)"crescendo");
              }

              if (thechord->diminuendo_begin_p)
              {
                if (directionElem == NULL)
                {
                  directionElem = xmlNewChild(measureElem, ns, (xmlChar *)"direction", NULL);
                  directionTypeElem = xmlNewChild(directionElem, ns, (xmlChar *)"direction-type", NULL);
                }
                xmlNodePtr wedgeElem = xmlNewChild(directionTypeElem, ns, (xmlChar *)"wedge", NULL);
                xmlSetProp(wedgeElem, (xmlChar *)"type", (xmlChar *)"diminuendo");
                xmlSetProp(wedgeElem, (xmlChar *)"spread", (xmlChar *)"11");
              }

              // the lowest note is plain, the rest in the chord have a xmlNewChild (noteElem, ns, (xmlChar *) "chord", NULL);
              // element and no bream tuplet slur

              GList *g;
              gboolean is_rest = FALSE;
              for (g = thechord->notes;; g = g->next)
              {

                noteElem = xmlNewChild(measureElem, ns, (xmlChar *)"note", NULL);

                if (g)
                {
                  if (g != thechord->notes)
                    xmlNewChild(noteElem, ns, (xmlChar *)"chord", NULL);
                  pitchElem = xmlNewChild(noteElem, ns, (xmlChar *)"pitch", NULL);
                  note *curnote = (note *)(g->data);
                  gchar *val = g_strdup_printf("%c", 'A' - 'a' + mid_c_offsettoname(curnote->mid_c_offset));
                  xmlNewTextChild(pitchElem, ns, (xmlChar *)"step", (xmlChar *)val); // the note-name from curnote
                  g_free(val);
                  newXMLIntChild(pitchElem, ns, (xmlChar *)"alter", curnote->enshift);
                  newXMLIntChild(pitchElem, ns, (xmlChar *)"octave", -OttavaVal + 3 + mid_c_offsettooctave(curnote->mid_c_offset)); // the octave from curnote
                                                                                                                                    // Crea un nodo "lyric" XML con la sillaba
                }
                else
                {
                  xmlNewChild(noteElem, ns, (xmlChar *)"rest", NULL);
                  is_rest = TRUE;
                }

                if (curObj->isinvisible)
                  xmlSetProp(noteElem, "print-object", "no");
                newXMLIntChild(noteElem, ns, (xmlChar *)"duration", curObj->durinticks); // 1 is duration a sounding duration quarter note

                gint m = thechord->numdots;
                for (; m; m--)
                  xmlNewChild(noteElem, ns, (xmlChar *)"dot", NULL);
                if (thechord->is_grace)
                {
                  xmlNodePtr graceElem = xmlNewChild(noteElem, ns, (xmlChar *)"grace", NULL);
                  if (thechord->is_grace & ACCIACCATURA)
                    xmlSetProp(graceElem, (xmlChar *)"slash", "yes");
                }
                gchar *durationType;
                determineDuration((thechord)->baseduration, &durationType);
                xmlNewTextChild(noteElem, ns, (xmlChar *)"type", (xmlChar *)durationType);


                // slurs

                // se lo staff ha lyrics, se non è una pausa, non è sotto una legatura
                if (has_lyrics && syllabe_iterator && !is_rest && !in_slur && !in_tie)
                {
                  sillaba = syllabe_iterator->data; // Ottieni la sillaba corrente
                  if (sillaba->syllable != NULL) {
                    char* type = sillaba->type;   // ottieni il tipo corrente
                    GString* s = sillaba->syllable;
                    xmlNodePtr lyricElem = xmlNewChild(noteElem, ns, (xmlChar *)"lyric", NULL);
                    xmlNodePtr typeElem = xmlNewTextChild(lyricElem, ns, (xmlChar *)"syllabic", (xmlChar *)type);
                    xmlNodePtr textElem = xmlNewTextChild(lyricElem, ns, (xmlChar *)"text", (xmlChar *)s->str);
                  }
                  // Passa alla prossima sillaba
                  syllabe_iterator = syllabe_iterator->next;
                }
                if (thechord->slur_begin_p)
                {
                  if (notationsElem == NULL)
                    notationsElem = xmlNewChild(noteElem, ns, (xmlChar *)"notations", NULL);
                  xmlNodePtr slurElem = xmlNewChild(notationsElem, ns, (xmlChar *)"slur", NULL);
                  xmlSetProp(slurElem, (xmlChar *)"number", "1");
                  xmlSetProp(slurElem, (xmlChar *)"type", "start");
                  in_slur = TRUE;
                }

                else if (thechord->slur_end_p)
                {
                  if (notationsElem == NULL)
                    notationsElem = xmlNewChild(noteElem, ns, (xmlChar *)"notations", NULL);
                  xmlNodePtr slurElem = xmlNewChild(notationsElem, ns, (xmlChar *)"slur", NULL);
                  xmlSetProp(slurElem, (xmlChar *)"number", "1");
                  xmlSetProp(slurElem, (xmlChar *)"type", "stop");
                  in_slur = FALSE;
                }
                // ties <tie type="start"/><tie type="stop"/>
                if (in_tie)
                {
                  if (notationsElem == NULL)
                    notationsElem = xmlNewChild(noteElem, ns, (xmlChar *)"notations", NULL);
                  xmlNodePtr tieElem = xmlNewChild(notationsElem, ns, (xmlChar *)"tied", NULL);
                  xmlSetProp(tieElem, (xmlChar *)"type", "stop");
                  tieElem = xmlNewChild(noteElem, ns, (xmlChar *)"tie", NULL); // for the sound
                  xmlSetProp(tieElem, (xmlChar *)"type", "stop");
                  in_tie = FALSE;
                }
                if (thechord->is_tied)
                {
                  if (notationsElem == NULL)
                    notationsElem = xmlNewChild(noteElem, ns, (xmlChar *)"notations", NULL);
                  xmlNodePtr tieElem = xmlNewChild(notationsElem, ns, (xmlChar *)"tied", NULL);
                  xmlSetProp(tieElem, (xmlChar *)"type", "start");
                  tieElem = xmlNewChild(noteElem, ns, (xmlChar *)"tie", NULL); // for the sound
                  xmlSetProp(tieElem, (xmlChar *)"type", "start");
                  in_tie = TRUE;
                }

                // tuplets
                if (in_tuplet)
                {
                  xmlNodePtr timemodElem = xmlNewChild(noteElem, ns, (xmlChar *)"time-modification", NULL);
                  newXMLIntChild(timemodElem, ns, (xmlChar *)"actual-notes", actual_notes);
                  newXMLIntChild(timemodElem, ns, (xmlChar *)"normal-notes", normal_notes);

                  if (tuplet_start)
                  {
                    tuplet_start = FALSE;
                    if (notationsElem == NULL)
                      notationsElem = xmlNewChild(noteElem, ns, (xmlChar *)"notations", NULL);
                    xmlNodePtr tupletElem = xmlNewChild(notationsElem, ns, (xmlChar *)"tuplet", NULL);
                    xmlSetProp(tupletElem, (xmlChar *)"type", (xmlChar *)"start");
                    xmlSetProp(tupletElem, (xmlChar *)"number", (xmlChar *)"1");
                  }
                  if (nextObj && nextObj->type == TUPCLOSE) // look ahead to see if there is a tuplet end coming up
                  {
                    if (notationsElem == NULL)
                      notationsElem = xmlNewChild(noteElem, ns, (xmlChar *)"notations", NULL);
                    xmlNodePtr tupletElem = xmlNewChild(notationsElem, ns, (xmlChar *)"tuplet", NULL);
                    xmlSetProp(tupletElem, (xmlChar *)"type", (xmlChar *)"stop");
                    xmlSetProp(tupletElem, (xmlChar *)"number", (xmlChar *)"1");
                  }
                }

                if ((g != NULL) && (g == thechord->notes)) // not for rests or subsequent notes in the chord
                {

                  //<beam number="1">begin</beam>
                  //<beam number="2">begin</beam>
                  //<beam number="1">end</beam>
                  //<beam number="2">end</beam> in <note>....
                  num_beams = beams(thechord);
                  next_beams = (nextObj && (nextObj->type == CHORD)) ? beams((chord *)nextObj->object) : 0;
                  gint outgoing_beams = 0;

                  if (!((incoming_beams == 0) && (next_beams == 0)))
                  {

                    if (curObj->isstart_beamgroup)
                    {
                      gint i; // g_assert (incoming_beams == 0);
                      for (i = 0; (i < num_beams) && (i < next_beams); i++)
                      {
                        xmlNodePtr beamElem = xmlNewTextChild(noteElem, ns, (xmlChar *)"beam", (xmlChar *)"begin");
                        gchar *val = g_strdup_printf("%d", i + 1);
                        xmlSetProp(beamElem, (xmlChar *)"number", val);
                        g_free(val); //<beam number="i+1">begin</beam>
                        outgoing_beams++;
                      }

                      for (; i < num_beams; i++)

                      {
                        xmlNodePtr beamElem = xmlNewTextChild(noteElem, ns, (xmlChar *)"beam", (xmlChar *)"forward hook");
                        gchar *val = g_strdup_printf("%d", i + 1);
                        xmlSetProp(beamElem, (xmlChar *)"number", val);
                        g_free(val); //<beam number="i+1">begin</beam>
                      }
                    }
                    else // not start of beam group
                    {
                      if (curObj->isend_beamgroup)
                      {

                        for (; num_beams > incoming_beams; num_beams--)
                        {
                          xmlNodePtr beamElem = xmlNewTextChild(noteElem, ns, (xmlChar *)"beam", (xmlChar *)"backward hook");
                          gchar *val = g_strdup_printf("%d", num_beams);
                          xmlSetProp(beamElem, (xmlChar *)"number", val);
                          g_free(val);
                        }
                        for (; num_beams > 0; num_beams--)
                        {
                          xmlNodePtr beamElem = xmlNewTextChild(noteElem, ns, (xmlChar *)"beam", (xmlChar *)"end");
                          gchar *val = g_strdup_printf("%d", num_beams);
                          xmlSetProp(beamElem, (xmlChar *)"number", val);
                          g_free(val); //<beam number="num_beams">end</beam>
                        }
                        outgoing_beams = 0;
                      }
                      else // not start nor end of beam group
                      {
                        if (incoming_beams)
                        {

                          outgoing_beams = incoming_beams;
                          for (; num_beams > next_beams; num_beams--)
                          {
                            xmlNodePtr beamElem;
                            if (num_beams > incoming_beams)
                              beamElem = xmlNewTextChild(noteElem, ns, (xmlChar *)"beam", (xmlChar *)"forward hook");
                            else
                            {
                              beamElem = xmlNewTextChild(noteElem, ns, (xmlChar *)"beam", (xmlChar *)"end");
                              outgoing_beams--;
                            }
                            gchar *val = g_strdup_printf("%d", num_beams);
                            xmlSetProp(beamElem, (xmlChar *)"number", val);
                            g_free(val); //<beam number="num_beams">end</beam>
                          }

                          for (; num_beams > incoming_beams; num_beams--)
                          {
                            xmlNodePtr beamElem = xmlNewTextChild(noteElem, ns, (xmlChar *)"beam", (xmlChar *)"begin");
                            gchar *val = g_strdup_printf("%d", num_beams);
                            xmlSetProp(beamElem, (xmlChar *)"number", val);
                            g_free(val);
                            outgoing_beams++;
                          }

                          for (; num_beams > 0; num_beams--)
                          {
                            xmlNodePtr beamElem = xmlNewTextChild(noteElem, ns, (xmlChar *)"beam", (xmlChar *)"continue");
                            gchar *val = g_strdup_printf("%d", num_beams);
                            xmlSetProp(beamElem, (xmlChar *)"number", val);
                            g_free(val); //<beam number="num_beams">end</beam>
                          }
                        }
                      }
                    }
                    incoming_beams = outgoing_beams;
                  }



                } // not for rests nor for chord notes above the lowest note

                if (g == NULL) // a rest
                  break;
                if (g->next == NULL)
                  break;
              } // for all notes in the chord

              if (thechord->crescendo_end_p)
              {
                if (directionElem == NULL)
                {
                  directionElem = xmlNewChild(measureElem, ns, (xmlChar *)"direction", NULL);
                  directionTypeElem = xmlNewChild(directionElem, ns, (xmlChar *)"direction-type", NULL);
                }
                xmlNodePtr wedgeElem = xmlNewChild(directionTypeElem, ns, (xmlChar *)"wedge", NULL);
                xmlSetProp(wedgeElem, (xmlChar *)"type", (xmlChar *)"stop");
                xmlSetProp(wedgeElem, (xmlChar *)"spread", (xmlChar *)"11");
              }

              if (thechord->diminuendo_end_p)
              {
                if (directionElem == NULL)
                {
                  directionElem = xmlNewChild(measureElem, ns, (xmlChar *)"direction", NULL);
                  directionTypeElem = xmlNewChild(directionElem, ns, (xmlChar *)"direction-type", NULL);
                }
                xmlNodePtr wedgeElem = xmlNewChild(directionTypeElem, ns, (xmlChar *)"wedge", NULL);
                xmlSetProp(wedgeElem, (xmlChar *)"type", (xmlChar *)"stop");
              }
            }
            break;
          case TUPOPEN:
            {
              tuplet_start = in_tuplet = TRUE;
              normal_notes = ((tupopen *)curObj->object)->numerator;
              actual_notes = ((tupopen *)curObj->object)->denominator;
            }
            break;
          case TUPCLOSE:
            {

              in_tuplet = FALSE;
            }
            break;

          case CLEF:
            {
              xmlNodePtr attrElem = xmlNewChild(measureElem, ns, (xmlChar *)"attributes", NULL);
              xmlNodePtr clefElem = xmlNewChild(attrElem, ns, (xmlChar *)"clef", NULL);
              gchar *sign;
              gint line;
              gint octave = 0;
              clef *theclef = (clef *)curObj->object;
              get_clef_sign(theclef->type, &sign, &line, &octave);
              xmlNewTextChild(clefElem, ns, (xmlChar *)"sign", (xmlChar *)sign);
              newXMLIntChild(clefElem, ns, (xmlChar *)"line", line);
              if (octave)
                newXMLIntChild(clefElem, ns, (xmlChar *)"clef-octave-change", octave);
            }
            break;
          case TIMESIG:
            {

              xmlNodePtr attrElem = xmlNewChild(measureElem, ns, (xmlChar *)"attributes", NULL);
              xmlNodePtr timeElem = xmlNewChild(attrElem, ns, (xmlChar *)"time", NULL);
              timesig *thetimesig = (timesig *)curObj->object;
              newXMLIntChild(timeElem, ns, (xmlChar *)"beats", thetimesig->time1);
              newXMLIntChild(timeElem, ns, (xmlChar *)"beat-type", thetimesig->time2);
            }
            break;

          case KEYSIG:
            {
              xmlNodePtr attrElem = xmlNewChild(measureElem, ns, (xmlChar *)"attributes", NULL);
              xmlNodePtr keyElem = xmlNewChild(attrElem, ns, (xmlChar *)"key", NULL);
              keysig *thekeysig = (keysig *)curObj->object;
              newXMLIntChild(keyElem, ns, (xmlChar *)"fifths", thekeysig->number);
              xmlNewTextChild(keyElem, ns, (xmlChar *)"mode", (xmlChar *)(thekeysig->isminor ? "minor" : "major"));
            }
            break;

          case LILYDIRECTIVE:
            // do dynamics etc,,,
            {
              xmlNodePtr directionElem = NULL;
              xmlNodePtr directionTypeElem = NULL;
              DenemoDirective *dir = (DenemoDirective *)curObj->object;
              if ((!strcmp(dir->tag->str, "TextAnnotation")) || (!strcmp(dir->tag->str, "MultiLineTextAnnotation")))
              {

                directionElem = xmlNewChild(measureElem, ns, (xmlChar *)"direction", NULL);
                if (dir->postfix && (*dir->postfix->str = '^'))
                  xmlSetProp(directionElem, (xmlChar *)"placement", (xmlChar *)"above");

                directionTypeElem = xmlNewChild(directionElem, ns, (xmlChar *)"direction-type", NULL);

                if (dir->display)
                  xmlNewChild(directionTypeElem, ns, (xmlChar *)"words", (xmlChar *)dir->display->str);
                break;
              }
              if (!strcmp(dir->tag->str, "DynamicText"))
              {
                pending_dynamic = (dir->postfix && (dir->postfix->len > 1) && (dir->postfix->len < 6)) ? dir->postfix->str + 2 : NULL; // pickup dynamic from Lily syntax e.g. /pp
                break;
              }
              if (!strcmp(dir->tag->str, "RepeatEndStart"))
              {

                //<barline location="left">
                //<bar-style>heavy-light</bar-style>
                //<repeat direction="forward" winged="none"/>
                //</barline>
                // xmlNodePtr  barlineElem = xmlNewChild (measureElem, ns, (xmlChar *) "barline", NULL);
                // xmlSetProp (barlineElem, (xmlChar *) "location", (xmlChar *) "left");
                // xmlNewChild (barlineElem, ns, (xmlChar *) "bar-style", (xmlChar *) "light-heavy");
                // xmlNodePtr repeatElem = xmlNewChild (barlineElem, ns, (xmlChar *) "repeat", NULL);
                // xmlSetProp (repeatElem, (xmlChar *) (xmlChar *) "direction", (xmlChar *) "forward");

                // barlineElem = xmlNewChild (measureElem, ns, (xmlChar *) "barline", NULL);
                // xmlSetProp (barlineElem, (xmlChar *) "location", (xmlChar *) "left");
                // xmlNewChild (barlineElem, ns, (xmlChar *) "bar-style", (xmlChar *) "heavy-light");
                // repeatElem = xmlNewChild (barlineElem, ns, (xmlChar *) "repeat", NULL);
                // xmlSetProp (repeatElem, (xmlChar *) (xmlChar *) "direction", (xmlChar *) "forward");

                xmlNodePtr barlineElem = xmlNewChild(measureElem, ns, (xmlChar *)"barline", NULL);
                xmlNodePtr repeatElem = xmlNewChild(barlineElem, ns, (xmlChar *)"repeat", NULL);
                xmlSetProp(repeatElem, (xmlChar *)"direction", (xmlChar *)"backward");
                barlineElem = xmlNewChild(measureElem, ns, (xmlChar *)"barline", NULL);
                repeatElem = xmlNewChild(barlineElem, ns, (xmlChar *)"repeat", NULL);
                xmlSetProp(repeatElem, (xmlChar *)"direction", (xmlChar *)"forward");

                break;
              }

              if (!strcmp(dir->tag->str, "RepeatEnd"))
              {
                xmlNodePtr barlineElem = xmlNewChild(measureElem, ns, (xmlChar *)"barline", NULL);
                xmlNodePtr repeatElem = xmlNewChild(barlineElem, ns, (xmlChar *)"repeat", NULL);
                xmlSetProp(repeatElem, (xmlChar *)"direction", (xmlChar *)"backward");
                break;
              }
              if (!strcmp(dir->tag->str, "RepeatStart"))
              {
                xmlNodePtr barlineElem = xmlNewChild(measureElem, ns, (xmlChar *)"barline", NULL);
                xmlNodePtr repeatElem = xmlNewChild(barlineElem, ns, (xmlChar *)"repeat", NULL);
                xmlSetProp(repeatElem, (xmlChar *)"direction", (xmlChar *)"forward");
                break;
              }

              // first time bar      <barline location="left">
              //<ending default-y="40" end-length="20" number="1" type="start"/>
              //</barline>
              // dir->data volta 1 otherwise (text . "3 and 4") etc

              if (!strcmp(dir->tag->str, "OpenNthTimeBar"))
              {
                xmlNodePtr barlineElem = xmlNewChild(measureElem, ns, (xmlChar *)"barline", NULL);
                xmlNodePtr endingElem = xmlNewChild(barlineElem, ns, (xmlChar *)"ending", NULL);
                xmlSetProp(barlineElem, (xmlChar *)"location", (xmlChar *)"left");
                nthTime = 1;
                if (dir->data)
                  (sscanf(dir->data->str, "'((volta . %d))", &nthTime));
                gchar *str = g_strdup_printf("%d", nthTime);
                xmlSetProp(endingElem, (xmlChar *)"number", (xmlChar *)str);
                xmlSetProp(endingElem, (xmlChar *)"type", (xmlChar *)"start");
                g_free(str);
                break;
              }

              //<barline location="right">
              //<bar-style>light-heavy</bar-style>
              //<ending number="1" type="stop"/>
              //<repeat direction="backward" winged="none"/>
              //</barline>

              if (!strcmp(dir->tag->str, "EndVolta"))
              {
                xmlNodePtr barlineElem = xmlNewChild(measureElem, ns, (xmlChar *)"barline", NULL);
                xmlSetProp(barlineElem, (xmlChar *)"location", (xmlChar *)"right");
                xmlNodePtr endingElem = xmlNewChild(barlineElem, ns, (xmlChar *)"ending", NULL);

                gchar *str = g_strdup_printf("%d", nthTime);
                xmlSetProp(endingElem, (xmlChar *)"number", (xmlChar *)str);
                xmlSetProp(endingElem, (xmlChar *)"type", (xmlChar *)"stop");
                g_free(str);
                break;
              }

              if (!strcmp(dir->tag->str, "RehearsalMark"))
                if (directionElem == NULL)
                {
                  directionElem = xmlNewChild(measureElem, ns, (xmlChar *)"direction", NULL);
                  directionTypeElem = xmlNewChild(directionElem, ns, (xmlChar *)"direction-type", NULL);
                }
              xmlNodePtr wedgeElem = xmlNewChild(directionTypeElem, ns, (xmlChar *)"rehearsal", "A");

              if (!strcmp(dir->tag->str, "Ottava"))
              {
                if (directionElem == NULL)
                {
                  directionElem = xmlNewChild(measureElem, ns, (xmlChar *)"direction", NULL);
                  directionTypeElem = xmlNewChild(directionElem, ns, (xmlChar *)"direction-type", NULL);
                }
                xmlNodePtr octElem = xmlNewChild(directionTypeElem, ns, (xmlChar *)"octave-shift", NULL);
                gint amount;
                get_ottava(dir, &amount);
                OttavaVal = amount;
                if (amount)
                  xmlSetProp(octElem, (xmlChar *)"type", amount > 0 ? (xmlChar *)"up" : (xmlChar *)"down");
                else
                  xmlSetProp(octElem, (xmlChar *)"type", (xmlChar *)"stop");
                amount = ABS(amount);
                if (amount)
                  xmlSetProp(octElem, (xmlChar *)"size", amount == 1 ? (xmlChar *)"8" : (xmlChar *)"15");
              }

            } // end of case LILYDIRECTIVE

            break;
          default:
            break;
        } // switch type

      } // for each object
    } // for each measure

  } // for each staff

  // for each movement not done
  /* Save the file. */

  xmlSaveCtxt *ctxt = xmlSaveToFilename(filename->str, "UTF-8", XML_SAVE_FORMAT | XML_SAVE_NO_EMPTY);
  if (!ctxt || xmlSaveDoc(ctxt, doc) < 0 || xmlSaveClose(ctxt) < 0)
  {
    g_warning("Could not save file %s", filename->str);
    ret = -1;
  }

  /* Clean up all the memory we've allocated. */

  xmlFreeDoc(doc);
  g_string_free(filename, TRUE);
  return ret;
}

