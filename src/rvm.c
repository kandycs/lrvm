#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>

#include "rvm_type.h"
#include "internal.h"

static int verbose = 0;
static int crash = 0;    //simulating crash
void rvm_truncate_log(rvm_t rvm);

void simcrash(int flag)
{
	crash = flag;
}

rvm_t rvm_init(const char * directory)
{
	char log_file_name[128]={0};
	char log_file[128]={0}; 
	struct rvm *rvmP;
	struct stat st={0};

	if (verbose)
		fprintf(stdout, "rvm_init: begin to initialize rvm\n");

	assert(directory != NULL);

	//check whether the given name is already existed
	if (stat(directory, &st) == 0) {
		//check whether there is a normal file existing with the same name
		if (S_ISDIR(st.st_mode) == 0){
			fprintf(stderr, "file %s has already existed, \
					but it is not a directory please choose a new name\n", directory);
			exit(-1);
		}
	} else if (errno == ENOENT) {		//dir not exiting, create one
		if (verbose)
			fprintf(stdout, "rvm_init: create rvm directory\n");
		mkdir(directory, 0700);
		sprintf(log_file, "%s/%s.log", directory, directory);
		creat(log_file, S_IRWXU);
	} else {	
		perror("error for checking backing store directory");
		exit(-1);
	}

	sprintf(log_file_name, "%s.log", directory);

	if (verbose)
		fprintf(stdout, "rvm_init: creating rvm data structure\n");
	rvmP = (struct rvm *) malloc (sizeof(struct rvm));
	memset(rvmP, 0, sizeof(struct rvm));
	rvmP->directory = strdup(directory);
	rvmP->log_file = strdup(log_file_name);
	rvmP->seg_num = 0;

	rvm_truncate_log((rvm_t)rvmP);

	return (rvm_t)rvmP;
}

/* self added API */
void rvm_terminate(rvm_t rvm)
{
	struct rvm * rvmP = (struct rvm *)rvm;
	free(rvmP);
}

void rvm_destroy(rvm_t rvm, const char *segname) 
{
	char path[512], segstatus[512];
	struct rvm *rvmP = (struct rvm *)rvm;
	int len = strlen(rvmP->directory);
	struct stat st={0};
	if (verbose)
		fprintf(stdout, "rvm_destroy: begin to destroy segment:%s\n", segname);

	if (rvmP->directory[len-1] == '/') {
		sprintf(path, "%s%s", rvmP->directory, segname);
		sprintf(segstatus, "%s%s.status", rvmP->directory, segname);
	} else { 
		sprintf(path, "%s/%s", rvmP->directory, segname);
		sprintf(segstatus, "%s/%s.status", rvmP->directory, segname);
	}

	/* check whether a file with the given segment name 
	 * exists, and is a regular file
	 */
	if ( stat(path, &st) == 0 && S_ISREG(st.st_mode) ) {
		if ( status(rvm, segname) == UNMAPPED) { // segment is currently unmppaed
			remove(path);
			remove(segstatus);
			/* free related memory */
			if (seg_free(rvmP->segs_map, rvmP->seg_num, segname))
				rvmP->seg_num--;
		} else {
			fprintf(stderr, "the segment %s (path: %s) has been mapped, \
					unmappit before destroying it", segname, path);
		}
	}
}


void * rvm_map(rvm_t rvm, const char * segname, int size)
{
	void *ptr;
	char path[512], segstatus[512];
	struct stat st = {0};
	int fd;
	struct rvm * rvmP = (struct rvm *)rvm;

	int len = strlen(rvmP->directory);
	off_t file_size;
	
	struct seg * segP;
	struct mapping *mapP;

	int index;
	
	if (verbose)
		fprintf(stdout, "rvm_map: begin to map segment:%s\n", segname);

	if (rvmP->directory[len-1] == '/') {
		sprintf(path, "%s%s", rvmP->directory, segname);
		sprintf(segstatus, "%s%s.status", rvmP->directory, segname);
	} else { 
		sprintf(path, "%s/%s", rvmP->directory, segname);
		sprintf(segstatus, "%s/%s.status", rvmP->directory, segname);
	}
	if (stat(path, &st) == 0) {
		if ( !S_ISREG(st.st_mode) ) {
			fprintf(stderr, "file(%s) for segment backing store has already existed, but it is not a regular file please choose another name\n", path);
			return NULL;
		}
	} else if (errno == ENOENT) {
		if (verbose)
			fprintf(stdout, "rvm_map: creating segment:%s\n", segname);
		creat(path, S_IRWXU);
		creat(segstatus, S_IRWXU);
		set(segstatus, UNMAPPED);
	} else {
		perror("segment backing store error");
		return NULL;
	}

	if (verbose)
		fprintf(stdout, "rvm_map: checking segment:%s status\n", segname);
	/* check whether segment has been mapped */
	if (status(rvm, segname) == MAPPED) {
		fprintf(stderr, "segment %s (file: %s) has been mapped\n", segname, path);
		return NULL;
	}

	if ( (fd = open(path, O_RDWR, S_IRWXU)) == -1) {
		perror("open segment backing store file error");
		return NULL;
	}
	
	file_size = lseek(fd, 0, SEEK_END);

	/* resize the backing store file if it is smaller than required size */
	if (file_size < size) {
		lseek(fd, size, SEEK_SET);
	}

	/* move back to the beginning of original file */
	lseek(fd, 0, SEEK_SET);

	if (verbose)
		fprintf(stdout, "rvm_map: mapping segment:%s to virtual address\n", segname);
	
	ptr = (void *)malloc(size);
	if (ptr == NULL) {
		fprintf(stderr, "memory application error\n");
		return NULL;
	}

	/* mapping the contend in file to the memory */
	read(fd, ptr, size);
	close(fd);

	segP = (struct seg *) malloc(sizeof(struct seg));
	segP->seg_name = strdup(segname);
	segP->seg_size = size;
	segP->modified = 0;
	segP->commited = 0;
	segP->updates = 0;

	mapP = (struct mapping *) malloc(sizeof(struct mapping));
	mapP->segbase = (uint64_t)ptr;
	mapP->segid	= (seg_t)segP;

	index = next_entry(rvmP->segs_map, rvmP->seg_num);
	rvmP->segs_map[index] = (map_t)mapP;
	rvmP->seg_num++;

	//set segment has been mapped
	set(segstatus, MAPPED);

	return ptr;
}

void rvm_unmap(rvm_t rvm, void *segbase)
{
	int len, inx;
	struct seg *segP;
	seg_t segid;
	char segstatus[256];
	struct mapping *mapP;
	struct rvm *rvmP = (struct rvm *) rvm;
	map_t *map = rvmP->segs_map;
	
	if(segbase == NULL) return;

	len = strlen(rvmP->directory);
	inx = get_segid(rvmP->segs_map, rvmP->seg_num, segbase);
	

	if (inx < 0) {
		fprintf(stderr, "did not find segmant with address:%lu\n", (uint64_t)segbase);
		return;
	}

	mapP = (struct mapping *)map[inx];

	segid = mapP->segid;
	segP = (struct seg *) segid;

	if (segP->modified && !(segP->commited)) {
		fprintf(stderr, "segment(%s) has been modified, \
				but not committed\n", segP->seg_name);
		return;
	}
	if (verbose)
		fprintf(stdout, "rvm_unmap: unmapping segment:%s\n", segP->seg_name);

	if (rvmP->directory[len-1] == '/') {
		sprintf(segstatus, "%s%s.status", rvmP->directory, segP->seg_name);
	} else { 
		sprintf(segstatus, "%s/%s.status", rvmP->directory, segP->seg_name);
	}

	free(segP);
	free(segbase);
	free(mapP);
	map[inx] = 0;
	rvmP->seg_num--;
	set(segstatus, UNMAPPED);
}


trans_t rvm_begin_trans(rvm_t rvm, int numsegs, void **segbase)
{
	/* set related segment as active */
	struct rvm * rvmP = (struct rvm *) rvm;
	struct trans *transP = (struct trans *) malloc (sizeof (struct trans));
	map_t *rvm_map, *trans_map;
	int i, inx;

	if (transP == NULL) {
		fprintf(stderr, "(%s:%d) allocating memory for transaction failed\n", __FILE__, __LINE__);
		return 0;
	}
	
	if (verbose)
		fprintf(stdout, "rvm_begin_trans: creating a transaction\n");

	transP->rvm		= rvm;
	transP->seg_num = 0;
	
	rvm_map = rvmP->segs_map;
	trans_map = transP->segs_map;

	for (i = 0; i < numsegs; i++) {
		if (segbase[i] == NULL) continue;
		inx = get_segid(rvmP->segs_map, rvmP->seg_num, segbase[i]);
		if (inx < 0)
			fprintf(stderr, "did not find segment for address (%lu)\n", (uint64_t)segbase[i]);
		trans_map[transP->seg_num] = rvm_map[inx];
		transP->seg_num++;
	}

	return (trans_t)transP;
}

void rvm_about_to_modify(trans_t id, void * segbase, int offset, int size)
{
	struct trans * transP = (struct trans *)id;
	map_t * map = transP->segs_map;	
	struct update *updateP = (struct update *)malloc(sizeof(struct update));
	struct seg *segP;
	struct mapping *mapP;
	int inx;
	
	if (verbose)
		fprintf(stdout, "rvm_about_to_modify: backing up old values\n");

	updateP->offset = offset;
	updateP->size  = size;
	updateP->buf = (void *) malloc (size);
	memcpy(updateP->buf, segbase + offset, (size_t)size);

	inx = get_segid(map, transP->seg_num, segbase);
	mapP = (struct mapping *) map[inx];
	segP = (struct seg *)mapP->segid;
	segP->old[segP->updates] = (update_t)updateP;
	segP->updates++;
}

void rvm_truncate_log(rvm_t rvm)
{
	struct rvm *rvmP = (struct rvm *)rvm;
	char * rvm_dir = strdup(rvmP->directory);
	char log_file[256], seg[256], seg_name[32];
//	char seg_status[256];
	long pos;
	int len = strlen(rvmP->directory); 
	int rdsize, seg_fd, tmp, skip;
	FILE *log_fp;
	uint64_t offset, size;
	char *buf, rec_st[8];
	
	struct stat st={0};

	if (rvmP->directory[len-1] == '/') {
		sprintf(log_file, "%s%s", rvmP->directory, rvmP->log_file);
	} else { 
		sprintf(log_file, "%s/%s", rvmP->directory, rvmP->log_file);
	}
	
	tmp = stat(log_file, &st);
	if (tmp != 0 || st.st_size == 0) return;
	if (verbose)
		fprintf(stdout, "rvm_truncate_log: replay the log\n");
	
	log_fp = fopen(log_file, "rw");
	if (log_fp == NULL) {
		fprintf(stderr, "open log file (%s) error: %s, %d\n", log_file, __func__, __LINE__);
		return;
	}

	skip = 0;
	while(1) {
		/* check status for a transaction */
		pos = ftell(log_fp);
		fscanf(log_fp, "%s", rec_st);
		if (strcmp(rec_st, "tbg") == 0) {
			skip = 1;
			fprintf(stderr, "found a break transaction record\n");
			continue;
		} else if (strcmp(rec_st, "tfs") == 0) { //found new successful transaction
			skip = 0;
			continue;
		}

		if (skip) continue; //skip unsuccessful records

		fseek(log_fp, pos, SEEK_SET);
		/* get log name, update offset and update size */
		rdsize = fscanf(log_fp, "%s %lu %lu\n", seg_name, &offset, &size);
		
		if (rdsize == 0) break;
		
		if (rvmP->directory[len-1] == '/') {
			sprintf(seg, "%s%s", rvmP->directory, seg_name);
//			sprintf(seg_status, "%s%s.status", rvmP->directory, seg_name);
		} else { 
			sprintf(seg, "%s/%s", rvmP->directory, seg_name);
//			sprintf(seg_status, "%s/%s.status", rvmP->directory, seg_name);
		}
		
//		set(seg_status, UNMAPPED);

		buf = (char *) malloc (size);
		seg_fd = open(seg, O_RDWR|O_CREAT);
		lseek(seg_fd, offset, SEEK_SET);

		rdsize = fread(buf, size, 1, log_fp);
		
		if (rdsize == 0) break;
		
		rdsize = write(seg_fd, buf, size);
		syncfs(seg_fd);
		close(seg_fd);

		free(buf);
	}
	fclose(log_fp);

	truncate(log_file, 0);
}

void rvm_commit_trans(trans_t tid)
{
	struct trans *transP = (struct trans *) tid;
	struct rvm *rvmP = (struct rvm *)(transP->rvm);
	char log_file[256], seg[256];

	int i, j, trans_num, len, fd;
	long pos;
	struct mapping *mapP;
	struct seg *segP;
	struct update *updateP;
	FILE *log_fp;
	map_t * map = transP->segs_map;

	trans_num = transP->seg_num;
	len = strlen(rvmP->directory); 
	
	if (rvmP->directory[len-1] == '/') {
		sprintf(log_file, "%s%s", rvmP->directory, rvmP->log_file);
	} else { 
		sprintf(log_file, "%s/%s", rvmP->directory, rvmP->log_file);
	}
	if(verbose)
		fprintf(stdout, "rvm_commit_trans: commit updates\n");

	log_fp = fopen(log_file, "r+");
	fseek(log_fp, 0, SEEK_END);
	pos = ftell(log_fp);
	fprintf(log_fp, "tbg\n");
	
	for (i = 0; i < trans_num; i++) {
		mapP = (struct mapping *)map[i];
		segP = (struct seg *)mapP->segid;

		for (j = 0; j < segP->updates; j++) {
			updateP = (struct update *)segP->old[j];
			fprintf(log_fp, "%s %lu %lu\n", segP->seg_name, updateP->offset, updateP->size);

			fwrite((void *)mapP->segbase + updateP->offset, updateP->size, 1, log_fp);
			fprintf(log_fp, "\n");
			fflush(log_fp);
		}

		segP->commited = 1;
#if 0
		/* writing data to segments */
		if (rvmP->directory[len-1] == '/') {
			sprintf(seg, "%s%s", rvmP->directory, segP->seg_name);
		} else { 
			sprintf(seg, "%s/%s", rvmP->directory, segP->seg_name);
		}

		size = segP->seg_size;

		fd = open(seg, O_RDWR);
		write(fd, (void *) mapP->segbase, (size_t)size);
		close(fd);
#endif
	}

	i = ftell(log_fp);
	fseek(log_fp, pos, SEEK_SET);
	i = ftell(log_fp);
	fprintf(log_fp, "tfs\n");
	fclose(log_fp);
	
	if (!crash)
		rvm_truncate_log(transP->rvm);
}


void rvm_abort_trans(trans_t tid)
{
	struct trans *transP = (struct trans *) tid;
	struct rvm *rvmP = (struct rvm *)(transP->rvm);

	int i, j, trans_num, len;
	struct mapping *mapP;
	struct seg *segP;
	struct update *updateP;
	map_t * map = transP->segs_map;

	trans_num = transP->seg_num;
	if (verbose)
		fprintf(stdout, "rvm_abort_trans: aborting updates, restoring old values\n");
	
	for (i = 0; i < trans_num; i++) {
		mapP = (struct mapping *)map[i];
		segP = (struct seg *)mapP->segid;

		for (j = 0; j < segP->updates; j++) {
			updateP = (struct update *)segP->old[j];
			memcpy(mapP->segbase+updateP->offset, updateP->buf, updateP->size);
			free(updateP);
			segP->old[j] = 0;
		}
		segP->commited = 0;
		segP->modified = 0;
	}	
}




void rvm_verbose(int enable_flag)
{	
	verbose = 1;
}

