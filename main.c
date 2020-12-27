#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct FILESYSTEM
{
	/* ファイル名 .も含む 足りない場合は0埋め */
	unsigned char fname[32];
	/* num 0なら初めのエントリ */
	unsigned short num;
	/*
	 * ファイル管理フラグ
	 *
	 * 0 not use
	 * 0 reserve
	 * 0 reserve
	 * 0 reserve
	 * 0 reserve
	 * 0 directory
	 * 0 hidden file
	 * 0 read only
	 */
	unsigned char flg;
	/* 予約領域 */
	unsigned char reserve0;
	/* size ファイルサイズ */
	/* addr ファイルデータの場所 */
	/* next 次のデータへの相対位置 */
	unsigned int size, addr, next;
	/* ディレクトリ番号 1始まり ルートは0 */
	/* ファイルの場合所属ディレクトリ */
	/* ディレクトリの場合ディレクトリ固有の番号 */
	unsigned int dirnum;
	unsigned int fs_length;
	/* 予約領域 パティング */
	unsigned char reserve1[8];
};

static int used_struc_fsys = 0;

void add_file(const char *fname, struct FILESYSTEM *fsys, unsigned char *buf);

void *zalloc(unsigned int size)
{
	void *mal = malloc(size);
	memset(mal, '\0', size);

	return mal;
}

#define FILESYSTEM_LENGTH 100000

int main(int argc, char **argv)
{
	unsigned int filesys_len = FILESYSTEM_LENGTH;
	struct FILESYSTEM *file = zalloc(sizeof(struct FILESYSTEM) * filesys_len);
	unsigned char *buf = zalloc(sizeof(unsigned char) * 1024 * filesys_len);
	int i, j;

	for(i = 0; i < 100000; i++){
		for(j = 0; j < 32; j++){
			file[i].fname[j] = 0x00;
		}
		/* no directory no hidden no readonly notuse */
		file[i].flg = 0x80;
		file[i].reserve0 = 0x00;
		file[i].size = 0x00000000;
		file[i].addr = 0x00000000;
		file[i].next = 0x00000000;
		file[i].dirnum = 0x00000000;
		file[i].fs_length = filesys_len;
		for(j = 0; j < 8; j++){
			file[i].reserve1[j] = 0x00;
		}
	}

	add_file("read.txt", file, buf);
	add_file("read2.txt", file, buf);
	add_file("read3.txt", file, buf);

	FILE *output = fopen("out", "wb");
	if(output == NULL){
		printf("file open error.\n");
		return -1;
	}

	fwrite((void *)file, sizeof(struct FILESYSTEM) * filesys_len, 1, output);
	fwrite((void *)buf, 1024 * filesys_len, 1, output);

	fclose(output);

	return 0;
}

void add_file(const char *fname, struct FILESYSTEM *fsys, unsigned char *buf)
{
	printf("[debug]:process %s\n", fname);
	printf("[debug]:file number %d\n", used_struc_fsys);
	FILE *in = fopen(fname, "rb");
	if(in == NULL){
		printf("read file open error.\n");
		exit(-1);
	}

	unsigned char fbuf[1024];
	unsigned int fsize, freadsize, writed = 0;
	unsigned short fnum = 0;

	fseek(in, 0, SEEK_END);
	fsize = ftell(in);
	fseek(in, 0, SEEK_SET);

	while(writed < fsize){
		memcpy(fsys[used_struc_fsys].fname, fname, strlen(fname));
		fsys[used_struc_fsys].num = fnum;
		fsys[used_struc_fsys].flg = 0x00;
		fsys[used_struc_fsys].size = fsize;
		fsys[used_struc_fsys].addr = sizeof(fsys) * fsys->fs_length + 1024 * used_struc_fsys;

		fread(buf + 1024 * used_struc_fsys, 1, 1024, in);

		writed += 1024;
		used_struc_fsys++;

		if(writed < fsize){
			fnum++;
			fsys[used_struc_fsys - 1].next = 1;
		}
	}

	fclose(in);

	return;
}
