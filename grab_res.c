#include <stdio.h>
#include <mintbind.h>
#include <falcon.h>
#include <linea.h>
#include "falconres.h"

VIDEL_DATA videl_data;

/* grab_res.tos; instead of saving the current resolution, this one
   prints the registers on standard output. Ideal for inclusion in
   the default fields in screen.c. -smoore */

/* Videl get/put routines.
**
** By Steven Moore, thanks to evil/dhs, chris/aura, 
** scandion/mugwumps, sage/escape. 
*/

void VIDEO_getvideo(VIDEL_DATA *vd);

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

void readvidel(void) { VIDEO_getvideo(&videl_data); }

int main()
{
	FILE *f;
	int n;

	linea0();
	videl_data.x_res = V_CEL_MX+1;
	videl_data.y_res = V_CEL_MY+1;

	Supexec(readvidel);
	/* strncpy(videl_data.ident, VIDEL_DATA_IDENT, sizeof(videl_data.ident)); */

	printf("VIDEL_DATA videl_data_yournamehere = { VIDEL_DATA_IDENT, %d, %d,\n", videl_data.x_res, videl_data.y_res);

	printf("  { ");
	for (n=0; n<5; n++) printf("0x%x, ", videl_data.nxreg[n]);
	printf("0x%x },\n", videl_data.nxreg[5]);

	printf("  { ");
	for (n=0; n<5; n++) printf("0x%x, ", videl_data.nyreg[n]);
	printf("0x%x },\n", videl_data.nyreg[5]);
	
	printf("  0x%x, 0x%x, { 0x%x, 0x%x }, \n", videl_data.nvco, videl_data.nc_s, videl_data.noff[0], videl_data.noff[1]);

	printf("  0x%x, 0x%x, %d, 0x%x, 0x%x };\n\n", videl_data.nsync, videl_data.np_o, videl_data.st_flag, videl_data.nsps, videl_data.nsts);

	return 0;
}
