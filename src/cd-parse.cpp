/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set ts=8 sts=4 et sw=4 tw=80: */
/*      
 * Ported from VAX MACRO to C++ by Jake Hamby <jake.hamby@gmail.com>.
 * Original source code in [.orig-src].
 */

#define __NEW_STARLET 1

#include "cd.h"
#include "vms-file-ops.h"
#include "vms-get-info.h"
#include "vms-quiet-setprv.h"
#include "vms-sys-command-io.h"
#include "vms-table-parser.h"

#include <inttypes.h>
#include <string.h>

#include <gen64def.h>
#include <lib$routines.h>
#include <rms.h>
#include <ssdef.h>
#include <starlet.h>

/* Import state table references. */

#define state_table_0 STATE_TABLE_0
#define key_table_0   KEY_TABLE_0

extern uint32_t *state_table_0;
extern uint32_t *key_table_0;;

/* Define global variables and action routines referenced by table parser. */

// Convert the names to uppercase in case we do case-sensitive linking.
#define do_op           DO_OP
#define fmt_node_dev    FMT_NODE_DEV
#define is_logical      IS_LOGICAL
#define is_personal_id  IS_PERSONAL_ID
#define switches        SWITCHES
#define retsts          RETSTS
#define retstv          RETSTV

extern "C" {
    int do_op(TableParser &parser);
    int fmt_node_dev(TableParser &parser);
    int is_logical(TableParser &parser);
    int is_personal_id(TableParser &parser);
    uint32_t switches;
    uint32_t retsts;    // return status
    uint32_t retstv;    // return status value (used by CD_IVIDENT)
}

int fmt_node_dev(TableParser &parser) {
    return SS$_BUGCHECK;
}

int is_logical(TableParser &parser) {
    return SS$_BUGCHECK;
}

int is_personal_id(TableParser &parser) {
    return SS$_BUGCHECK;
}

/* Local constants and macros */

// do_op() opcodes: these must match the values in cdparse_table.mar.
enum opcode {
    OP_C_BSL = 1,   // '\<eos>' and '/<eos>' processing
    OP_C_RST = 2,   // reset parser to top of string
    OP_C_DOL = 3,   // '$<eos>' processing
    OP_C_DD  = 4,   // '..' --> '-' processing
    OP_C_DEV = 5,   // devnam extraction
    OP_C_BRI = 6,   // '['..']' add w/insert
    OP_C_BRO = 7,   // '['..']' add w/overwrite
    OP_C_BDI = 8,   // '[.'..']' add w/insert
    OP_C_DOT = 9,   // change something at current token to '.'
    OP_C_DEL = 10,  // Deletes current char
    OP_C_RMV = 11,  // Deletes present parse position to beginning of line
    OP_C_BCK = 12,  // Back parser up one char                                                                
    OP_C_AT  = 13,  // code for username lookup                                                               
    OP_C_PND = 14,  // '#<eos>' processing                                                                    
    OP_C_SCF = 15,  // Store Command File                                                                     
    OP_C_NOD = 16,  // nodnam extraction                                                                      
    OP_C_UPC = 17,  // Force accepted token uppercase & back up the parser                                    
    OP_C_FID = 18,  // Cvt/UnCvt to/from FIDded specification                                                 
    OP_C_PAR = 19   // Force use of parent DID for current dir                    
};

/*** Switch flags (must match definitions in cdparse_table.mar). ***/

static const int SW_M_LOG               = 1 << 0;
static const int SW_M_VERIFY            = 1 << 1;
static const int SW_M_FULL              = 1 << 2;
static const int SW_M_COM               = 1 << 3;
static const int SW_M_INHIBIT           = 1 << 4;
static const int SW_M_PARTIAL           = 1 << 5;
static const int SW_M_AUTO_ANSWER       = 1 << 6;
static const int SW_M_COM_FSPEC         = 1 << 7;
static const int SW_M_NO_ARGS_HOME      = 1 << 8;
static const int SW_M_CSH_HISTORY       = 1 << 9;
static const int SW_M_CSH_PUSHD         = 1 << 10;
static const int SW_M_CSH_POPD          = 1 << 11;
static const int SW_M_VERSID            = 1 << 12;
static const int SW_M_NO_INHIBIT        = 1 << 13;
static const int SW_M_TRACE             = 1 << 14;

/*** Use dynamic string descriptors for string handling. ***/

// Primary translation buffer (Holds cmd string segment being parsed/executed)
static DynamicStringDesc buffer;

static DynamicStringDesc node_name;         // holds node name
static DynamicStringDesc device_name;       // holds device name
static DynamicStringDesc command_file;      // command file

STATIC_FIXED_STRING_DESC(username, 12);     // always 12 characters

/* Static string descriptors used by the code. */

static StaticStringDesc sys_command(    C_STR_INIT("SYS$COMMAND"));
static StaticStringDesc lnm_file_dev(   C_STR_INIT("LNM$FILE_DEV"));
static StaticStringDesc cd_ident(       C_STR_INIT("CDI_"));
static StaticStringDesc dash(           C_STR_INIT("-"));
static StaticStringDesc left_br_dot(    C_STR_INIT("[."));
static StaticStringDesc left_br(        C_STR_INIT("["));
static StaticStringDesc right_br(       C_STR_INIT("]"));
static StaticStringDesc dir_delimiters( C_STR_INIT("./\\"));
static StaticStringDesc root(           C_STR_INIT("[000000]"));
static StaticStringDesc previous_dir(   C_STR_INIT("LAST_DEFAULT_DIRECTORY"));
static StaticStringDesc previous_dir_0( C_STR_INIT("CD$0"));
static StaticStringDesc stack_dir(      C_STR_INIT("CD$!UL"));  // CD$0...CD$9
static StaticStringDesc default_image(  C_STR_INIT("CD_USER"));
static StaticStringDesc default_symbol( C_STR_INIT("CD_PROCESS"));
static StaticStringDesc did_format(     C_STR_INIT("[!UL,!UL,!UL]"));
static StaticStringDesc star_star_dir(  C_STR_INIT("sys$disk:[]*.dir;1"));
static StaticStringDesc buffer_trace(   C_STR_INIT("Trace: !AS"));

/* TODO: these strings should be loaded from the message library. */
//static StaticStringDesc not_defined,     "not defined");
//static StaticStringDesc select_prompt,   "_Selection: ");
//static StaticStringDesc exit_announce,   "\x1b[7mExit\x1b[m");
//static StaticStringDesc quit_announce,   "\x1b[7mQuit\x1b[m");
//static StaticStringDesc yes_no_prompt,   " ? (Y/N/Q) [N]: ");

/*** Define function prototypes for cd_parse(). ***/

/**
 * Helper to call the table parser for a single argument, which has
 * been copied to the buffer by cd_parse(). The action handlers for
 * the parser will manipulate the buffer, so it's simpler to make it
 * a static global variable. (Everything in this file could be turned
 * into a class, but it probably wouldn't add much value.)
 */
int cd_parse_helper() {
    int status = lib$put_output(&buffer);   // debug
    // clear node name, device name
    node_name.clear();
    device_name.clear();

    TableParser parser(buffer);
    if (false) {
        status = parser.parse(state_table_0,
                              key_table_0);
    }

    status = lib$put_output(&buffer);   // debug
    int err_msgvec[] = { 2, status, 0 };
    status = sys$putmsg(err_msgvec, 0, 0, 0);   // debug
    return status;
}

/**
 * Parse the command line arguments with the help of lib$table_parse().
 * The state machine action handlers will do most of the work.
 */
int cd_parse(int argc, char *argv[], struct cd_parse_results &results) {
    int status;
    READALL(0);         // disable READALL if we have it

    // DEBUG only: print current directory
    FIXED_STRING_DESC(curdir, 4096);
    status = Filesystem::get_current_directory(curdir);
    int err_msgvec[] = { 2, status, 0 };
    status = sys$putmsg(err_msgvec, 0, 0, 0);
    lib$put_output(&curdir);
    // DEBUG end

    // iterate over the args
    for (int i = 1; i < argc; ++i) {
        // copy the arg to the shared buffer, so we can manipulate it
        buffer.assign(argv[i]);
        
        // look for "@" or "~" to split into two pieces (first, change to
        // the specified home directory, then proceed from there). This is
        // how CD V6.0A works (outside of the table parser), and it does
        // seem to simplify the logic of the parser helper.
        if (buffer[0] == '@' || buffer[0] == '~') {
            // helper method returns 0-based position to avoid confusion.
            int split_pos = buffer.find_first_in(dir_delimiters);

            if (split_pos > 0) {
                buffer.truncate(split_pos);
                status = cd_parse_helper();
                ERRCHK_RETURN(status);

                buffer.assign(argv[i] + split_pos + 1);
                // fallthrough to parse second half
            }
        }

        // the buffer may be empty for, e.g. "~/"
        if (buffer.length() != 0) {
            status = cd_parse_helper();
        }
        ERRCHK_RETURN(status);
    }

    // print final messages and set return flags
//    results.flags = RESULT_LAUNCH_HELP;

    return SS$_NORMAL;
}

/*** Local helper functions for do_op(). ***/

/* print dollar translation */
static int print_dollar_translation(void) {
    // TODO
    return SS$_BUGCHECK;
}

/* ask user to select choice for `CD$*`. */
static int get_dollar_choice(void) {
    // acc_syscom
    // prt_dol_tran
    // get_syscom_num_r5
    // deacc_syscom
    return SS$_BUGCHECK;
}

/* helper for OP_C_DOL handler. */

/* helper for OP_C_PND handler. */
static void dol_dmp_prev(int digit) {
}

/**
 * TPARSE action routines: Perform an operation by operation number.
 */
int do_op(TableParser &parser) {
    int status = SS$_NORMAL;        // initialize to success

    // switch on the opcode passed from our state table.
    // cases are reordered slightly from the MACRO version,
    // to match the order of the enums, and without goto.
    switch (parser.user_param()) {
        // 1. '\<eos>' or '/<eos>'
        case OP_C_BSL:
            buffer.assign(root);                // copy '[000000]'
            parser.reset(buffer);
            break;

        // 2. reset operation
        case OP_C_RST:
            parser.reset(buffer);
            break;

        // 3. '$[n]<eos>'
        case OP_C_DOL: {
            // back up 1 char to get the digit
            char digit = *(parser.next_char_ptr() - 1);
            parser.reset(buffer);
            // TODO: finish me
            return lib$stop(SS$_BUGCHECK);  // stop and signal an error
            break;
            }

        // 4. convert '..' --> '-'
        case OP_C_DD: {
            size_t dd_pos = parser.position(buffer) - 2;    // point to '..'
            buffer[dd_pos++] = '-';         // replace with '-' and advance
            buffer.slide_left_at(1, dd_pos + 1);    // slide back a char
            parser.reset(buffer, dd_pos);   // update parser pointers
            break;
            }

        // 5. copy token to dev name
        case OP_C_DEV: {
            // copy device name, including ':'
            size_t name_length = parser.position(buffer);
            device_name.assign(buffer.buf_ptr(), name_length);

            // slide string back and reset parser
            buffer.slide_left(name_length);
            parser.reset(buffer);
            break;
            }

        // 6. '['..']' add w/insert
        case OP_C_BRI: {
            buffer.prefix_with(left_br);
            buffer.append(right_br);
            parser.reset(buffer);
            break;
            }

        // 7. '['..']' add w/overwrite
        case OP_C_BRO: {
            buffer.prefix_with(left_br);
            buffer.append(right_br);
            parser.reset(buffer);
            break;
            }

        // 8. '[.'..']' add w/insert
        case OP_C_BDI: {
            buffer.prefix_with(left_br_dot);
            buffer.append(right_br);
            parser.reset(buffer);
            break;
            }

        // 9. '$[n]<eos>'
        case OP_C_DOT: {
            // TODO: finish me
            return lib$stop(SS$_BUGCHECK);  // stop and signal an error
            break;
            }

        // 10. Deletes current char
        case OP_C_DEL: {
            size_t pos = parser.position(buffer);
            buffer.slide_left_at(1, pos);
            parser.reset(buffer, pos);      // Note: just in case buffer moved
            break;
            } 

        // 11. Deletes present parse position to beginning of line
        case OP_C_RMV:
            break;

        // 12. Back parser up one char (known to exist)
        case OP_C_BCK: {
            parser.rewind_by(1);
            break;
            }

        // 13. '@username<eos>' or '~username<eos>'
        case OP_C_AT:
            // look up sys$login for our own user, or another user
            break;

        // 14. '#{n|*}<eos>' (the {} is forced by the tparse table)
        case OP_C_PND: {
            // back up 1 char to get the digit (or *)
            char digit = *(parser.next_char_ptr() - 1);

            // go try to get translation
            dol_dmp_prev(digit);
            break;
            }

        // 15. store command filename
        case OP_C_SCF:
            command_file.assign(parser.token_ptr(), parser.token_length());
            break;

        // 16. node_name extraction
        case OP_C_NOD: {
            // copy node name, including '::'
            size_t name_length = parser.position(buffer);
            node_name.assign(buffer.buf_ptr(), name_length);

            // slide string back and reset parser
            buffer.slide_left(name_length);
            parser.reset(buffer);
            break;
            }

        // 17. make current token uppercase & back up the parser
        case OP_C_UPC: {
            size_t token_length = parser.token_length();
            // make static descriptor for uppercasing substring
            StaticStringDesc token(token_length, parser.token_ptr());
            status = str$upcase(&token, &token);  // uppercase in place
            parser.rewind_by(token_length);
            break;
            }

        // 18. Cvt/UnCvt to/from FIDded specification
        case OP_C_FID:
            break;

        // 19. Force use of parent DID for current dir
        case OP_C_PAR:
            break;

        default:
            return lib$stop(SS$_BUGCHECK);  // stop and signal an error
    }

    return status;  // return any previous failure, or SS$_NORMAL
}
