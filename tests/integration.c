#include <glib.h>
#include <unistd.h>
#include <config.h>

/* Integration tests are those which launch the program in different 
 * environments, and with different parameters.
 */

#define DENEMO "../src/denemo"
#define EXAMPLE_DIR "examples"
#define DATA_DIR "integration-data"
#define TEMP_DIR "integration-tmp"

static gchar* data_dir = NULL;
static gchar* temp_dir = NULL;
static gchar* example_dir = NULL;

/*******************************************************************************
 * SETUP AND TEARDOWN
 ******************************************************************************/

static void
setup(gpointer fixture, gconstpointer data)
{
  if(!data_dir)
    data_dir = g_build_filename(PACKAGE_SOURCE_DIR, "tests", DATA_DIR, NULL);

  if(!temp_dir)
    temp_dir = g_build_filename(g_get_current_dir (), TEMP_DIR, NULL);

  if(!example_dir)
    example_dir = g_build_filename(PACKAGE_SOURCE_DIR, EXAMPLE_DIR, NULL);
  
  if(!g_file_test(temp_dir, G_FILE_TEST_EXISTS)){
    if(g_mkdir(temp_dir, 0777) < 0)
      g_warning("Could not create %s", temp_dir);
  }
}

static void
teardown(gpointer fixture, gconstpointer data)
{  
  if(g_file_test(temp_dir, G_FILE_TEST_EXISTS)){
    if(g_remove(temp_dir) < 0)
      g_warning("Could not remove %s", temp_dir);
  }
}

/*******************************************************************************
 * TEST FUNCTIONS
 ******************************************************************************/

/** test_run_and_quit:
 * This is the simpliest test. It just launches denemo and quit.
 */
static void
test_run_and_quit (gpointer fixture, gconstpointer data)
{
  if (g_test_trap_fork (0, 0))
    {
      execl(DENEMO, DENEMO, "-n", "-e", "-a", "(d-Quit)", NULL);
      g_warn_if_reached ();
    }
  g_test_trap_assert_passed ();
}

/** test_open_blank_file
 * Opens a blank file and quits.
 */
static void
test_open_blank_file(gpointer fixture, gconstpointer data)
{
  gchar* input = g_build_filename(data_dir, "blank.denemo", NULL);
  if (g_test_trap_fork (0, 0))
    {
      execl(DENEMO, DENEMO, "-n", "-e", input, NULL);
      g_warn_if_reached ();
    }
  g_test_trap_assert_passed ();
}

/** test_open_save_blank_file
 * Opens a blank file, saves it, tries to reopen it and quits.
 * the input one.
 */
static void
test_open_save_blank_file(gpointer fixture, gconstpointer data)
{
  const gchar* output = g_build_filename(temp_dir, "blank.denemo", NULL);
  const gchar* input  = g_build_filename(data_dir, "blank.denemo", NULL);
  gchar* input_contents = NULL;
  gchar* output_contents = NULL;

  if (g_test_trap_fork (0, 0))
    {
      gchar* scheme = g_strdup_printf("(d-SaveAs \"%s\")(d-Quit)", output);
      execl(DENEMO, DENEMO, "-n", "-e", "-a", scheme, input, NULL);
      g_warn_if_reached ();
    }
  g_test_trap_assert_passed ();

  g_assert(g_file_test(output, G_FILE_TEST_EXISTS));

  if (g_test_trap_fork (0, 0))
    {
      execl(DENEMO, DENEMO, "-n", "-e", output, NULL);
      g_warn_if_reached ();
    }
  g_test_trap_assert_passed ();
  
  g_remove(output);
  /* TODO:
  g_file_get_contents(input, &input_contents, NULL, NULL);
  g_file_get_contents(output, &output_contents, NULL, NULL);
  g_assert_cmpstr(input_contents, ==, output_contents);
  */
}

/** test_open_save_complex_file
 * Opens a complex file, saves, tries to reopen it and quits.
 * the input one.
 */
static void
test_open_save_complex_file(gpointer fixture, gconstpointer data)
{
  const gchar* output = g_build_filename(temp_dir, "AllFeaturesExplained.denemo", NULL);
  const gchar* input  = g_build_filename(example_dir, "AllFeaturesExplained.denemo", NULL);
  gchar* input_contents = NULL;
  gchar* output_contents = NULL;

  if (g_test_trap_fork (0, 0))
    {
      gchar* scheme = g_strdup_printf("(d-SaveAs \"%s\")(d-Quit)", output);
      execl(DENEMO, DENEMO, "-n", "-e", "-a", scheme, input, NULL);
      g_warn_if_reached ();
    }
  g_test_trap_assert_passed ();

  g_assert(g_file_test(output, G_FILE_TEST_EXISTS));

  if (g_test_trap_fork (0, 0))
  {
    execl(DENEMO, DENEMO, "-n", "-e", output, NULL);
    g_warn_if_reached ();
  }
  g_test_trap_assert_passed ();
  
  g_remove(output);
  /* TODO:
  g_file_get_contents(input, &input_contents, NULL, NULL);
  g_file_get_contents(output, &output_contents, NULL, NULL);
  g_assert_cmpstr(input_contents, ==, output_contents);
  */
}

/** test_invalid_scheme
 * Tests the --fatal-scheme-errors program argument.
 */
static void
test_invalid_scheme(gpointer fixture, gconstpointer data)
{
  if (g_test_trap_fork (0, 0))
    {
      execl(DENEMO, DENEMO, "-n", "--fatal-scheme-errors", "-a", "(d-InvalidSchemeFunction)(d-Quit)", NULL);
      g_warn_if_reached ();
    }
  g_test_trap_assert_failed ();
}

/** test_scheme_log
 * Tests (d-LogError) scheme function
 */
static void
test_scheme_log(gpointer fixture, gconstpointer data)
{
  if (g_test_trap_fork (0, 0))
    {
      execl(DENEMO, DENEMO, "-n", "-e", "--verbose", "-a",
            "(d-Debug \"This is debug\")"
            "(d-Info \"This is info\")"
            "(d-Message \"This is message\")"
            "(d-Warning \"This is warning\")"
            "(d-Critical \"This is critical\")"
            "(d-Quit)",
            NULL);
      g_warn_if_reached ();
    }
  g_test_trap_assert_passed ();
}

/** test_scheme_log_error
 * Tests (d-LogError) scheme function
 */
static void
test_scheme_log_error(gpointer fixture, gconstpointer data)
{
  if (g_test_trap_fork (0, 0))
    {
      execl(DENEMO, DENEMO, "-n", "--fatal-scheme-errors", "-a", "(d-Error \"This error is fatal\")(d-Quit)", NULL);
      g_warn_if_reached ();
    }
  g_test_trap_assert_failed ();
}

/** test_thumbnailer
 * Tries to create a thumbnail from a file and check that its exists
 */
static void
test_thumbnailer(gpointer fixture, gconstpointer data)
{
  gchar* thumbnail = g_build_filename(temp_dir, "thumbnail.png", NULL);
  gchar* scheme = g_strdup_printf( "(d-CreateThumbnail #f \"%s\")(d-Exit)", thumbnail, temp_dir);
  gchar* input = g_build_filename(data_dir, "blank.denemo", NULL);
  
  g_printf("Running scheme: %s %s\n", scheme, input);
  if (g_test_trap_fork (0, 0))
    {
      execl(DENEMO, DENEMO, "-n", "-e", "-V", "-a", scheme, input, NULL);
      g_warn_if_reached ();
    }

  g_test_trap_assert_passed ();
  g_assert(g_file_test(thumbnail, G_FILE_TEST_EXISTS));
  g_assert(g_remove(thumbnail) >= 0);
}

/*******************************************************************************
 * MAIN
 ******************************************************************************/

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  if(!g_file_test(DENEMO, G_FILE_TEST_EXISTS))
    g_error("Denemo has not been compiled successfully");

  g_test_add ("/integration/run-and-quit", void, NULL, setup, test_run_and_quit, teardown);
  g_test_add ("/integration/open-blank-file", void, NULL, setup, test_open_blank_file, teardown);
  g_test_add ("/integration/open-and-save-blank-file", void, NULL, setup, test_open_save_blank_file, teardown);
  g_test_add ("/integration/open-and-save-complex-file", void, NULL, setup, test_open_save_complex_file, teardown);
  //g_test_add ("/integration/invalid-scheme", void, NULL, setup, test_invalid_scheme, teardown);
  g_test_add ("/integration/scheme-log", void, NULL, setup, test_scheme_log, teardown);
  g_test_add ("/integration/scheme-log-error", void, NULL, setup, test_scheme_log_error, teardown);
  g_test_add ("/integration/thumbnailer", void, NULL, setup, test_thumbnailer, teardown);

  return g_test_run ();
}