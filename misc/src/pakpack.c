// from http://www.gsea.com.cn/blog/post/285/

/**
 * 龙之谷PAK资源文件打包程序
 * 
 * 编译平台：GNU/Linux 内核 2.6.32 (Ubuntu 10.04)
 * 编译命令：gcc -Wall -O2 -o pakpack pakpack.c -lz
 * （！）编译需要安装zlib库
 * 
 * 用法：./pakpack 资源文件名 [原始文件路径1[ 原始文件路径2[ ...[原始文件路径n]]]]
 * 
 * 作者：greensea (gs@bbxy.net) 2010年8月14日于广西百色
 * 协力：GPBeta
 * 
 * 许可：GPLv3
 */

#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <zlib.h>
#include <libgen.h>

/**
 * PAK文件头信息
 */
typedef struct pak_info_t {
	char head[256];
	unsigned char static1[4];	// 意义不明的固定字节
	unsigned int file_num;	// 文件总数
	unsigned int filelist_offset;	// 文件信息偏移地址
} pak_info_t;

/**
 * PAK文件中，单个虚拟文件的信息块。该信息块大小固定，为316字节
 */
typedef struct pak_file_info_t {
	char path[256];
	unsigned int zip_size;	// 原始文件大小
	unsigned int size;	// 压缩后的文件大小
	unsigned int zip_size1;	// 原始文件大小，和size一样
	unsigned int offset;	// 文件内容偏移地址
	char empty[44];	//填充用
} pak_file_info_t;

/**
 * 文件列表链表节点
 */
typedef struct filelist_t {
	struct filelist_t* next;
	char* path;
	pak_file_info_t pak_file_info;
} filelist_t;


/**
 * 全局变量
 */
pak_info_t pakinfo;


/**
 * 递归获取目录下的文件列表
 * 
 * @param filelist_t*	文件列表链表头指针，函数将会直接在此链表的最后添加文件
 * @param const char*	欲扫描的目录
 * @return int	0为成功
 */
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
		
		// d_type 4是目录，8是文件
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
		
			/* 由于是递归调用，可以被上一层的递归调用增加了文件，
			 * 那么这里指向的链表节点就不是最后一个节点了，因此要重新查找尾节点
			 */
			while (foot->next != NULL) foot = foot->next;
			
			foot->next = (filelist_t*)malloc(sizeof(filelist_t));
			foot = foot->next;
			foot->next = NULL;
			foot->path = sfile;
		}
	}

	return 0;
}

/**
 * 写入资源文件文件头
 */
int write_head(FILE* fp, filelist_t* p) {
	fseek(fp, 0, SEEK_SET);
	fwrite(&pakinfo, sizeof(pakinfo), 1, fp);
	
	return 0;
}

/**
 * 写入单个文件的已编码（即已压缩）内容
 */
int write_file_content(FILE* fp, filelist_t* p) {
	FILE* fpin;
	int r, i;
	unsigned int size;
	unsigned int zip_size;
	unsigned long con_size, zip_con_size;
	void* con;
	void* zip_con;
	
	// 获得文件数据，并压缩文件
	fpin = fopen(p->path, "r");
	if (fpin == NULL) {
		printf("无法读取文件: %s\n", p->path);
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
		printf("压缩 %s 时发生错误\n", p->path);
	}
	zip_size = zip_con_size;
	
	fwrite(zip_con, zip_size, 1, fp);
	
	free(con);
	free(zip_con);
	
	printf("%s 压缩前大小：%ld 压缩后大小：%ld\n", p->path, con_size, zip_con_size);
	
	p->pak_file_info.zip_size = zip_size;
	p->pak_file_info.zip_size1 = zip_size;
	
	p->pak_file_info.path[0] = '\\';
	i = 0;
	while (p->path[i] != 0 && i < 254) {
		p->pak_file_info.path[i + 1] = p->path[i] == '/' ? '\\' : p->path[i];	//瘟都死不认斜杠
		i++;
	}
	
	return 0;
}

/**
 * 写入所有文件的已编码（即已压缩）内容
 */
int write_files_content(FILE* fp, filelist_t* p) {
	pakinfo.file_num = 0;
	
	while ((p = p->next) != NULL) {
		p->pak_file_info.offset = ftell(fp);
		write_file_content(fp, p);
		
		pakinfo.file_num++;	// 顺便计算文件总数了
	}
	
	return 0;
}

/**
 * 写入单个文件的信息
 */
int write_file_info(FILE* fp, filelist_t* p) {
	fwrite(&(p->pak_file_info), sizeof(p->pak_file_info), 1, fp);
	
	return 0;
}

/**
 * 写入所有的文件信息
 */
int write_files_info(FILE* fp, filelist_t* p) {
	// 记录一下当前的编译地址，文件头需要用的
	pakinfo.filelist_offset = ftell(fp);
	
	// 依次写入文件信息
	while ((p = p->next) != NULL) {
		write_file_info(fp, p);
	}
	
	return 0;
}


/**
 * 写入资源文件
 */
int write_file(const char* file, filelist_t* p) {
	FILE* fp;
	char empty[0x400] = {0};
	
	fp = fopen(file, "w");
	if (fp == NULL) {
		printf("无法访问文件 %s\n", file);
		exit(1);
	}
	
	fwrite(empty, 0x400, 1, fp);	//先写400个字节的数据，因为文件内容必须从0x400开始
	write_files_content(fp, p);
	write_files_info(fp, p);
	write_head(fp, p);	// 因为文件头中有一个字段是文件信息偏移地址，所以需要在最后才写入文件头
	
	fclose(fp);
	
	printf("文件已经成功写入\n");
	
	return 0;
}


/**
 * 打印帮助
 */
int print_usage(const char* argv0) {
	printf("用法：%s 资源文件名 [原始文件路径1[ 原始文件路径2[ ...[原始文件路径n]]]]\n", strrchr(argv0, '/') + 1);
	
	return 0;
}

int main(int argc, char** argv) {
	filelist_t* p;
	int i;
	
	if (argc < 3) {
		print_usage(argv[0]);
		exit(0);
	}
	
	// 初始化各种东西
	p = (filelist_t*)malloc(sizeof(filelist_t));
	p->next = NULL;
	
	for (i = 0; i < 256; i++) {
		pakinfo.head[i] = 0;
	}
	strcpy(pakinfo.head, "EyedentityGames Packing File 0.1");
	pakinfo.static1[0] = 0x0B;
	pakinfo.static1[1] = pakinfo.static1[2] = pakinfo.static1[3] = 0;
	
	
	// 获取文件列表
	for (i = 2; i < argc; i++) {
		get_folder_files(p, argv[i]);
	}
	
	// 写入文件
	write_file(argv[1], p);
	
	return 0;
}
