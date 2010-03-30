#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/stat.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <assert.h>

#include "log.h"
#include "stat_cache.h"
#include "fdevent.h"
#include "etag.h"

#ifdef HAVE_ATTR_ATTRIBUTES_H
#include <attr/attributes.h>
#endif

#ifdef HAVE_FAM_H
# include <fam.h>
/**
 * File Alteration Monitor, also known as FAM and sgi_fam, provides a subsystem developed by Silicon Graphics for 
 * Unix-like operating systems. The FAM subsystem allows applications to watch certain files and be notified when 
 * they were modified. This greatly aids the applications, because before FAM existed, such applications would have 
 * to read the disk repeatedly to detect any changes: this resulted in high disk and CPU usage.
 * For example, a file manager application can detect if some file has changed and can then update a displayed 
 * icon and/or filename.
 *
 * The FAM system consists of two parts:
 * 		famd — the FAM Daemon, which provides notifications and listens for requests. Administrators can configure 
 * 				it by editing the file /etc/fam.conf
 * 		libfam — the interface to the client
 * Although FAM may seem unnecessary now that many newer kernels include built-in notification support 
 * (inotify in Linux, for example), using FAM provides two benefits:
 * 		Consistently using FAM enables applications to work on a greater variety of platforms, agnostic of the kernel
 * 		FAM is network aware, and if a monitor is started on an NFS share, it will attempt to contact a FAM server on 
 * 				the NFS server and have it monitor the file locally, which is more efficient.
 *
 * The main problem of FAM is that during the creation of a large amount of files (for example during the first login in 
 * a desktop environment) it slows down the entire system, using many CPU cycles.
 *
 * Usage:
 *
 * 1.FAMOpen() connects to the FAM service:
 * 		FAMOpen( fc ) ;
 * Call this once per program.
 *
 * 2.FAMMonitorFile() registers filenames of interest:
 * 		FAMMonitorFile( fc , file , fr , NULL ) ;
 * fc and fr are the FAMConnection and FAMRequest objects defined earlier. file is the filename to register. 
 * The last parameter is user-defined data included in the event object(store in fe->userdata). 
 * It's useful in more complex apps that create FAM-related objects in one scope but use them in another. 
 *
 * 3.A FAMEvent object encapsulates an event on a watched target. 
 * Its member variables include the watch target's name, an event code, and the user-data parameter 
 * set in FAMMonitorFile(). 
 * FAMNextEvent() catches FAM events--that is, changes to watch targets--and populates the provided FAMEvent pointer, fe, 
 * with that information:
 * 		FAMNextEvent( fc , fe ) ;
 *
 * 4.You use FAMPending() to ensure that an event is ready before calling FAMNextEvent(). 
 * FAMPending() doesn't block, and it returns 1 if there's at least one event to process.
 *
 * 5.Calls FAMCancelMonitor() to cancel the FAM monitoring.
 *  	FAMCancelMonitor( fc , fr )
 *
 * 6.Watching a directory means reporting events on its immediate children as well as the directory itself. 
 * It's very similar to watching files, except that you call FAMMonitorDirectory() instead of FAMMonitorFile(). 
 * 	Calling FAMMonitorDirectory() on a file doesn't fail. (The same goes for calling FAMMonitorFile() for a directory.) 
 * FAM will still report some events, though. Have your code stat() or lstat() the target to determine 
 * which FAM call to use.
 *  Registering a directory with FAM will report several FAMExists events: one for the directory itself, 
 *  plus one for each element contained therein. 
 *  (This includes hidden elements, except for the special directory entries . and ...) 
 *  A single FAMEndExist event marks the end of this list.
 */
#endif

#include "sys-mmap.h"

/*
 * NetBSD 1.3.x needs it 
 */
#ifndef MAP_FAILED
# define MAP_FAILED -1
#endif

#ifndef O_LARGEFILE
# define O_LARGEFILE 0
#endif

#ifndef HAVE_LSTAT
#define lstat stat
#endif

#if 0
/*
 * enables debug code for testing if all nodes in the stat-cache as accessable 
 */
#define DEBUG_STAT_CACHE
#endif

/*
 * stat-cache
 *
 * we cache the stat() calls in our own storage, the directories are cached in FAM
 * 缓存stat()函数获得的文件状态，目录状态由FAM缓存。
 * if we get a change-event from FAM, we increment the version in the FAM->dir mapping
 * 如果我们从FAM得到了一个状态事件，我们将提高FAM->dir所对应的版本。
 * if the stat()-cache is queried we check if the version id for the directory is the
 * same and return immediatly.
 * 如果状态缓存被查询，我们将检验目录的版本ID是否形同并立即返回。
 *
 * What we need:
 *
 * - for each stat-cache entry we need a fast indirect lookup on the directory name
 * - for each FAMRequest we have to find the version in the directory cache (index as userdata)
 * - 对于每一个缓存状态，我们需要一次对目录名称的快速的间接的查询。
 * - 对于每一个FAMRequest，我们需要在目录缓存中找到版本号。(由useddata索引)
 * stat <<-> directory <-> FAMRequest
 *
 * if file is deleted, directory is dirty, file is rechecked ...
 * if directory is deleted, directory mapping is removed
 *
 * */

#ifdef HAVE_FAM_H
//使用FAM来监控目录的状态，这个结构体就是FAM请求和被监视目录之间的映射。
typedef struct 
{
	FAMRequest *req;
	FAMConnection *fc;

	buffer *name;

	int version;
} fam_dir_entry;
#endif

/*
 * the directory name is too long to always compare on it - we need a hash -
 * the hash-key is used as sorting criteria for a tree - a splay-tree is used
 * as we can use the caching effect of it 
 * 目录名称通常过长而无法总是比较它们。因此我们需要一次哈希。哈希值将作为一棵树
 * 的保存标准。
 */

/*
 * we want to cleanup the stat-cache every few seconds, let's say 10 - remove
 * entries which are outdated since 30s - remove entries which are fresh but
 * havn't been used since 60s - if we don't have a stat-cache entry for a
 * directory, release it from the monitor 
 * 我们希望每隔几秒就清理一下状态缓存，比如10秒。删除过期30秒的缓存，删除60秒未被访问的缓存，
 * 如果我们没有一个目录的状态缓存，则从FAM中释放它。
 */

#ifdef DEBUG_STAT_CACHE
typedef struct 
{
	int *ptr;

	size_t used;
	size_t size;
} fake_keys;

static fake_keys ctrl;
#endif

stat_cache *stat_cache_init(void)
{
	stat_cache *fc = NULL;

	fc = calloc(1, sizeof(*fc));

	fc->dir_name = buffer_init();
	fc->hash_key = buffer_init();
#ifdef HAVE_FAM_H
	fc->fam = calloc(1, sizeof(*fc->fam));
#endif

#ifdef DEBUG_STAT_CACHE
	ctrl.size = 0;
#endif

	return fc;
}

static stat_cache_entry *stat_cache_entry_init(void)
{
	stat_cache_entry *sce = NULL;

	sce = calloc(1, sizeof(*sce));

	sce->name = buffer_init();
	sce->etag = buffer_init();
	sce->content_type = buffer_init();

	return sce;
}

static void stat_cache_entry_free(void *data)
{
	stat_cache_entry *sce = data;
	if (!sce)
		return;

	buffer_free(sce->etag);
	buffer_free(sce->name);
	buffer_free(sce->content_type);

	free(sce);
}

#ifdef HAVE_FAM_H
static fam_dir_entry *fam_dir_entry_init(void)
{
	fam_dir_entry *fam_dir = NULL;

	fam_dir = calloc(1, sizeof(*fam_dir));

	fam_dir->name = buffer_init();

	return fam_dir;
}

static void fam_dir_entry_free(void *data)
{
	fam_dir_entry *fam_dir = data;

	if (!fam_dir)
		return;
	//
	FAMCancelMonitor(fam_dir->fc, fam_dir->req);

	buffer_free(fam_dir->name);
	free(fam_dir->req);

	free(fam_dir);
}
#endif

void stat_cache_free(stat_cache * sc)
{
	while (sc->files)
	{
		int osize;
		splay_tree *node = sc->files;

		osize = sc->files->size;

		stat_cache_entry_free(node->data);
		sc->files = splaytree_delete(sc->files, node->key);

		assert(osize - 1 == splaytree_size(sc->files));
	}

	buffer_free(sc->dir_name);
	buffer_free(sc->hash_key);

#ifdef HAVE_FAM_H
	while (sc->dirs)
	{
		int osize;
		splay_tree *node = sc->dirs;

		osize = sc->dirs->size;

		fam_dir_entry_free(node->data);
		sc->dirs = splaytree_delete(sc->dirs, node->key);

		if (osize == 1)
		{
			assert(NULL == sc->dirs);
		} else
		{
			assert(osize == (sc->dirs->size + 1));
		}
	}

	if (sc->fam)
	{
		FAMClose(sc->fam);
		free(sc->fam);
	}
#endif
	free(sc);
}

#ifdef HAVE_XATTR
static int stat_cache_attr_get(buffer * buf, char *name)
{
	int attrlen;
	int ret;

	attrlen = 1024;
	buffer_prepare_copy(buf, attrlen);
	attrlen--;
	if (0 == (ret = attr_get(name, "Content-Type", buf->ptr, &attrlen, 0)))
	{
		buf->used = attrlen + 1;
		buf->ptr[attrlen] = '\0';
	}
	return ret;
}
#endif

/*
 * the famous DJB hash function for strings 
 */
static uint32_t hashme(buffer * str)
{
	uint32_t hash = 5381;
	const char *s;
	for (s = str->ptr; *s; s++)
	{
		hash = ((hash << 5) + hash) + *s;
	}

	hash &= ~(1 << 31);			/* strip the highest bit */

	return hash;
}

#ifdef HAVE_FAM_H
handler_t stat_cache_handle_fdevent(void *_srv, void *_fce, int revent)
{
	size_t i;
	server *srv = _srv;
	stat_cache *sc = srv->stat_cache;
	size_t events;

	UNUSED(_fce);
	/*
	 */

	if ((revent & FDEVENT_IN) && sc->fam)
	{

		events = FAMPending(sc->fam);

		for (i = 0; i < events; i++)
		{
			FAMEvent fe;
			fam_dir_entry *fam_dir;
			splay_tree *node;
			int ndx, j;

			FAMNextEvent(sc->fam, &fe);

			/*
			 * handle event 
			 */

			switch (fe.code)
			{
			case FAMChanged:
			case FAMDeleted:
			case FAMMoved:
				/*
				 * if the filename is a directory remove the entry 
				 */

				fam_dir = fe.userdata;
				fam_dir->version++;

				/*
				 * file/dir is still here 
				 */
				if (fe.code == FAMChanged)
					break;

				/*
				 * we have 2 versions, follow and no-follow-symlink 
				 */

				for (j = 0; j < 2; j++)
				{
					buffer_copy_string(sc->hash_key, fe.filename);
					buffer_append_long(sc->hash_key, j);

					ndx = hashme(sc->hash_key);

					sc->dirs = splaytree_splay(sc->dirs, ndx);
					node = sc->dirs;

					if (node && (node->key == ndx))
					{
						int osize = splaytree_size(sc->dirs);

						fam_dir_entry_free(node->data);
						sc->dirs = splaytree_delete(sc->dirs, ndx);

						assert(osize - 1 == splaytree_size(sc->dirs));
					}
				}
				break;
			default:
				break;
			}
		}
	}

	if (revent & FDEVENT_HUP)
	{
		/*
		 * fam closed the connection 
		 */
		srv->stat_cache->fam_fcce_ndx = -1;

		fdevent_event_del(srv->ev, &(sc->fam_fcce_ndx), FAMCONNECTION_GETFD(sc->fam));
		fdevent_unregister(srv->ev, FAMCONNECTION_GETFD(sc->fam));

		FAMClose(sc->fam);
		free(sc->fam);

		sc->fam = NULL;
	}

	return HANDLER_GO_ON;
}
/**
 * 将file的目录路径拷贝到dst中。
 * 也就是删去file中的文件名。
 */
static int buffer_copy_dirname(buffer * dst, buffer * file)
{
	size_t i;

	if (buffer_is_empty(file))
		return -1;

	for (i = file->used - 1; i + 1 > 0; i--)
	{
		if (file->ptr[i] == '/')
		{
			buffer_copy_string_len(dst, file->ptr, i);
			return 0;
		}
	}

	return -1;
}
#endif

#ifdef HAVE_LSTAT
//判断dname是否是一个链接文件。
//是:1,否:0
static int stat_cache_lstat(server * srv, buffer * dname, struct stat *lst)
{
	if (lstat(dname->ptr, lst) == 0)
	{
		return S_ISLNK(lst->st_mode) ? 0 : 1;
	} else
	{
		log_error_write(srv, __FILE__, __LINE__, "sbs",
						"lstat failed for:", dname, strerror(errno));
	};
	return -1;
}
#endif

/***
 *
 * returns:
 *  - HANDLER_FINISHED on cache-miss (don't forget to reopen the file)
 *  - HANDLER_ERROR on stat() failed -> see errno for problem
 */

handler_t stat_cache_get_entry(server * srv, connection * con, buffer * name,
					 stat_cache_entry ** ret_sce)
{
#ifdef HAVE_FAM_H
	fam_dir_entry *fam_dir = NULL;
	int dir_ndx = -1;
	splay_tree *dir_node = NULL;
#endif
	stat_cache_entry *sce = NULL;
	stat_cache *sc;
	struct stat st;
	size_t k;
	int fd;
	struct stat lst;
#ifdef DEBUG_STAT_CACHE
	size_t i;
#endif

	int file_ndx;
	splay_tree *file_node = NULL;

	*ret_sce = NULL;

	/*
	 * check if the directory for this file has changed
	 */

	sc = srv->stat_cache;

	buffer_copy_string_buffer(sc->hash_key, name);
	buffer_append_long(sc->hash_key, con->conf.follow_symlink);

	file_ndx = hashme(sc->hash_key);
	sc->files = splaytree_splay(sc->files, file_ndx);

#ifdef DEBUG_STAT_CACHE
	for (i = 0; i < ctrl.used; i++)
	{
		if (ctrl.ptr[i] == file_ndx)
			break;
	}
#endif

	if (sc->files && (sc->files->key == file_ndx))
	{
#ifdef DEBUG_STAT_CACHE
		/*
		 * it was in the cache 
		 */
		assert(i < ctrl.used);
#endif

		/*
		 * we have seen this file already and don't stat() it again in the same 
		 * second 
		 */

		file_node = sc->files;

		sce = file_node->data;

		/*
		 * check if the name is the same, we might have a collision 
		 */

		if (buffer_is_equal(name, sce->name))
		{
			if (srv->srvconf.stat_cache_engine == STAT_CACHE_ENGINE_SIMPLE)
			{
				if (sce->stat_ts == srv->cur_ts)
				{
					*ret_sce = sce;
					return HANDLER_GO_ON;
				}
			}
		} else
		{
			/*
			 * oops, a collision, file_node is used by the FAM check below to
			 * see if we know this file and if we can save a stat(). BUT, the
			 * sce is not reset here as the entry into the cache is ok, we it
			 * is just not pointing to our requested file.  
			 */

			file_node = NULL;
		}
	} else
	{
#ifdef DEBUG_STAT_CACHE
		if (i != ctrl.used)
		{
			fprintf(stderr,
					"%s.%d: %08x was already inserted but not found in cache, %s\n",
					__FILE__, __LINE__, file_ndx, name->ptr);
		}
		assert(i == ctrl.used);
#endif
	}

#ifdef HAVE_FAM_H
	/*
	 * dir-check 
	 */
	if (srv->srvconf.stat_cache_engine == STAT_CACHE_ENGINE_FAM)
	{
		if (0 != buffer_copy_dirname(sc->dir_name, name))
		{
			log_error_write(srv, __FILE__, __LINE__, "sb",
							"no '/' found in filename:", name);
			return HANDLER_ERROR;
		}

		buffer_copy_string_buffer(sc->hash_key, sc->dir_name);
		buffer_append_long(sc->hash_key, con->conf.follow_symlink);

		dir_ndx = hashme(sc->hash_key);

		sc->dirs = splaytree_splay(sc->dirs, dir_ndx);

		if (sc->dirs && (sc->dirs->key == dir_ndx))
		{
			dir_node = sc->dirs;
		}

		if (dir_node && file_node)
		{
			/*
			 * we found a file 
			 */

			sce = file_node->data;
			fam_dir = dir_node->data;

			if (fam_dir->version == sce->dir_version)
			{
				/*
				 * the stat()-cache entry is still ok 
				 */

				*ret_sce = sce;
				return HANDLER_GO_ON;
			}
		}
	}
#endif

	/*
	 * *lol*
	 * - open() + fstat() on a named-pipe results in a (intended) hang.
	 * - stat() if regular file + open() to see if we can read from it is better
	 *
	 * */
	if (-1 == stat(name->ptr, &st))
	{
		return HANDLER_ERROR;
	}


	if (S_ISREG(st.st_mode))
	{
		/*
		 * try to open the file to check if we can read it 
		 */
		if (-1 == (fd = open(name->ptr, O_RDONLY)))
		{
			return HANDLER_ERROR;
		}
		close(fd);
	}

	if (NULL == sce)
	{
		int osize = 0;

		if (sc->files)
		{
			osize = sc->files->size;
		}

		sce = stat_cache_entry_init();
		buffer_copy_string_buffer(sce->name, name);

		sc->files = splaytree_insert(sc->files, file_ndx, sce);
#ifdef DEBUG_STAT_CACHE
		if (ctrl.size == 0)
		{
			ctrl.size = 16;
			ctrl.used = 0;
			ctrl.ptr = malloc(ctrl.size * sizeof(*ctrl.ptr));
		} else if (ctrl.size == ctrl.used)
		{
			ctrl.size += 16;
			ctrl.ptr = realloc(ctrl.ptr, ctrl.size * sizeof(*ctrl.ptr));
		}

		ctrl.ptr[ctrl.used++] = file_ndx;

		assert(sc->files);
		assert(sc->files->data == sce);
		assert(osize + 1 == splaytree_size(sc->files));
#endif
	}

	sce->st = st;
	sce->stat_ts = srv->cur_ts;

	/*
	 * catch the obvious symlinks this is not a secure check as we still have
	 * a race-condition between the stat() and the open. We can only solve this 
	 * by 1. open() the file 2. fstat() the fd and keeping the file open for
	 * the rest of the time. But this can only be done at network level. per
	 * default it is not a symlink 
	 */
#ifdef HAVE_LSTAT
	sce->is_symlink = 0;

	/*
	 * we want to only check for symlinks if we should block symlinks. 
	 */
	if (!con->conf.follow_symlink)
	{
		if (stat_cache_lstat(srv, name, &lst) == 0)
		{
#ifdef DEBUG_STAT_CACHE
			log_error_write(srv, __FILE__, __LINE__, "sb",
							"found symlink", name);
#endif
			sce->is_symlink = 1;
		}

		/*
		 * we assume "/" can not be symlink, so
		 * skip the symlink stuff if our path is /
		 **/
		else if ((name->used > 2))
		{
			buffer *dname;
			char *s_cur;

			dname = buffer_init();
			buffer_copy_string_buffer(dname, name);

			while ((s_cur = strrchr(dname->ptr, '/')))
			{
				*s_cur = '\0';
				dname->used = s_cur - dname->ptr + 1;
				if (dname->ptr == s_cur)
				{
#ifdef DEBUG_STAT_CACHE
					log_error_write(srv, __FILE__, __LINE__, "s", "reached /");
#endif
					break;
				}
#ifdef DEBUG_STAT_CACHE
				log_error_write(srv, __FILE__, __LINE__, "sbs",
								"checking if", dname, "is a symlink");
#endif
				if (stat_cache_lstat(srv, dname, &lst) == 0)
				{
					sce->is_symlink = 1;
#ifdef DEBUG_STAT_CACHE
					log_error_write(srv, __FILE__, __LINE__, "sb",
									"found symlink", dname);
#endif
					break;
				};
			};
			buffer_free(dname);
		};
	};
#endif

	if (S_ISREG(st.st_mode))
	{
		/*
		 * determine mimetype 
		 */
		buffer_reset(sce->content_type);

		for (k = 0; k < con->conf.mimetypes->used; k++)
		{
			data_string *ds = (data_string *) con->conf.mimetypes->data[k];
			buffer *type = ds->key;

			if (type->used == 0)
				continue;

			/*
			 * check if the right side is the same 
			 */
			if (type->used > name->used)
				continue;

			if (0 ==
				strncasecmp(name->ptr + name->used - type->used,
							type->ptr, type->used - 1))
			{
				buffer_copy_string_buffer(sce->content_type, ds->value);
				break;
			}
		}
		etag_create(sce->etag, &(sce->st), con->etag_flags);
#ifdef HAVE_XATTR
		if (con->conf.use_xattr && buffer_is_empty(sce->content_type))
		{
			stat_cache_attr_get(sce->content_type, name->ptr);
		}
#endif
	} else if (S_ISDIR(st.st_mode))
	{
		etag_create(sce->etag, &(sce->st), con->etag_flags);
	}
#ifdef HAVE_FAM_H
	if (sc->fam && (srv->srvconf.stat_cache_engine == STAT_CACHE_ENGINE_FAM))
	{
		/*
		 * is this directory already registered ? 
		 */
		if (!dir_node)
		{
			fam_dir = fam_dir_entry_init();
			fam_dir->fc = sc->fam;

			buffer_copy_string_buffer(fam_dir->name, sc->dir_name);

			fam_dir->version = 1;

			fam_dir->req = calloc(1, sizeof(FAMRequest));

			if (0 != FAMMonitorDirectory(sc->fam, fam_dir->name->ptr,
										 fam_dir->req, fam_dir))
			{

				log_error_write(srv, __FILE__, __LINE__, "sbsbs",
								"monitoring dir failed:",
								fam_dir->name,
								"file:", name, FamErrlist[FAMErrno]);

				fam_dir_entry_free(fam_dir);
			} else
			{
				int osize = 0;

				if (sc->dirs)
				{
					osize = sc->dirs->size;
				}

				sc->dirs = splaytree_insert(sc->dirs, dir_ndx, fam_dir);
				assert(sc->dirs);
				assert(sc->dirs->data == fam_dir);
				assert(osize == (sc->dirs->size - 1));
			}
		} else
		{
			fam_dir = dir_node->data;
		}

		/*
		 * bind the fam_fc to the stat() cache entry 
		 */

		if (fam_dir)
		{
			sce->dir_version = fam_dir->version;
			sce->dir_ndx = dir_ndx;
		}
	}
#endif

	*ret_sce = sce;

	return HANDLER_GO_ON;
}

/**
 * remove stat() from cache which havn't been stat()ed for
 * more than 10 seconds
 * 从缓存中删除超过10s没有stat()ed的stat()
 *
 * walk though the stat-cache, collect the ids which are too old
 * and remove them in a second loop
 * 遍历stat缓存，收集较旧的id并在第二个循环中删除之。
 *
 * 先序遍历伸展树，将stat_ts比服务器的当前时间cur_ts小2的stat_cache_entry的id存入
 * 数组key中。
 */

static int stat_cache_tag_old_entries(server * srv, splay_tree * t, int *keys,
						   size_t * ndx)
{
	stat_cache_entry *sce;

	if (!t)
		return 0;

	stat_cache_tag_old_entries(srv, t->left, keys, ndx);
	stat_cache_tag_old_entries(srv, t->right, keys, ndx);

	sce = t->data;

	if (srv->cur_ts - sce->stat_ts > 2)
	{
		keys[(*ndx)++] = t->key;
	}

	return 0;
}

//清理文件状态缓存
int stat_cache_trigger_cleanup(server * srv)
{
	stat_cache *sc;
	size_t max_ndx = 0, i;
	int *keys;

	sc = srv->stat_cache;

	if (!sc->files)
		return 0;

	keys = calloc(1, sizeof(size_t) * sc->files->size);

	stat_cache_tag_old_entries(srv, sc->files, keys, &max_ndx);

	//删除上面的函数调用中收集到的存放在keys数组中的id
	//对应的伸展树中的节点。
	for (i = 0; i < max_ndx; i++)
	{
		int ndx = keys[i];
		splay_tree *node;

		sc->files = splaytree_splay(sc->files, ndx);

		node = sc->files;

		if (node && (node->key == ndx))
		{
#ifdef DEBUG_STAT_CACHE
			size_t j;
			int osize = splaytree_size(sc->files);
			stat_cache_entry *sce = node->data;
#endif
			stat_cache_entry_free(node->data);
			sc->files = splaytree_delete(sc->files, ndx);

#ifdef DEBUG_STAT_CACHE
			for (j = 0; j < ctrl.used; j++)
			{
				if (ctrl.ptr[j] == ndx)
				{
					ctrl.ptr[j] = ctrl.ptr[--ctrl.used];
					break;
				}
			}

			assert(osize - 1 == splaytree_size(sc->files));
#endif
		}
	}

	free(keys);

	return 0;
}
