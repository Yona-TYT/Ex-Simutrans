/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SYS_SIMSYS_H
#define SYS_SIMSYS_H


#include "../simtypes.h"
#include "../display/scr_coord.h"
#include "../simcolor.h"

#ifndef NETTOOL
#include <zlib.h>
#endif

#include <cstddef>

// Provide chdir().
#if defined(_WIN32) && !defined(__CYGWIN__)
#	include <direct.h>
#else
#	include <unistd.h>
#endif


enum sim_event_type_t : uint32
{
	SIM_NOEVENT         = 0,
	SIM_MOUSE_BUTTONS   = 1,
	SIM_KEYBOARD        = 2,
	SIM_MOUSE_MOVE      = 3,
	SIM_STRING          = 4,
	SIM_SYSTEM          = 254,
	SIM_IGNORE_EVENT    = 255
};


/// Mouse actions
enum sim_mouse_action_t : unsigned long
{
	SIM_MOUSE_LEFTUP      = 1,
	SIM_MOUSE_RIGHTUP     = 2,
	SIM_MOUSE_MIDUP       = 3,
	SIM_MOUSE_LEFTBUTTON  = 4,
	SIM_MOUSE_RIGHTBUTTON = 5,
	SIM_MOUSE_MIDBUTTON   = 6,
	SIM_MOUSE_MOVED       = 7,
	SIM_MOUSE_WHEELUP     = 8,
	SIM_MOUSE_WHEELDOWN   = 9
};


struct sys_event_t
{
	sim_event_type_t type;
	union {
		unsigned long code;
		void *ptr;
	};

	// mouse position; negative coords are possible
	scr_coord_val mx;
	scr_coord_val my;
	uint16 mb; // Combination of MOUSE_* flags from simevent.h

	/// new window size for SYSTEM_RESIZE
	uint16 new_window_size_w;
	uint16 new_window_size_h;

	/// pressed modifiers, combination of SIM_MOD_* flags from simevent.h
	unsigned int key_mod;
};

enum { WINDOWED, FULLSCREEN, BORDERLESS };

/// Global Variable for message processing
extern sys_event_t sys_event;

extern char const PATH_SEPARATOR[];


/// @param scale_percent
///   Possible values:
///     -1:    auto (scale according to screen DPI setting)
///      0:    off (default 1:1 scale)
///     other: specific scaling, in percent
///     < -1:  invalid
bool dr_set_screen_scale(sint16 scale_percent);

/// @returns Relative size of the virtual display, in percent
sint16 dr_get_screen_scale();


bool dr_os_init(int const* parameter);

/* maximum size possible (if there) */
struct resolution
{
	scr_coord_val w;
	scr_coord_val h;
};
resolution dr_query_screen_resolution();

int dr_os_open(scr_size window_size, sint16 fullscreen);
void dr_os_close();

// returns the locale; NULL if unknown
const char *dr_get_locale_string();

// Functions the same as normal mkdir except path must be UTF-8 encoded and a default mode of 0777 is assumed.
int dr_mkdir(char const* path);

/**
 * Moves the specified file to the system's trash bin.
 * If trash is not available on the platform, removes file.
 * @param path UTF-8 path to the file to delete.
 * @return False on success.
 */
bool dr_movetotrash(const char *path);

/**
 * Returns true if platform supports recycle bin, otherwise false.
 * Used to control which UI tooltip is shown for deletion.
 */
bool dr_cantrash();

// Functions the same as cstdio remove except path must be UTF-8 encoded.
int dr_remove(const char *path);

// rename a file and delete eventually existing file new_utf8
int dr_rename( const char *existing_utf8, const char *new_utf8 );

// Functions the same as chdir except path must be UTF-8 encoded.
int dr_chdir(const char *path);

// Functions the same as getcwd except path must be UTF-8 encoded.
char *dr_getcwd(char *buf, size_t size);

// Functions the same as fopen except filename must be UTF-8 encoded.
FILE *dr_fopen(const char *filename, const char *mode);

#ifndef NETTOOL
// Functions the same as gzopen except path must be UTF-8 encoded.
gzFile dr_gzopen(const char *path, const char *mode);
#endif

// Functions the same as stat except path must be UTF-8 encoded.
int dr_stat(const char *path, struct stat *buf);

/**
* Check if the directory exists and if so set the result variable to it
* If the directory doesn't exist previously, it will attempt to create it if testfile is not provided
* @param path : Path to directory to check
* @param info : String to provide extra log info
* @param result : Variable to store the path to directory if it exists
* @param testfile : Optional file to check for existence
*/
bool check_and_set_dir(const char *path, const char *info, char *result, const char *testfile=NULL);

/**
* Sets the base_dir
* @param base_dir_arg : command line argument "-set_basedir"
* @param executable_path : Path to executable (as stored in argv[0])
*/
bool dr_set_basedir(const char *base_dir_arg, char *executable_path);

/* query home directory */
char const* dr_query_homedir();

/* query install directory (all user writable) */
char const* dr_query_installdir();

unsigned short* dr_textur_init();

// returns the file path to a font file (or more than one, if used with number higher than zero)
const char *dr_query_fontpath( int );

// return a default TTF (windows/android) or a BDF for now
std::string dr_get_system_font();

void dr_textur(int xp, int yp, int w, int h);

/* returns the actual width (might be larger than requested! */
int dr_textur_resize(unsigned short** textur, int w, int h);

// needed for screen update
void dr_prepare_flush(); // waits, if previous update not yet finished
void dr_flush();         // copy to screen (eventually multithreaded)

/**
 * Transform a 24 bit RGB color into the system format.
 * @return converted color value
 */
PIXVAL get_system_color(rgb888_t rgb);

void show_pointer(int yesno);

void set_pointer(int loading);

bool move_pointer(int x, int y);

scr_coord get_mouse_pos();

void ex_ord_update_mx_my();

void GetEvents();

uint8 dr_get_max_threads();

uint32 dr_time();
void dr_sleep(uint32 millisec);

// error message in case of fatal events
void dr_fatal_notify(char const* msg);

/**
 * Copy text to the clipboard
 * @param source : pointer to the start of the source text
 * @param length : number of character bytes to copy
 */
void dr_copy(const char *source, size_t length);

/**
 * Paste text from the clipboard
 * @param target : pointer to the insertion position in the target text
 * @param max_length : maximum number of character bytes which could be inserted
 * @return number of character bytes actually inserted -> for cursor advancing
 */
size_t dr_paste(char *target, size_t max_length);

/**
 * Open a program/starts a script to download pak sets.
 * @param data_dir : The current simutrans data directory (usually the same as env_t::base_dir)
 * @param portable : true if local files to be save in simutransdir
 * @return false, if nothing was downloaded
 */
bool dr_download_pakset( const char *data_dir, bool portable );

/**
 * Shows the touch keyboard when using systems without a hardware keyboard.
 * Will be ignored if there is an hardware keyboard available.
 */
void dr_start_textinput();

/**
 * Hides the touch keyboard when using systems without a hardware keyboard.
 * Will be ignored it there is no on-display keyboard shown.
 */
void dr_stop_textinput();

/**
 * Inform the IME of a ideal place to open its popup.
 */
void dr_notify_input_pos(scr_coord pos);

///  returns current two byte languange ID
const char* dr_get_locale();

/// true, if there is a hardware fullcreen mode
bool dr_has_fullscreen();

/**
 * @return
 *  0: if windowed
 *  1: if fullscreen
 *  2: if borderless fullscreen
 */
sint16 dr_get_fullscreen();

/**
 * Toggle between borderless and windowed mode
 * @return the fullscreen state after the toggle
 */
sint16 dr_toggle_borderless();

/* temparily minimizes window and restore it */
sint16 dr_suspend_fullscreen();
void dr_restore_fullscreen(sint16 old_fullscreen);

const char* get_version();

int sysmain(int argc, char** argv);

#endif
