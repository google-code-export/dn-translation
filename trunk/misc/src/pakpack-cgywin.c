//
// Dragon Nest PAK file packer
//
// To compile, run:
// gcc -Wall -O2 -o pakpack-cygwin.exe pakpack-cygwin.c -lz
//
// Requires zlib
//
// Based on pakpack.c from http://www.gsea.com.cn/blog/post/285/
// 

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <zlib.h>
#include <libgen.h>

typedef struct pak_info_t {
	char head[256];
	unsigned char static1[4];
	unsigned int file_num;
	unsigned int filelist_offset;
} pak_info_t;

typedef struct pak_file_info_t {
	char path[256];
	unsigned int zip_size;
	unsigned int size;
	unsigned int zip_size1;
	unsigned int offset;
	char empty[44];	
} pak_file_info_t;

typedef struct filelist_t {
	struct filelist_t* next;
	char* path;
	pak_file_info_t pak_file_info;
} filelist_t;

pak_info_t pakinfo;

int get_folder_files(filelist_t* p, const char* folder) {
	filelist_t* foot;
	DIR* dir;
	struct dirent* file;
	
	foot = p;
	while (foot->next != NULL) foot = foot->next;
	
	dir = opendir(folder);
	if (dir == NULL) return 0;
	
	while ((file = readdir(dir)) != NULL) {
		if (file->d_name[0] == '.') continue;
		
		if (file->d_type == 4) {
			if (file->d_name[0] != '.') {
				char sfolder[strlen(folder) + 257];
				sprintf(sfolder, "%s/%s", folder, file->d_name);	
				get_folder_files(p, sfolder);
			}
		}
		else {
			char* sfile = (char*)malloc(strlen(folder) +257);
			
			sprintf(sfile, "%s/%s", folder, file->d_name);
		
			while (foot->next != NULL) foot = foot->next;
			
			foot->next = (filelist_t*)malloc(sizeof(filelist_t));
			foot = foot->next;
			foot->next = NULL;
			foot->path = sfile;
		}
	}

	return 0;
}

int write_head(FILE* fp, filelist_t* p) {
	fseek(fp, 0, SEEK_SET);
	fwrite(&pakinfo, sizeof(pakinfo), 1, fp);
	
	return 0;
}

int write_file_content(FILE* fp, filelist_t* p) {
	FILE* fpin;
	int r, i;
	unsigned int size;
	unsigned int zip_size;
	unsigned long con_size, zip_con_size;
	void* con;
	void* zip_con;
	
	fpin = fopen(p->path, "r");
	if (fpin == NULL) {
		printf("Cannot read: %s\n", p->path);
		exit(1);
	}
	
	fseek(fpin, 0, SEEK_END);
	size = ftell(fpin);
	con_size = size;
	zip_con_size = con_size * 2;
	p->pak_file_info.size = size;
	
	con = malloc(size);
	zip_con = malloc(zip_con_size);
	
	fseek(fpin, 0, SEEK_SET);
	r = fread(con, size, 1, fpin);
	fclose(fpin);
	
	if (Z_OK != compress2(zip_con, &zip_con_size, con, con_size, 1)) {
		printf("Error compressing %s \n", p->path);
	}
	zip_size = zip_con_size;
	
	fwrite(zip_con, zip_size, 1, fp);
	
	free(con);
	free(zip_con);
	
	printf("%s\n original size: %ld packed size: %ld\n", p->path, con_size, zip_con_size);
	
	p->pak_file_info.zip_size = zip_size;
	p->pak_file_info.zip_size1 = zip_size;
	
	p->pak_file_info.path[0] = '\\';
	i = 0;
	while (p->path[i] != 0 && i < 254) {
		p->pak_file_info.path[i + 1] = p->path[i] == '/' ? '\\' : p->path[i];
		i++;
	}
	
	return 0;
}

int write_files_content(FILE* fp, filelist_t* p) {
	pakinfo.file_num = 0;
	
	while ((p = p->next) != NULL) {
		p->pak_file_info.offset = ftell(fp);
		write_file_content(fp, p);
		
		pakinfo.file_num++;
	}
	
	return 0;
}

int write_file_info(FILE* fp, filelist_t* p) {
	fwrite(&(p->pak_file_info), sizeof(p->pak_file_info), 1, fp);
	
	return 0;
}

int write_files_info(FILE* fp, filelist_t* p) {
	
	pakinfo.filelist_offset = ftell(fp);
	
	while ((p = p->next) != NULL) {
		write_file_info(fp, p);
	}
	
	return 0;
}

int write_file(const char* file, filelist_t* p) {
	FILE* fp;
	char empty[0x400] = {0};
	
	fp = fopen(file, "w");
	if (fp == NULL) {
		printf("Cannot visit %s\n", file);
		exit(1);
	}
	
	fwrite(empty, 0x400, 1, fp);
	write_files_content(fp, p);
	write_files_info(fp, p);
	write_head(fp, p);
	
	fclose(fp);
	
	printf("Files successfully written\n");
	
	return 0;
}

int print_usage(const char* argv0) {
	printf("Usage: %s output.pak folder1 folder2 ...\n", strrchr(argv0, '/') + 1);
	
	return 0;
}

int main(int argc, char** argv) {
	filelist_t* p;
	int i;
	
	if (argc < 3) {
		print_usage(argv[0]);
		exit(0);
	}
	
	p = (filelist_t*)malloc(sizeof(filelist_t));
	p->next = NULL;
	
	for (i = 0; i < 256; i++) {
		pakinfo.head[i] = 0;
	}
	strcpy(pakinfo.head, "EyedentityGames Packing File 0.1");
	pakinfo.static1[0] = 0x0B;
	pakinfo.static1[1] = pakinfo.static1[2] = pakinfo.static1[3] = 0;
	
	for (i = 2; i < argc; i++) {
		get_folder_files(p, argv[i]);
	}
	
	write_file(argv[1], p);
	
	return 0;
}
