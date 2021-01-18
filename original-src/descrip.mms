!
! MMS/MMK build file for CD V5.5b
!
!Global build flag macros
!
MDEBUG = /DEB
SHRDBG = "/debug"
MFLAGS = /DEB

.IFDEF __AXP__
.SUFFIXES .ALPHA_OBJ
MFLAGS = /MIGRATE$(MFLAGS)/NOWARN
DBG = .ALPHA_DBG
EXE = .ALPHA_EXE
OBJ = .ALPHA_OBJ
OPT = .ALPHA_OPT
TARG = .AXP
MISS = .VAX
.ELSE
DBG = .DBG
EXE = .EXE
OPT = .OPT
OBJ = .OBJ
TARG = .VAX
MISS = .AXP
.ENDIF

PURGEOBJ = if f$search("$(MMS$TARGET_NAME)$(OBJ);-1").nes."" then purge/log $(MMS$TARGET_NAME)$(OBJ)

!
!Bend the default build rules for C, MACRO, and MESSAGE
!
.MAR$(OBJ) :
	$(MACRO) $(MFLAGS) $(MMS$SOURCE)$(MDEBUG)/OBJECT=$(MMS$TARGET_NAME)$(OBJ)
	$(PURGEOBJ)
.MSG$(OBJ) :
	MESSAGE $(MMS$SOURCE)/OBJECT=$(MMS$TARGET_NAME)$(OBJ)
	$(PURGEOBJ)

DEFAULT				:	CD$(EXE),-
					CD_USER$(EXE)

DEBUG				:	CD$(DBG)

!
!CD image build
!
CD$(EXE)			:	CD$(OPT),-
					CD$(OBJ),-
					CDMSG$(OBJ),-
					CDPARSE$(OBJ),-
					PUTMSG$(OBJ)
	$ link/contig/notrace cd$(OPT)/opt/exe=cd$(EXE)

CD$(DBG)			:	CD$(EXE),-
					CD_USER$(EXE)
	$ link/contig cd$(OPT)/opt/exe=cd$(DBG)/deb

!
!CD_USER example image build
!
CD_USER$(EXE)			:	CD_USER$(OPT),-
					CD_USER$(OBJ)
	$ link/contig/notrace/share=cd_user$(EXE) cd_user$(OPT)/opt

!
!Begin object module dependency lists
!
CD$(OBJ)			:	CD.MAR

CDPARSE$(OBJ)			:	CDPARSE.MAR

CD_USER$(OBJ)			:	CD_USER.MAR

PUTMSG$(OBJ)			:	PUTMSG.MAR

CDMSG$(OBJ)			:	CDMSG.MSG
