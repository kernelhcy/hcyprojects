/**
 * bookmarks:
 *  	1535
 *
 */
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <assert.h>
#include <locale.h>

#include <stdio.h>

#include "server.h"
#include "buffer.h"
#include "network.h"
#include "log.h"
#include "keyvalue.h"
#include "response.h"
#include "request.h"
#include "chunk.h"
#include "http_chunk.h"
#include "fdevent.h"
#include "connections.h"
#include "stat_cache.h"
#include "plugin.h"
#include "joblist.h"
#include "network_backends.h"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#ifdef HAVE_VALGRIND_VALGRIND_H
#include <valgrind/valgrind.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_PWD_H
#include <grp.h>
#include <pwd.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_SYS_PRCTL_H
#include <sys/prctl.h>
#endif

#ifdef USE_OPENSSL
# include <openssl/err.h>
#endif

#ifndef __sgi
/*
 * IRIX doesn't like the alarm based time() optimization 
 */
/*
 * #define USE_ALARM 
 */
#endif
/**
 * sig_atomic_c类型有ISO C标准定义。
 * 在写这种类型的变量时不会被终端。这意味着在具有虚拟存储器的系统上这种变量不回跨越页边界。
 * 可以用一条机器指令对其进行访问。
 * 类型的定义包含在signal.h，定义为tpyedef __sig_atomic_t sig_atomic_t.在文件bits/sigsets.h中
 * 定义了__sig_atomic_t: typedef int __sig_atomic_t.
 * 这种类型的变量总是包含ISO类型修饰符volatile，其原因是该变量将由不同的控制线程访问。
 *
 * volatile类型修饰符
 * volatile用于强制某个实现屏蔽可能的优化。例如，对于具有内存映像输入/输出的机器，指向设备寄存器的
 * 指针可以声明为指向volatile的指针，目的是防止编译器通过指针删除明显多余的引用。
 * 通常，由于寄存器的访问速度要快于内存，因此为了提高运行速度，编译器会对一些变量进行优化，使得
 * 减少对内存的访问，这样就可能导致某些变量在内存中的值是过时的（与寄存器中的副本不一样）。当在多
 * 线程的环境中时，这就可能导致错误。因此，加上volatile修饰符可以保证这个变量的值是最新的，不会出现
 * 不一致的现象。（volatile本意是易失的。）
 *
 * 由于lighttpd采用的是多进程的模型，因此下面的一些变量需要被不同的进程访问，因此要保持其原子性和
 * 一致性。
 */
static volatile sig_atomic_t srv_shutdown = 0;
static volatile sig_atomic_t graceful_shutdown = 0;
static volatile sig_atomic_t handle_sig_alarm = 1;
static volatile sig_atomic_t handle_sig_hup = 0;
static volatile sig_atomic_t forwarded_sig_hup = 0; //用于标记是否发送SIGHUP信号。

#if defined(HAVE_SIGACTION) && defined(SA_SIGINFO)
static volatile siginfo_t last_sigterm_info;
static volatile siginfo_t last_sighup_info;
/**
 * 信号处理函数。用于sigaction函数。
 * @PARM sig 	 : 信号的值 
 * @PARM si  	 : 包含信号产生原因的有关信息。
 * @PARM context : 标识信号传递时的进程上下文。可以被强制转换成ucntext_t类型。
 */
static void sigaction_handler(int sig, siginfo_t * si, void *context)
{
	UNUSED(context);

	switch (sig)
	{
	case SIGTERM: 	//终止 由kill命令发送的系统默认终止信号
		srv_shutdown = 1;
		last_sigterm_info = *si;
		break;
	case SIGINT: 	//终端中断符 Ctrl-C或DELETE
		if (graceful_shutdown)
		{
			srv_shutdown = 1;
		} else
		{
			graceful_shutdown = 1;
		}
		last_sigterm_info = *si;

		break;
	case SIGALRM: 	//超时信号
		handle_sig_alarm = 1;
		break;
	case SIGHUP: 	//连接断开信号
		/** 
		 * we send the SIGHUP to all procs in the process-group
		 * this includes ourself
		 * 发送给所有的进程，包括自己。
		 * make sure we only send it once and don't create a 
		 * infinite loop
		 * 保证只发送一次，防止产生死循环。
		 */
		if (!forwarded_sig_hup)
		{
			handle_sig_hup = 1;
			last_sighup_info = *si;
		} else
		{
			forwarded_sig_hup = 0;
		}
		break;
	case SIGCHLD: 	//子进程终止或停止。
		break;
	}
}
#elif defined(HAVE_SIGNAL) || defined(HAVE_SIGACTION)
/**
 * 信号处理函数。用于signal函数。
 */
static void signal_handler(int sig)
{
	switch (sig)
	{
	case SIGTERM:
		srv_shutdown = 1;
		break;
	case SIGINT:
		if (graceful_shutdown)
			srv_shutdown = 1;
		else
			graceful_shutdown = 1;

		break;
	case SIGALRM:
		handle_sig_alarm = 1;
		break;
	case SIGHUP:
		handle_sig_hup = 1;
		break;
	case SIGCHLD:
		break;
	}
}
#endif

#ifdef HAVE_FORK
/*
 * 将程序设置成后台进程。
 */
static void daemonize(void)
{
	/*
	 * 忽略和终端读写有关的信号
	 */
#ifdef SIGTTOU
	signal(SIGTTOU, SIG_IGN);
#endif
#ifdef SIGTTIN
	signal(SIGTTIN, SIG_IGN);
#endif
#ifdef SIGTSTP
	signal(SIGTSTP, SIG_IGN);
#endif
	/* 产生子进程，父进程退出 */
	if (0 != fork())
		exit(0);

	/* 设置子进程的设置ID */
	if (-1 == setsid())
		exit(0);

	/* 忽略SIGHUP信号 */
	signal(SIGHUP, SIG_IGN);

	/* 再次产生子进程，父进程退出 */
	if (0 != fork())
		exit(0);

	/* 更改工作目录为根目录 */
	if (0 != chdir("/"))
		exit(0);
}
#endif

/**
 * 初始化服务器运行环境。
 */
static server *server_init(void)
{
	int i;

	server *srv = calloc(1, sizeof(*srv));
	assert(srv);
#define CLEAN(x) \
	srv->x = buffer_init();

	CLEAN(response_header);
	CLEAN(parse_full_path);
	CLEAN(ts_debug_str);
	CLEAN(ts_date_str);
	CLEAN(errorlog_buf);
	CLEAN(response_range);
	CLEAN(tmp_buf);
	srv->empty_string = buffer_init_string("");
	CLEAN(cond_check_buf);

	CLEAN(srvconf.errorlog_file);
	CLEAN(srvconf.groupname);
	CLEAN(srvconf.username);
	CLEAN(srvconf.changeroot);
	CLEAN(srvconf.bindhost);
	CLEAN(srvconf.event_handler);
	CLEAN(srvconf.pid_file);

	CLEAN(tmp_chunk_len);
#undef CLEAN

#define CLEAN(x) \
	srv->x = array_init();

	CLEAN(config_context);
	CLEAN(config_touched);
	CLEAN(status);
#undef CLEAN

	for (i = 0; i < FILE_CACHE_MAX; i++)
	{
		srv->mtime_cache[i].mtime = (time_t) - 1;
		srv->mtime_cache[i].str = buffer_init();
	}
	
	//设定当前时间和服务器启动时间。
	srv->cur_ts = time(NULL);
	srv->startup_ts = srv->cur_ts;

	srv->conns = calloc(1, sizeof(*srv->conns));
	assert(srv->conns);

	srv->joblist = calloc(1, sizeof(*srv->joblist));
	assert(srv->joblist);

	srv->fdwaitqueue = calloc(1, sizeof(*srv->fdwaitqueue));
	assert(srv->fdwaitqueue);

	srv->srvconf.modules = array_init();
	srv->srvconf.modules_dir = buffer_init_string(LIBRARY_DIR);
	srv->srvconf.network_backend = buffer_init();
	srv->srvconf.upload_tempdirs = array_init();

	/*
	 * use syslog 
	 * 默认使用系统的日志系统。
	 */
	srv->errorlog_fd = -1;
	srv->errorlog_mode = ERRORLOG_STDERR;

	srv->split_vals = array_init();

	return srv;
}

//清理服务器，释放资源。
static void server_free(server * srv)
{
	size_t i;

	for (i = 0; i < FILE_CACHE_MAX; i++)
	{
		buffer_free(srv->mtime_cache[i].str);
	}

#define CLEAN(x) \
	buffer_free(srv->x);

	CLEAN(response_header);
	CLEAN(parse_full_path);
	CLEAN(ts_debug_str);
	CLEAN(ts_date_str);
	CLEAN(errorlog_buf);
	CLEAN(response_range);
	CLEAN(tmp_buf);
	CLEAN(empty_string);
	CLEAN(cond_check_buf);

	CLEAN(srvconf.errorlog_file);
	CLEAN(srvconf.groupname);
	CLEAN(srvconf.username);
	CLEAN(srvconf.changeroot);
	CLEAN(srvconf.bindhost);
	CLEAN(srvconf.event_handler);
	CLEAN(srvconf.pid_file);
	CLEAN(srvconf.modules_dir);
	CLEAN(srvconf.network_backend);

	CLEAN(tmp_chunk_len);
#undef CLEAN

#if 0
	fdevent_unregister(srv->ev, srv->fd);
#endif
	fdevent_free(srv->ev);

	free(srv->conns);

	if (srv->config_storage)
	{
		for (i = 0; i < srv->config_context->used; i++)
		{
			specific_config *s = srv->config_storage[i];

			if (!s)
				continue;

			buffer_free(s->document_root);
			buffer_free(s->server_name);
			buffer_free(s->server_tag);
			buffer_free(s->ssl_pemfile);
			buffer_free(s->ssl_ca_file);
			buffer_free(s->ssl_cipher_list);
			buffer_free(s->error_handler);
			buffer_free(s->errorfile_prefix);
			array_free(s->mimetypes);
#ifdef USE_OPENSSL
			SSL_CTX_free(s->ssl_ctx);
#endif
			free(s);
		}
		free(srv->config_storage);
		srv->config_storage = NULL;
	}
#define CLEAN(x) \
	array_free(srv->x);

	CLEAN(config_context);
	CLEAN(config_touched);
	CLEAN(status);
	CLEAN(srvconf.upload_tempdirs);
#undef CLEAN

	joblist_free(srv, srv->joblist);
	fdwaitqueue_free(srv, srv->fdwaitqueue);

	if (srv->stat_cache)
	{
		stat_cache_free(srv->stat_cache);
	}

	array_free(srv->srvconf.modules);
	array_free(srv->split_vals);

#ifdef USE_OPENSSL
	if (srv->ssl_is_init)
	{
		CRYPTO_cleanup_all_ex_data();
		ERR_free_strings();
		ERR_remove_state(0);
		EVP_cleanup();
	}
#endif

	free(srv);
}

//显示服务器版本
static void show_version(void)
{
#ifdef USE_OPENSSL
# define TEXT_SSL " (ssl)"
#else
# define TEXT_SSL
#endif
	char *b = PACKAGE_NAME "-" PACKAGE_VERSION TEXT_SSL
		" - a light and fast webserver\n"
		"Build-Date: " __DATE__ " " __TIME__ "\n";
	;
#undef TEXT_SSL
	write(STDOUT_FILENO, b, strlen(b));
}

//显示特性。
static void show_features(void)
{
	const char features[] = ""
#ifdef USE_SELECT
		"\t+ select (generic)\n"
#else
		"\t- select (generic)\n"
#endif
#ifdef USE_POLL
		"\t+ poll (Unix)\n"
#else
		"\t- poll (Unix)\n"
#endif
#ifdef USE_LINUX_SIGIO
		"\t+ rt-signals (Linux 2.4+)\n"
#else
		"\t- rt-signals (Linux 2.4+)\n"
#endif
#ifdef USE_LINUX_EPOLL
		"\t+ epoll (Linux 2.6)\n"
#else
		"\t- epoll (Linux 2.6)\n"
#endif
#ifdef USE_SOLARIS_DEVPOLL
		"\t+ /dev/poll (Solaris)\n"
#else
		"\t- /dev/poll (Solaris)\n"
#endif
#ifdef USE_FREEBSD_KQUEUE
		"\t+ kqueue (FreeBSD)\n"
#else
		"\t- kqueue (FreeBSD)\n"
#endif
		"\nNetwork handler:\n\n"
#if defined(USE_LINUX_SENDFILE) || defined(USE_FREEBSD_SENDFILE) || defined(USE_SOLARIS_SENDFILEV) || defined(USE_AIX_SENDFILE)
		"\t+ sendfile\n"
#else
#ifdef USE_WRITEV
		"\t+ writev\n"
#else
		"\t+ write\n"
#endif
#ifdef USE_MMAP
		"\t+ mmap support\n"
#else
		"\t- mmap support\n"
#endif
#endif
		"\nFeatures:\n\n"
#ifdef HAVE_IPV6
		"\t+ IPv6 support\n"
#else
		"\t- IPv6 support\n"
#endif
#if defined HAVE_ZLIB_H && defined HAVE_LIBZ
		"\t+ zlib support\n"
#else
		"\t- zlib support\n"
#endif
#if defined HAVE_BZLIB_H && defined HAVE_LIBBZ2
		"\t+ bzip2 support\n"
#else
		"\t- bzip2 support\n"
#endif
#ifdef HAVE_LIBCRYPT
		"\t+ crypt support\n"
#else
		"\t- crypt support\n"
#endif
#ifdef USE_OPENSSL
		"\t+ SSL Support\n"
#else
		"\t- SSL Support\n"
#endif
#ifdef HAVE_LIBPCRE
		"\t+ PCRE support\n"
#else
		"\t- PCRE support\n"
#endif
#ifdef HAVE_MYSQL
		"\t+ mySQL support\n"
#else
		"\t- mySQL support\n"
#endif
#if defined(HAVE_LDAP_H) && defined(HAVE_LBER_H) && defined(HAVE_LIBLDAP) && defined(HAVE_LIBLBER)
		"\t+ LDAP support\n"
#else
		"\t- LDAP support\n"
#endif
#ifdef HAVE_MEMCACHE_H
		"\t+ memcached support\n"
#else
		"\t- memcached support\n"
#endif
#ifdef HAVE_FAM_H
		"\t+ FAM support\n"
#else
		"\t- FAM support\n"
#endif
#ifdef HAVE_LUA_H
		"\t+ LUA support\n"
#else
		"\t- LUA support\n"
#endif
#ifdef HAVE_LIBXML_H
		"\t+ xml support\n"
#else
		"\t- xml support\n"
#endif
#ifdef HAVE_SQLITE3_H
		"\t+ SQLite support\n"
#else
		"\t- SQLite support\n"
#endif
#ifdef HAVE_GDBM_H
		"\t+ GDBM support\n"
#else
		"\t- GDBM support\n"
#endif
		"\n";
	show_version();
	printf("\nEvent Handlers:\n\n%s", features);
}

static void show_help(void)
{
#ifdef USE_OPENSSL
# define TEXT_SSL " (ssl)"
#else
# define TEXT_SSL
#endif
	char *b =
		PACKAGE_NAME "-" PACKAGE_VERSION TEXT_SSL " (" __DATE__ " "
		__TIME__ ")" " - a light and fast webserver\n" "usage:\n"
		" -f <name>  filename of the config-file\n"
		" -m <name>  module directory (default: " LIBRARY_DIR ")\n"
		" -p         print the parsed config-file in internal form, and exit\n"
		" -t         test the config-file, and exit\n"
		" -D         don't go to background (default: go to background)\n"
		" -v         show version\n"
		" -V         show compile-time features\n"
		" -h         show this help\n" "\n";
#undef TEXT_SSL
#undef TEXT_IPV6
	write(STDOUT_FILENO, b, strlen(b));
}


/*
 ******************
 * 程序的入口点
 *****************
 */
int main(int argc, char **argv)
{
	server *srv = NULL;
	int print_config = 0;
	int test_config = 0;
	int i_am_root;
	int o;
	int num_childs = 0;
	int pid_fd = -1, fd;
	size_t i;
#ifdef HAVE_SIGACTION
	struct sigaction act;
#endif
#ifdef HAVE_GETRLIMIT
	struct rlimit rlim;
#endif

#ifdef USE_ALARM
	struct itimerval interval;

	interval.it_interval.tv_sec = 1;
	interval.it_interval.tv_usec = 0;
	interval.it_value.tv_sec = 1;
	interval.it_value.tv_usec = 0;
#endif


	/*
	 * for nice %b handling in strfime() 
	 */
	setlocale(LC_TIME, "C");

	if (NULL == (srv = server_init()))
	{
		fprintf(stderr, "did this really happen?\n");
		return -1;
	}

	/*
	 * init structs done 
	 */

	srv->srvconf.port = 0;
	//
#ifdef HAVE_GETUID
	i_am_root = (getuid() == 0);
#else
	i_am_root = 0;
#endif

	//程序将被设置为守护进程。
	srv->srvconf.dont_daemonize = 0;

	//处理参数。
	while (-1 != (o = getopt(argc, argv, "f:m:hvVDpt")))
	{
		switch (o)
		{
		case 'f':
			if (config_read(srv, optarg))
			{
				server_free(srv);
				return -1;
			}
			break;
		case 'm':
			buffer_copy_string(srv->srvconf.modules_dir, optarg);
			break;
		case 'p':
			print_config = 1;
			break;
		case 't':
			test_config = 1;
			break;
		case 'D':
			srv->srvconf.dont_daemonize = 1;
			break;
		case 'v':
			show_version();
			return 0;
		case 'V':
			show_features();
			return 0;
		case 'h':
			show_help();
			return 0;
		default:
			show_help();
			server_free(srv);
			return -1;
		}
	}

	if (!srv->config_storage)
	{
		log_error_write(srv, __FILE__, __LINE__, "s",
						"No configuration available. Try using -f option.");

		server_free(srv);
		return -1;
	}

	if (print_config)
	{
		data_unset *dc = srv->config_context->data[0];
		if (dc)
		{
			dc->print(dc, 0);
			fprintf(stdout, "\n");
		} else
		{
			/*
			 * shouldn't happend 
			 */
			fprintf(stderr, "global config not found\n");
		}
	}

	if (test_config) //没有进行任何测试。。。
	{
		printf("Syntax OK\n");
	}

	if (test_config || print_config)
	{
		server_free(srv);
		return 0;
	}

	/*
	 * close stdin and stdout, as they are not needed 
	 * 关闭标准输入和标准输出。
	 */
	openDevNull(STDIN_FILENO);
	openDevNull(STDOUT_FILENO);

	//设置为默认的配置。
	if (0 != config_set_defaults(srv))
	{
		log_error_write(srv, __FILE__, __LINE__, "s",
						"setting default values failed");
		server_free(srv);
		return -1;
	}

	/*
	 * UID handling 
	 */
#ifdef HAVE_GETUID
	//检查有效用户ID和有效组ID是否是0（root）。
	if (!i_am_root && (geteuid() == 0 || getegid() == 0))
	{
		/*
		 * we are setuid-root 
		 * 程序的实际用户ID不是0,也就是程序不是由超级用户运行的，但是程序的有效用户ID
		 * 或者有效组ID是超级用户（组），因此，程序可以访问任何文件而不受限制！这样很
		 * 不安全。因此程序退出并提示用户。
		 */

		log_error_write(srv, __FILE__, __LINE__, "s",
						"Are you nuts ? Don't apply a SUID bit to this binary");

		server_free(srv);
		return -1;
	}
#endif

	/*
	 * check document-root 
	 */
	if (srv->config_storage[0]->document_root->used <= 1)
	{
		log_error_write(srv, __FILE__, __LINE__, "s",
						"document-root is not set\n");

		server_free(srv);

		return -1;
	}

	//加载插件
	//以后再说。。。
	if (plugins_load(srv))
	{
		log_error_write(srv, __FILE__, __LINE__, "s",
						"loading plugins finally failed");

		plugins_free(srv);
		server_free(srv);

		return -1;
	}

	/*
	 * open pid file BEFORE chroot 
	 * 打开pid文件，并将进程号写入pid文件。
	 */
	if (srv->srvconf.pid_file->used)
	{
		if (-1 ==
			(pid_fd =
			 open(srv->srvconf.pid_file->ptr,
				  O_WRONLY | O_CREAT | O_EXCL | O_TRUNC,
				  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)))
			/**
			 * O_EXCL和O_CREAT同时使用，测试文件是否存在，如果存在
			 * 则报错。
			 */
		{
			//pid文件打开失败。。。
			struct stat st;
			if (errno != EEXIST)  //不是报文件已经存在的错误。
			{
				log_error_write(srv, __FILE__, __LINE__, "sbs",
								"opening pid-file failed:",
								srv->srvconf.pid_file, strerror(errno));
				return -1;
			}

			//pid文件已经存在，测试文件的状态。
			if (0 != stat(srv->srvconf.pid_file->ptr, &st))
			{
				log_error_write(srv, __FILE__, __LINE__, "sbs",
								"stating existing pid-file failed:",
								srv->srvconf.pid_file, strerror(errno));
			}

			if (!S_ISREG(st.st_mode)) //pid文件是普通文件。
			{
				log_error_write(srv, __FILE__, __LINE__, "sb",
								"pid-file exists and isn't regular file:",
								srv->srvconf.pid_file);
				return -1;
			}

			//重新打开pid文件。
			//这里不在使用O_EXCL参数，由于pid文件已经存在且是普通文件，则覆盖原先的文件。
			if (-1 ==
				(pid_fd =
				 open(srv->srvconf.pid_file->ptr,
					  O_WRONLY | O_CREAT | O_TRUNC,
					  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)))
			{
				log_error_write(srv, __FILE__, __LINE__, "sbs",
								"opening pid-file failed:",
								srv->srvconf.pid_file, strerror(errno));
				return -1;
			}
		}
	}

	if (srv->event_handler == FDEVENT_HANDLER_SELECT)
	{
		/*
		 * select limits itself as it is a hard limit and will lead to a segfault 
		 * we add some safety 
		 * select的硬限制。减去200是为了增加安全性，防止出现段错误。
		 */
		srv->max_fds = FD_SETSIZE - 200;
	} 
	else
	{
		srv->max_fds = 4096;
	}

	//程序是在超级用户模式下运行的。
	if (i_am_root)
	{
		struct group *grp = NULL;
		struct passwd *pwd = NULL;
		int use_rlimit = 1;

#ifdef HAVE_VALGRIND_VALGRIND_H
		if (RUNNING_ON_VALGRIND)
			use_rlimit = 0;
#endif

#ifdef HAVE_GETRLIMIT
		/**
		 * getrlimit和setrlimit函数用于查询和修改进程的资源限制。
		 *
		 * include <sys/resource.h>
		 * int getrlimit(int resource, struct rlimit *rlim);
		 * int setrlimit(int resource, const struct rlimit *rlim);
		 * 	返回：若成功为0，出错为非0
		 *
		 * 对这两个函数的每一次调用都指定一个资源以及一个指向下列结构的指针。
		 *
		 * struct rlimit
		 * {
		 * 		rlim_t rlim_cur; 	//软限制：当前限制
		 * 		rlim_t rlim_max; 	//硬限制：rlimcur的最大值
		 * }；
		 *
		 * 这两个函数不属于POSIX.1，但SVR4和4.3+BSD提供它们。SVR4在上面的结构中使用基本系统数据类型rlim_t。
		 * 其它系统则将这两个成员定义为整型或长整型。
		 *
		 * 程序中使用的参数RLIMIT_NOFILE：Specifies a value one greater than the maximum  file  descriptor
		 * number  that  can be opened by this process. 设置最大的文件打开数，且实际打开的文件数要比这个数
		 * 小一。
		 *
		 * 详细使用：man getrlimit
		 */
		if (0 != getrlimit(RLIMIT_NOFILE, &rlim)) //获得当前的文件打开数限制。
		{
			log_error_write(srv, __FILE__, __LINE__,
							"ss", "couldn't get 'max filedescriptors'", strerror(errno));
			return -1;
		}

		if (use_rlimit && srv->srvconf.max_fds)
		{
			/*
			 * set rlimits. 设置限制。
			 */
			rlim.rlim_cur = srv->srvconf.max_fds; //软限制。
			rlim.rlim_max = srv->srvconf.max_fds; //硬限制。

			if (0 != setrlimit(RLIMIT_NOFILE, &rlim))
			{
				log_error_write(srv, __FILE__, __LINE__,
								"ss", "couldn't set 'max filedescriptors'", strerror(errno));
				return -1;
			}
		}

		//根据实际设置情况，重新设置max_fds。
		if (srv->event_handler == FDEVENT_HANDLER_SELECT)
		{
			srv->max_fds = rlim.rlim_cur < FD_SETSIZE - 200 ? rlim.rlim_cur : FD_SETSIZE - 200;
		} 
		else
		{
			srv->max_fds = rlim.rlim_cur;
		}

		/*
		 * set core file rlimit, if enable_cores is set 
		 * 设置core文件的限制。如果设置了enable_cores。
		 */
		if (use_rlimit && srv->srvconf.enable_cores
			&& getrlimit(RLIMIT_CORE, &rlim) == 0)
		{
			rlim.rlim_cur = rlim.rlim_max;
			setrlimit(RLIMIT_CORE, &rlim);
		}
#endif
		if (srv->event_handler == FDEVENT_HANDLER_SELECT)
		{
			/*
			 * don't raise the limit above FD_SET_SIZE 
			 */
			if (srv->max_fds > FD_SETSIZE - 200)
			{
				log_error_write(srv, __FILE__, __LINE__, "sd",
								"can't raise max filedescriptors above",
								FD_SETSIZE - 200,
								"if event-handler is 'select'. Use 'poll' or something else or reduce server.max-fds.");
				return -1;
			}
		}
#ifdef HAVE_PWD_H
		/*
		 * set user and group 设置用户和组。
		 */
		if (srv->srvconf.username->used)
		{
			//根据配置中的用户名获取用户信息。
			if (NULL == (pwd = getpwnam(srv->srvconf.username->ptr)))
			{
				log_error_write(srv, __FILE__, __LINE__, "sb", "can't find username", srv->srvconf.username);
				return -1;
			}

			if (pwd->pw_uid == 0) 
			{
				log_error_write(srv, __FILE__, __LINE__, "s", "I will not set uid to 0\n");
				return -1;
			}
		}

		if (srv->srvconf.groupname->used)
		{
			//根据上面得到的用户所在的组的组名，获得组的信息。
			if (NULL == (grp = getgrnam(srv->srvconf.groupname->ptr)))
			{
				log_error_write(srv, __FILE__, __LINE__, "sb", "can't find groupname", srv->srvconf.groupname);
				return -1;
			}
			if (grp->gr_gid == 0)
			{
				log_error_write(srv, __FILE__, __LINE__, "s", "I will not set gid to 0\n");
				return -1;
			}
		}
#endif
		/*
		 * we need root-perms for port < 1024 
		 * 使用超级用户模式获得小于1024的端口。初始化网络。
		 */
		if (0 != network_init(srv))
		{
			plugins_free(srv);
			server_free(srv);

			return -1;
		}
#ifdef HAVE_PWD_H
		/*
		 * Change group before chroot, when we have access
		 * to /etc/group
		 * */
		if (srv->srvconf.groupname->used)
		{
			setgid(grp->gr_gid);
			setgroups(0, NULL); //返回用户组的数目。
			if (srv->srvconf.username->used)
			{
				//Initialize the group access list by reading the group database /etc/group and using all groups of which
				//user is a member.  The additional group group is also added to the list.
				initgroups(srv->srvconf.username->ptr, grp->gr_gid);
			}
		}
#endif
#ifdef HAVE_CHROOT
		if (srv->srvconf.changeroot->used)
		{
			//The tzset() function initializes the tzname variable from the TZ environment variable.  
			//This function is automatically called by the other time conversion functions that depend on the time zone.  
			//In a SysV-like environment it will also set the variables  time-zone  (seconds  West  of GMT) and daylight 
			//(0 if this time zone does not have any daylight saving time rules, nonzero if there is a
			//time during the year when daylight saving time applies).
			tzset();

			//设置程序所参考的根目录，将被所有的子进程继承。
			//也就是对于本程序而言，"/"并不是系统的根目录，而是这设置的目录。
			if (-1 == chroot(srv->srvconf.changeroot->ptr))
			{
				log_error_write(srv, __FILE__, __LINE__, "ss", "chroot failed: ", strerror(errno));
				return -1;
			}
			//修改工作目录.
			/*
			 * 注意：
			 * 		由于前面已经设置了根目录。因此这里将工作目录切换到"/"并不是系统的根目录，而是
			 * 		上面通过函数chroot设置的根目录。
			 */
			if (-1 == chdir("/"))
			{
				log_error_write(srv, __FILE__, __LINE__, "ss", "chdir failed: ", strerror(errno));
				return -1;
			}
		}
#endif
#ifdef HAVE_PWD_H
		/*
		 * drop root privs 放弃超级管理员权限。
		 */
		if (srv->srvconf.username->used)
		{
			setuid(pwd->pw_uid);
		}
#endif
#if defined(HAVE_SYS_PRCTL_H) && defined(PR_SET_DUMPABLE)
		/**
		 * on IRIX 6.5.30 they have prctl() but no DUMPABLE
		 */
		if (srv->srvconf.enable_cores)
		{
			prctl(PR_SET_DUMPABLE, 1, 0, 0, 0);
		}
#endif
	} 
	/*
	 * 下面的是程序在非root用户下执行的设置。
	 */
	else
	{

#ifdef HAVE_GETRLIMIT
		if (0 != getrlimit(RLIMIT_NOFILE, &rlim))
		{
			log_error_write(srv, __FILE__, __LINE__,
							"ss", "couldn't get 'max filedescriptors'",
							strerror(errno));
			return -1;
		}

		/**
		 * we are not root can can't increase the fd-limit, but we can reduce it
		 * 我们不是root，不能增加fd-limit，但我们可以减少。
		 */
		if (srv->srvconf.max_fds && srv->srvconf.max_fds < rlim.rlim_cur)
		{
			/*
			 * set rlimits 
			 */

			rlim.rlim_cur = srv->srvconf.max_fds; //只能设置软限制。

			if (0 != setrlimit(RLIMIT_NOFILE, &rlim))
			{
				log_error_write(srv, __FILE__, __LINE__,
								"ss", "couldn't set 'max filedescriptors'",
								strerror(errno));
				return -1;
			}
		}

		if (srv->event_handler == FDEVENT_HANDLER_SELECT)
		{
			srv->max_fds = rlim.rlim_cur < FD_SETSIZE - 200 ? rlim.rlim_cur : FD_SETSIZE - 200;
		} 
		else
		{
			srv->max_fds = rlim.rlim_cur;
		}

		/*
		 * set core file rlimit, if enable_cores is set 
		 */
		if (srv->srvconf.enable_cores && getrlimit(RLIMIT_CORE, &rlim) == 0)
		{
			rlim.rlim_cur = rlim.rlim_max;
			setrlimit(RLIMIT_CORE, &rlim);
		}
#endif
		if (srv->event_handler == FDEVENT_HANDLER_SELECT)
		{
			/*
			 * don't raise the limit above FD_SET_SIZE 
			 */
			if (srv->max_fds > FD_SETSIZE - 200)
			{
				log_error_write(srv, __FILE__, __LINE__, "sd",
								"can't raise max filedescriptors above", FD_SETSIZE - 200,
								"if event-handler is 'select'. Use 'poll' or something else or reduce server.max-fds.");
				return -1;
			}
		}

		if (0 != network_init(srv))
		{
			plugins_free(srv);
			server_free(srv);

			return -1;
		}
	}

	/*
	 * set max-conns 设置最大连接数。
	 */
	if (srv->srvconf.max_conns > srv->max_fds)
	{
		/*
		 * we can't have more connections than max-fds 
		 * 最大连接数要小于最大文件打开数(max-fds)
		 */
		srv->max_conns = srv->max_fds;
	}  
	else if (srv->srvconf.max_conns)
	{
		/*
		 * otherwise respect the wishes of the user 
		 * 根据用户设置。
		 */
		srv->max_conns = srv->srvconf.max_conns;
	} 
	else
	{
		/*
		 * or use the default  默认。
		 */
		srv->max_conns = srv->max_fds;
	}

	if (HANDLER_GO_ON != plugins_call_init(srv)) //初始化插件。
	{
		log_error_write(srv, __FILE__, __LINE__, "s",
						"Initialization of plugins failed. Going down.");

		plugins_free(srv);
		network_close(srv);
		server_free(srv);

		return -1;
	}
#ifdef HAVE_FORK
	/*
	 * network is up, let's deamonize ourself 
	 * 设置为守护进程。
	 */
	if (srv->srvconf.dont_daemonize == 0)
		daemonize();
#endif

	srv->gid = getgid();
	srv->uid = getuid();

	/*
	 * write pid file 写pid文件。
	 */
	if (pid_fd != -1)
	{
		buffer_copy_long(srv->tmp_buf, getpid());
		buffer_append_string_len(srv->tmp_buf, CONST_STR_LEN("\n"));
		write(pid_fd, srv->tmp_buf->ptr, srv->tmp_buf->used - 1);
		close(pid_fd);
		pid_fd = -1;
	}

	/*
	 * Close stderr ASAP in the child process to make sure that nothing is
	 * being written to that fd which may not be valid anymore. 
	 * 关闭向标准输出的输出，打开日志文件。
	 */
	if (-1 == log_error_open(srv))
	{
		log_error_write(srv, __FILE__, __LINE__, "s",
						"Opening errorlog failed. Going down.");

		plugins_free(srv);
		network_close(srv);
		server_free(srv);
		return -1;
	}

	if (HANDLER_GO_ON != plugins_call_set_defaults(srv))
	{
		log_error_write(srv, __FILE__, __LINE__, "s",
						"Configuration of plugins failed. Going down.");

		plugins_free(srv);
		network_close(srv);
		server_free(srv);

		return -1;
	}

	/*
	 * dump unused config-keys 
	 */
	for (i = 0; i < srv->config_context->used; i++)
	{
		array *config = ((data_config *) srv->config_context->data[i])->value;
		size_t j;

		for (j = 0; config && j < config->used; j++)
		{
			data_unset *du = config->data[j];

			/*
			 * all var.* is known as user defined variable 
			 */
			if (strncmp(du->key->ptr, "var.", sizeof("var.") - 1) == 0)
			{
				continue;
			}

			if (NULL == array_get_element(srv->config_touched, du->key->ptr))
			{
				log_error_write(srv, __FILE__, __LINE__, "sbs",
								"WARNING: unknown config-key:", du->key,
								"(ignored)");
			}
		}
	}

	if (srv->config_unsupported)
	{
		log_error_write(srv, __FILE__, __LINE__, "s",
						"Configuration contains unsupported keys. Going down.");
	}

	if (srv->config_deprecated)
	{
		log_error_write(srv, __FILE__, __LINE__, "s",
						"Configuration contains deprecated keys. Going down.");
	}

	if (srv->config_unsupported || srv->config_deprecated)
	{
		plugins_free(srv);
		network_close(srv);
		server_free(srv);

		return -1;
	}

//设置一些信号的处理方法。
//SIGPIPE:在写管道时，读管道的进程终止，产生此信号。
#ifdef HAVE_SIGACTION
	memset(&act, 0, sizeof(act));
	act.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &act, NULL);
	sigaction(SIGUSR1, &act, NULL);
# if defined(SA_SIGINFO)
	act.sa_sigaction = sigaction_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = SA_SIGINFO;
# else
	act.sa_handler = signal_handler;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
# endif
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	sigaction(SIGHUP, &act, NULL);
	sigaction(SIGALRM, &act, NULL);
	sigaction(SIGCHLD, &act, NULL);

#elif defined(HAVE_SIGNAL)
	/*
	 * ignore the SIGPIPE from sendfile() 
	 */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGUSR1, SIG_IGN);
	signal(SIGALRM, signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGHUP, signal_handler);
	signal(SIGCHLD, signal_handler);
	signal(SIGINT, signal_handler);
#endif

#ifdef USE_ALARM
	signal(SIGALRM, signal_handler);

	/*
	 * setup periodic timer (1 second) 
	 *  The  system provides each process with three interval timers, each decrementing in a distinct time domain.  
	 *  When any timer expires a signal is sent to the process, and the timer (potentially) restarts.
	 *
	 *     ITIMER_REAL    decrements in real time, and delivers SIGALRM upon expiration.
	 *     ITIMER_VIRTUAL decrements only when the process is executing, and delivers SIGVTALRM upon expiration.
	 *     ITIMER_PROF    decrements both when the process executes and when the system is executing on behalf of the process.   
	 *     				  Coupled with ITIMER_VIRTUAL,  this  timer  is usually used to profile the time spent 
	 *     				  by the application in user and kernel space.
	 *  SIGPROF is delivered upon expiration.
	 *  Timer values are defined by the following structures:
	 *  struct itimerval 
	 *  {
	 *    	struct timeval it_interval; //next value
	 *    	struct timeval it_value;    //current value
	 *  };
	 *  struct timeval 
	 *  {
	 *  	long tv_sec;                // seconds 
	 *  	long tv_usec;               //microseconds
	 *  };
	 *  The function getitimer() fills the structure indicated by value with the current setting for the timer 
	 *  indicated by which  (one  of ITIMER_REAL, ITIMER_VIRTUAL, or ITIMER_PROF).  The element it_value is 
	 *  set to the amount of time remaining on the timer, or zero ifthe timer is disabled.  
	 *  Similarly, it_interval is set to the reset value.  The function setitimer() sets the indicated timer to the
	 *  value in value.  If ovalue is nonzero, the old value of the timer is stored there.
	 */
	if (setitimer(ITIMER_REAL, &interval, NULL))
	{
		log_error_write(srv, __FILE__, __LINE__, "s", "setting timer failed");
		return -1;
	}

	getitimer(ITIMER_REAL, &interval);
#endif

#ifdef HAVE_FORK
	/*
	 * *************************
	 * start watcher and workers
	 * *************************
	 *
	 * 下面程序将产生多个子进程。这些子进程成为worker，也就是用于接受处理用户的连接的进程。而当前的主进程将
	 * 成为watcher，主要工作就是监视workers的工作状态，当有worker因为意外而退出时，产生新的worker。
	 * 在程序退出时，watcher负责停止所有的workers并清理资源。
	 */
	num_childs = srv->srvconf.max_worker;//最大worker数。
	if (num_childs > 0)
	{
		int child = 0;
		while (!child && !srv_shutdown && !graceful_shutdown)
		{
			if (num_childs > 0) //继续产生worker
			{
				switch (fork())
				{
				case -1:
					return -1;
				case 0:
					child = 1;
					break;
				default:
					num_childs--;
					break;
				}
			} 
			else 		//watcher
			{
				/**
				 * 当产生了足够的worker时，watcher就在这个while中不断的循环。
				 * 一但发现有worker退出（进程死亡），立即产生新的worker。
				 * 如果发生错误并接受到SIGHUP信号，向所有的进程（父进程及其子进程）包括自己发送SIGHUP信号。
				 * 并退出。
				 */
				int status;

				if (-1 != wait(&status))
				{
					/** 
					 * one of our workers went away 
					 */
					num_childs++;
				} 
				else
				{
					switch (errno)
					{
					case EINTR:
						/**
						 * if we receive a SIGHUP we have to close our logs ourself as we don't 
						 * have the mainloop who can help us here
						 */
						if (handle_sig_hup)
						{
							handle_sig_hup = 0;

							log_error_cycle(srv);

							/**
							 * forward to all procs in the process-group
							 * 向所有进程发送SIGHUP信号。（父进程及其子进程）
							 * we also send it ourself
							 */
							if (!forwarded_sig_hup)
							{
								forwarded_sig_hup = 1;
								kill(0, SIGHUP);
							}
						}
						break;
					default:
						break;
					}
				}
			}
		}

		/**
		 * for the parent this is the exit-point 
		 * *****************************************************
		 * 父进程，也就是watcher在执行完这个if语句中就直接退出了。
		 * 后面是worker执行的代码。
		 * *****************************************************
		 */
		if (!child)
		{
			/** 
			 * kill all children too 。杀死所有的子进程。
			 */
			if (graceful_shutdown)
			{
				kill(0, SIGINT);
			} 
			else if (srv_shutdown)
			{
				kill(0, SIGTERM);
			}

			log_error_close(srv);
			network_close(srv);
			connections_free(srv);
			plugins_free(srv);
			server_free(srv);
			return 0;
		}
	}
#endif

	/* 
	 * **************************
	 * 从这开始是worker执行的代码。
	 * **************************
	 */

	
	if (NULL == (srv->ev = fdevent_init(srv->max_fds + 1, srv->event_handler)))
	{
		log_error_write(srv, __FILE__, __LINE__, "s", "fdevent_init failed");
		return -1;
	}
	/*
	 * kqueue() is called here, select resets its internals,
	 * all server sockets get their handlers
	 * 以后再说。。。
	 * */
	if (0 != network_register_fdevents(srv))
	{
		plugins_free(srv);
		network_close(srv);
		server_free(srv);

		return -1;
	}

	/*
	 * might fail if user is using fam (not gamin) and famd isn't running 
	 * famd没有运行，则运行失败。。。
	 */
	if (NULL == (srv->stat_cache = stat_cache_init()))
	{
		log_error_write(srv, __FILE__, __LINE__, "s",
						"stat-cache could not be setup, dieing.");
		return -1;
	}
#ifdef HAVE_FAM_H
	/*
	 * setup FAM 设置FAM。
	 */
	if (srv->srvconf.stat_cache_engine == STAT_CACHE_ENGINE_FAM)
	{
		if (0 != FAMOpen2(srv->stat_cache->fam, "lighttpd"))
		{
			log_error_write(srv, __FILE__, __LINE__, "s",
							"could not open a fam connection, dieing.");
			return -1;
		}
#ifdef HAVE_FAMNOEXISTS
		FAMNoExists(srv->stat_cache->fam);
#endif

		srv->stat_cache->fam_fcce_ndx = -1;
		fdevent_register(srv->ev,
						 FAMCONNECTION_GETFD(srv->stat_cache->fam),
						 stat_cache_handle_fdevent, NULL);
		fdevent_event_add(srv->ev, &(srv->stat_cache->fam_fcce_ndx),
						  FAMCONNECTION_GETFD(srv->stat_cache->fam),
						  FDEVENT_IN);
	}
#endif


	/*
	 * get the current number of FDs 获得当前可用的fd值
	 */
	srv->cur_fds = open("/dev/null", O_RDONLY);
	close(srv->cur_fds);

	for (i = 0; i < srv->srv_sockets.used; i++)
	{
		server_socket *srv_socket = srv->srv_sockets.ptr[i];
	 	/*
		 * close fd on exec (cgi) 
		 */
		if (-1 == fdevent_fcntl_set(srv->ev, srv_socket->fd))
		{
			log_error_write(srv, __FILE__, __LINE__, "ss", "fcntl failed:", strerror(errno));
			return -1;
		}
	}

	/*
	 * main-loop 
	 * *******************
	 * worker工作的主循环。
	 * *******************
	 */
	while (!srv_shutdown)
	{
		int n;
		size_t ndx;
		time_t min_ts;

		/**
		 * 收到SIGHUP信号。主要是重新开始日志的周期并提示插件。
		 * 这个信号表示连接已经断开，通常是做一些清理和准备工作，等待下一次的连接。
		 */
		if (handle_sig_hup)
		{
			handler_t r;

			/*
			 * reset notification 重置
			 */
			handle_sig_hup = 0;

			/*
			 * cycle logfiles 
			 * 重新开始新一轮日志。
			 * 这里使用了switch而不是if语句，有意思。。。
			 * 调用插件关于SIGHUP信号的处理函数。
			 */

			switch (r = plugins_call_handle_sighup(srv))
			{
				case HANDLER_GO_ON:
					break;
				default:
					log_error_write(srv, __FILE__, __LINE__, "sd", "sighup-handler return with an error", r);
					break;
			}

			if (-1 == log_error_cycle(srv))
			{
				log_error_write(srv, __FILE__, __LINE__, "s", "cycling errorlog failed, dying");

				return -1;
			} 
			else
			{
#ifdef HAVE_SIGACTION
				log_error_write(srv, __FILE__, __LINE__, "sdsd", "logfiles cycled UID =",
								last_sighup_info.si_uid, "PID =", last_sighup_info.si_pid);
#else
				log_error_write(srv, __FILE__, __LINE__, "s", "logfiles cycled");
#endif
			}
		}

		/**
		 * alarm函数发出的信号，表示一秒钟已经过去了。
		 */
		if (handle_sig_alarm)
		{
			/*
			 * a new second  新的一秒开始了。。。
			 */

#ifdef USE_ALARM
			/*
			 * reset notification 重置
			 */
			handle_sig_alarm = 0;
#endif

			/*
			 * get current time  当前时间。精确到一秒
			 */
			min_ts = time(NULL);

			/**
			 * 这里判断和服务器记录的当前时间是否相同。
			 * 相同，则表示服务器还在这一秒中，继续处理请求等。
			 * 如果不相同，则进入了一个新的周期（当然周期是一秒）。这就要做一些触发和检查以及清理的动作。
			 * 如插件的触发连接的超时清理状态缓存等。
			 * 其中，最主要的工作是检查连接的超时。
			 */
			if (min_ts != srv->cur_ts)
			{
				int cs = 0;
				connections *conns = srv->conns;
				handler_t r;

				switch (r = plugins_call_handle_trigger(srv))
				{
					case HANDLER_GO_ON:
						break;
					case HANDLER_ERROR:
						log_error_write(srv, __FILE__, __LINE__, "s", "one of the triggers failed");
						break;
					default:
						log_error_write(srv, __FILE__, __LINE__, "d", r);
						break;
				}

				/*
				 * trigger waitpid 么意思？？
				 */
				srv->cur_ts = min_ts;

				/*
				 * cleanup stat-cache 清理状态缓存。每秒钟清理一次。
				 */
				stat_cache_trigger_cleanup(srv);

				/**
				 * check all connections for timeouts 检查所有的连接是否超时。
				 */
				for (ndx = 0; ndx < conns->used; ndx++)
				{
					int changed = 0;
					connection *con;
					int t_diff;

					con = conns->ptr[ndx];

					//连接的状态是在读
					if (con->state == CON_STATE_READ || con->state == CON_STATE_READ_POST)
					{
						if (con->request_count == 1) //连接正在处理一个请求
						{
							if (srv->cur_ts - con->read_idle_ts > con->conf.max_read_idle)
							{
								/*
								 * time - out 
								 */
#if 0
								log_error_write(srv, __FILE__, __LINE__, "sd",
												"connection closed - read-timeout:", con->fd);
#endif
								connection_set_state(srv, con, CON_STATE_ERROR);
								changed = 1;
							}
						}  //这个连接同时处理多个请求
						else
						{
							if (srv->cur_ts - con->read_idle_ts > con->conf.max_keep_alive_idle)
							{
								/*
								 * time - out 
								 */
#if 0
								log_error_write(srv, __FILE__, __LINE__, "sd",
												"connection closed - read-timeout:", con->fd);
#endif
								connection_set_state(srv, con, CON_STATE_ERROR);
								changed = 1;
							}
						}
					}

					//连接的状态是写
					if ((con->state == CON_STATE_WRITE) && (con->write_request_ts != 0))
					{
#if 0
						if (srv->cur_ts - con->write_request_ts > 60)
						{
							log_error_write(srv, __FILE__, __LINE__, "sdd",
											"connection closed - pre-write-request-timeout:",
											con->fd, srv->cur_ts - con->write_request_ts);
						}
#endif

						if (srv->cur_ts - con->write_request_ts > con->conf.max_write_idle)
						{
							/*
							 * time - out 
							 */
#if 1
							log_error_write(srv, __FILE__, __LINE__, "sbsosds", "NOTE: a request for",
											con->request.uri, "timed out after writing", con->bytes_written, "bytes. We waited",
											(int) con->conf. max_write_idle,
											"seconds. If this a problem increase server.max-write-idle");
#endif
							connection_set_state(srv, con, CON_STATE_ERROR);
							changed = 1;
						}
					}

					/*
					 * we don't like div by zero 防止除0。。。
					 */
					if (0 == (t_diff = srv->cur_ts - con->connection_start))
						t_diff = 1;

					/**
					 * 
					 */
					if (con->traffic_limit_reached &&
						(con->conf.kbytes_per_second == 0 || ((con->bytes_written / t_diff) < con->conf.kbytes_per_second * 1024)))
					{
						/*
						 * enable connection again 
						 */
						con->traffic_limit_reached = 0;
						changed = 1;
					}

					if (changed)
					{
						connection_state_machine(srv, con);
					}
					con->bytes_written_cur_second = 0;
					*(con->conf.global_bytes_per_second_cnt_ptr) = 0;

#if 0
					if (cs == 0)
					{
						fprintf(stderr, "connection-state: ");
						cs = 1;
					}
					fprintf(stderr, "c[%d,%d]: %s ", con->fd, con->fcgi.fd, connection_get_state(con->state));
#endif
				}

				if (cs == 1)
					fprintf(stderr, "\n");
			}
		}//end of if (handle_sig_alarm)...

		if (srv->sockets_disabled)
		{
			/*
			 * our server sockets are disabled, why ? 
			 * 服务器socket连接失效。为什么捏？？？后面的服务器过载处理中。。。
			 *
			 * 将所有连接重新加入的fdevent中。
			 */

			if ((srv->cur_fds + srv->want_fds < srv->max_fds * 0.8) &&	/* we have enough unused fds */
				(srv->conns->used < srv->max_conns * 0.9) && (0 == graceful_shutdown))
			{
				for (i = 0; i < srv->srv_sockets.used; i++)
				{
					server_socket *srv_socket = srv->srv_sockets.ptr[i];
					fdevent_event_add(srv->ev, &(srv_socket->fde_ndx), srv_socket->fd, FDEVENT_IN);
				}
				log_error_write(srv, __FILE__, __LINE__, "s", "[note] sockets enabled again");
				srv->sockets_disabled = 0;
			}
		} 
		else
		{
			/*
			 * 下面处理服务器过载的情况。
			 */
			if ((srv->cur_fds + srv->want_fds > srv->max_fds * 0.9) ||	/* out of fds */
				(srv->conns->used > srv->max_conns) ||	/* out of connections */
				(graceful_shutdown)) /* graceful_shutdown */
			{
				/*
				 * disable server-fds 关闭所有的服务socket
				 */
				for (i = 0; i < srv->srv_sockets.used; i++)
				{
					server_socket *srv_socket = srv->srv_sockets.ptr[i];
					fdevent_event_del(srv->ev, &(srv_socket->fde_ndx), srv_socket->fd);

					if (graceful_shutdown)
					{
						/*
						 * we don't want this socket anymore, closing it right 
						 * away will make it possible for the next lighttpd to
						 * take over (graceful restart) 
						 */

						fdevent_unregister(srv->ev, srv_socket->fd);
						close(srv_socket->fd);
						srv_socket->fd = -1;

						/*
						 * network_close() will cleanup after us 
						 */

						if (srv->srvconf.pid_file->used && srv->srvconf.changeroot->used == 0)
						{
							if (0 != unlink(srv->srvconf.pid_file->ptr))
							{
								if (errno != EACCES && errno != EPERM)
								{
									log_error_write(srv, __FILE__, __LINE__, "sbds", "unlink failed for:",
													srv -> srvconf.pid_file, errno, strerror(errno));
								}
							}
						}
					}
				}//end of for(i = 0; ...

				if (graceful_shutdown)
				{
					log_error_write(srv, __FILE__, __LINE__, "s", "[note] graceful shutdown started");
				} 
				else if (srv->conns->used > srv->max_conns)
				{
					log_error_write(srv, __FILE__, __LINE__, "s","[note] sockets disabled, connection limit reached");
				} 
				else
				{
					log_error_write(srv, __FILE__, __LINE__, "s", "[note] sockets disabled, out-of-fds");
				}

				srv->sockets_disabled = 1; //服务器过载了，socket失效。
			}
		}

		if (graceful_shutdown && srv->conns->used == 0)
		{
			/*
			 * we are in graceful shutdown phase and all connections are closed
			 * we are ready to terminate without harming anyone 
			 */
			srv_shutdown = 1;
		}

		/*
		 * we still have some fds to share 
		 */
		if (srv->want_fds)
		{
			/*
			 * check the fdwaitqueue for waiting fds 
			 */
			int free_fds = srv->max_fds - srv->cur_fds - 16;
			connection *con;

			for (; free_fds > 0 && NULL != (con = fdwaitqueue_unshift(srv, srv->fdwaitqueue)); free_fds--)
			{
				connection_state_machine(srv, con);
				srv->want_fds--;
			}
		}

		/**
		 **********************************************************
		 * 至此，上面那些杂七杂八的事全部处理结束。下面，干正事！！
		 * 也就是处理服务请求。
		 **********************************************************
		 */

		//启动事件轮询。底层使用的是IO多路转接。
		if ((n = fdevent_poll(srv->ev, 1000)) > 0)
		{
			/*
			 * n is the number of events n是事件的数量（服务请求啦，文件读写啦什么的。。。）
			 */
			int revents;
			int fd_ndx;
#if 0
			if (n > 0)
			{
				log_error_write(srv, __FILE__, __LINE__, "sd", "polls:", n);
			}
#endif
			fd_ndx = -1;
			/**
			 * 这个循环中逐个的处理已经准备好的请求，知道所有的请求处理结束。
			 */
			do
			{
				fdevent_handler handler;
				void *context;
				handler_t r;

				fd_ndx = fdevent_event_next_fdndx(srv->ev, fd_ndx);
				revents = fdevent_event_get_revent(srv->ev, fd_ndx);
				fd = fdevent_event_get_fd(srv->ev, fd_ndx);
				handler = fdevent_get_handler(srv->ev, fd);
				context = fdevent_get_context(srv->ev, fd);

				/*
				 * connection_handle_fdevent needs a joblist_append 
				 */
#if 0
				log_error_write(srv, __FILE__, __LINE__, "sdd",
								"event for", fd, revents);
#endif
				/**
				 * 这里，调用请求的处理函数handler处理请求！
				 * 这才是重点中的重点！！
				 */
				switch (r = (*handler) (srv, context, revents))
				{
				case HANDLER_FINISHED:
				case HANDLER_GO_ON:
				case HANDLER_WAIT_FOR_EVENT:
				case HANDLER_WAIT_FOR_FD:
					break;
				case HANDLER_ERROR:
					/*
					 * should never happen 
					 */
					SEGFAULT();
					break;
				default:
					log_error_write(srv, __FILE__, __LINE__, "d", r);
					break;
				}
			}while (--n > 0);

			//到这里，本次的请求都处理结束了。。。累啊！
		} 
		else if (n < 0 && errno != EINTR)
		{
			log_error_write(srv, __FILE__, __LINE__, "ss", "fdevent_poll failed:", strerror(errno));
		}

		//这里折腾一下作业列表。
		//检查作业列表 
		for (ndx = 0; ndx < srv->joblist->used; ndx++)
		{
			connection *con = srv->joblist->ptr[ndx];
			handler_t r;

			connection_state_machine(srv, con);

			switch (r = plugins_call_handle_joblist(srv, con))
			{
			case HANDLER_FINISHED:
			case HANDLER_GO_ON:
				break;
			default:
				log_error_write(srv, __FILE__, __LINE__, "d", r);
				break;
			}

			con->in_joblist = 0;
		}

		srv->joblist->used = 0;
	} /* end of main loop  */
	/*
	 * 主循环的结束
	 */

	if (srv->srvconf.pid_file->used && srv->srvconf.changeroot->used == 0 && 0 == graceful_shutdown)
	{
		if (0 != unlink(srv->srvconf.pid_file->ptr))
		{
			if (errno != EACCES && errno != EPERM)
			{
				log_error_write(srv, __FILE__, __LINE__, "sbds", "unlink failed for:",
								srv->srvconf.pid_file, errno, strerror(errno));
			}
		}
	}
#ifdef HAVE_SIGACTION
	log_error_write(srv, __FILE__, __LINE__, "sdsd", "server stopped by UID =",
					last_sigterm_info.si_uid,"PID =", last_sigterm_info.si_pid);
#else
	log_error_write(srv, __FILE__, __LINE__, "s", "server stopped");
#endif

	/*
	 * clean-up 
	 */
	log_error_close(srv);
	network_close(srv);
	connections_free(srv);
	plugins_free(srv);
	server_free(srv);

	return 0;
}
