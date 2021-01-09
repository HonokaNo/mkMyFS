# mkMyFS
自作OS用ファイルシステム作成ソフトです。
今のところファイルシステムに名称はありません。

# オプション
mkfs \[syslen:\] \[out:\] file: ...

syslen: ファイルシステムの最大長 1024\*syslen byteのデータを格納できます 一番初めに指定しなければいけません<br>
out: 出力するファイル名<br>
file: 追加するファイルの名称 いくつでも指定可能<br>

# 基本構成
このファイルシステムはヘッダと一定数のバイトの集まりで構成されています
```
ファイルヘッダ[0]
ファイルヘッダ[1]
...
ファイルヘッダ[n]
一定数バイトの集まり(例として1024バイト)[0]
一定数バイトの集まり(例として1024バイト)[1]
...
一定数バイトの集まり(例として1024バイト)[n]
```

ヘッダは下のようになっています

```
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
```

# ざっくりとした使い方<br>

* ファイルリストを取得する<br>
1.ヘッダ配列を取得する<br>
2.ヘッダ内のfs_lengthを取得する<br>
3.ヘッダ配列[0] - ヘッダ配列[fs_length - 1] の要素に対し<br>
  flg & 0x80 が0 かつ num == 0 の要素を探す<br>
4.一致したヘッダからデータを取得する<br>
