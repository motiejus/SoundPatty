#ifndef __SP_LOGGER_H_INCLUDED__
#define __SP_LOGGER_H_INCLUDED__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <pthread.h>

#define PRE_PRE __FILE__,__LINE__,__PRETTY_FUNCTION__
#define LOG_TRACE(...)  log_mo(5,PRE_PRE, __VA_ARGS__)
#define LOG_DEBUG(...)  log_mo(4,PRE_PRE, __VA_ARGS__)
#define LOG_INFO(...)   log_mo(3,PRE_PRE, __VA_ARGS__)
#define LOG_WARN(...)   log_mo(2,PRE_PRE, __VA_ARGS__)
#define LOG_ERROR(...)  log_mo(1,PRE_PRE, __VA_ARGS__)
#define LOG_FATAL(...)  log_mo(0,PRE_PRE, __VA_ARGS__)

enum LogLevels{TRACE,DEBUG,INFO,WARN,ERROR,FATAL};
//const char **LOGLEVELS = {"TRACE","DEBUG","INFO","WARN","ERROR","FATAL"}; TODO
extern int LogLevel; 
extern void log_mo (const int log_level, const char *file, const int line, const char *pretty_fn, const char *format, ...);

#endif //__SP_LOGGER_H_INCLUDED__
