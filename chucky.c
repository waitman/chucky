/* 
chucky.c

Copyright 2013 Waitman Gobble <ns@waitman.net> 
see LICENSE for details 

Usage: chucky
This program reads /var/db/pkg/local.sqlite and compares
install timestamp with date in $FreeBSD header in the 
Makefile of the port. If there is no $FreeBSD header 
then it is assumed to be a beta port.

Command line switches:
	-u	show ports with update status only
	-o	show ports with current status only
	-b	show ports with beta status only
	-t	show timestamps (debug, etc.)
	-a	create shar of files (usefull with -b)
	-i	ignore fread errors*
	-h	help

NOTE: shar files are created with absolute path (ie /usr/ports/src/dir) 
so executing them will restore to that path. Multiple shars are concatenated 
by chucky stripping the 'exit' command from the output. (ie there is no 'exit'
in the shar output. the shar output is to stdout, if you want to save:

# chucky -b -a > save.shar

This will save all the ports marked 'beta' in the save.shar archive, in the 
cwd.

*ignore fread errors: if you are working on developing ports outside the 
ports tree you may receive an error because the port is listed in the database
but it does not exist in the ports tree. if you use the -i flag it will continue
without an error message.

*/
#include <stdio.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define FULLSIZE 300

int showbeta,showok,showupdates,showtimestamps,makeshar,ignoreerrors;

static int callback(void *NotUsed, int argc, char **argv, char **coln) {

	char		fn[255];
	char		*ptr;
        int		matters,eol,i,beta;
        struct tm	etm;
        time_t		m;
	char		*buffer;
	int		doshow=0;

	/*	
		argv[0] = origin
		argv[1] = timestamp
		argv[2] = version
	*/

	/* location of Makefile - change if root not /usr/ports */
	sprintf(fn,"/usr/ports/%s/Makefile",argv[0]);

	/* Read beginning of file, just need $FreeBSD line */
	buffer = malloc(sizeof(char)*(FULLSIZE+1));
	FILE *file;
	if ((file = fopen(fn,"r"))) {
		fread(buffer,1,FULLSIZE,file);
		fclose(file);
	} else {
		if (ignoreerrors>0) {
			return(0);
		} else {
			printf("Error - could not read %s\n",fn);
			exit(1);
		}
	}

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
						if (showupdates>0) {
							doshow=1;
							if (makeshar<1) {
								if (showtimestamps>0) {
									printf(" {updates}\tp:%i i:%i\t",(int)m,atoi(argv[1]));
								} else {
									printf(" {updates}\t");
								}
							}
						}
					} else {
						if (showok>0) {
							doshow=1;
							if (makeshar<1) {
								if (showtimestamps>0) {
									printf(" {OK}\tp:%i i:%i\t",(int)m,atoi(argv[1]));
								} else {
									printf(" {OK}\t");
								}
							}
						}
					} 
					/* standard ports */
					beta=0;
					break;
				}
			}
		}
	}
	
	if (beta==1) {
		if (showbeta>0) {
			doshow=1;
			if (makeshar<1) {
				if (showtimestamps>0) {
					printf(" {beta}\tp:---------- i:%i\t",atoi(argv[1]));
				} else {
					printf(" {beta}\t");
				}
			}
		}
	}
	if (doshow>0) {
		if (makeshar<1) {
			printf("%s %s\n",argv[0],argv[2]);
		} else {
			char	cmd[255] = {0};
			char	readbuf[80] = {0};
			FILE	*fp;
			sprintf(cmd,"/usr/bin/shar `find /usr/ports/%s`",argv[0]);
			if ((fp = popen(cmd, "r"))) {
				while(fgets(readbuf, 80, fp)) {
					char *eloc;
					eloc = strstr(readbuf,"exit");
					if (eloc == NULL) {
						printf("%s",readbuf);
					} else {
						if ((eloc-readbuf)>0) {
							printf("%s",readbuf);
						}
					}
				}
				pclose(fp);
			} else {
				printf("Oops - Could not popen shar command.\n");
				exit(1);
			}
		}
	}

	return 0;
}

void usage(void) {
	printf("\nusage: chucky [-hbuot]\n\n");
	printf("This program reads /var/db/pkg/local.sqlite and compares \
install timestamp with\ndate in $FreeBSD header in the \
Makefile of the port. If there is no $FreeBSD\nheader \
then it is assumed to be a beta port. \
\n\n \
Command line switches: \n\
\t\t-u\tshow ports with update status only \n\
\t\t-o\tshow ports with current status only \n\
\t\t-b\tshow ports with beta status only \n\
\t\t-t\tshow timestamps (debug, etc)\n\
\t\t-a\tcreate shar of files (usefull with -a). output to stdout\n\
\t\t-h\thelp \n\n\
NOTE: shar files are created with absolute path (ie /usr/ports/src/dir)\n\
so executing them will restore to that path. Multiple shars are concatenated\n\
by chucky stripping the 'exit' command from the output. (ie there is no 'exit'\n\
in the shar output. the shar output is to stdout, if you want to save:\n\
\n\
# chucky -b -a > save.shar\n\
\n\
This will save all the ports marked 'beta' in the save.shar archive,\n\
in the cwd \n\
\n\n\
");
	exit(0);
}

int main(int argc, char **argv){

	sqlite3		*db;	/* sqlite database object */
	int	 	rc;	/* db handle */
	int		c;	/* command line parse */

	showbeta = 0;
	showupdates = 0;
	showok = 0;
	showtimestamps = 0;
	makeshar = 0;
	ignoreerrors = 0;

	while ((c = getopt (argc, argv, "buohtai")) != -1) {
		switch (c) {
			case 'b':    /* show beta */
				showbeta = 1;
				break;
			case 'u':    /* show updates */
				showupdates = 1;
				break;
			case 'o':    /* show OK */
				showok = 1;
				break;
			case 't':    /* show timestamps */
				showtimestamps = 1;
				break;
			case 'h':    /* help me */
			        usage();
				break;
			case 'a':    /* dump shar archive to stdout */
				makeshar = 1;
				break;
			case 'i':    /* ignore fread() errors */
				ignoreerrors = 1;
				break;
                        default:     /* no switches */
                                break;
		}
	}
	if ((showok+showbeta+showupdates)==0) {
		/* default is to show all */
		showok = 1;
		showbeta = 1;
		showupdates = 1;
	}

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

/* EOF */
