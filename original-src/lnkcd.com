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
$   else
$     write sys$output "Building a VAX/VMS image"
$     opt="OPT"
$     exe = "EXE"
$   endif
$ link/contig/notrace cd.'opt'/opt/exe=cd.'exe'
$ link/contig/notrace/share=cd_user.'exe' cd_user.'opt'/opt
$ if cdhbld then -
    write sys$output "Don't forget to define CDHELP or put CDHELP.HLB into SYS$HELP!"
$ exit