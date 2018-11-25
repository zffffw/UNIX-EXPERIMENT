 #include <apue.h>
#include <dirent.h>
#include <limits.h>
#include "pathalloc.c"
#include "error2e.c"
#include <fcntl.h>
#include  <unistd.h>
/* function type that is called for each filename */
typedef	int	Myfunc(const char *, const struct stat *, int);

static Myfunc	myfunc;
static Myfunc   myfun2;
static Myfunc   myfun3;
static int		myftw(char *, Myfunc *);
static int		dopath(Myfunc *);
static long	nreg, nreg_4096, ntot;
char* find_file_name_from_path(const char* pathname);
struct stat stat_target;
char* target_path;
char** target_names;
int file_number;

int main(int argc, char *argv[])
{
	int		ret;

	if (argc == 2){
	    ret = myftw(argv[1], myfunc);
	    ntot = nreg_4096 + nreg;
	    printf("less than 4096:%ld\n", nreg_4096);
	    printf("ntot:%ld\n", ntot);
	    printf("percent:%f\n", (float)(nreg_4096)/(float)(ntot));
	} else if (argc == 4 && strcmp(argv[2], "-comp") == 0) {
			target_path = malloc(strlen(argv[3]));
			strcpy(target_path, argv[3]);
			if (lstat(target_path, &stat_target) < 0)	/* stat error */
				err_quit("lstat error!\n");
			ret = myftw(argv[1], myfun2);
	} else if (strcmp(argv[2], "-name") == 0) {
		target_names = argv + 3;
		file_number = argc - 3;
		ret = myftw(argv[1], myfun3);
	}
}

/*
 * Descend through the hierarchy, starting at "pathname".
 * The caller's func() is called for every file.
 */
#define	FTW_F	1		/* file other than directory */
#define	FTW_D	2		/* directory */
#define	FTW_DNR	3		/* directory that can't be read */
#define	FTW_NS	4		/* file that we can't stat */

static char	*fullpath;		/* contains full pathname for every file */
static int pathlen;

static int					/* we return whatever func() returns */
myftw(char *pathname, Myfunc *func)
{
	fullpath = path_alloc(&pathlen);	/* malloc PATH_MAX+1 bytes */
										/* ({Prog pathalloc}) */
	if (pathlen <= strlen(pathname)) {
		pathlen = strlen(pathname) * 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("realloc failed");
	}
	strcpy(fullpath, pathname);
	return(dopath(func));
}




/*
 * Descend through the hierarchy, starting at "fullpath".
 * If "fullpath" is anything other than a directory, we lstat() it,
 * call func(), and return.  For a directory, we call ourself
 * recursively for each name in the directory.
 */
static int					/* we return whatever func() returns */
dopath(Myfunc* func)
{
	struct stat		statbuf;
	struct dirent	*dirp;
	DIR				*dp;
	int				ret, n;

	if (lstat(fullpath, &statbuf) < 0)	/* stat error */
		return(func(fullpath, &statbuf, FTW_NS));
	if (S_ISDIR(statbuf.st_mode) == 0)	/* not a directory */
		return(func(fullpath, &statbuf, FTW_F));

	/*
	 * It's a directory.  First call func() for the directory,
	 * then process each filename in the directory.
	 */
	if ((ret = func(fullpath, &statbuf, FTW_D)) != 0)
		return(ret);

	n = strlen(fullpath);
	if (n + NAME_MAX + 2 > pathlen) {	/* expand path buffer */
		pathlen *= 2;
		if ((fullpath = realloc(fullpath, pathlen)) == NULL)
			err_sys("realloc failed");
	}
	fullpath[n++] = '/';
	fullpath[n] = 0;

	if ((dp = opendir(fullpath)) == NULL)	/* can't read directory */
		return(func(fullpath, &statbuf, FTW_DNR));

	while ((dirp = readdir(dp)) != NULL) {
		if (strcmp(dirp->d_name, ".") == 0  ||
		    strcmp(dirp->d_name, "..") == 0)
				continue;		/* ignore dot and dot-dot */
		strcpy(&fullpath[n], dirp->d_name);	/* append name after "/" */
		if ((ret = dopath(func)) != 0)		/* recursive */
			break;	/* time to leave */
	}
	fullpath[n-1] = 0;	/* erase everything from slash onward */

	if (closedir(dp) < 0)
		err_ret("can't close directory %s", fullpath);
	return(ret);
}

static int myfun2(const char *pathname, const struct stat *statptr, int type) {
	if (type == FTW_F) {
		if (statptr->st_size != stat_target.st_size) {
			return 0;
		} else {
			// printf("pathname:%s\n", pathname);
			int cur_file, tar_file;
			if ((cur_file = open(pathname, O_RDONLY, FILE_MODE)) < 0) {
				err_quit("open file failed:%s", cur_file);
			}
			if ((tar_file = open(target_path, O_RDONLY, FILE_MODE)) < 0) {
				err_quit("open file failed:%s", cur_file);
			}
			if (lseek(cur_file, 0, SEEK_SET) < 0) {
				err_quit("lseek error");
			}
			if (lseek(tar_file, 0, SEEK_SET) < 0) {
				err_quit("lseek error");
			}
			char *buf_cur, *buf_tar;
			int size = 1024;
			buf_cur = malloc((size + 1)*sizeof(char));
			buf_tar = malloc((size + 1)*sizeof(char));
			memset(buf_cur, 0, size + 1);
			memset(buf_tar, 0, size + 1);
			while(read(cur_file, buf_cur, size) > 0 && read(tar_file, buf_tar, size) > 0) {
				if (strcmp(buf_cur, buf_tar) != 0) {
					// printf("%ld, %ld\n", strlen(buf_cur), strlen(buf_tar));
					return 0;
				} 
				memset(buf_cur, 0, size + 1);
				memset(buf_tar, 0, size + 1);
			}
		}
		char absolute_path[1024];
		realpath(pathname, absolute_path);
		// char* filename = find_file_name_from_path(pathname);
		printf("%s\n", absolute_path);
	}
	return 0;
}

static int myfun3(const char *pathname, const struct stat *statptr, int type) {
	char* filename = find_file_name_from_path(pathname);
	for (int m = 0; m < file_number; ++m) {
		if(strcmp(filename, *(target_names + m)) == 0) {
			char absolute_path[1024];
			realpath(pathname, absolute_path);
			printf("%s\n", absolute_path);
		}
	}
	return 0;
}

char* find_file_name_from_path(const char* pathname) {
	char* file_name;
	int i;
	for(i = strlen(pathname) - 1; i >= 0; --i) {
		if(pathname[i] == '/') {
			break;
		}
	}
	int length = strlen(pathname + i + 1);
	file_name = malloc(length*sizeof(char));
	strcpy(file_name, pathname + i + 1);
	return file_name;
}

static int
myfunc(const char *pathname, const struct stat *statptr, int type)
{
	//printf("111\n");
	if(type == FTW_F) {
	//	printf("type:%d\n", type);
		switch(statptr->st_mode & S_IFMT) {
		case S_IFREG:{
	//		printf("size:%lld\n", statptr->st_size);	
			if (statptr->st_size <= 4096) {
				nreg_4096++;
			} else {
				nreg++;
			}
		}; break;
		}
	}
	return(0);
}
