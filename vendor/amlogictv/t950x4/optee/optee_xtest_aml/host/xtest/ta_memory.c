#include <err.h>
#include <stdio.h>
#include <string.h>
#include <tee_client_api.h>

int main(int argc, char *argv[])
{
	TEEC_Result res;
	TEEC_Context ctx[8];
	TEEC_Session sess[8];
	TEEC_Operation op;
	TEEC_UUID uuid_list[] = {
		{ 0x807798e0, 0xf011, 0x11e5,
			0xa5, 0xfe, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b },
		{ 0xe043cde0, 0x61d0, 0x11e5,
			0x9c, 0x26, 0x00, 0x02, 0xa5, 0xd5, 0xc5, 0x1b},
		{ 0x9a04f079, 0x9840, 0x4286,
			0xab, 0x92, 0xe6, 0x5b, 0xe0, 0x88, 0x5f, 0x95 },
		{ 0xc1106894, 0x9655, 0x4c43,
			0xa8, 0x70, 0xb1, 0xe6, 0xf9, 0xa7, 0x15, 0x79 },
		{ 0x3ce00ef5, 0x1fc4, 0x4b63,
			0xa6, 0x6b, 0xe2, 0x76, 0xb0, 0x59, 0x42, 0x4b },
		{ 0xbbb5e112, 0xa722, 0x4df7,
			0x90, 0xdf, 0x5a, 0xb3, 0x8e, 0xf1, 0x1b, 0x9e },
		{ 0x1987f328, 0x1755, 0x9321,
			0x54, 0x41, 0x43, 0x49, 0x43, 0x43, 0x54, 0x4c },//for aml ci+ TA
		{ 0x5f440c5c,0x87cc,0x4e97,
			0x96, 0x32, 0x74, 0xe4, 0x0a, 0x19, 0x4a, 0x1f },//for aml fvp TA
	};
	uint32_t err_origin;
	uint32_t num;
	char buff[256];
	uint32_t u[8];
	uint32_t a, b, c, d, e, f,g,h, i;
	int cycle = 0;

	/* Initialize a context connecting us to the TEE */
	for (a = 0; a < 8; a++)
		for (b = 0; b < 8; b++)
			for (c = 0; c < 8; c++)
				for (d = 0; d < 8; d++)
					for (e = 0; e < 8; e++)
						for (f = 0; f < 8; f++)
                                                  	for (g = 0; g < 8; g++)
                                                          	for (h = 0; h < 8; h++)
									if (a != b && a != c && a != d 
									&& a != e && a != f && a != g && a != h
									&& b != c && b != d && b != e && b != f && b != g && b != h
									&& c != d && c != e && c != f && c != g && c != h
									&& d != e && d != f && d != g && d != h
									&& e != f && e != g && e != h
									&& f != g && f != h 
									&& g != h ) {

	u[0] = a;
	u[1] = b;
	u[2] = c;
	u[3] = d;
	u[4] = e;
	u[5] = f;
	u[6] = g;
	u[7] = h;

	printf("--------cycle %d-----------------------\n", cycle);
	for ( i = 0; i < 8; i++) {
		num = u[i];
		printf("num = %d\n", u[i]);
		printf("open uuid = 0x%x\n", uuid_list[num].timeLow);
		res = TEEC_InitializeContext(NULL, &(ctx[i]));
		if (res != TEEC_SUCCESS)
			errx(1, "TEEC_InitializeContext failed with code 0x%x", res);

		res = TEEC_OpenSession(&(ctx[i]), &(sess[i]), &(uuid_list[num]),
				       TEEC_LOGIN_PUBLIC, NULL, NULL, &err_origin);
		if (res != TEEC_SUCCESS)
			errx(1, "TEEC_Opensession failed with code 0x%x origin 0x%x",
				res, err_origin);

		usleep(10000);
	}
	for ( i = 0; i < 8; i++) {
		num = u[i];
		printf("close uuid = 0x%x\n", uuid_list[num].timeLow);
		TEEC_CloseSession(&(sess[i]));
		TEEC_FinalizeContext(&(ctx[i]));
		usleep(10000);
	}
	cycle++;
						}

	return 0;
}
