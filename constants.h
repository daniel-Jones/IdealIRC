/*
 *   IdealIRC - Internet Relay Chat client
 *   Copyright (C) 2015  Tom-Andre Barstad
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License along
 *   with this program; if not, write to the Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

/*! \file constants.h
 *  \brief Constant values that should be accessible throughout most classes of IdealIRC.
 *
 * To get some deeper understanding, it's better to look through constants.h to get the
 * idea of how things work.
 *
 * Notable constants:\n
 * SYSTEM_NAME - Returns Linux, Windows, etc. The platform which it's compiled on.\n
 * CONF_PATH - The folder path to which iirc.ini resides. / is not appended.\n
 * COMMON_PATH - Common files. This is where IdealIRC can write files to; scripts for example.\n
 * SKEL_PATH - Skeleton path. Only used for PACKAGED builds. This is where IdealIRC copies default configs and scripts to users home folder.\n
 * CONF_FILE - /path/to/iirc.ini\n
 * SERV_FILE - /path/to/servers.ini\n
 * SW_EMPTY_SET - Construct an empty/"invalid" subwindow_t with this. See constants.h for example.\n\n
 *
 * Constants prepended with...\n
 * C_: Colors defined as QColor.\n
 * WT_: Window type defined as integer.\n
 * PT_: Print type defined as integer\n
 * HL_: Highlight type (window switcher (button bar and tree view) colors, essentially) defined as integer\n
 * CTRL_: Control codes defined as char, the actual char for, for example, bold or underline.\n
 * These constants will be rewritten to enums.
 */

#ifndef CONSTANTS_H
#define CONSTANTS_H

/* Version constants */
#define VERSION_STRING "0.4.2" //<! Human readable version, the official version.
#define VERSION_INTEGER 20 //<! Increased by one for each release. Used to check for new versions if enabled.

//#define IIRC_DEBUG_SCRIPT 1
//#define IIRC_DEBUG_TWIN 1


// Define if this compile is a Standalone or Packaged version
// STANDALONE: .zip'ed IIRC to be used on memory sticks, etc.
// PACKAGED: Installed IIRC to system and read configs from ~/.idealirc
#define PACKAGED


// Define system names and OS specifics IIRC supports here.
// SYSTEM_NAME defines system/kernel type.
// CONF_PATH contains where to put config file when running as packaged. DO NOT append /.

#ifdef Q_OS_DARWIN // Not confirmed support
  //#define SYSTEM_NAME "Mac OS X"
#endif

#ifdef Q_OS_WIN32
  #define SYSTEM_NAME "Windows"
#endif

#ifdef Q_OS_LINUX
  // GNU fanatics may set this to GNU/Linux, but IdealIRC does not _need_ GNU to work.
  #define SYSTEM_NAME "Linux"
#endif

#ifdef Q_OS_FREEBSD
  #define SYSTEM_NAME "FreeBSD"
#endif


#ifndef SYSTEM_NAME
  #error IIRC does not support this platform! If so, update 'constants.h'.
#endif



#ifdef STANDALONE
  #include <QApplication>
  #define CONF_PATH QApplication::applicationDirPath()
#endif

#ifdef PACKAGED
  #include <QStandardPaths>
  #ifdef Q_OS_WIN32
    #include <QApplication>
    #define CONF_PATH QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
    #define COMMON_PATH QApplication::applicationDirPath()
  #else
    #define CONF_PATH QString("%1/.idealirc").arg(QStandardPaths::writableLocation(QStandardPaths::HomeLocation))
    #define COMMON_PATH "/usr/share/idealirc"
  #endif
  #define SKEL_PATH QString("%1/skel").arg(COMMON_PATH)
#endif

#ifndef CONF_PATH
  #error This build is set as neither STANDALONE nor PACKAGED. Cannot compile!
#endif

#define CONF_FILE QString("%1/iirc.ini").arg(CONF_PATH)
#define SERV_FILE QString("%1/servers.ini").arg(CONF_PATH)


/* Colors */
#define C_WHITE           QColor(0xFF, 0xFF, 0xFF)
#define C_BLACK           QColor(0x00, 0x00, 0x00)
#define C_BLUE            QColor(0x00, 0x00, 0x84)
#define C_GREEN           QColor(0x00, 0x84, 0x00)
#define C_BRIGHTRED       QColor(0xFF, 0x00, 0x00)
#define C_RED             QColor(0x84, 0x00, 0x00)
#define C_MAGENTA         QColor(0x84, 0x00, 0x84)
#define C_BROWN           QColor(0x84, 0x84, 0x00)
#define C_YELLOW          QColor(0xFF, 0xFF, 0x00)
#define C_BRIGHTGREEN     QColor(0x00, 0xFF, 0x00)
#define C_CYAN            QColor(0x00, 0x84, 0x84)
#define C_BRIGHTCYAN      QColor(0x00, 0xFF, 0xFF)
#define C_BRIGHTBLUE      QColor(0x00, 0x00, 0xFF)
#define C_BRIGHTMAGENTA   QColor(0xFF, 0x00, 0xFF)
#define C_DARKGRAY        QColor(0x84, 0x84, 0x84)
#define C_LIGHTGRAY       QColor(0xC6, 0xC6, 0xC6)

/* Window type */
#define WT_NOTHING  0  //!< Nothing to view (usually default value, displaying an error)
#define WT_STATUS   1  //!< Text display with input box (Only for status windows, if its the only status window, it cannot be closed)
#define WT_DCCSEND  2  //!< Custom widget for file sending
#define WT_DCCRECV  3  //!< Custom widget for file receiving
#define WT_DCCCHAT  4  //!< Text display with input box
#define WT_CHANNEL  5  //!< Text display with input box and list box
#define WT_PRIVMSG  6  //!< Text display with input box
#define WT_TXTONLY  7  //!< Scriptable/Custom window. Text display only
#define WT_TXTINPUT 8  //!< Scriptable/Custom window. Text display with input
#define WT_GRAPHIC  9  //!< Scriptable/Custom window. Graphic window
#define WT_GWINPUT  10 //!< Scriptable/Custom window. Graphic window with input box

/* Print type */
#define PT_NORMAL     0  //!< Normal text
#define PT_LOCALINFO  1  //!< Information text from IIRC
#define PT_SERVINFO   2  //!< Information text from server
#define PT_NOTICE     3  //!< Notice from server, user, user:channel
#define PT_ACTION     4  //!< ACTION style message (/me is afk)
#define PT_CTCP       5  //!< CTCP request / reply
#define PT_OWNTEXT    7  //!< Messages sent from us
#define PT_HIGHLIGHT  8  //!< Messages marked as highlighted

/* Window highlight type */
#define HL_NONE       0     //!< No activity (black)
#define HL_ACTIVITY   1     //!< Join, Quit, etc (dark red)
#define HL_MSG        2     //!< Messages (red)
#define HL_HIGHLIGHT  3     //!< Highlighted messages (blue)

/* Control codes */
#define CTRL_BOLD           0x02
#define CTRL_UNDERLINE      0x1F
#define CTRL_COLOR          0x03
#define CTRL_ITALIC         0x09 //!< Not implemented.
#define CTRL_REVERSE        0x16 //!< Not implemented.
#define CTRL_RESET          0x0F
#define CTRL_STRIKETHROUGH  0x13 //!< Not implemented.

/*! \enum e_painting Script painting commands -- DEPRECATED - Most likely not used anymore. */
enum e_painting {
    pc_clear = 0, // clear the window, this can also be used in text windows.
    pc_paintdot,
    pc_paintline,
    pc_paintrect,
    pc_paintimage,
    pc_clearimagebuffer = 5,
    pc_paintcircle,
    pc_painttext,
    pc_paintfill,
    pc_buffer,
    pc_setlayer = 10,
    pc_dellayer
};

/*! \enum e_iircevent Script events. The names used are same ones found in wiki documentation, pretty self-explainatory too. */
enum e_iircevent {
    te_noevent = 0,
    te_load,
    te_unload,
    te_start,
    te_exit,
    te_connect = 5,
    te_disconnect,
    te_join,
    te_part,
    te_quit,
    te_msg = 10,
    te_sockopen,
    te_sockread,
    te_sockclose,
    te_sockerror,
    te_socklisten = 15,
    te_mousemove,
    te_mouseleftdown,
    te_mouseleftup,
    te_mousemiddledown,
    te_mousemiddleup = 20,
    te_mousemiddleroll,
    te_mouserightdown,
    te_mouserightup,
    te_urlclick,
    te_dbuttonclick = 25,
    te_dlistboxselect,
    te_ialhostget,
    te_input,
    te_numeric,
    te_activate = 30,
    te_deactivate
};

/*! \enum e_scriptresult Results TScript::runf() can give. */
enum e_scriptresult {
    se_None = 0,
    se_Finished,
    se_FileNotExist,
    se_FileEmpty,
    se_FileCannotOpen,
    se_UnexpectedFinish = 5,
    se_RunfDone,
    se_BreakNoWhile,
    se_ContinueNoWhile,
    se_InvalidParamCount,
    se_FunctionEmpty = 10,
    se_InvalidFunction,
    se_InvalidCommand,
    se_InvalidMetaCommand,
    se_InvalidSwitches,
    se_InvalidIncludeFile = 15,
    se_InvalidEvent,
    se_InvalidBlockType,
    se_InvalidTimer,
    se_InvalidFileDescriptor,
    se_MissingVariable = 20,
    se_FunctionIdxOutOfBounds,
    se_TimerAlreadyDefined,
    se_UnexpectedToken,
    se_EscapeOnEndLine,
    se_UnexpectedNewline = 25,
    se_NegativeNotAllowed,
    se_FSeekFailed,
    se_UnrecognizedMenu
};

class IWin;
class QMdiSubWindow;
class QTreeWidgetItem;
class IConnection;
/*!
 * Subwindow type.\n
 * All subwindows is stored within one of these,.
 */
typedef struct SUBWINDOW_T {
  IWin *widget; //!< The widget that goes onto QMdiSubWindow
  IConnection *connection; //!< IRC connection this window handles on. Customn windows set this to NULL.
  QMdiSubWindow *subwin; //!< The dialog
  QTreeWidgetItem *treeitem; //!< Tree widget item
  int wid; //!< Window ID.
  int parent; //!< 0 = custom windows. >0 represents a network we're on.
  int type; //!< Window type. See constants.h for WT_*
  int highlight; //!< Highlight type. See constants.h for HL_*
} subwindow_t;

#define SW_EMPTY_SET {nullptr, nullptr, nullptr, nullptr, -1, -1, WT_NOTHING, 0};
/* Construct an empty/"invalid" subwindow_t with this.
   for example:
     ...
     subwindow_t sw_empty = SW_EMPTY_SET;
     subwindow_t sw = winlist->value(windowName, sw_empty); // 'sw_empty' will be default lookup result if 'windowName' weren't found.
     if (sw.type == WT_NOTHING) {
         // subwindow was not found
     }
     ...
*/

#endif // CONSTANTS_H
