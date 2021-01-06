#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>

#define VERSION "1.0.0"

struct TIME
{
	unsigned short year;
	unsigned char month, day, hour, min, sec;
};

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
	 * 0 deleted
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
	/* ファイルヘッダ/データの構造体配列の要素数 */
	unsigned int fs_length;
	/* ファイル作成時刻 */
	struct TIME createTime;
	/* 予約領域 パティング */
	unsigned char reserve1;
	/* ファイルシステムのリスト数 */
	unsigned int filesys_len;
	/* 予約領域 */
	unsigned char reserve2[12];
};

#define FILESYSTEM_CYLINDER 1024
#define FILESYSTEM_LENGTH 100000

static unsigned int used_struc_fsys = 0;
static unsigned int filesys_len = FILESYSTEM_LENGTH;

void add_file(const char *fname, struct FILESYSTEM *fsys, unsigned char *buf);

void filesys_init(struct FILESYSTEM *file)
{
	int i, j;

	for(i = 0; i < filesys_len; i++){
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
		file[i].reserve1 = 0x00;
		file[i].filesys_len = 0;
		for(j = 0; j < 12; j++){
			file[i].reserve2[j] = 0x00;
		}
	}

	return;
}

int main(int argc, char *argv[])
{
	struct FILESYSTEM *file = calloc(1, sizeof(struct FILESYSTEM) * filesys_len);
	unsigned char *buf = calloc(1, sizeof(unsigned char) * FILESYSTEM_CYLINDER * filesys_len);
	int i;
	char *outname = "out";

	/* 空のイメージを作る */
	filesys_init(file);

	/* コマンドライン処理/本処理 */
	for(i = 0; i < argc; i++){
		if(!strcmp(argv[i], "-help")){
			printf("version" VERSION "\n");
			printf("mkfs [syslen:] [out:] file: ...\n");
			printf("\n");
			printf("--- options ---");
			printf("syslen:    ファイルシステムの最大長 初めに指定しなければいけない\n");
			printf("out:       出力ファイル名\n");
			printf("file:      入力ファイル名\n");
			exit(-1);
		}
		/* 入れるファイルを指定 file:入力ファイル */
		if(!strncmp(argv[i], "file:", 5) && strlen(argv[i]) > 5){
			printf("add file : name length %d\n", strlen(argv[i]));
			add_file(argv[i] + 5, file, buf);
		}
		/* 出力ファイル */
		if(!strncmp(argv[i], "out:", 4) && strlen(argv[i]) > 4){
			outname = argv[i] + 4;
		}
		/* ファイルシステムのサイズ */
		if(!strncmp(argv[i], "syslen:", 7) && strlen(argv[i]) > 7){
			unsigned long len = strtoul(argv[i] + 7, NULL, 0);
			if(len != 0 && len < UINT_MAX + 1){
				filesys_len = len;
				/* 必要メモリを取る */
				file = realloc(file, sizeof(struct FILESYSTEM) * filesys_len);
				free(buf);
				buf = calloc(1, sizeof(unsigned char) * FILESYSTEM_CYLINDER * filesys_len);
				filesys_init(file);
			}else{
				printf("filessystem length error.");
				exit(-1);
			}
		}
	}

//	add_file("read.txt", file, buf);
//	add_file("read2.txt", file, buf);
//	add_file("read3.txt", file, buf);

	FILE *output = fopen(outname, "wb");
	if(output == NULL){
		printf("file open error.\n");
		return -1;
	}

	fwrite((void *)file, sizeof(struct FILESYSTEM) * filesys_len, 1, output);
	fwrite((void *)buf, FILESYSTEM_CYLINDER * filesys_len, 1, output);

	fclose(output);

	return 0;
}

void add_file(const char *fname, struct FILESYSTEM *fsys, unsigned char *buf)
{
	int i;
//	printf("[debug]:process %s\n", fname);
//	printf("[debug]:fname length %d\n", strlen(fname));
//	for(i = 0; i < strlen(fname); i++){
//		printf("[debug]:fname[%d] %02x\n", i, fname + i);
//	}
//	printf("[debug]:file number %d\n", used_struc_fsys);
	FILE *in = fopen(fname, "rb");
	if(in == NULL){
		printf("read file open error.\n");
		exit(-1);
	}

	unsigned char fbuf[FILESYSTEM_CYLINDER];
	unsigned int fsize, freadsize, writed = 0;
	unsigned short fnum = 0;

	fseek(in, 0, SEEK_END);
	fsize = ftell(in);
	fseek(in, 0, SEEK_SET);

	while(writed < fsize){
		if(used_struc_fsys <= filesys_len){
			memcpy(fsys[used_struc_fsys].fname, fname, strlen(fname));
			fsys[used_struc_fsys].num = fnum;
			fsys[used_struc_fsys].flg = 0x00;
			fsys[used_struc_fsys].size = fsize;
			fsys[used_struc_fsys].addr = sizeof(fsys) * fsys->fs_length + FILESYSTEM_CYLINDER * used_struc_fsys;
			fsys[used_struc_fsys].filesys_len = filesys_len;

			fread(buf + FILESYSTEM_CYLINDER * used_struc_fsys, 1, FILESYSTEM_CYLINDER, in);

			writed += FILESYSTEM_CYLINDER;
			used_struc_fsys++;

			if(writed < fsize){
				fnum++;
				fsys[used_struc_fsys - 1].next = 1;
			}
		}else{
			printf("FileSystem Overflow.");
			exit(-1);
		}
	}

	fclose(in);

	return;
}
