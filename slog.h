#ifndef  _SLOG_PRIV_LIB_H_
#define  _SLOG_PRIV_LIB_H_

#ifdef _WIN32
#include <Windows.h>
#pragma warning(disable:4996)
#pragma warning(disable:4819)
#pragma warning(disable:4018)
#include "io.h"

#define __FUNCTION_NAME__ __FUNCTION__
#else
#include <pthread.h>
#include <unistd.h>
#include <stdarg.h>

#define __FUNCTION_NAME__ __func__
#endif

#include <string.h>
#include "time.h"
#include <stdio.h>


#ifdef	__NO_FUNC_NAME__
#undef	__FUNCTION_NAME__
#define __FUNCTION_NAME__ "__FUNC__"
#endif



namespace splog
{

/*
mutex begin
*/
namespace internal
{



class  mutex
{
public:
	mutex();
	~mutex();
	void lock();
	void unlock();
private:
	#ifdef _WIN32
	CRITICAL_SECTION m_lock;
	#else
	pthread_mutex_t m_lock;
	#endif
};
}
/*
mutex end
*/


/*
log begin
*/
typedef enum{ LOGTOFILE, LOGTOCONSOLE } LOGTYPE;
typedef enum{ LOGL_DEBUG, LOGL_INFO, LOGL_WARNING, LOGL_ERROR, LOGL_FATAL } LOGLEVEL;



#define DEFAULT_LOG_FILENAME "splog.log"
#define DEFAULT_CFG_FILENAME "splog.cfg"

#define SLTIME  1
#define SLTID   2
#define SLSRCFL 4
#define SLLEVEL 8
#define SLMSG   16
#define SLFUNCNAME 32


#define SLALL (SLTIME|SLTID|SLSRCFL|SLLEVEL|SLMSG|SLFUNCNAME)


/*
splog.cfg ascii file

filename@loglevel@logtype

logtype=[tofile toconsole]
loglevel=[debug,info,warning,error,fatal]

example
log.txt@warning@tofile
*/

#define MAX_LOG_LEN 1024



#define log_fmt_common(level,format,...) log_fmt_common_msg(level,__FUNCTION_NAME__,__FILE__,__LINE__,format,##__VA_ARGS__)


#define log_fmt_debug(format,...)	log_fmt_common(splog::LOGL_DEBUG,format,##__VA_ARGS__)
#define log_fmt_info(format,...)	log_fmt_common(splog::LOGL_INFO,format,##__VA_ARGS__)
#define log_fmt_warning(format,...) log_fmt_common(splog::LOGL_WARNING,format,##__VA_ARGS__)
#define log_fmt_error(format,...)	log_fmt_common(splog::LOGL_ERROR,format,##__VA_ARGS__)
#define log_fmt_fatal(format,...)	log_fmt_common(splog::LOGL_FATAL,format,##__VA_ARGS__)

class logger
{
public:
	logger(const char* cfgfilename=NULL,LOGTYPE logtype = LOGTOCONSOLE, const char* filename = DEFAULT_LOG_FILENAME, LOGLEVEL loglevel = LOGL_DEBUG);
	~logger();


	bool init(LOGTYPE logtype = LOGTOFILE,const char* filename = DEFAULT_LOG_FILENAME, LOGLEVEL loglevel = LOGL_DEBUG);
	bool init_from_file(const char* cfg_filename);
 	void uninit(bool needlock=true);
 	void set_log_level(LOGLEVEL loglevel);

	/*
	for convenience
	auto promote
	*/
	logger& log_debug(const char *msg, const char* funcname = NULL, const char* srcfile = __FILE__, int srcline = __LINE__);
	logger& log_info(const char *msg, const char* funcname = NULL, const char* srcfile = __FILE__, int srcline = __LINE__);
	logger& log_warning(const char *msg, const char* funcname = NULL, const char* srcfile = __FILE__, int srcline = __LINE__);
	logger& log_error(const char *msg, const char* funcname = NULL, const char* srcfile = __FILE__, int srcline = __LINE__);
	logger& log_fatal(const char *msg, const char* funcname = NULL, const char* srcfile = __FILE__, int srcline = __LINE__);
	logger& log_msg(LOGLEVEL level, const char* msg, const char* funcname = NULL, const char* srcfile = __FILE__, int srcline = __LINE__);
	/*
	common msg write
	*/
	logger& log_fmt_common_msg(LOGLEVEL level, const char* srcfunction, const char* srcfile, int srcline, const char* format, ...);

	void set_log_flag(long flag);
private:
	/*
	global mutex for console
	*/
	static internal::mutex g_console_mutex;

	/*
	mutex for each object
	*/
	internal::mutex m_mutex;

	long m_log_flag;
	bool m_initok;

	LOGTYPE  m_cur_logtype;
	LOGLEVEL m_cur_loglevel;
	FILE*	 m_logfile_fp;
	FILE*    m_console_fp;
};
/*
log end
*/

unsigned int get_current_thread_id();

}



#define GLOG g_unique_slog
extern splog::logger GLOG;

/*
default log init
*/
#define SLOG_DEFAULT_INIT splog::logger GLOG;
#define SLOG_COMMON_INIT(log_filename,loglevel,cfg_filename) splog::logger GLOG(cfg_filename,splog::LOGTOFILE,(log_filename),(loglevel));

#define SLOG_TOFILE_LEVEL_INIT(log_filename,loglevel) SLOG_COMMON_INIT(log_filename,loglevel,NULL)
#define SLOG_TOFILE_INIT(log_filename) SLOG_TOFILE_LEVEL_INIT(log_filename,(splog::LOGL_DEBUG))


#define SLOG_CFG_INIT(cfg_filename) SLOG_TOFILE_LEVEL_INIT(DEFAULT_LOG_FILENAME,(splog::LOGL_DEBUG),cfg_filename)
#define SLOG_CFG_DEFAULT_INIT	    SLOG_CFG_INIT(DEFAULT_CFG_FILENAME)

/*
default log write
*/
#define SLOG_DEBUG(foramt,...)	  GLOG.log_fmt_debug(foramt,##__VA_ARGS__)
#define SLOG_INFO(foramt,...)	  GLOG.log_fmt_info(foramt,##__VA_ARGS__)
#define SLOG_WARNING(foramt,...)  GLOG.log_fmt_warning(foramt,##__VA_ARGS__)
#define SLOG_ERROR(foramt,...)    GLOG.log_fmt_error(foramt,##__VA_ARGS__)
#define SLOG_FATAL(foramt,...)    GLOG.log_fmt_fatal(foramt,##__VA_ARGS__)
#define SLOG_FUNC()               SLOG_DEBUG("")


/*
set log flag
*/
#define SLOG_SET_FLAG(x)          GLOG.set_log_flag((x))
#endif
