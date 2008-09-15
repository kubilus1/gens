/**
 * Gens: Main loop. (Linux specific code)
 */


#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "g_main_linux.hpp"


/**
 * GENS_Default_Save_Path(): Create the default save path.
 * @param *buf Buffer to store the default save path in.
 */
void Get_Save_Path(char *buf, size_t n)
{
	strncpy(buf, getenv ("HOME"), n);
	strcat(buf, "/.gens/");
}

/**
 * GENS_Create_Default_Save_Directory(): Create the default save directory.
 * @param *dir Directory name.
 */
void Create_Save_Directory(const char *dir)
{
	mkdir(dir, 0700);
}
