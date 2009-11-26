#ifndef _FILE_CACHE_H_
#define _FILE_CACHE_H_

#include "base.h"
/*

typedef struct 
{
	buffer *name;
	buffer *etag;
	struct stat st;
	time_t stat_ts;
#ifdef HAVE_LSTAT
	char is_symlink;
#endif
#ifdef HAVE_FAM_H
	int dir_version;
	int dir_ndx;
#endif
	buffer *content_type;
} stat_cache_entry;

//状态缓存
typedef struct 
{
	splay_tree *files;		//保存文件名
	buffer *dir_name;		//目录名
#ifdef HAVE_FAM_H
	splay_tree *dirs;		//保存目录名
	FAMConnection *fam; 	//FAM系统
	int fam_fcce_ndx;
#endif
	buffer *hash_key;
} stat_cache;

*/

/**
 * 初始化和删除缓存树
 */
stat_cache *stat_cache_init(void);
void stat_cache_free(stat_cache * fc);

handler_t stat_cache_get_entry(server * srv, connection * con,
							   buffer * name, stat_cache_entry ** fce);
handler_t stat_cache_handle_fdevent(void *_srv, void *_fce, int revent);

int stat_cache_trigger_cleanup(server * srv);
#endif
