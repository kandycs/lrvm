#ifndef RVM_TYPE_H
#define RVM_TYPE_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

/* data struct definition */

#define MAX_SEGS		128
#define MAX_UPDATES		128

#define UNMAPPED 0
#define MAPPED	 1

/* defines a struct recording old valudes */
struct update {
	uint64_t	offset;		//offset in segment
	uint64_t	size;		//size
	void *		buf;		//buf for old value
};

typedef uint64_t update_t;

/* defines a struct descripping mapped segment */
struct seg {
	char *		seg_name;				//segment name, also the related backing store file name
	size_t		seg_size;				//mapped segment size, as API has no base addres, base is assumed as 0
	int			modified;				//indicate whether has been modified: 0-unmodified, 1-modified
	int			commited;				//indicate whether updates has been commited, 0-uncomminted, 1-comminited
	int			updates;				//recording the number of changes to this segment
	update_t	old[MAX_UPDATES];		//recording the acture changes for the old value
};

typedef uint64_t seg_t;

/* defines a mapping between segment virtual address and segment id */
struct mapping {
	uint64_t	segbase;
	seg_t		segid;
};

typedef uint64_t map_t;

/* define a struct descriping created rvm */
struct rvm {
	char		* directory;			//backing store directory
	char		* log_file;				//log file
	int			seg_num;				//the number of mapped segments
	map_t		segs_map[MAX_SEGS];		//define a mapping between mapped segments and its virtual address
};
typedef uint64_t rvm_t;

/* define an transaction */
struct trans {			
	rvm_t		rvm;					//the rvm it belongs to
	int			seg_num;
	map_t		segs_map[MAX_SEGS];		//a mapping between involved segments and its virtual address
};
typedef uint64_t trans_t;

#endif
