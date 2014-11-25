#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <string.h>
#include <stdlib.h>
#include "rvm_type.h"

void set(const char *filename, int value)
{
	int fd;
	fd  = open(filename, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "open status file (%s) error", filename);
		return;
	}

	write(fd, &value, 1);
	close(fd);
}

int status(rvm_t rvm, const char *segname)
{
	char status_file[128];
	struct rvm *rvmP = (struct rvm *)rvm;
	int fd;
	int buf = 0;
	int len = strlen(rvmP->directory);

	if (rvmP->directory[len-1] == '/') {
		sprintf(status_file, "%s%s.status", rvmP->directory, segname);
	} else {
		sprintf(status_file, "%s/%s.status", rvmP->directory, segname);		
	}

	fd = open(status_file, O_RDWR);
	if (fd == -1) {
		fprintf(stderr, "open status file (%s) error\n", status_file);
		return -1;
	} 
	
	read(fd, &buf, 1);
	close(fd);

	return buf;
}

int seg_free(map_t maps[], int size, const char *segname)
{
	int i = 0, count = 0;

	struct mapping * mapP;
	struct seg * segP;

	for (i = 0; count < size; i++) {
		mapP = (struct mapping *) maps[i];
		if (maps[i] == 0)
			continue;

		segP = (struct seg *)(mapP->segid);
		if ( 0 == strcmp(segP->seg_name, segname)) {
			free(segP);
			free(mapP);
			maps[i] = 0;
			return 1;
		}

		count++;

	}

	return 0;
}

int next_entry(map_t map[], int size)
{
	int next;
	int i = 0;
	struct mapping *mapP;

	// search the hole
	while (i < size) {
		if(map[i] == 0)
			return i;
		i++;
	}

	return (size >= MAX_SEGS)? -1 : size;
}

int get_segid(map_t map[], int seg_num, const void *segbase)
{
	uint64_t base_addr = (uint64_t) segbase;
	struct mapping *mapP;
	int count = 0;
	int i = 0;

	for (i = 0; count < seg_num; i++) {
		mapP = (struct mapping *)map[i];
		if (map[i] == 0) 
			continue;
		if (mapP->segbase == 0 || mapP->segid == 0)
			continue;
		if (mapP->segbase == base_addr)
			return i;
		count++;
	} 
	return -1;
}
