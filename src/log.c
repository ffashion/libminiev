/*
 * Copyright (c) 2020 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include "types.h"
#include "log.h"

#define MAX_CALLBACKS 32

#define LOG_FILE_RDONLY          O_RDONLY
#define LOG_FILE_WRONLY          O_WRONLY
#define LOG_FILE_RDWR            O_RDWR
#define LOG_FILE_CREATE_OR_OPEN  O_CREAT
#define LOG_FILE_OPEN            0
#define LOG_FILE_TRUNCATE        (O_CREAT|O_TRUNC)
#define LOG_FILE_APPEND          (O_WRONLY|O_APPEND)
#define LOG_FILE_NONBLOCK        O_NONBLOCK

#define log_open_file(name, mode, create, access)                            \
    open((const char *) name, mode|create, access)

#define LOG_FILE_DEFAULT_ACCESS  0644
#define LOG_FILE_OWNER_ACCESS    0600
#define LOG_FILE_READ_ACCESS     0666

int log_create_file_mapping(struct log_file_mapping_s *fm)
{
    fm->fd = log_open_file(fm->name, LOG_FILE_RDWR , LOG_FILE_TRUNCATE | LOG_FILE_CREATE_OR_OPEN,
                        LOG_FILE_DEFAULT_ACCESS);
    if (fm->fd == -1) {
        perror("open file error");
        return -1;
    }

    if (ftruncate(fm->fd, fm->size) == -1) {
        goto failed;
    }

    fm->addr = mmap(NULL, fm->size, PROT_READ|PROT_WRITE, MAP_SHARED,
                    fm->fd, 0);
    if (fm->addr != MAP_FAILED) {
        return 0;
    }
failed:
    if (close(fm->fd) == -1) {
        return -1;
    }

    return -1;
}
typedef struct {
  log_LogFn fn;
  void *udata;
  int level;
} Callback;

static struct {
  void *udata;
  log_LockFn lock;
  int level;
  bool quiet;
  Callback callbacks[MAX_CALLBACKS];
} L;


static const char *level_strings[] = {
  "TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {
  "\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"
};
#endif


static void stdout_callback(log_Event *ev) {
  char buf[16];
  buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';
#ifdef LOG_USE_COLOR
  fprintf(
    ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ",
    buf, level_colors[ev->level], level_strings[ev->level],
    ev->file, ev->line);
#else
  fprintf(
    ev->udata, "%s %-5s %s:%d: ",
    buf, level_strings[ev->level], ev->file, ev->line);
#endif
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}


static void file_callback(log_Event *ev) {
  char buf[64];
  struct timeval tv;
  buf[strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", ev->time)] = '\0';

  gettimeofday(&tv, NULL);
  buf[sprintf(buf, "%ld", 1000000L * tv.tv_sec + tv.tv_usec)]= '\0';

  fprintf(
    ev->udata, "%s %-5s %s:%d: ",
    buf, level_strings[ev->level], ev->file, ev->line);
  vfprintf(ev->udata, ev->fmt, ev->ap);
  fprintf(ev->udata, "\n");
  fflush(ev->udata);
}


static void lock(void)   {
  if (L.lock) { L.lock(true, L.udata); }
}


static void unlock(void) {
  if (L.lock) { L.lock(false, L.udata); }
}


const char* log_level_string(int level) {
  return level_strings[level];
}


void log_set_lock(log_LockFn fn, void *udata) {
  L.lock = fn;
  L.udata = udata;
}


void log_set_level(int level) {
  L.level = level;
}


void log_set_quiet(bool enable) {
  L.quiet = enable;
}


int log_add_callback(log_LogFn fn, void *udata, int level) {
  for (int i = 0; i < MAX_CALLBACKS; i++) {
    if (!L.callbacks[i].fn) {
      L.callbacks[i] = (Callback) { fn, udata, level };
      return 0;
    }
  }
  return -1;
}


int log_add_fp(FILE *fp, int level) {
  return log_add_callback(file_callback, fp, level);
}


static void init_event(log_Event *ev, void *udata) {
  if (!ev->time) {
    time_t t = time(NULL);
    ev->time = localtime(&t);
  }
  ev->udata = udata;
}


void log_log(int level, const char *file, int line, const char *fmt, ...) {
  log_Event ev = {
    .fmt   = fmt,
    .file  = file,
    .line  = line,
    .level = level,
  };

  lock();

  if (!L.quiet && level >= L.level) {
    init_event(&ev, stderr);
    va_start(ev.ap, fmt);
    stdout_callback(&ev);
    va_end(ev.ap);
  }

  for (int i = 0; i < MAX_CALLBACKS && L.callbacks[i].fn; i++) {
    Callback *cb = &L.callbacks[i];
    if (level >= cb->level) {
      init_event(&ev, cb->udata);
      va_start(ev.ap, fmt);
      cb->fn(&ev);
      va_end(ev.ap);
    }
  }

  unlock();
}