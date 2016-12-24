#include "slog.h"

splog::internal::mutex::mutex()
{
#ifdef _WIN32
	InitializeCriticalSection(&m_lock);
#else
	pthread_mutex_init(&m_lock, NULL);
#endif
}
splog::internal::mutex::~mutex()
{
#ifdef _WIN32
	DeleteCriticalSection(&m_lock);
#else
	pthread_mutex_destroy(&m_lock);
#endif 
}
void splog::internal::mutex::lock()
{
#ifdef _WIN32
	EnterCriticalSection(&m_lock);
#else
	pthread_mutex_lock(&m_lock);
#endif
}
void splog::internal::mutex::unlock()
{
#ifdef _WIN32
	LeaveCriticalSection(&m_lock);
#else
	pthread_mutex_unlock(&m_lock);
#endif
}


/*
global mutex for console
*/
splog::internal::mutex splog::logger::g_console_mutex;


splog::logger::logger(const char* cfgfilename, LOGTYPE logtype, const char* filename, LOGLEVEL loglevel)
{
	m_logfile_fp = NULL;
	m_console_fp = stdout;
	uninit();
	if (cfgfilename)
	{
		init_from_file(cfgfilename);
	}
	else
	{
		init(logtype, filename, loglevel);
	}
	
};

splog::logger::~logger()
{
	uninit();
};

splog::logger& splog::logger::log_msg(LOGLEVEL level, const char* msg, const char* srcfile, int srcline)
{
	return log_fmt_common_msg(level, srcfile, srcline, msg);
};

splog::logger& splog::logger::log_fmt_common_msg(LOGLEVEL level, const char* srcfile, int srcline, const char* format, ...)
{
	FILE* selected_fp = NULL;


	m_mutex.lock();

	if (m_cur_logtype == LOGTOFILE)
	{
		selected_fp = m_logfile_fp;
		g_console_mutex.lock();
	}
	else
	{
		selected_fp = m_console_fp;
	}

	do 
	{
		if (!m_initok)
		{
			break;
		}
		if (!format||!srcfile)
		{
			break;
		}

		if (level<m_cur_loglevel)
		{
			break;
		}
		
		if (!selected_fp)
		{
			break;
		}


		char buffer[MAX_LOG_LEN];
		/*
		time
		*/
		time_t pt;
		time(&pt);
		struct tm * timeinfo;
		timeinfo = localtime(&pt);
		sprintf(buffer, "[%d-%d-%d %d:%d:%d]", timeinfo->tm_year + 1900, \
			timeinfo->tm_mon + 1, \
			timeinfo->tm_mday, \
			timeinfo->tm_hour, \
			timeinfo->tm_min, \
			timeinfo->tm_sec);
		fwrite(buffer, 1, strlen(buffer), selected_fp);
		/*
		line and filename
		*/
		int flen = strlen(srcfile);
		if (flen)
		{
			/*
			not full path but filename
			*/
			while (flen)
			{
				flen--;
				if (srcfile[flen] == '\\' || srcfile[flen] == '/')
				{
					flen++;
					break;
				}
			}

#ifdef _MSC_VER
#define snprintf _snprintf
#endif
			snprintf(buffer,MAX_LOG_LEN, " [filename: %s line: %d] ", &srcfile[flen], srcline);
			fwrite(buffer, 1, strlen(buffer), selected_fp);
		}
		/*
		level prefix
		*/
		const char* prefix = "[DEFAULT]";
		switch (level)
		{
		case LOGL_DEBUG:
			prefix = "[DEBUG] ";
			break;
		case LOGL_INFO:
			prefix = "[INFO] ";
			break;
		case LOGL_WARNING:
			prefix = "[WARNING] ";
			break;
		case LOGL_ERROR:
			prefix = "[ERROR] ";
			break;
		case LOGL_FATAL:
			prefix = "[FATAL] ";
			break;
		}
		fwrite(prefix, 1, strlen(prefix), selected_fp);
		/*
		msg 
		be careful
		may overflow 
		*/
		va_list msg_list;
		va_start(msg_list, format);
		vsnprintf(buffer,MAX_LOG_LEN-1,format, msg_list);
		va_end(msg_list);
		fwrite(buffer, 1, strlen(buffer), selected_fp);

		/*
		new line
		*/
#ifdef _WIN32
		fwrite("\r\n", 1, 2, selected_fp);
#else
		fwrite("\n", 1, 1, selected_fp);
#endif
		fflush(selected_fp);

	} while (false);

	if (m_cur_logtype == LOGTOFILE)
	{
		g_console_mutex.unlock();
	}

	m_mutex.unlock();


	return *this;
};




bool splog::logger::init(LOGTYPE logtype,const char* filename, LOGLEVEL loglevel)
{
	bool bret = false;

	m_mutex.lock();
	do
	{
		if (m_initok)
		{
			//no lock needed
			uninit(false);
		}

		m_cur_logtype = logtype;
		m_cur_loglevel = loglevel;
		if (m_cur_logtype == LOGTOFILE)
		{
			if (!filename)
			{
				break;
			}
			m_logfile_fp = fopen(filename, "wb");
			if (!m_logfile_fp)
			{
				break;
			}
		}
		m_initok = true;
		bret = true;
	} while (false);
	m_mutex.unlock();
	return bret;
};

bool _is_valid_char(char ch)
{
	if (ch >= '0'&&ch <= '9')
	{
		return true;
	}
	if (ch >= 'a'&&ch <= 'z')
	{
		return true;
	}
	if (ch >= 'A'&&ch <= 'Z')
	{
		return true;
	}
	if (ch == '_' || ch == '@' || ch == '.')
	{
		return true;
	}
	return false;
};
bool splog::logger::init_from_file(const char* cfgfilename)
{
	bool cfgfileok = false;
	LOGTYPE logtype = LOGTOFILE;
	LOGLEVEL loglevel = LOGL_INFO;
	FILE* fcfg = NULL;
	char cfgstr[100];
	memset(cfgstr, 0, 100);
	char* ptype = NULL;
	char* pfilename = NULL;
	char* plevel = NULL;
	char* errsr = "ok";
	do
	{
		if (!cfgfilename)
		{
			errsr = "cfgfilename=NULL";
			break;
		}
		fcfg = fopen(cfgfilename, "r");
		if (!fcfg)
		{
			errsr = "can't open cfg file";
			break;
		}
		size_t rdl = fread(cfgstr, 1, 80, fcfg);
		if (rdl >= 80)
		{
			errsr = "cfg file too big";
			break;
		}
		//filename
		pfilename = cfgstr;
		for (int i = 0; i < rdl; i++)
		{
			if (!_is_valid_char(cfgstr[i]))
				break;

			if (cfgstr[i] == '@')
			{
				cfgstr[i] = 0;
				if (plevel == NULL)
				{
					plevel = &cfgstr[i + 1];
				}
				else if (ptype == NULL)
				{
					ptype = &cfgstr[i + 1];
				}
			};
			if (cfgstr[i] >= 'A'&&cfgstr[i] <= 'Z')
				cfgstr[i] = cfgstr[i] + ('a' - 'A');
		}

		if (plevel&&ptype)
		{
			if (strcmp(ptype, "tofile") == 0)
			{
				logtype = LOGTOFILE;
			}
			else if (strcmp(ptype, "toconsole") == 0)
			{
				logtype = LOGTOCONSOLE;
			}
			else
			{
				errsr = "invalid logtype";
				break;
			}
			if (strcmp(plevel, "debug") == 0)
			{
				loglevel = LOGL_DEBUG;
			}
			else if (strcmp(plevel, "info") == 0)
			{
				loglevel = LOGL_INFO;
			}
			else if (strcmp(plevel, "warning") == 0)
			{
				loglevel = LOGL_WARNING;
			}
			else if (strcmp(plevel, "error") == 0)
			{
				loglevel = LOGL_ERROR;
			}
			else if (strcmp(plevel, "fatal") == 0)
			{
				loglevel = LOGL_FATAL;
			}
			else
			{
				errsr = "invalid loglevel";
				break;
			}
			cfgfileok = true;
		}

	} while (false);

	if (cfgfileok)
	{
		return init(logtype,pfilename, loglevel);
	}
	else
	{
		FILE* f = fopen("invalidcfgfile.txt","wb");
		if (f)
		{
			fprintf(f, "please check config file\r\n%s\r\n",errsr);
			fclose(f);
		}
	}

	return init();
};


void splog::logger::uninit(bool needlock)
{
	if (needlock)
	{
		m_mutex.lock();
	}
	
	if (m_logfile_fp)
	{
		fflush(m_logfile_fp);
		fclose(m_logfile_fp);
	}

	m_logfile_fp = NULL;
	m_initok = false;
	m_cur_loglevel = LOGL_DEBUG;
	m_cur_logtype = LOGTOFILE;

	if (needlock)
	{
		m_mutex.unlock();
	}
	
};

void splog::logger::set_log_level(LOGLEVEL loglevel)
{
	m_cur_loglevel = loglevel;
};

/*
for convenience
auto promote
*/
splog::logger& splog::logger::log_debug(const char *msg, const char* srcfile, int srcline)
{
	return log_msg(LOGL_DEBUG, msg, srcfile, srcline);
};
splog::logger& splog::logger::log_info(const char *msg, const char* srcfile, int srcline)
{
	return log_msg(LOGL_INFO, msg, srcfile, srcline);
};
splog::logger& splog::logger::log_warning(const char *msg, const char* srcfile, int srcline)
{
	return log_msg(LOGL_WARNING, msg, srcfile, srcline);
};
splog::logger& splog::logger::log_error(const char *msg, const char* srcfile, int srcline)
{
	return log_msg(LOGL_ERROR, msg, srcfile, srcline);
};
splog::logger& splog::logger::log_fatal(const char *msg, const char* srcfile, int srcline)
{
	return log_msg(LOGL_FATAL, msg, srcfile, srcline);
};
