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
	/* �t�@�C���� .���܂� ����Ȃ��ꍇ��0���� */
	unsigned char fname[32];
	/* num 0�Ȃ珉�߂̃G���g�� */
	unsigned short num;
	/*
	 * �t�@�C���Ǘ��t���O
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
	/* �\��̈� */
	unsigned char reserve0;
	/* size �t�@�C���T�C�Y */
	/* addr �t�@�C���f�[�^�̏ꏊ */
	/* next ���̃f�[�^�ւ̑��Έʒu */
	unsigned int size, addr, next;
	/* �f�B���N�g���ԍ� 1�n�܂� ���[�g��0 */
	/* �t�@�C���̏ꍇ�����f�B���N�g�� */
	/* �f�B���N�g���̏ꍇ�f�B���N�g���ŗL�̔ԍ� */
	unsigned int dirnum;
	/* �t�@�C���w�b�_/�f�[�^�̍\���̔z��̗v�f�� */
	unsigned int fs_length;
	/* �t�@�C���쐬���� */
	struct TIME createTime;
	/* �\��̈� �p�e�B���O */
	unsigned char reserve1;
	/* �t�@�C���V�X�e���̃��X�g�� */
	unsigned int filesys_len;
	/* �\��̈� */
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

	/* ��̃C���[�W����� */
	filesys_init(file);

	/* �R�}���h���C������/�{���� */
	for(i = 0; i < argc; i++){
		if(!strcmp(argv[i], "-help")){
			printf("version" VERSION "\n");
			printf("mkfs [syslen:] [out:] file: ...\n");
			printf("\n");
			printf("--- options ---");
			printf("syslen:    �t�@�C���V�X�e���̍ő咷 ���߂Ɏw�肵�Ȃ���΂����Ȃ�\n");
			printf("out:       �o�̓t�@�C����\n");
			printf("file:      ���̓t�@�C����\n");
			exit(-1);
		}
		/* �����t�@�C�����w�� file:���̓t�@�C�� */
		if(!strncmp(argv[i], "file:", 5) && strlen(argv[i]) > 5){
			printf("add file : name length %d\n", strlen(argv[i]));
			add_file(argv[i] + 5, file, buf);
		}
		/* �o�̓t�@�C�� */
		if(!strncmp(argv[i], "out:", 4) && strlen(argv[i]) > 4){
			outname = argv[i] + 4;
		}
		/* �t�@�C���V�X�e���̃T�C�Y */
		if(!strncmp(argv[i], "syslen:", 7) && strlen(argv[i]) > 7){
			unsigned long len = strtoul(argv[i] + 7, NULL, 0);
			if(len != 0 && len < UINT_MAX + 1){
				filesys_len = len;
				/* �K�v����������� */
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
