!
! Build CD.EXE and help library.
!

! Note: the default build setting optimizes for the host architecture.
! Remove "/ARCH=HOST" if you need to build an .exe to run on older CPUs.
! This is mostly important for Alpha (to use the byte-word extensions).

.IFDEF DEBUG
OPTFLAGS = /DEBUG
.ELSE
OPTFLAGS = /OPTIMIZE/ARCH=HOST
.ENDIF

! NOTE: comment out the following #define if you don't want CD to
! quietly enable/disable the READALL privilege, if the user or image
! is authorized for it (for "cd ~user" and directory traversal).

DEFS = /DEFINE=QUIET_ENABLE_READALL

! Turn off C++ exceptions and RTTI to reduce the .exe size.
! The I64 compiler should silently ignore the /NORTTI option.
! Also, add our OPTFLAGS and DEFS.

CXXFLAGS = $(OPTFLAGS) $(DEFS) /NOEXCEPTIONS/NORTTI

! Add .cpp as a recognized suffix.
.SUFFIXES : .cpp

all DEPENDS_ON cd.exe, cdhelp.hlb
        ! all targets built

! Build rule for the exe.
cd.exe DEPENDS_ON cd.obj, cd-parse.obj, cd-parse-table.obj,-
                  cd-messages.obj, vms-sys-command-io.obj,-
                  vms-file-ops.obj, vms-quiet-setprv.obj
	CXXLINK cd.opt/OPT/EXEC=cd.exe

! Use a shorter name for the compiled help library.
cdhelp.hlb DEPENDS_ON cd-help.hlp
        library/create/help cdhelp cd-help

cd-messages.obj         DEPENDS_ON cd-messages.msg

cd.obj                  DEPENDS_ON cd.cpp, cd.h, vms-descriptors.h,-
                                   vms-file-ops.h, cd-messages.msg

cd-parse.obj            DEPENDS_ON cd-parse.cpp, cd.h, vms-descriptors.h,-
                                   vms-file-ops.h, vms-get-info.h,-
                                   vms-quiet-setprv.h, vms-sys-command-io.h,-
                                   vms-table-parser.h

vms-quiet-setprv.obj    DEPENDS_ON vms-quiet-setprv.cpp, vms-quiet-setprv.h,-
                                   vms-get-info.h, vms-descriptors.h

vms-file-ops.obj        DEPENDS_ON vms-file-ops.cpp, vms-file-ops.h,-
                                   vms-descriptors.h, cd-messages.msg

vms-sys-command-io.obj  DEPENDS_ON vms-sys-command-io.cpp,-
                                   vms-sys-command-io.h,-
                                   vms-descriptors.h

.cpp.obj
        CXX $(CXXFLAGS) $(MMS$SOURCE)

clean :
        DEL *.obj;*, cd.exe;*, cdhelp.hlb;*
