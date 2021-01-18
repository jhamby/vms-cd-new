! Message format:
!    3128 27          16 15             32 0
!    cccc ffff ffff ffff mmmm mmmm mmmm msss
!    |    |              |               |
!    |    |              |               +--- Severity
!    |    |              +------------------- Message number
!    |    +---------------------------------- Facility number
!    +--------------------------------------- Control
.Facility	CD,1/prefix=CD_

!Success messages
DIREX		<Specified directory exists>/fao=0/suc
DIRSETDEF	<>/fao=0/suc
.literal CD_DIRSET=(CD_DIRSETDEF+^x10000000) !Poke silent bit on
HELPDONEDEF	<>/fao=0/suc
.literal CD_HELPDONE=(CD_HELPDONEDEF+^x10000000) !Poke silent bit on
OUTPUTDONEDEF	<>/fao=0/suc
.literal CD_OUTPUTDONE=(CD_OUTPUTDONEDEF+^x10000000) !Poke silent bit on
VERSION		<Version 6.0A, ©1999, TECSys Development, Inc.>/fao=0/suc
!
!Debugging junk - cheap-easy (cheasy) way to do $FAO!
FAO_AS		<!AS>/fao=1/suc
FAO_AS_AS	<!AS_!AS>/fao=2/suc
FAO_UL		<!UL>/fao=1/suc
FAO_UL_UL	<!UL_!UL>/fao=2/suc

!Informational messages
CURIS		<Current directory is !AS>/fao=1/inf
CURDIR		<!AS>/fao=1/inf
PRVIS		<Previous directory is !AS>/fao=1/inf
PRVDIR		<!AS>/fao=1/inf
PRVNDF		<Previous directory is not defined>/fao=0/inf
TRGWAS		<Target directory was !AS!AS>/fao=2/inf
NOCHNG		<Directory did not change>/fao=0/inf
HELP1		<Define the logical name CDHELP to point to the CD help library>/fao=0/inf
HELP2		<or place the CD help library in SYS$HELP:>/fao=0/inf
HELP3		<Refer to the CD distribution contents for further information>/fao=0/inf

!Warning messages

!Error messages
DIRNEX		<Specified directory does not exist>/fao=0/err
NOHELP		<Help usage is inhibited in module CDPARSE>/fao=0/err
MEMALCFAI	<Memory allocation failure %X!XL from LIB$GET_VM>/fao=1/err
NOCDHELP	<CD help library could not be located>/fao=0/err
EOMSGVEC	<End of message vector reached>/fao=0/err

!Fatal messages and associated sub-messages
SYNTAX		<Syntax error in command>/fao=0/fat
SYNTXT		<!_/!AS/>/fao=1/inf
UNKSTS		<Unrecognized status returned: %X!XL>/fao=1/fat
UNKTXT		<Buffer contained /!AS/>/fao=1/inf
DIRERR		<Error %X!XL recovering new directory... abort>/fao=1/fat
BUGCHECK	<Internal consistency failure>/fao=0/fat
CURDIRBAD	<Cannot get/set current directory... abort>/fao=0/fat
USERNF		<Requested username not found>/fao=0/fat
IVIDENT		<Invalid identifier format>/fao=0/fat
FSPERROR	<Cannot TRIM target filespec, code %X!XL>/fao=1/fat
UNEXPERR	<Unexpected error occurred: %X!XL>/fao=1/fat
EOMSGERR	<End of message vector reached in error>/fao=0/fat

.end