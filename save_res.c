#include <stdio.h>
#include <mintbind.h>
#include <falcon.h>
#include <linea.h>
#include "vcon.h"
#include "vtdev.h"
#include "falconres.h"

VIDEL_DATA videl_data;

void read_hw(void)
{
	int i;

	for(i=0;i<6;i++)
		videl_data.nxreg[i] = f030_xreg[i];

	for(i=0;i<6;i++)
		videl_data.nyreg[i] = f030_yreg[i];

	videl_data.nsreg[0] = videl_data.nsreg[1] = videl_data.nsreg[2] = 0;
	videl_data.nsreg[3] = f030_sreg[3];

	for(i=0;i<2;i++)
		videl_data.ncreg[i] = f030_creg[i];

	videl_data.nmreg[0] = f030_mreg[0];
	videl_data.nmreg[1] = videl_data.nmreg[2] = 0;
	videl_data.nmreg[3] = f030_mreg[3];
}

#define ALT_FILENAME	"etc_res"

int main()
{
	FILE *f;
	int alt_file = 0;

	linea0();
	videl_data.x_res = V_CEL_MX+1;
	videl_data.y_res = V_CEL_MY+1;
	if (V_BYTES_LIN != videl_data.x_res) {
		printf("Switch to mono resolution (2 colour graphics mode)\n");
		getchar();
		return 1;
	}
	printf("Detected resolution: %d x %d\n", videl_data.x_res, videl_data.y_res);
	Supexec(read_hw);
	strncpy(videl_data.ident, VIDEL_DATA_IDENT, sizeof(videl_data.ident));
	f = fopen(VIDEL_DATA_FILENAME, "wb");
	if (f == NULL) {
		printf("Cannot write to "VIDEL_DATA_FILENAME"\n");
		f = fopen(ALT_FILENAME, "wb");
		if (f == NULL) {
			printf("Writting to current dir failed as well. What's up?\n");
			getchar();
			return 2;
		}
		else {
			printf("Writting to alternate file '"ALT_FILENAME"' in current dir.\nPlease copy that file to "VIDEL_DATA_FILENAME" at your earliest convenience\n");
			alt_file = 1;
		}
	}
	fwrite(&videl_data, sizeof(videl_data), 1, f);
	fclose(f);

	printf("Resolution data written to '%s' correctly.\n", alt_file ? ALT_FILENAME : VIDEL_DATA_FILENAME);
	getchar();

	return 0;
}
