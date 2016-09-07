#ifndef LOGGING_H_
#define LOGGING_H_

#define LOG_DEBUG 0
#define LOG_INFO  1
#define LOG_WARN  2
#define LOG_ERROR 3

#define LOG_LEVEL LOG_INFO

#if LOG_LEVEL <= LOG_DEBUG
#define Debug(format, ...) {\
    char buffer[100];\
    sprintf (buffer, "DEBUG %s\n", format);\
    fprintf (stderr, buffer, ##__VA_ARGS__);\
  }
#else
#define Debug(format, ...)
#endif

#if LOG_LEVEL <= LOG_INFO
#define Info(format, ...) {\
    char buffer[100];\
    sprintf (buffer, "INFO %s\n", format);\
    fprintf (stderr, buffer, ##__VA_ARGS__);\
  }
#else
#define Info(format, ...)
#endif

#if LOG_LEVEL <= LOG_WARN
#define Warn(format, ...) {\
    char buffer[100];\
    sprintf (buffer, "WARN %s\n", format);\
    fprintf (stderr, buffer, ##__VA_ARGS__);\
  }
#else
#define Warn(format, ...)
#endif

#if LOG_LEVEL <= LOG_ERROR
#define Error(format, ...) {\
    char buffer[100];\
    sprintf (buffer, "ERROR %s\n", format);\
    fprintf (stderr, buffer, ##__VA_ARGS__);\
  }
#else
#define Error(format, ...)
#endif

#endif /* LOGGING_H_ */
