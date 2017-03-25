#include "diff.h"
#include "fs.h"

const char* missing_from_fs(char ** dbList, struct fs_discovered_migrations* fsList){
	int dbi = 0;
	int fsi = 0;

	while (dbList[dbi] != '\0') {

		printf("%s\n", dbList[dbi]);

		dbi++;
	}

	while (strcmp(fsList[fsi].name,"\0")) {
		printf("%s\n", fsList[fsi].name);

		fsi++;
	}
}


const char*missing_from_db(char ** dbList, struct fs_discovered_migrations* fsList){

}

