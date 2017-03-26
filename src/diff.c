#include "diff.h"
#include "fs.h"

char **missing_from_fs(char **dbList, struct fs_discovered_migrations *fsList) {
	int fsi = 0;
	char **list = malloc(1000 * sizeof(char *));
	int filesLocated = 0;

	while (strcmp(fsList[fsi].name, "\0")) {
		bool located = false;
		int dbi = 0;
		while (dbList[dbi] != '\0') {
			if (!strcmp(fsList[fsi].name, dbList[dbi])) {
				located = true;
				break;
			}
			dbi++;
		}
		if (!located) {
			list[filesLocated] = (char *) malloc(PATH_MAX + 1);
			strcpy(list[filesLocated], fsList[fsi].name);
			filesLocated++;
//			printf("FOUND %s \n", fsList[fsi].name);
		}
		fsi++;
	}
	list[filesLocated+1] = (char *)malloc(PATH_MAX + 1);
	strcpy(list[filesLocated+1],"\0");
	return list;

}

