#include <stdio.h>
#include <mintbind.h>
#include <falcon.h>
#include <linea.h>
#include "vcon.h"
#include "vtdev.h"
#include "falconres.h"

/* Videl get/put routines.
**
** By Steven Moore, thanks to evil/dhs, chris/aura, 
** scandion/mugwumps, sage/escape. 
*/

/* VIDEO_putvideo : takes a VIDEL_DATA (VCF3) structure, 
   and writes it (correctly) to the Videl hardware.
   Translation: it changes resolutions. 

   First revision: smoore. 

   Caveat: Must be run in supervisor mode (i.e., Supexec().) 
*/
void VIDEO_putvideo(VIDEL_DATA *vd)
{
	int i;
	VIDEL_hw_spshift[0]=0; /* Falcon shift clear */
	for(i=0; i<6; i++)
		VIDEL_hw_xregs[i]=vd->nxreg[i]; /* copy xregs */
	for(i=0; i<6; i++)
		VIDEL_hw_yregs[i]=vd->nyreg[i]; /* copy yregs */
	VIDEL_hw_vco[0]=vd->nvco; /* control flags */
	VIDEL_hw_c_s[0]=vd->nc_s;
	VIDEL_hw_offsets[0]=vd->noff[0]; /* line offset regs */
	VIDEL_hw_offsets[1]=vd->noff[1];
	VIDEL_hw_sync[0]=vd->nsync; /* sync reg */
	VIDEL_hw_p_o[0]=vd->np_o; /* horizontal planar offset */
	if (vd->st_flag) {
		VIDEL_hw_stshift[0]=vd->nsts; /* st compatible - write as follows */
		VIDEL_hw_c_s[0]=vd->nc_s; /* rewrite these three afterwards! */
		VIDEL_hw_offsets[0]=vd->noff[0];
		VIDEL_hw_offsets[1]=vd->noff[1];
	} else {
		Vsync(); /* necessary to avoid the famous sync-shifting-by-a-word bug */
		VIDEL_hw_spshift[0]=vd->nsps;
	}
}

/* VIDEO_getvideo : grabs the data on the
   current resolution, and saves it into a 
   VIDEL_DATA (VCF3) structure.

   First revision: smoore. 

   Caveat: Must be run in supervisor mode (i.e., Supexec().) 
*/
void VIDEO_getvideo(VIDEL_DATA *vd)
{
	int i;
	for(i=0; i<6; i++)
		vd->nxreg[i]=VIDEL_hw_xregs[i];
	for(i=0; i<6; i++)
  	vd->nyreg[i]=VIDEL_hw_yregs[i];
	vd->nvco=VIDEL_hw_vco[0];
	vd->nc_s=VIDEL_hw_c_s[0];
	vd->noff[0]=VIDEL_hw_offsets[0];
	vd->noff[1]=VIDEL_hw_offsets[1];
	vd->nsync=VIDEL_hw_sync[0];
	vd->np_o=VIDEL_hw_p_o[0];
  vd->st_flag=(VIDEL_hw_xregs[0] <= 0x00b0);  /* look out! logical expression */
  vd->nsps=VIDEL_hw_spshift[0];
  vd->nsts=VIDEL_hw_stshift[0];
}
