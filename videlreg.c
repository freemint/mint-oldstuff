#include <stdio.h>
#include <mintbind.h>
#include <falcon.h>
#include "vcon.h"
#include "vtdev.h"
#include "falconres.h"

void read_hw(void)
{
	int n;

	printf("int nxreg[6]={0x%x",f030_xreg[0]);
	for(n=1;n<6;n++) {
		printf(",0x%x",f030_xreg[n]);
	}
	printf("};\n");

	printf("int nyreg[6]={0x%x",f030_yreg[0]);
	for(n=1;n<6;n++) {
		printf(",0x%x",f030_yreg[n]);
	}
	printf("};\n");

	printf("int nsreg[4]={0,0,0,0x%x};\n",f030_sreg[3]);

	printf("int ncreg[2]={0x%x,0x%x};\n",f030_creg[0],f030_creg[1]);

	printf("int nmreg[4]={0x%x,0,0,0x%x};\n",f030_mreg[0],f030_mreg[3]);
}

void main(void)
{
	Supexec(read_hw);
}
