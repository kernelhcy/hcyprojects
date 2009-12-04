/*
 ***************************************************************************
 *									   
 * ftplib.c - callable ftp access routines				   
 * Copyright(C) 1996-2001 Thomas Pfau, pfau@eclipse.net		   
 *	1407 Thomas Ave, North Brunswick, NJ, 08902			   
 *									   
 * This library is free software; you can redistribute it and/or	   
 * modify it under the terms of the GNU Library General Public		   
 * License as published by the Free Software Foundation; either		   
 * version 2 of the License, or(at your option) any later version.	   
 * 									   
 * This library is distributed in the hope that it will be useful,	   
 * but WITHOUT ANY WARRANTY; without even the implied warranty of	   
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU	   
 * Library General Public License for more details.			   
 * 									   
 * You should have received a copy of the GNU Library General Public	   
 * License along with this progam; if not, write to the			   
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,		   
 * Boston, MA 02111-1307, USA.						   
 * 									   
 ***************************************************************************
 */

#ifndef __FTPLIB_H
#define __FTPLIB_H

/* ftp_access() type codes */
#define FTPLIB_DIR 1 			//列出当前目录中的文件，只显示文件名
#define FTPLIB_DIR_VERBOSE 2 	//相当于ls -l。以长格式显示文件信息
#define FTPLIB_FILE_READ 3 		//
#define FTPLIB_FILE_WRITE 4 	//

/* ftp_access() mode codes */
#define FTPLIB_ASCII 'A'
#define FTPLIB_IMAGE 'I'
#define FTPLIB_TEXT FTPLIB_ASCII
#define FTPLIB_BINARY FTPLIB_IMAGE

/* connection modes */
#define FTPLIB_PASSIVE 1
#define FTPLIB_PORT 2

/* connection option names */
#define FTPLIB_CONNMODE 1
#define FTPLIB_CALLBACK 2
#define FTPLIB_IDLETIME 3
#define FTPLIB_CALLBACKARG 4
#define FTPLIB_CALLBACKBYTES 5

typedef struct _netbuf netbuf;
typedef int (*ftp_callback)(netbuf *n_control, int xfered, void *arg);

int ftplib_debug;
/*
 * ftp_site() - Send a 'SITE' command
 * ftp_last_response() - Retrieve last server response
 * ftp_systype() - Determine remote system type <ftplib V3.1>
 * ftp_size() - Determine size of remote file <ftplib V3.1>
 * ftp_mod_date() - Determine modification time of file <ftplib V3.1>
 *
 * ftp_connect() - Connect to a remote server
 * ftp_login() - Login to remote machine
 * ftp_quit() - Disconnect from remote server
 * ftp_options() - Set Connection Options <ftplib V3.1>
 *
 * ftp_chdir() - Change working directory
 * ftp_mkdir() - Create a directory
 * ftp_rmdir() - Remove a directory
 * ftp_ls() - List a remote directory long mode
 * ftp_dir() - List a remote directory
 *
 * ftp_cdup() - Change to parent directory <ftplib V3.1>
 * ftp_pwd() - Determine current working directory <ftplib V3.1>
 * ftp_get() - Retreive a remote file
 * ftp_put() - Send a local file to remote
 * ftp_delete() - Delete a remote file
 * ftp_rename() - Rename a remote file
 * ftp_access() - Open a remote file or directory
 *
 * ftp_data_read() - Read from remote file or directory
 * ftp_data_write() - Write to remote file
 * ftp_data_close() - Close data connection
 */
char *ftp_last_response(netbuf *n_control);
int ftp_connect(const char *host, netbuf **n_control);
int ftp_options(int opt, long val, netbuf *n_control);
int ftp_login(const char *user, const char *pass, netbuf *n_control);

int ftp_access(const char *path, int typ, int mode, netbuf *n_control, netbuf **nData);
int ftp_data_read(void *buf, int max, netbuf *nData);
int ftp_data_write(void *buf, int len, netbuf *nData);
int ftp_data_close(netbuf *nData);

int ftp_site(const char *cmd, netbuf *n_control);
int ftp_systype(char *buf, int max, netbuf *n_control);

int ftp_mkdir(const char *path, netbuf *n_control);
int ftp_chdir(const char *path, netbuf *n_control);
int ftp_cdup(netbuf *n_control);
int ftp_rmdir(const char *path, netbuf *n_control);
int ftp_pwd(char *path, int max, netbuf *n_control);
int ftp_dir(const char *output, const char *path, netbuf *n_control);
int ftp_ls(const char *output, const char *path, netbuf *n_control);
int ftp_size(const char *path, int *size, char mode, netbuf *n_control);
int ftp_mod_date(const char *path, char *dt, int max, netbuf *n_control);

int ftp_get(const char *output, const char *path, char mode,netbuf *n_control);
int ftp_put(const char *input, const char *path, char mode,netbuf *n_control);
int ftp_rename(const char *src, const char *dst, netbuf *n_control);
int ftp_delete(const char *fnm, netbuf *n_control);
void ftp_quit(netbuf *n_control);
#endif /* __FTPLIB_H */
