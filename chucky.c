/* 
Copyright 2013 Waitman Gobble <ns@waitman.net> 
see COPYING for details 
Usage: chucky
This program reads /var/db/pkg/local.sqlite and compares
install timestamp with date in $FreeBSD header in the 
Makefile of the port. If there is no $FreeBSD header 
then it is assumed to be a beta port.
At the moment it displays the timestamp in the db 
and the timestamp parsed in the Makefile, for debug 
purposes.
*/
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FULLSIZE 300

static int callback(void *NotUsed, int argc, char **argv, char **coln) {

	char		fn[255];
	char		*ptr;
        int		matters,eol,i,beta;
        struct tm	etm;
        time_t		m;
	char		*buffer;

	/* location of Makefile - change if root not /usr/ports */
	sprintf(fn,"/usr/ports/%s/Makefile",argv[0]);

	/* Read beginning of file, just need $FreeBSD line */
	buffer = malloc(FULLSIZE+1);
	FILE *file = fopen(fn,"r");
	fread(buffer,1,FULLSIZE,file);
	fclose(file);

	/* beta ports */
	/* beta = ports installed over top of your ports installation,
		ie for tests. (ex: xorg 7.7) 
		these do not generally have the $FreeBSD header in 
		the Makefile
	*/
	beta = 1;

	ptr = strstr(buffer,"$FreeBSD:");
	if (ptr!=NULL) {
		matters = ptr - buffer + 12;
		ptr = strstr(&buffer[matters],"\n");
		eol = ptr - buffer;
		
		m = time(NULL);
		for (i=matters;i<eol;i++) {
			/* try to parse the date, if the char is 1 or 2 */
			if (((buffer[i]=='2')||(buffer[i]=='1')) && 
				(buffer[i+4]=='-')) {
				memset(&etm, 0, sizeof(struct tm));
				strptime(&buffer[i],"%Y-%m-%d %H:%M:%S", &etm);
				m = mktime(&etm);
				if (m>0) {
					if ((int)m > atoi(argv[1])) {
						printf(" {updates}\tp:%i i:%i\t",(int)m,atoi(argv[1]));
					} else {
						printf(" {OK}\tp:%i i:%i\t",(int)m,atoi(argv[1]));
					} 
					/* standard ports */
					beta=0;
					break;
				}
			}
		}
	}
	
	if (beta==1) {
		printf(" {beta}\tp:---------- i:%i\t",atoi(argv[1]));
	}
	printf("%s %s\n",argv[0],argv[2]);

	return 0;
}

int main(int argc, char **argv){
	sqlite3 *db;
	int rc;
	rc = sqlite3_open("/var/db/pkg/local.sqlite", &db);
	if( rc )
	{
		printf("oops cannot open /var/db/pkg/local.sqlite\n");
		return 1;
	}
	rc = sqlite3_exec(db, "SELECT origin,time,version FROM packages ORDER BY origin ASC", callback, 0, NULL);
	sqlite3_close(db);
	return 0;
}

