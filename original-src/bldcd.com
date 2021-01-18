$!x='f$ver(0)'
$ cdhbld=f$search("cdhelp.hlb").eqs.""
$ cdhsrc=f$search("cdhelp.hlp").nes.""
$ if cdhbld.and.cdhsrc then -
    library/create/help sys$disk:[]cdhelp sys$disk:[]cdhelp
$ if cdhbld.and.cdhsrc then -
    write sys$output "Built CDHELP - place in SYS$HELP or define logical name (see documentation)"
$ if f$getsyi("hw_model") .ge. 1024
$   then
$     write sys$output "Building an ALPHA/VMS image"
$     write sys$output "Images will be called *.ALPHA_EXE"
$     opt = "ALPHA_OPT"
$     exe = "ALPHA_EXE"
$     dbg = "ALPHA_DBG"
$     macro/migrate/deb=all/noop cd/obj=cd.alpha_obj
$     macro/migrate/deb=all/noop cdparse/noflag/obj=cdparse.alpha_obj
$     macro/migrate/deb=all/noop putmsg/noflag/obj=putmsg.alpha_obj
$     macro/migrate/deb=all/noop cd_user/noflag/obj=cd_user.alpha_obj
$     message cdmsg/obj=cdmsg.alpha_obj
$   else
$     write sys$output "Building a VAX/VMS image"
$     opt="OPT"
$     exe = "EXE"
$     dbg = "DBG"
$     macro cd/debug/obj=cd.obj
$     macro cdparse/debug/obj=cdparse.obj
$     macro putmsg/debug/obj=putmsg.obj
$     macro cd_user/debug/obj=cd_user.obj
$     message cdmsg/obj=cdmsg.obj
$   endif
$ if "''p1'"
$ then
$   link/debug cd.'opt'/opt/exe=cd.'dbg'
$   link/debug/share=cd_user.'dbg' cd_user.'opt'/opt
$ else
$   link/contig/notrace cd.'opt'/opt/exe=cd.'exe'
$   link/contig/notrace/share=cd_user.'exe' cd_user.'opt'/opt
$ endif
$ if cdhbld then -
    write sys$output "Don't forget to define CDHELP or put CDHELP.HLB into SYS$HELP!"
$ exit