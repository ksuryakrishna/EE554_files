/*
 tools/sw_cmdline.c
 url: https://github.com/noporpoise/seq-align
 maintainer: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain, no warranty
 date: Nov 2013
 */

// request decent POSIX version
#define _XOPEN_SOURCE 700
#define _BSD_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define ARR_2D_INDEX(width,i,j) (((unsigned long)(j)*(width)) + (i))
#define ARR_LOOKUP(arr,width,i,j) arr[ARR_2D_INDEX((width),(i),(j))]
#define ARR_2D_X(arr_index, arr_width) ((arr_index) % (arr_width))
#define ARR_2D_Y(arr_index, arr_width) ((arr_index) / (arr_width))

#define QUOTE(str) #str

#define MAX2(x,y) ((x) >= (y) ? (x) : (y))
#define MIN2(x,y) ((x) <= (y) ? (x) : (y))
#define MAX3(x,y,z) ((x) >= (y) && (x) >= (z) ? (x) : MAX2(y,z))
#define MIN3(x,y,z) ((x) <= (y) && (x) <= (z) ? (x) : MIN2(y,z))
#define MAX4(w,x,y,z) MAX2(MAX3(w, x, y), z)

#define ABSDIFF(a,b) ((a) > (b) ? (a)-(b) : (b)-(a))

// my utility functions
// #include "seq_file/seq_file.h"
/*
https://github.com/noporpoise/seq_file
Isaac Turner <turner.isaac@gmail.com>
Sep 2015, Public Domain
*/

#ifndef _SEQ_FILE_HEADER
#define _SEQ_FILE_HEADER

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h> // strcasecmp
#include <stdbool.h>
#include <ctype.h>
#include <limits.h>
#include <zlib.h>
#include <assert.h>

// #define _USESAM 1

#if defined(_USESAM) && _USESAM == 0
#undef _USESAM
#endif

#ifdef _USESAM
#include "htslib/hfile.h"
#include "htslib/hts.h"
#include "htslib/sam.h"
#endif

// #include "stream_buffer.h"
/*
 stream_buffer.h
 project: string_buffer
 url: https://github.com/noporpoise/StringBuffer
 author: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain
 Sep 2015
*/

#ifndef _STREAM_BUFFER_HEADER
#define _STREAM_BUFFER_HEADER

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <zlib.h>
#include <limits.h>

/*
   Generic string buffer functions
*/

#ifndef ROUNDUP2POW
  #define ROUNDUP2POW(x) (0x1UL << (64 - __builtin_clzl(x)))
#endif

static inline void cbuf_capacity(char **buf, size_t *sizeptr, size_t len)
{
  len++; // for nul byte
  if(*sizeptr < len) {
    *sizeptr = ROUNDUP2POW(len);
    if((*buf = realloc(*buf, *sizeptr)) == NULL) {
      fprintf(stderr, "Out of memory\n");
      exit(EXIT_FAILURE);
    }
  }
}

static inline void cbuf_append_char(char **buf, size_t *lenptr, size_t *sizeptr,
                                    char c)
{
  cbuf_capacity(buf, sizeptr, *lenptr+1);
  (*buf)[(*lenptr)++] = c;
  (*buf)[*lenptr] = '\0';
}

static inline void cbuf_append_str(char **buf, size_t *lenptr, size_t *sizeptr,
                                   char *str, size_t len)
{
  cbuf_capacity(buf, sizeptr, *lenptr+len);
  memcpy(*buf + *lenptr, str, len);
  *lenptr += len;
  (*buf)[*lenptr] = '\0';
}

static inline void cbuf_chomp(char *buf, size_t *lenptr)
{
  while(*lenptr && (buf[(*lenptr)-1] == '\n' || buf[(*lenptr)-1] == '\r'))
    (*lenptr)--;
  buf[*lenptr] = '\0';
}

/*
   Stream buffer
*/

typedef struct
{
  char *b;
  // begin is index of first char (unless begin >= end)
  // end is index of \0
  // size should be >= end+1 to allow for \0
  // (end-begin) is the number of bytes in buffer
  size_t begin, end, size;
} StreamBuffer;


#define strm_buf_init {.b = NULL, .begin = 0, .end = 0, .size = 0}

#define strm_buf_reset(sb) do { (b)->begin = (b)->end = 1; } while(0)

// Returns NULL if out of memory, @b otherwise
static inline StreamBuffer* strm_buf_alloc(StreamBuffer *b, size_t s)
{
  b->size = (s < 16 ? 16 : s) + 1;
  if((b->b = malloc(b->size)) == NULL) return NULL;
  b->begin = b->end = 1;
  b->b[b->end] = b->b[b->size-1] = 0;
  return b;
}

static inline void strm_buf_dealloc(StreamBuffer *b)
{
  free(b->b);
  memset(b, 0, sizeof(StreamBuffer));
}

static inline StreamBuffer* strm_buf_new(size_t s)
{
  StreamBuffer *b = (StreamBuffer*)malloc(sizeof(StreamBuffer));
  if(b == NULL) return NULL;
  else if(strm_buf_alloc(b,s)) return b;
  else { free(b); return NULL; } /* couldn't malloc */
}

static inline void strm_buf_free(StreamBuffer *cbuf)
{
  free(cbuf->b);
  free(cbuf);
}

// len is the number of bytes you want to be able to store
// Adds one to len for NULL terminating byte
static inline void strm_buf_ensure_capacity(StreamBuffer *cbuf, size_t len)
{
  cbuf_capacity(&cbuf->b, &cbuf->size, len);
}


/*
Unbuffered

fgetc(f)
gzgetc(gz)
fungetc(f,c)
gzungetc(gz,c)
gzread2(gz,buf,len)
fread2(f,buf,len)
gzgets2(gz,buf,len)
fgets2(f,buf,len)
gzreadline(gz,out)
freadline(f,out)
*/

#define ferror2(fh) ferror(fh)
static inline int gzerror2(gzFile gz) { int e; gzerror(gz, &e); return (e < 0); }

// Define read for gzFile and FILE (unbuffered)
// Check ferror/gzerror on return for error
#define fread2(fh,buf,len) fread(buf,1,len,fh)

// gzread returns -1 on error, otherwise number of bytes read
// gzread can only read 2^32 bytes at a time, and returns -1 on error
// Check ferror/gzerror on return for error
static inline size_t gzread2(gzFile gz, void *ptr, size_t len)
{
  size_t nread = 0, n;
  int s;
  while(nread < len) {
    n = len - nread;
    if(n > UINT_MAX) n = UINT_MAX;
    s = gzread(gz, (char*)ptr+nread, n);
    if(s <= 0) break;
    nread += s;
  }
  return nread;
}

// TODO: these should support len > 2^32 by looping fgets/gzgets
#define gzgets2(gz,buf,len) gzgets(gz,buf,(int)(len))
#define fgets2(fh,buf,len) fgets(buf,(int)(len),fh)

// fgetc(f), gzgetc(gz) are already good to go
// fungetc(c,f), gzungetc(c,gz) are already good to go

#define fungetc(c,f) ungetc(c,f)

// Define readline for gzFile and FILE (unbuffered)
// Check ferror/gzerror on return for error
#define _func_readline(name,type_t,__gets) \
  static inline size_t name(type_t file, char **buf, size_t *len, size_t *size)\
  {                                                                            \
    if(*len+1 >= *size) cbuf_capacity(buf, size, *len+1);                      \
    /* Don't read more than 2^32 bytes at once (gzgets limit) */               \
    size_t r = *size-*len > UINT_MAX ? UINT_MAX : *size-*len, origlen = *len;  \
    while(__gets(file, *buf+*len, r) != NULL)                                  \
    {                                                                          \
      *len += strlen(*buf+*len);                                               \
      if((*buf)[*len-1] == '\n') return *len-origlen;                          \
      else *buf = realloc(*buf, *size *= 2);                                   \
      r = *size-*len > UINT_MAX ? UINT_MAX : *size-*len;                       \
    }                                                                          \
    return *len-origlen;                                                       \
  }

_func_readline(gzreadline,gzFile,gzgets2)
_func_readline(freadline,FILE*,fgets2)

// Define skipline
#define _func_skipline(fname,ftype,readc) \
  static inline size_t fname(ftype file)                                       \
  {                                                                            \
    int c;                                                                     \
    size_t skipped_bytes = 0;                                                  \
    while((c = readc(file)) != -1) {                                           \
      skipped_bytes++;                                                         \
      if(c == '\n') break;                                                     \
    }                                                                          \
    return skipped_bytes;                                                      \
  }

_func_skipline(gzskipline,gzFile,gzgetc)
_func_skipline(fskipline,FILE*,fgetc)

/* Buffered */

/*
fgetc_buf(f,in)
gzgetc_buf(gz,in)
ungetc_buf(c,in)
fread_buf(f,ptr,len,in)
gzread_buf(f,ptr,len,in)
gzreadline_buf(gz,in,out)
freadline_buf(f,in,out)
*/

// __read is either gzread2 or fread2
// offset of 1 so we can unget at least one char
// Beware: read-in buffer is not null-terminated
// Returns fail on error
#define _READ_BUFFER(file,in,__read) do                                        \
{                                                                              \
  (in)->end = 1+__read(file,(in)->b+1,(in)->size-1);                           \
  (in)->begin = 1;                                                             \
} while(0)

// Define getc for gzFile and FILE (buffered)
// Returns character or -1 at EOF / error
// Check ferror/gzerror on return for error
#define _func_getc_buf(fname,type_t,__read)                                    \
  static inline int fname(type_t file, StreamBuffer *in)                       \
  {                                                                            \
    if(in->begin >= in->end) {                                                 \
      _READ_BUFFER(file,in,__read);                                            \
      return in->begin >= in->end ? -1 : in->b[in->begin++];                   \
    }                                                                          \
    return in->b[in->begin++];                                                 \
  }

_func_getc_buf(fgetc_buf,FILE*,fread2)
_func_getc_buf(gzgetc_buf,gzFile,gzread2)

// Define ungetc for buffers
// returns c if successful, otherwise -1
static inline int ungetc_buf(int c, StreamBuffer *in)
{
  if(in->begin == 0) {
    if(in->end == 0) {
      in->b[0] = (char)c;
      in->end = 1;
      return c;
    }
    else return -1;
  }
  in->b[--(in->begin)] = (char)c;
  return c;
}

#define fungetc_buf(c,in) ungetc_buf(c,in)
#define gzungetc_buf(c,in) ungetc_buf(c,in)

// Returns number of bytes read
// Check ferror/gzerror on return for error
#define _func_read_buf(fname,type_t,__read)                                    \
  static inline size_t fname(type_t file, void *ptr, size_t len, StreamBuffer *in)\
  {                                                                            \
    size_t remaining = len, next;                                              \
    if(len > 2*in->size) { /* first empty buffer then read directly */         \
      next = in->end - in->begin;                                              \
      memcpy(ptr, in->b+in->begin, in->end-in->begin);                         \
      in->begin = in->end; ptr = (char*)ptr + next; remaining -= next;         \
      remaining -= __read(file,ptr,remaining);                                 \
    }                                                                          \
    else {                                                                     \
      while(1) {                                                               \
        if(remaining == 0) { break; }                                          \
        if(in->begin >= in->end) {                                             \
          _READ_BUFFER(file,in,__read);                                        \
          if(in->begin >= in->end) { break; }                                  \
        }                                                                      \
        next = in->end - in->begin;                                            \
        if(remaining < next) next = remaining;                                 \
        memcpy(ptr, in->b+in->begin, next);                                    \
        in->begin += next; ptr = (char*)ptr + next; remaining -= next;         \
      }                                                                        \
    }                                                                          \
    return len - remaining;                                                    \
  }

_func_read_buf(gzread_buf,gzFile,gzread2)
_func_read_buf(fread_buf,FILE*,fread2)

// Define readline for gzFile and FILE (buffered)
// Check ferror/gzerror on return for error
#define _func_readline_buf(fname,type_t,__read)                                \
  static inline size_t fname(type_t file, StreamBuffer *in,                    \
                             char **buf, size_t *len, size_t *size)            \
  {                                                                            \
    if(in->begin >= in->end) { _READ_BUFFER(file,in,__read); }                 \
    size_t offset, buffered, total_read = 0;                                   \
    while(in->end > in->begin)                                                 \
    {                                                                          \
      for(offset = in->begin; offset < in->end && in->b[offset++] != '\n'; ){} \
      buffered = offset - in->begin;                                           \
      cbuf_capacity(buf, size, (*len)+buffered);                               \
      memcpy((*buf)+(*len), in->b+in->begin, buffered);                        \
      *len += buffered;                                                        \
      in->begin = offset;                                                      \
      total_read += buffered;                                                  \
      if((*buf)[*len-1] == '\n') break;                                        \
      _READ_BUFFER(file,in,__read);                                            \
    }                                                                          \
    (*buf)[*len] = 0;                                                          \
    return total_read;                                                         \
  }

_func_readline_buf(gzreadline_buf,gzFile,gzread2)
_func_readline_buf(freadline_buf,FILE*,fread2)

// Define buffered skipline
// Check ferror/gzerror on return for error
#define _func_skipline_buf(fname,ftype,__read)                                 \
  static inline size_t fname(ftype file, StreamBuffer *in)                     \
  {                                                                            \
    if(in->begin >= in->end) { _READ_BUFFER(file,in,__read); }                 \
    size_t offset, skipped_bytes = 0;                                          \
    while(in->end > in->begin)                                                 \
    {                                                                          \
      for(offset = in->begin; offset < in->end && in->b[offset++] != '\n'; )   \
      skipped_bytes += offset - in->begin;                                     \
      in->begin = offset;                                                      \
      if(in->b[offset-1] == '\n') break;                                       \
      _READ_BUFFER(file,in,__read);                                            \
    }                                                                          \
    return skipped_bytes;                                                      \
  }

_func_skipline_buf(gzskipline_buf,gzFile,gzread2)
_func_skipline_buf(fskipline_buf,FILE*,fread2)

// Define buffered gzgets_buf, fgets_buf

// Reads upto len-1 bytes (or to the first \n if first) into str
// Adds null-terminating byte
// returns pointer to `str` or NULL if EOF
// Check ferror/gzerror on return for error
#define _func_gets_buf(fname,type_t,__read)                                    \
  static inline char* fname(type_t file, StreamBuffer *in, char *str, size_t len)\
  {                                                                            \
    if(len == 0) return NULL;                                                  \
    if(len == 1) {str[0] = 0; return str; }                                    \
    if(in->begin >= in->end) { _READ_BUFFER(file,in,__read); }                 \
    size_t i, buffered, limit, total_read = 0, remaining = len-1;              \
    while(in->end > in->begin)                                                 \
    {                                                                          \
      limit = (in->begin+remaining < in->end ? in->begin+remaining : in->end); \
      for(i = in->begin; i < limit && in->b[i++] != '\n'; );                   \
      buffered = i - in->begin;                                                \
      memcpy(str+total_read, in->b+in->begin, buffered);                       \
      in->begin += buffered;                                                   \
      total_read += buffered;                                                  \
      remaining -= buffered;                                                   \
      if(remaining == 0 || str[total_read-1] == '\n') break;                   \
      _READ_BUFFER(file,in,__read);                                            \
    }                                                                          \
    str[total_read] = 0;                                                       \
    return total_read == 0 ? NULL : str;                                       \
  }

_func_gets_buf(gzgets_buf,gzFile,gzread2)
_func_gets_buf(fgets_buf,FILE*,fread2)


// Buffered ftell/gztell, fseek/gzseek

// ftell
static inline off_t ftell_buf(FILE *fh, StreamBuffer *strm) {
  return ftell(fh) - (strm->end - strm->begin);
}

static inline off_t gztell_buf(gzFile gz, StreamBuffer *strm) {
  return gztell(gz) - (strm->end - strm->begin);
}

// fseek
static inline long fseek_buf(FILE *fh, off_t offset, int whence,
                             StreamBuffer *strm)
{
  int x = fseek(fh, offset, whence);
  if(x == 0) { strm->begin = strm->end = 1; }
  return x;
}

static inline long gzseek_buf(gzFile gz, off_t offset, int whence,
                              StreamBuffer *strm)
{
  int x = gzseek(gz, offset, whence);
  if(x == 0) { strm->begin = strm->end = 1; }
  return x;
}

/*
 Output (unbuffered)

fputc2(fh,c)
gzputc2(gz,c)
fputs2(fh,c)
gzputs2(gz,c)
fprintf(fh,fmt,...) / gzprintf(gz,fmt,...) already useable
fwrite2(fh,ptr,len)
gzwrite2(gz,ptr,len)
*/

#define fputc2(fh,c) fputc(c,fh)
#define gzputc2(gz,c) gzputc(gz,c)

#define fputs2(fh,str) fputs(str,fh)
#define gzputs2(gz,str) gzputs(gz,str)

#define fwrite2(fh,ptr,len) fwrite(ptr,len,1,fh)
#define gzwrite2(gz,ptr,len) gzwrite(gz,ptr,len)

/*
 Output (buffered)

// TODO
fputc_buf(fh,buf,c)
gzputc_buf(gz,buf,c)
fputs_buf(fh,buf,str)
gzputs_buf(gz,buf,str)
fprintf_buf(fh,buf,fmt,...)
gzprintf_buf(gz,buf,fmt,...)
fwrite_buf(fh,buf,ptr,len)
gzwrite_buf(gz,buf,ptr,len)
strm_buf_flush(fh,buf)
strm_buf_gzflush(gz,buf)
*/


#endif


typedef enum
{
  SEQ_FMT_UNKNOWN = 0,
  SEQ_FMT_PLAIN = 1, SEQ_FMT_FASTA = 2, SEQ_FMT_FASTQ = 4,
  SEQ_FMT_SAM = 8, SEQ_FMT_BAM = 16, SEQ_FMT_CRAM = 16
} seq_format;

typedef struct seq_file_struct seq_file_t;
typedef struct read_struct read_t;

struct seq_file_struct
{
  char *path;
  FILE *f_file;
  gzFile gz_file;
  void *hts_file; // cast to (htsFile*)
  void *bam_hdr; // cast to (bam_hdr_t*)

  int (*readfunc)(seq_file_t *sf, read_t *r);
  StreamBuffer in;
  seq_format format;

  // Reads pushed onto a 'read stack' aka buffer
  read_t *rhead, *rtail; // 'unread' reads, add to tail, return from head
  int (*origreadfunc)(seq_file_t *sf, read_t *r); // used when read = _seq_read_pop
};

typedef struct {
  char *b;
  size_t end, size;
} seq_buf_t;

struct read_struct
{
  seq_buf_t name, seq, qual;
  void *bam; // cast to (bam1_t*) get/set with seq_read_bam()
  read_t *next; // for use in a linked list
  bool from_sam; // from sam or bam
};

#define seq_read_init {.name = {.b = NULL, .end = 0, .size = 0}, \
                       .seq  = {.b = NULL, .end = 0, .size = 0}, \
                       .qual = {.b = NULL, .end = 0, .size = 0}, \
                       .bam = NULL, .next = NULL, .from_sam = false}

#ifdef _USESAM
  #define seq_read_bam(r) ((bam1_t*)(r)->bam)
#endif

#define seq_is_cram(sf) ((sf)->format == SEQ_FMT_CRAM)
#define seq_is_bam(sf) ((sf)->format == SEQ_FMT_BAM)
#define seq_is_sam(sf) ((sf)->format == SEQ_FMT_SAM)
#define seq_use_gzip(sf) ((sf)->gz_file != NULL)

// The following require a read to have been read successfully first
// using seq_read
#define seq_is_fastq(sf) ((sf)->format == SEQ_FMT_FASTQ)
#define seq_is_fasta(sf) ((sf)->format == SEQ_FMT_FASTA)
#define seq_is_plain(sf) ((sf)->format == SEQ_FMT_PLAIN)

#define seq_get_path(sf) ((sf)->path)

// return 1 on success, 0 on eof, -1 if partially read / syntax error
#define seq_read(sf,r) ((sf)->readfunc(sf,r))

/**
 * Fetch a read that is not a secondary or supplementary alignment
 */
static inline int seq_read_primary(seq_file_t *sf, read_t *r)
{
  int s = seq_read(sf, r);
#ifdef _USESAM
  if(s > 0 && r->from_sam) {
    while(s > 0 && seq_read_bam(r)->core.flag & (BAM_FSECONDARY|BAM_FSUPPLEMENTARY))
      s = seq_read(sf, r);
  }
#endif
  return s;
}

static inline void seq_close(seq_file_t *sf);

// File format information (http://en.wikipedia.org/wiki/FASTQ_format)
static const char * const FASTQ_FORMATS[]
  = {"Sanger / Illumina 1.9+ (Phred+33)", // range: [0,71] "catch all / unknown"
     "Sanger (Phred+33)", // range: [0,40]
     "Solexa (Solexa+64)", // range: [-5,40]
     "Illumina 1.3+ (Phred+64)", // range: [0,40]
     "Illumina 1.5+ (Phred+64)", // range: [3,40]
     "Illumina 1.8+ (Phred+33)"}; // range: [0,41]

static const int FASTQ_MIN[6]    = { 33, 33, 59, 64, 67, 33};
static const int FASTQ_MAX[6]    = {126, 73,104,104,104, 74};
static const int FASTQ_OFFSET[6] = { 33, 33, 64, 64, 64, 33};

#define DEFAULT_BUFSIZE (1<<20)

//
// Create and destroy read structs
//

static inline void seq_read_reset(read_t *r) {
  r->name.end = r->seq.end = r->qual.end = 0;
  r->name.b[0] = r->seq.b[0] = r->qual.b[0] = '\0';
  r->from_sam = false;
}

static inline void seq_read_dealloc(read_t *r)
{
  free(r->name.b);
  free(r->seq.b);
  free(r->qual.b);
  #ifdef _USESAM
    bam_destroy1(r->bam);
  #endif
  memset(r, 0, sizeof(read_t));
}

static inline read_t* seq_read_alloc(read_t *r)
{
  memset(r, 0, sizeof(read_t));

  r->name.b = malloc(256);
  r->seq.b  = malloc(256);
  r->qual.b = malloc(256);
  r->name.size = r->seq.size = r->qual.size = 256;

  if(!r->name.b || !r->seq.b || !r->qual.b)
  {
    seq_read_dealloc(r);
    return NULL;
  }
  #ifdef _USESAM
    if((r->bam = bam_init1()) == NULL) {
      seq_read_dealloc(r);
      return NULL;
    }
  #endif

  return r;
}

static inline read_t* seq_read_new()
{
  read_t *r = calloc(1, sizeof(read_t));
  if(r == NULL) return NULL;
  if(seq_read_alloc(r) == NULL) { free(r); return NULL; }
  return r;
}

static inline void seq_read_free(read_t *r)
{
  seq_read_dealloc(r);
  free(r);
}

// file could be sam,bam,FASTA,FASTQ,txt (+gzip)

// Complement SAM/BAM bases
static const int8_t seq_comp_table[16] = { 0, 8, 4, 12, 2, 10, 9, 14,
                                           1, 6, 5, 13, 3, 11, 7, 15 };


#ifdef _USESAM
// Read a sam/bam file
static inline int _seq_read_sam(seq_file_t *sf, read_t *r)
{
  r->name.end = r->seq.end = r->qual.end = 0;
  r->name.b[0] = r->seq.b[0] = r->qual.b[0] = '\0';

  if(sam_read1(sf->hts_file, sf->bam_hdr, r->bam) < 0) return 0;

  const bam1_t *b = seq_read_bam(r);
  char *str = bam_get_qname(b);
  cbuf_append_str(&r->name.b, &r->name.end, &r->name.size, str, strlen(str));

  size_t qlen = (size_t)b->core.l_qseq;
  cbuf_capacity(&r->seq.b, &r->seq.end, qlen);
  cbuf_capacity(&r->qual.b, &r->qual.end, qlen);
  const uint8_t *bamseq = bam_get_seq(b);
  const uint8_t *bamqual = bam_get_qual(b);

  size_t i, j;
  if(bam_is_rev(b))
  {
    for(i = 0, j = qlen - 1; i < qlen; i++, j--)
    {
      int8_t c = bam_seqi(bamseq, j);
      r->seq.b[i] = seq_nt16_str[seq_comp_table[c]];
      r->qual.b[i] = (char)(33 + bamqual[j]);
    }
  }
  else
  {
    for(i = 0; i < qlen; i++)
    {
      int8_t c = bam_seqi(bamseq, i);
      r->seq.b[i] = seq_nt16_str[c];
      r->qual.b[i] = (char)(33 + bamqual[i]);
    }
  }

  r->seq.end = r->qual.end = qlen;
  r->seq.b[qlen] = r->qual.b[qlen] = '\0';
  r->from_sam = true;

  return 1;
}
#endif /* _USESAM */


#define _func_read_fastq(_read_fastq,__getc,__ungetc,__readline)               \
  static inline int _read_fastq(seq_file_t *sf, read_t *r)                     \
  {                                                                            \
    seq_read_reset(r);                                                         \
    int c = __getc(sf);                                                        \
                                                                               \
    if(c == -1) return 0;                                                      \
    if(c != '@' || __readline(sf, r->name) == 0) return -1;                    \
    cbuf_chomp(r->name.b, &r->name.end);                                       \
                                                                               \
    while((c = __getc(sf)) != '+') {                                           \
      if(c == -1) return -1;                                                   \
      if(c != '\r' && c != '\n') {                                             \
        cbuf_append_char(&r->seq.b, &r->seq.end, &r->seq.size, (char)c);       \
        if(__readline(sf, r->seq) == 0) return -1;                             \
        cbuf_chomp(r->seq.b, &r->seq.end);                                     \
      }                                                                        \
    }                                                                          \
    while((c = __getc(sf)) != -1 && c != '\n');                                \
    if(c == -1) return -1;                                                     \
    do {                                                                       \
      if(__readline(sf,r->qual) > 0) cbuf_chomp(r->qual.b, &r->qual.end);      \
      else return 1;                                                           \
    } while(r->qual.end < r->seq.end);                                         \
    while((c = __getc(sf)) != -1 && c != '@');                                 \
    __ungetc(sf, c);                                                           \
    return 1;                                                                  \
  }

#define _func_read_fasta(_read_fasta,__getc,__ungetc,__readline)               \
  static inline int _read_fasta(seq_file_t *sf, read_t *r)                     \
  {                                                                            \
    seq_read_reset(r);                                                         \
    int c = __getc(sf);                                                        \
                                                                               \
    if(c == -1) return 0;                                                      \
    if(c != '>' || __readline(sf, r->name) == 0) return -1;                    \
    cbuf_chomp(r->name.b, &r->name.end);                                       \
                                                                               \
    while((c = __getc(sf)) != '>') {                                           \
      if(c == -1) return 1;                                                    \
      if(c != '\r' && c != '\n') {                                             \
        cbuf_append_char(&r->seq.b, &r->seq.end, &r->seq.size, (char)c);       \
        long nread = (long)__readline(sf, r->seq);                             \
        cbuf_chomp(r->seq.b, &r->seq.end);                                     \
        if(nread <= 0) return 1;                                               \
      }                                                                        \
    }                                                                          \
    __ungetc(sf, c);                                                           \
    return 1;                                                                  \
  }


#define _func_read_plain(_read_plain,__getc,__readline,__skipline)             \
  static inline int _read_plain(seq_file_t *sf, read_t *r)                     \
  {                                                                            \
    int c;                                                                     \
    seq_read_reset(r);                                                         \
    while((c = __getc(sf)) != -1 && isspace(c)) if(c != '\n') __skipline(sf);  \
    if(c == -1) return 0;                                                      \
    cbuf_append_char(&r->seq.b, &r->seq.end, &r->seq.size, (char)c);           \
    __readline(sf, r->seq);                                                    \
    cbuf_chomp(r->seq.b, &r->seq.end);                                         \
    return 1;                                                                  \
  }

#define _func_read_unknown(_read_unknown,__getc,__ungetc,__skipline,__fastq,__fasta,__plain)\
  static inline int _read_unknown(seq_file_t *sf, read_t *r)                   \
  {                                                                            \
    int c;                                                                     \
    seq_read_reset(r);                                                         \
    while((c = __getc(sf)) != -1 && isspace(c)) if(c != '\n') __skipline(sf);  \
    if(c == -1) return 0;                                                      \
    if(c == '@') { sf->format = SEQ_FMT_FASTQ; sf->origreadfunc = __fastq; }   \
    else if(c == '>') { sf->format = SEQ_FMT_FASTA; sf->origreadfunc = __fasta;}\
    else { sf->format = SEQ_FMT_PLAIN; sf->origreadfunc = __plain; }           \
    __ungetc(sf, c);                                                           \
    return sf->origreadfunc(sf,r);                                             \
  }

#define _SF_SWAP(x,y) do { __typeof(x) _tmp = (x); (x) = (y); (y) = _tmp; } while(0)
#define _SF_MIN(x,y) ((x) < (y) ? (x) : (y))

// Remove a read from the stack
// Undefined behaviour if you have not previously called _seq_read_shift
static inline int _seq_read_pop(seq_file_t *sf, read_t *r)
{
  read_t *nxtread = sf->rhead;
  sf->rhead = sf->rhead->next;
  _SF_SWAP(*r, *nxtread);
  seq_read_free(nxtread);
  if(sf->rhead == NULL) {
    sf->readfunc = sf->origreadfunc;
    sf->rtail = NULL;
  }
  r->next = NULL;
  return 1;
}

// Add a read onto the read buffer (linked list FIFO)
static inline void _seq_read_shift(seq_file_t *sf, read_t *r)
{
  if(sf->rhead == NULL) {
    sf->readfunc = _seq_read_pop;
    sf->rhead = sf->rtail = r;
  }
  else {
    sf->rtail->next = r;
    sf->rtail = r;
  }
  r->next = NULL;
}

// Load reads until we have at least nbases loaded or we hit EOF
static inline void _seq_buffer_reads(seq_file_t *sf, size_t nbases)
{
  int (*readfunc)(seq_file_t *sf, read_t *r) = sf->origreadfunc;

  // Sum bases already in buffer
  read_t *r = sf->rhead;
  size_t currbases = 0;
  while(r != NULL) { currbases += r->seq.end; r = r->next; }

  while(currbases < nbases) {
    if((r = seq_read_new()) == NULL) {
      fprintf(stderr, "[%s:%i] Error out of memory\n", __FILE__, __LINE__);
      break;
    }
    if(readfunc(sf,r) <= 0) { seq_read_free(r); break; }
    currbases += r->seq.end;
    _seq_read_shift(sf, r);
  }
}

// perform reading on seq_file_t

// getc on seq_file_t
#define _sf_gzgetc(sf)              gzgetc((sf)->gz_file)
#define _sf_gzgetc_buf(sf)          gzgetc_buf((sf)->gz_file,&(sf)->in)
#define _sf_fgetc(sf)               fgetc((sf)->f_file)
#define _sf_fgetc_buf(sf)           fgetc_buf((sf)->f_file,&(sf)->in)

// ungetc on seq_file_t
#define _sf_gzungetc(sf,c)          gzungetc(c,(sf)->gz_file)
#define _sf_gzungetc_buf(sf,c)      ungetc_buf(c,&(sf)->in)
#define _sf_fungetc(sf,c)           fungetc(c,(sf)->f_file)
#define _sf_fungetc_buf(sf,c)       ungetc_buf(c,&(sf)->in)

// readline on seq_file_t using buffer into read
#define _sf_gzreadline(sf,buf)      gzreadline((sf)->gz_file,&(buf).b,&(buf).end,&(buf).size)
#define _sf_gzreadline_buf(sf,buf)  gzreadline_buf((sf)->gz_file,&(sf)->in,&(buf).b,&(buf).end,&(buf).size)
#define _sf_freadline(sf,buf)       freadline((sf)->f_file,&(buf).b,&(buf).end,&(buf).size)
#define _sf_freadline_buf(sf,buf)   freadline_buf((sf)->f_file,&(sf)->in,&(buf).b,&(buf).end,&(buf).size)

// skipline on seq_file_t
#define _sf_gzskipline(sf)          gzskipline((sf)->gz_file)
#define _sf_gzskipline_buf(sf)      gzskipline_buf((sf)->gz_file,&(sf)->in)
#define _sf_fskipline(sf)           fskipline((sf)->f_file)
#define _sf_fskipline_buf(sf)       fskipline_buf((sf)->f_file,&(sf)->in)

// Read FASTQ
_func_read_fastq(_seq_read_fastq_f,      _sf_fgetc,      _sf_fungetc,      _sf_freadline)
_func_read_fastq(_seq_read_fastq_gz,     _sf_gzgetc,     _sf_gzungetc,     _sf_gzreadline)
_func_read_fastq(_seq_read_fastq_f_buf,  _sf_fgetc_buf,  _sf_fungetc_buf,  _sf_freadline_buf)
_func_read_fastq(_seq_read_fastq_gz_buf, _sf_gzgetc_buf, _sf_gzungetc_buf, _sf_gzreadline_buf)

// Read FASTA
_func_read_fasta(_seq_read_fasta_f,      _sf_fgetc,      _sf_fungetc,      _sf_freadline)
_func_read_fasta(_seq_read_fasta_gz,     _sf_gzgetc,     _sf_gzungetc,     _sf_gzreadline)
_func_read_fasta(_seq_read_fasta_f_buf,  _sf_fgetc_buf,  _sf_fungetc_buf,  _sf_freadline_buf)
_func_read_fasta(_seq_read_fasta_gz_buf, _sf_gzgetc_buf, _sf_gzungetc_buf, _sf_gzreadline_buf)

// Read plain
_func_read_plain(_seq_read_plain_f,      _sf_fgetc,      _sf_freadline,      _sf_fskipline)
_func_read_plain(_seq_read_plain_gz,     _sf_gzgetc,     _sf_gzreadline,     _sf_gzskipline)
_func_read_plain(_seq_read_plain_f_buf,  _sf_fgetc_buf,  _sf_freadline_buf,  _sf_fskipline_buf)
_func_read_plain(_seq_read_plain_gz_buf, _sf_gzgetc_buf, _sf_gzreadline_buf, _sf_gzskipline_buf)

// Read first entry
_func_read_unknown(_seq_read_unknown_f,      _sf_fgetc,      _sf_fungetc,      _sf_fskipline,  _seq_read_fastq_f,      _seq_read_fasta_f,      _seq_read_plain_f)
_func_read_unknown(_seq_read_unknown_gz,     _sf_gzgetc,     _sf_gzungetc,     _sf_gzskipline, _seq_read_fastq_gz,     _seq_read_fasta_gz,     _seq_read_plain_gz)
_func_read_unknown(_seq_read_unknown_f_buf,  _sf_fgetc_buf,  _sf_fungetc_buf,  _sf_fskipline,  _seq_read_fastq_f_buf,  _seq_read_fasta_f_buf,  _seq_read_plain_f_buf)
_func_read_unknown(_seq_read_unknown_gz_buf, _sf_gzgetc_buf, _sf_gzungetc_buf, _sf_gzskipline, _seq_read_fastq_gz_buf, _seq_read_fasta_gz_buf, _seq_read_plain_gz_buf)

// Returns 1 on success 0 if out of memory
static inline char _seq_setup(seq_file_t *sf, bool use_zlib, size_t buf_size)
{
  if(buf_size) {
    if(!strm_buf_alloc(&sf->in, buf_size)) { free(sf); return 0; }
    sf->origreadfunc = use_zlib ? _seq_read_unknown_gz_buf : _seq_read_unknown_f_buf;
  }
  else sf->origreadfunc = use_zlib ? _seq_read_unknown_gz : _seq_read_unknown_f;
  sf->readfunc = sf->origreadfunc;
  return 1;
}

#define NUM_SEQ_EXT 29

// Guess file type from file path or contents
static inline seq_format seq_guess_filetype_from_extension(const char *path)
{
  size_t plen = strlen(path);
  const char *exts[NUM_SEQ_EXT]
    = {".fa", ".fasta", ".fsa", ".fsa.gz", "fsa.gzip", // FASTA
       ".faz", ".fagz", ".fa.gz", ".fa.gzip", ".fastaz", ".fasta.gzip",
       ".fq", ".fastq", ".fsq", ".fsq.gz", "fsq.gzip", // FASTQ
       ".fqz", ".fqgz", ".fq.gz", ".fq.gzip", ".fastqz", ".fastq.gzip",
       ".txt", ".txtgz", ".txt.gz", ".txt.gzip", // Plain
       ".sam", ".bam", ".cram"}; // SAM / BAM / CRAM

  const seq_format types[NUM_SEQ_EXT]
    = {SEQ_FMT_FASTA, SEQ_FMT_FASTA, SEQ_FMT_FASTA, SEQ_FMT_FASTA, SEQ_FMT_FASTA,
       SEQ_FMT_FASTA, SEQ_FMT_FASTA, SEQ_FMT_FASTA, SEQ_FMT_FASTA, SEQ_FMT_FASTA,
       SEQ_FMT_FASTA,
       SEQ_FMT_FASTQ, SEQ_FMT_FASTQ, SEQ_FMT_FASTQ, SEQ_FMT_FASTQ, SEQ_FMT_FASTQ,
       SEQ_FMT_FASTQ, SEQ_FMT_FASTQ, SEQ_FMT_FASTQ, SEQ_FMT_FASTQ, SEQ_FMT_FASTQ,
       SEQ_FMT_FASTQ,
       SEQ_FMT_PLAIN, SEQ_FMT_PLAIN, SEQ_FMT_PLAIN, SEQ_FMT_PLAIN,
       SEQ_FMT_SAM, SEQ_FMT_BAM, SEQ_FMT_CRAM};

  size_t extlens[NUM_SEQ_EXT];
  size_t i;
  for(i = 0; i < NUM_SEQ_EXT; i++)
    extlens[i] = strlen(exts[i]);

  for(i = 0; i < NUM_SEQ_EXT; i++)
    if(extlens[i] <= plen && strcasecmp(path+plen-extlens[i], exts[i]) == 0)
      return types[i];

  return SEQ_FMT_UNKNOWN;
}

#undef NUM_SEQ_EXT

static inline seq_file_t* seq_open2(const char *p, bool ishts,
                                    bool use_zlib, size_t buf_size)
{
  seq_file_t *sf = calloc(1, sizeof(seq_file_t));
  sf->path = strdup(p);

  if(ishts)
  {
    #ifdef _USESAM
      if((sf->hts_file = hts_open(p, "r")) == NULL) {
        seq_close(sf);
        return NULL;
      }
      sf->bam_hdr = sam_hdr_read(sf->hts_file);
      sf->readfunc = sf->origreadfunc = _seq_read_sam;

      const htsFormat *hts_fmt = hts_get_format(sf->hts_file);
      if(hts_fmt->format == sam) sf->format = SEQ_FMT_SAM;
      else if(hts_fmt->format == bam) sf->format = SEQ_FMT_BAM;
      else if(hts_fmt->format == cram) sf->format = SEQ_FMT_CRAM;
      else {
        fprintf(stderr, "[%s:%i] Cannot identify hts file format\n",
                __FILE__, __LINE__);
        exit(EXIT_FAILURE);
      }
    #else
      fprintf(stderr, "[%s:%i] Error: not compiled with sam/bam support\n",
              __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    #endif
  }
  else
  {
    if(( use_zlib && ((sf->gz_file = gzopen(p, "r")) == NULL)) ||
       (!use_zlib && ((sf->f_file  =  fopen(p, "r")) == NULL))) {
      seq_close(sf);
      return NULL;
    }

    if(!_seq_setup(sf, use_zlib, buf_size)) return NULL;
  }

  return sf;
}

// Returns pointer to new seq_file_t on success, seq_close will close the fh,
// so you shouldn't call fclose(fh)
// Returns NULL on error, in which case FILE will not have been closed (caller
// should then call fclose(fh))
static inline seq_file_t* seq_dopen(int fd, char ishts,
                                    bool use_zlib, size_t buf_size)
{
  seq_file_t *sf = calloc(1, sizeof(seq_file_t));
  sf->path = strdup("-");

  if(ishts)
  {
    #ifdef _USESAM
      if((sf->hts_file = hts_hopen(hdopen(fd, "r"), sf->path, "r")) == NULL) {
        seq_close(sf);
        return NULL;
      }
      sf->bam_hdr = sam_hdr_read(sf->hts_file);
      sf->readfunc = sf->origreadfunc = _seq_read_sam;

      const htsFormat *hts_fmt = hts_get_format(sf->hts_file);
      if(hts_fmt->format == sam) sf->format = SEQ_FMT_SAM;
      else if(hts_fmt->format == bam) sf->format = SEQ_FMT_BAM;
      else if(hts_fmt->format == cram) sf->format = SEQ_FMT_CRAM;
      else {
        fprintf(stderr, "[%s:%i] Cannot identify hts file format\n",
                __FILE__, __LINE__);
        exit(EXIT_FAILURE);
      }
    #else
      fprintf(stderr, "[%s:%i] Error: not compiled with sam/bam support\n",
              __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    #endif
  }
  else
  {
    if((!use_zlib && (sf->f_file  = fdopen(fd,  "r")) == NULL) ||
       ( use_zlib && (sf->gz_file = gzdopen(fd, "r")) == NULL)) {
      seq_close(sf);
      return NULL;
    }

    if(!_seq_setup(sf, use_zlib, buf_size)) return NULL;
  }

  return sf;
}

static inline seq_file_t* seq_open(const char *p)
{
  assert(p != NULL);
  if(strcmp(p,"-") == 0) return seq_dopen(fileno(stdin), 0, 1, 0);

  seq_format fmt = seq_guess_filetype_from_extension(p);
  bool ishts = (fmt == SEQ_FMT_SAM || fmt == SEQ_FMT_BAM || fmt == SEQ_FMT_CRAM);
  return seq_open2(p, ishts, 1, DEFAULT_BUFSIZE);
}

/*
// returns 0 on success, -1 on failure
static inline int seq_seek_start(seq_file_t *sf)
{
  strm_buf_reset(&sf->in);
  if((sf->f_file  != NULL &&  fseek(sf->f_file,  0, SEEK_SET) != 0) ||
     (sf->gz_file != NULL && gzseek(sf->gz_file, 0, SEEK_SET) != 0))
  {
    fprintf(stderr, "%s failed: %s\n", sf->f_file ? "fseek" : "gzseek", sf->path);
    return -1;
  } else if(sf->f_file == NULL && sf->gz_file == NULL) {
    fprintf(stderr, "Cannot currently reopen sam/bam files");
    return -1;
  }
  return 0;
}
*/

// Close file handles, free resources
static inline void seq_close(seq_file_t *sf)
{
  int e;
  if(sf->f_file != NULL && (e = fclose(sf->f_file)) != 0) {
    fprintf(stderr, "[%s:%i] Error closing file: %s [%i]\n", __FILE__, __LINE__,
                    sf->path, e);
  }
  if(sf->gz_file != NULL && (e = gzclose(sf->gz_file)) != Z_OK) {
    fprintf(stderr, "[%s:%i] Error closing gzfile: %s [%i]\n", __FILE__, __LINE__,
                    sf->path, e);
  }
  #ifdef _USESAM
    if(sf->hts_file != NULL)  hts_close(sf->hts_file);
    bam_hdr_destroy(sf->bam_hdr);
  #endif
  strm_buf_dealloc(&sf->in);
  free(sf->path);
  read_t *r = sf->rhead, *tmpr;
  while(r != NULL) { tmpr = r->next; seq_read_free(r); r = tmpr; }
  memset(sf, 0, sizeof(*sf));
  free(sf);
}

static inline seq_file_t* seq_reopen(seq_file_t *sf)
{
  char *path = strdup(sf->path);
  seq_close(sf);
  sf = seq_open(path);
  free(path);
  return sf;
}

// Get min/max qual scores by reading sequences into buffer and reporting min/max
// Returns 0 if no qual scores, 1 on success, -1 if read error
static inline int seq_get_qual_limits(seq_file_t *sf, int *minq, int *maxq)
{
  read_t *r;
  int min = INT_MAX, max = 0;
  size_t count = 0, qcount = 0, limit = 1000, len;
  const char *str, *end;

  _seq_buffer_reads(sf, limit);
  r = sf->rhead;

  while(count < limit && r != NULL)
  {
    len = _SF_MIN(r->qual.end, limit - qcount);
    for(str = r->qual.b, end = str + len; str < end; str++) {
      if(*str > max) max = *str;
      if(*str < min) min = *str;
    }
    count += r->seq.end;
    qcount += r->qual.end;
    r = r->next;
  }

  if(qcount > 0) { *minq = min; *maxq = max; }

  return (qcount > 0);
}

// Returns -1 on error
// Returns 0 if not in FASTQ format/ not recognisable (offset:33, min:33, max:104)
// max_read_bases is the number of quality scores to read
static inline int seq_guess_fastq_format(seq_file_t *sf, int *minq, int *maxq)
{
  // Detect fastq offset
  *minq = INT_MAX;
  *maxq = 0;

  if(seq_get_qual_limits(sf, minq, maxq) <= 0) return -1;

  // See: http://en.wikipedia.org/wiki/FASTQ_format
  // Usually expect 0,40, but new software can report 41, so using <= MAX+1
  if(*minq >= 33 && *maxq <= 73) return 1; // sanger
  else if(*minq >= 33 && *maxq <= 75) return 5; // Illumina 1.8+
  else if(*minq >= 67 && *maxq <= 105) return 4; // Illumina 1.5+
  else if(*minq >= 64 && *maxq <= 105) return 3; // Illumina 1.3+
  else if(*minq >= 59 && *maxq <= 105) return 2; // Solexa
  else return 0; // Unknown, assume 33 offset max value 104
}

// Returns 1 if valid, 0 otherwise
static inline char _seq_read_looks_valid(read_t *r, const char *alphabet)
{
  char valid[128] = {0};
  for(; *alphabet; alphabet++) valid[(unsigned int)*alphabet] = 1;
  size_t i;
  unsigned int b, q;

  if(r->qual.end != 0) {
    if(r->qual.end != r->seq.end) return 0;
    for(i = 0; i < r->seq.end; i++) {
      b = tolower(r->seq.b[i]);
      q = r->qual.b[i];
      if(b >= 128 || !valid[b] || q < 33 || q > 105) return 0;
    }
  }
  else {
    for(i = 0; i < r->seq.end; i++) {
      b = (char)tolower(r->seq.b[i]);
      if(b >= 128 || !valid[b]) return 0;
    }
  }
  return 1;
}

// Returns 1 if valid, 0 otherwise
#define seq_read_looks_valid_dna(r) _seq_read_looks_valid(r,"acgtn")
#define seq_read_looks_valid_rna(r) _seq_read_looks_valid(r,"acgun")
#define seq_read_looks_valid_protein(r) \
        _seq_read_looks_valid(r,"acdefghiklmnopqrstuvwy")

static inline char seq_char_complement(char c) {
  switch(c) {
    case 'a': return 't'; case 'A': return 'T';
    case 'c': return 'g'; case 'C': return 'G';
    case 'g': return 'c'; case 'G': return 'C';
    case 't': return 'a'; case 'T': return 'A';
    default: return c;
  }
}

// Force quality score length to match seq length
static inline void _seq_read_force_qual_seq_lmatch(read_t *r)
{
  size_t i;
  if(r->qual.end < r->seq.end) {
    cbuf_capacity(&r->qual.b, &r->qual.end, r->seq.end);
    for(i = r->qual.end; i < r->seq.end; i++) r->qual.b[i] = '.';
  }
  r->qual.b[r->qual.end = r->seq.end] = '\0';
}

static inline void seq_read_reverse(read_t *r)
{
  size_t i, j;
  if(r->qual.end > 0) _seq_read_force_qual_seq_lmatch(r);
  if(r->seq.end <= 1) return;

  for(i=0, j=r->seq.end-1; i <= j; i++, j--)
    _SF_SWAP(r->seq.b[i], r->seq.b[j]);

  if(r->qual.end > 0) {
    for(i=0, j=r->qual.end-1; i <= j; i++, j--)
      _SF_SWAP(r->qual.b[i], r->qual.b[j]);
  }
}

static inline void seq_read_complement(read_t *r)
{
  size_t i;
  for(i=0; i < r->seq.end; i++)
    r->seq.b[i] = seq_char_complement(r->seq.b[i]);
}

static inline void seq_read_reverse_complement(read_t *r)
{
  size_t i, j;
  char swap;

  if(r->qual.end > 0) _seq_read_force_qual_seq_lmatch(r);
  if(r->seq.end == 0) return;
  if(r->seq.end == 1){ r->seq.b[0] = seq_char_complement(r->seq.b[0]); return; }

  for(i=0, j=r->seq.end-1; i <= j; i++, j--) {
    swap = r->seq.b[i];
    r->seq.b[i] = seq_char_complement(r->seq.b[j]);
    r->seq.b[j] = seq_char_complement(swap);
  }

  if(r->qual.end > 0) {
    for(i=0, j=r->qual.end-1; i <= j; i++, j--)
      _SF_SWAP(r->qual.b[i], r->qual.b[j]);
  }
}

#define SNAME_END(c) (!(c) || isspace(c))

// Compare read names up to first whitespace / end of string.
// Returns 0 if they match or match with /1 /2 appended
static inline int seq_read_names_cmp(const char *aa, const char *bb)
{
  const unsigned char *a = (const unsigned char*)aa, *b = (const unsigned char*)bb;
  const unsigned char *start_a = a, *start_b = b;

  // Both match until end of string, or whitespace
  while(*a && *b && *a == *b && !isspace(*a)) { a++; b++; }

  // Special case '/1' '/2'
  if(a > start_a && b > start_b && *(a-1) == '/' && *(b-1) == '/' &&
     ((*a == '1' && *b == '2') || (*a == '2' && *b == '1')) &&
     SNAME_END(a[1]) && SNAME_END(b[1])) {
    return 0;
  }

  // One or both of the strings ended
  return SNAME_END(*a) && SNAME_END(*b) ? 0 : (int)*a - *b;
}

#undef SNAME_END

// Formally, FASTA/Q entry names stop at the first space character
// Truncates read name and returns new length
static inline size_t seq_read_truncate_name(read_t *r)
{
  char *tmp = r->name.b;
  size_t len;
  for(len = 0; tmp[len] != '\0' && !isspace(tmp[len]); len++) {}
  r->name.b[r->name.end = len] = '\0';
  return len;
}

static inline void seq_read_to_uppercase(read_t *r)
{
  char *tmp;
  for(tmp = r->seq.b; *tmp != '\0'; tmp++) *tmp = (char)toupper(*tmp);
}

static inline void seq_read_to_lowercase(read_t *r)
{
  char *tmp;
  for(tmp = r->seq.b; *tmp != '\0'; tmp++) *tmp = (char)tolower(*tmp);
}

// j is newline counter
#define _seq_print_wrap(fh,str,len,wrap,j,_putc) do \
{ \
  size_t _i; \
  for(_i = 0; _i < len; _i++, j++) { \
    if(j == wrap) { _putc((fh), '\n'); j = 0; } \
    _putc((fh), str[_i]); \
  } \
} while(0)

// These functions return -1 on error or 0 otherwise
#define _seq_print_fasta(fname,ftype,_puts,_putc)                              \
  static inline int fname(const read_t *r, ftype fh, size_t linewrap) {        \
    size_t j = 0;                                                              \
    _putc(fh, '>');                                                            \
    _puts(fh, r->name.b);                                                      \
    _putc(fh, '\n');                                                           \
    if(linewrap == 0) _puts(fh, r->seq.b);                                     \
    else _seq_print_wrap(fh, r->seq.b, r->seq.end, linewrap, j, _putc);        \
    return _putc(fh, '\n') == '\n' ? 0 : -1;                                   \
  }                                                                            \

_seq_print_fasta(seq_print_fasta,FILE*,fputs2,fputc2)
_seq_print_fasta(seq_gzprint_fasta,gzFile,gzputs2,gzputc2)

// These functions return -1 on error or 0 otherwise
#define _seq_print_fastq(fname,ftype,_puts,_putc)                              \
  static inline int fname(const read_t *r, ftype fh, size_t linewrap) {        \
    _putc(fh, '@');                                                            \
    _puts(fh, r->name.b);                                                      \
    _putc(fh, '\n');                                                           \
    size_t i, j = 0, qlimit = _SF_MIN(r->qual.end, r->seq.end);                \
    if(linewrap == 0) {                                                        \
      _puts(fh, r->seq.b);                                                     \
      _puts(fh, "\n+\n");                                                      \
      for(i = 0;      i < qlimit;      i++) { _putc(fh, r->qual.b[i]); }       \
      for(i = qlimit; i < r->seq.end;  i++) { _putc(fh, '.'); }                \
    }                                                                          \
    else {                                                                     \
      _seq_print_wrap(fh, r->seq.b, r->seq.end, linewrap, j, _putc);           \
      _puts(fh, "\n+\n");                                                      \
      j=0; /* reset j after printing new line */                               \
      _seq_print_wrap(fh, r->qual.b, qlimit, linewrap, j, _putc);              \
      /* If i < seq.end, pad quality scores */                                 \
      for(i = qlimit; i < r->seq.end; i++, j++) {                              \
        if(j == linewrap) { _putc(fh, '\n'); j = 0; }                          \
        _putc(fh, '.');                                                        \
      }                                                                        \
    }                                                                          \
    return _putc(fh, '\n') == '\n' ? 0 : -1;                                   \
  }

_seq_print_fastq(seq_print_fastq,FILE*,fputs2,fputc2)
_seq_print_fastq(seq_gzprint_fastq,gzFile,gzputs2,gzputc2)

#undef DEFAULT_BUFSIZE
#undef _SF_SWAP
#undef _SF_MIN
#undef _sf_gzgetc
#undef _sf_gzgetc_buf
#undef _sf_fgetc
#undef _sf_fgetc_buf
#undef _sf_gzungetc
#undef _sf_gzungetc_buf
#undef _sf_fungetc
#undef _sf_fungetc_buf
#undef _sf_gzreadline
#undef _sf_gzreadline_buf
#undef _sf_freadline
#undef _sf_freadline_buf
#undef _sf_gzskipline
#undef _sf_gzskipline_buf
#undef _sf_fskipline
#undef _sf_fskipline_buf
#undef _seq_print_wrap
#undef _seq_print_fasta
#undef _seq_print_fastq

// New read on the stack
// read_t* seq_read_alloc(read_t*)
// seq_read_dealloc(read_t* r)

// New read on the heap
// read_t* seq_read_new()
// seq_read_free(read_t* r)

// seq_open(path)
// seq_open2(path,ishts,use_gzip,buffer_size)
// seq_dopen(fileno(fh),use_gzip,buffer_size)
// seq_close(seq_file_t *sf)

#endif


// Alignment scoring and loading
// #include "alignment_scoring_load.h"
// #include "alignment_cmdline.h"


// #include "alignment_macros.h"

// #include "smith_waterman.h"
/*
 smith_waterman.h
 url: https://github.com/noporpoise/seq-align
 maintainer: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain, no warranty
 date: Nov 2013
 */

#ifndef SMITH_WATERMAN_HEADER_SEEN
#define SMITH_WATERMAN_HEADER_SEEN

// #include "seq_align.h"
#define SEQ_ALIGN_VERSION_STR  "1.0.0"
#define SEQ_ALIGN_VERSION      0x100

// #include "alignment.h"
/*
 alignment.h
 author: Isaac Turner <turner.isaac@gmail.com>
 url: https://github.com/noporpoise/seq-align
 May 2013
 */

#ifndef ALIGNMENT_HEADER_SEEN
#define ALIGNMENT_HEADER_SEEN

#include <string.h> // memset
// #include "alignment_scoring_load.h"
/*
 alignment_scoring_load.h
 url: https://github.com/noporpoise/seq-align
 maintainer: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain, no warranty
 date: Nov 2013
 */

#ifndef ALIGNMENT_SCORING_LOAD_HEADER_SEEN
#define ALIGNMENT_SCORING_LOAD_HEADER_SEEN

// #include "alignment_scoring.h"
/*
 alignment_scoring.h
 url: https://github.com/noporpoise/seq-align
 maintainer: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain, no warranty
 date: Nov 2013
 */

#ifndef ALIGNMENT_SCORING_HEADER_SEEN
#define ALIGNMENT_SCORING_HEADER_SEEN

#include <inttypes.h>
#include <stdbool.h>
#include <limits.h> // INT_MIN

typedef int score_t;
#define SCORE_MIN INT_MIN

typedef struct
{
  int gap_open, gap_extend;

  // Needleman Wunsch only
  // Turn these on to turn off penalties for gaps at the start/end of alignment
  bool no_start_gap_penalty, no_end_gap_penalty;

  // Turn at most one of these on at a time to prevent gaps/mismatches
  bool no_gaps_in_a, no_gaps_in_b, no_mismatches;

  // If swap_score not set, should we use match/mismatch values?
  bool use_match_mismatch;
  int match, mismatch;

  bool case_sensitive;

  // Array of characters that match to everything with the same penalty (i.e. 'N's)
  uint32_t wildcards[256/32], swap_set[256][256/32];
  score_t wildscores[256], swap_scores[256][256];
  int min_penalty, max_penalty; // min, max {match/mismatch,gapopen etc.}
} scoring_t;

#ifndef bitset32_get
  #define bitset32_get(arr,idx)   (((arr)[(idx)>>5] >> ((idx)&31)) & 0x1)
  #define bitset32_set(arr,idx)   ((arr)[(idx)>>5] |=   (1<<((idx)&31)))
  #define bitset32_clear(arr,idx) ((arr)[(idx)>>5] &=  ~(1<<((idx)&31)))
#endif

#define get_wildcard_bit(scoring,c) bitset32_get((scoring)->wildcards,c)
#define set_wildcard_bit(scoring,c) bitset32_set((scoring)->wildcards,c)

#define get_swap_bit(scoring,a,b) bitset32_get((scoring)->swap_set[(size_t)(a)],b)
#define set_swap_bit(scoring,a,b) bitset32_set((scoring)->swap_set[(size_t)(a)],b)


void scoring_init(scoring_t* scoring, int match, int mismatch,
                  int gap_open, int gap_extend,
                  bool no_start_gap_penalty, bool no_end_gap_penalty,
                  bool no_gaps_in_a, bool no_gaps_in_b,
                  bool no_mismatches, bool case_sensitive);

#define scoring_is_wildcard(scoring,c) (get_wildcard_bit(scoring,c))

void scoring_add_wildcard(scoring_t* scoring, char c, int s);

void scoring_add_mutation(scoring_t* scoring, char a, char b, int score);

void scoring_print(const scoring_t* scoring);

void scoring_lookup(const scoring_t* scoring, char a, char b,
                    int *score, bool *is_match);

// Some scoring systems
void scoring_system_PAM30(scoring_t *scoring);
void scoring_system_PAM70(scoring_t *scoring);
void scoring_system_BLOSUM80(scoring_t *scoring);
void scoring_system_BLOSUM62(scoring_t *scoring);
void scoring_system_DNA_hybridization(scoring_t *scoring);
void scoring_system_default(scoring_t *scoring); // DNA/RNA default

/*
 alignment_scoring.c
 url: https://github.com/noporpoise/seq-align
 maintainer: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain, no warranty
 date: Nov 2013
 */

// Turn on debugging output by defining SEQ_ALIGN_VERBOSE
//#define SEQ_ALIGN_VERBOSE

#include <stdlib.h>
#include <stdio.h>
#include <limits.h> // INT_MAX
#include <string.h> // memset
#include <ctype.h> // tolower

void scoring_init(scoring_t* scoring,
                  int match, int mismatch,
                  int gap_open, int gap_extend,
                  bool no_start_gap_penalty, bool no_end_gap_penalty,
                  bool no_gaps_in_a, bool no_gaps_in_b,
                  bool no_mismatches, bool case_sensitive)
{
  // Gap of length 1 has penalty (gap_open+gap_extend)
  // of length N: (gap_open + gap_extend*N)
  scoring->gap_open = gap_open;
  scoring->gap_extend = gap_extend;

  scoring->no_start_gap_penalty = no_start_gap_penalty;
  scoring->no_end_gap_penalty = no_end_gap_penalty;

  scoring->no_gaps_in_a = no_gaps_in_a;
  scoring->no_gaps_in_b = no_gaps_in_b;
  scoring->no_mismatches = no_mismatches;

  scoring->use_match_mismatch = 1;
  scoring->match = match;
  scoring->mismatch = mismatch;

  scoring->case_sensitive = case_sensitive;

  memset(scoring->wildcards, 0, sizeof(scoring->wildcards));
  memset(scoring->swap_set, 0, sizeof(scoring->swap_set));

  scoring->min_penalty = MIN2(match, mismatch);
  scoring->max_penalty = MAX2(match, mismatch);
  if(!no_gaps_in_a || !no_gaps_in_b) {
    scoring->min_penalty = MIN3(scoring->min_penalty,gap_open+gap_extend,gap_extend);
    scoring->max_penalty = MAX3(scoring->max_penalty,gap_open+gap_extend,gap_extend);
  }
}

void scoring_add_wildcard(scoring_t* scoring, char c, int score)
{
  if(!scoring->case_sensitive) c = tolower(c);
  set_wildcard_bit(scoring,c);
  scoring->wildscores[(size_t)c] = score;
  scoring->min_penalty = MIN2(scoring->min_penalty, score);
  scoring->max_penalty = MAX2(scoring->max_penalty, score);
}

void scoring_add_mutation(scoring_t* scoring, char a, char b, int score)
{
  scoring->swap_scores[(size_t)a][(size_t)b] = score;
  set_swap_bit(scoring,a,b);
  scoring->min_penalty = MIN2(scoring->min_penalty, score);
  scoring->max_penalty = MAX2(scoring->max_penalty, score);
}

void scoring_add_mutations(scoring_t* scoring, const char *str, const int *scores,
                           char use_match_mismatch)
{
  size_t i, j, len = strlen(str);
  char a, b;
  int score;

  for(i = 0; i < len; i++)
  {
    a = scoring->case_sensitive ? str[i] : tolower(str[i]);

    for(j = 0; j < len; j++)
    {
      b = scoring->case_sensitive ? str[j] : tolower(str[j]);
      score = ARR_LOOKUP(scores, len, i, j);

      scoring_add_mutation(scoring, a, b, score);
    }
  }

  scoring->use_match_mismatch = use_match_mismatch;
}

void scoring_print(const scoring_t* scoring)
{
  printf("scoring:\n");
  printf("  match: %i; mismatch: %i; (use_match_mismatch: %i)\n",
         scoring->match, scoring->mismatch, scoring->use_match_mismatch);

  printf("  gap_open: %i; gap_extend: %i;\n",
         scoring->gap_open, scoring->gap_extend);

  printf("  no_gaps_in_a: %i; no_gaps_in_b: %i; no_mismatches: %i;\n",
         scoring->no_gaps_in_a, scoring->no_gaps_in_b, scoring->no_mismatches);

  printf("  no_start_gap_penalty: %i; no_end_gap_penalty: %i;\n",
         scoring->no_start_gap_penalty, scoring->no_end_gap_penalty);
}


// a, b must be lowercase if !scoring->case_sensitive
static char _scoring_check_wildcards(const scoring_t* scoring, char a, char b,
                                     int* score)
{
  // Check if either characters are wildcards
  int tmp_score = INT_MAX;
  if(get_wildcard_bit(scoring,a)) tmp_score = scoring->wildscores[(size_t)a];
  if(get_wildcard_bit(scoring,b)) tmp_score = MIN2(scoring->wildscores[(size_t)b],tmp_score);
  if(tmp_score != INT_MAX) {
    *score = tmp_score;
    return 1;
  }

  *score = 0;
  return 0;
}

// Considered match if lc(a)==lc(b) or if a or b are wildcards
// Always sets score and is_match
void scoring_lookup(const scoring_t* scoring, char a, char b,
                    int *score, bool *is_match)
{
  if(!scoring->case_sensitive)
  {
    a = tolower(a);
    b = tolower(b);
  }

  //#ifdef SEQ_ALIGN_VERBOSE
  //printf(" scoring_lookup(%c,%c)\n", a, b);
  //#endif

  *is_match = (a == b);

  if(scoring->no_mismatches && !*is_match)
  {
    // Check wildcards
    *is_match = _scoring_check_wildcards(scoring, a, b, score);
    return;
  }

  // Look up in table
  if(get_swap_bit(scoring,a,b))
  {
    *score = scoring->swap_scores[(size_t)a][(size_t)b];
    return;
  }

  // Check wildcards
  // Wildcards are used in the order they are given
  // e.g. if we specify '--wildcard X 2 --wildcard Y 3' X:Y align with score 2
  if(_scoring_check_wildcards(scoring, a, b, score))
  {
    *is_match = 1;
    return;
  }

  // Use match/mismatch
  if(scoring->use_match_mismatch)
  {
    *score = (*is_match ? scoring->match : scoring->mismatch);
    return;
  }

  // Error
  fprintf(stderr, "Error: Unknown character pair (%c,%c) and "
                   "match/mismatch have not been set\n", a, b);
  exit(EXIT_FAILURE);
}

//
// Some scoring systems
//

static char amino_acids[] = "ARNDCQEGHILKMFPSTWYVBZX*";

static const int pam30[576] =
{ 6, -7, -4, -3, -6, -4, -2, -2, -7, -5, -6, -7, -5, -8, -2,  0, -1,-13, -8, -2, -3, -3, -3,-17,
 -7,  8, -6,-10, -8, -2, -9, -9, -2, -5, -8,  0, -4, -9, -4, -3, -6, -2,-10, -8, -7, -4, -6,-17,
 -4, -6,  8,  2,-11, -3, -2, -3,  0, -5, -7, -1, -9, -9, -6,  0, -2, -8, -4, -8,  6, -3, -3,-17,
 -3,-10,  2,  8,-14, -2,  2, -3, -4, -7,-12, -4,-11,-15, -8, -4, -5,-15,-11, -8,  6,  1, -5,-17,
 -6, -8,-11,-14, 10,-14,-14, -9, -7, -6,-15,-14,-13,-13, -8, -3, -8,-15, -4, -6,-12,-14, -9,-17,
 -4, -2, -3, -2,-14,  8,  1, -7,  1, -8, -5, -3, -4,-13, -3, -5, -5,-13,-12, -7, -3,  6, -5,-17,
 -2, -9, -2,  2,-14,  1,  8, -4, -5, -5, -9, -4, -7,-14, -5, -4, -6,-17, -8, -6,  1,  6, -5,-17,
 -2, -9, -3, -3, -9, -7, -4,  6, -9,-11,-10, -7, -8, -9, -6, -2, -6,-15,-14, -5, -3, -5, -5,-17,
 -7, -2,  0, -4, -7,  1, -5, -9,  9, -9, -6, -6,-10, -6, -4, -6, -7, -7, -3, -6, -1, -1, -5,-17,
 -5, -5, -5, -7, -6, -8, -5,-11, -9,  8, -1, -6, -1, -2, -8, -7, -2,-14, -6,  2, -6, -6, -5,-17,
 -6, -8, -7,-12,-15, -5, -9,-10, -6, -1,  7, -8,  1, -3, -7, -8, -7, -6, -7, -2, -9, -7, -6,-17,
 -7,  0, -1, -4,-14, -3, -4, -7, -6, -6, -8,  7, -2,-14, -6, -4, -3,-12, -9, -9, -2, -4, -5,-17,
 -5, -4, -9,-11,-13, -4, -7, -8,-10, -1,  1, -2, 11, -4, -8, -5, -4,-13,-11, -1,-10, -5, -5,-17,
 -8, -9, -9,-15,-13,-13,-14, -9, -6, -2, -3,-14, -4,  9,-10, -6, -9, -4,  2, -8,-10,-13, -8,-17,
 -2, -4, -6, -8, -8, -3, -5, -6, -4, -8, -7, -6, -8,-10,  8, -2, -4,-14,-13, -6, -7, -4, -5,-17,
  0, -3,  0, -4, -3, -5, -4, -2, -6, -7, -8, -4, -5, -6, -2,  6,  0, -5, -7, -6, -1, -5, -3,-17,
 -1, -6, -2, -5, -8, -5, -6, -6, -7, -2, -7, -3, -4, -9, -4,  0,  7,-13, -6, -3, -3, -6, -4,-17,
-13, -2, -8,-15,-15,-13,-17,-15, -7,-14, -6,-12,-13, -4,-14, -5,-13, 13, -5,-15,-10,-14,-11,-17,
 -8,-10, -4,-11, -4,-12, -8,-14, -3, -6, -7, -9,-11,  2,-13, -7, -6, -5, 10, -7, -6, -9, -7,-17,
 -2, -8, -8, -8, -6, -7, -6, -5, -6,  2, -2, -9, -1, -8, -6, -6, -3,-15, -7,  7, -8, -6, -5,-17,
 -3, -7,  6,  6,-12, -3,  1, -3, -1, -6, -9, -2,-10,-10, -7, -1, -3,-10, -6, -8,  6,  0, -5,-17,
 -3, -4, -3,  1,-14,  6,  6, -5, -1, -6, -7, -4, -5,-13, -4, -5, -6,-14, -9, -6,  0,  6, -5,-17,
 -3, -6, -3, -5, -9, -5, -5, -5, -5, -5, -6, -5, -5, -8, -5, -3, -4,-11, -7, -5, -5, -5, -5,-17,
-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,-17,  1};

static const int pam70[576] =
{ 5, -4, -2, -1, -4, -2, -1,  0, -4, -2, -4, -4, -3, -6,  0,  1,  1, -9, -5, -1, -1, -1, -2,-11,
 -4,  8, -3, -6, -5,  0, -5, -6,  0, -3, -6,  2, -2, -7, -2, -1, -4,  0, -7, -5, -4, -2, -3,-11,
 -2, -3,  6,  3, -7, -1,  0, -1,  1, -3, -5,  0, -5, -6, -3,  1,  0, -6, -3, -5,  5, -1, -2,-11,
 -1, -6,  3,  6, -9,  0,  3, -1, -1, -5, -8, -2, -7,-10, -4, -1, -2,-10, -7, -5,  5,  2, -3,-11,
 -4, -5, -7, -9,  9, -9, -9, -6, -5, -4,-10, -9, -9, -8, -5, -1, -5,-11, -2, -4, -8, -9, -6,-11,
 -2,  0, -1,  0, -9,  7,  2, -4,  2, -5, -3, -1, -2, -9, -1, -3, -3, -8, -8, -4, -1,  5, -2,-11,
 -1, -5,  0,  3, -9,  2,  6, -2, -2, -4, -6, -2, -4, -9, -3, -2, -3,-11, -6, -4,  2,  5, -3,-11,
  0, -6, -1, -1, -6, -4, -2,  6, -6, -6, -7, -5, -6, -7, -3,  0, -3,-10, -9, -3, -1, -3, -3,-11,
 -4,  0,  1, -1, -5,  2, -2, -6,  8, -6, -4, -3, -6, -4, -2, -3, -4, -5, -1, -4,  0,  1, -3,-11,
 -2, -3, -3, -5, -4, -5, -4, -6, -6,  7,  1, -4,  1,  0, -5, -4, -1, -9, -4,  3, -4, -4, -3,-11,
 -4, -6, -5, -8,-10, -3, -6, -7, -4,  1,  6, -5,  2, -1, -5, -6, -4, -4, -4,  0, -6, -4, -4,-11,
 -4,  2,  0, -2, -9, -1, -2, -5, -3, -4, -5,  6,  0, -9, -4, -2, -1, -7, -7, -6, -1, -2, -3,-11,
 -3, -2, -5, -7, -9, -2, -4, -6, -6,  1,  2,  0, 10, -2, -5, -3, -2, -8, -7,  0, -6, -3, -3,-11,
 -6, -7, -6,-10, -8, -9, -9, -7, -4,  0, -1, -9, -2,  8, -7, -4, -6, -2,  4, -5, -7, -9, -5,-11,
  0, -2, -3, -4, -5, -1, -3, -3, -2, -5, -5, -4, -5, -7,  7,  0, -2, -9, -9, -3, -4, -2, -3,-11,
  1, -1,  1, -1, -1, -3, -2,  0, -3, -4, -6, -2, -3, -4,  0,  5,  2, -3, -5, -3,  0, -2, -1,-11,
  1, -4,  0, -2, -5, -3, -3, -3, -4, -1, -4, -1, -2, -6, -2,  2,  6, -8, -4, -1, -1, -3, -2,-11,
 -9,  0, -6,-10,-11, -8,-11,-10, -5, -9, -4, -7, -8, -2, -9, -3, -8, 13, -3,-10, -7,-10, -7,-11,
 -5, -7, -3, -7, -2, -8, -6, -9, -1, -4, -4, -7, -7,  4, -9, -5, -4, -3,  9, -5, -4, -7, -5,-11,
 -1, -5, -5, -5, -4, -4, -4, -3, -4,  3,  0, -6,  0, -5, -3, -3, -1,-10, -5,  6, -5, -4, -2,-11,
 -1, -4,  5,  5, -8, -1,  2, -1,  0, -4, -6, -1, -6, -7, -4,  0, -1, -7, -4, -5,  5,  1, -2,-11,
 -1, -2, -1,  2, -9,  5,  5, -3,  1, -4, -4, -2, -3, -9, -2, -2, -3,-10, -7, -4,  1,  5, -3,-11,
 -2, -3, -2, -3, -6, -2, -3, -3, -3, -3, -4, -3, -3, -5, -3, -1, -2, -7, -5, -2, -2, -3, -3,-11,
-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,-11,  1};

static const int blosum80[576] =
{ 7,-3,-3,-3,-1,-2,-2, 0,-3,-3,-3,-1,-2,-4,-1, 2, 0,-5,-4,-1,-3,-2,-1,-8,
 -3, 9,-1,-3,-6, 1,-1,-4, 0,-5,-4, 3,-3,-5,-3,-2,-2,-5,-4,-4,-2, 0,-2,-8,
 -3,-1, 9, 2,-5, 0,-1,-1, 1,-6,-6, 0,-4,-6,-4, 1, 0,-7,-4,-5, 5,-1,-2,-8,
 -3,-3, 2,10,-7,-1, 2,-3,-2,-7,-7,-2,-6,-6,-3,-1,-2,-8,-6,-6, 6, 1,-3,-8,
 -1,-6,-5,-7,13,-5,-7,-6,-7,-2,-3,-6,-3,-4,-6,-2,-2,-5,-5,-2,-6,-7,-4,-8,
 -2, 1, 0,-1,-5, 9, 3,-4, 1,-5,-4, 2,-1,-5,-3,-1,-1,-4,-3,-4,-1, 5,-2,-8,
 -2,-1,-1, 2,-7, 3, 8,-4, 0,-6,-6, 1,-4,-6,-2,-1,-2,-6,-5,-4, 1, 6,-2,-8,
  0,-4,-1,-3,-6,-4,-4, 9,-4,-7,-7,-3,-5,-6,-5,-1,-3,-6,-6,-6,-2,-4,-3,-8,
 -3, 0, 1,-2,-7, 1, 0,-4,12,-6,-5,-1,-4,-2,-4,-2,-3,-4, 3,-5,-1, 0,-2,-8,
 -3,-5,-6,-7,-2,-5,-6,-7,-6, 7, 2,-5, 2,-1,-5,-4,-2,-5,-3, 4,-6,-6,-2,-8,
 -3,-4,-6,-7,-3,-4,-6,-7,-5, 2, 6,-4, 3, 0,-5,-4,-3,-4,-2, 1,-7,-5,-2,-8,
 -1, 3, 0,-2,-6, 2, 1,-3,-1,-5,-4, 8,-3,-5,-2,-1,-1,-6,-4,-4,-1, 1,-2,-8,
 -2,-3,-4,-6,-3,-1,-4,-5,-4, 2, 3,-3, 9, 0,-4,-3,-1,-3,-3, 1,-5,-3,-2,-8,
 -4,-5,-6,-6,-4,-5,-6,-6,-2,-1, 0,-5, 0,10,-6,-4,-4, 0, 4,-2,-6,-6,-3,-8,
 -1,-3,-4,-3,-6,-3,-2,-5,-4,-5,-5,-2,-4,-6,12,-2,-3,-7,-6,-4,-4,-2,-3,-8,
  2,-2, 1,-1,-2,-1,-1,-1,-2,-4,-4,-1,-3,-4,-2, 7, 2,-6,-3,-3, 0,-1,-1,-8,
  0,-2, 0,-2,-2,-1,-2,-3,-3,-2,-3,-1,-1,-4,-3, 2, 8,-5,-3, 0,-1,-2,-1,-8,
 -5,-5,-7,-8,-5,-4,-6,-6,-4,-5,-4,-6,-3, 0,-7,-6,-5,16, 3,-5,-8,-5,-5,-8,
 -4,-4,-4,-6,-5,-3,-5,-6, 3,-3,-2,-4,-3, 4,-6,-3,-3, 3,11,-3,-5,-4,-3,-8,
 -1,-4,-5,-6,-2,-4,-4,-6,-5, 4, 1,-4, 1,-2,-4,-3, 0,-5,-3, 7,-6,-4,-2,-8,
 -3,-2, 5, 6,-6,-1, 1,-2,-1,-6,-7,-1,-5,-6,-4, 0,-1,-8,-5,-6, 6, 0,-3,-8,
 -2, 0,-1, 1,-7, 5, 6,-4, 0,-6,-5, 1,-3,-6,-2,-1,-2,-5,-4,-4, 0, 6,-1,-8,
 -1,-2,-2,-3,-4,-2,-2,-3,-2,-2,-2,-2,-2,-3,-3,-1,-1,-5,-3,-2,-3,-1,-2,-8,
 -8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8,-8, 1};

int blosum62[576] =
{ 4,-1,-2,-2, 0,-1,-1, 0,-2,-1,-1,-1,-1,-2,-1, 1, 0,-3,-2, 0,-2,-1, 0,-4,
 -1, 5, 0,-2,-3, 1, 0,-2, 0,-3,-2, 2,-1,-3,-2,-1,-1,-3,-2,-3,-1, 0,-1,-4,
 -2, 0, 6, 1,-3, 0, 0, 0, 1,-3,-3, 0,-2,-3,-2, 1, 0,-4,-2,-3, 3, 0,-1,-4,
 -2,-2, 1, 6,-3, 0, 2,-1,-1,-3,-4,-1,-3,-3,-1, 0,-1,-4,-3,-3, 4, 1,-1,-4,
  0,-3,-3,-3, 9,-3,-4,-3,-3,-1,-1,-3,-1,-2,-3,-1,-1,-2,-2,-1,-3,-3,-2,-4,
 -1, 1, 0, 0,-3, 5, 2,-2, 0,-3,-2, 1, 0,-3,-1, 0,-1,-2,-1,-2, 0, 3,-1,-4,
 -1, 0, 0, 2,-4, 2, 5,-2, 0,-3,-3, 1,-2,-3,-1, 0,-1,-3,-2,-2, 1, 4,-1,-4,
  0,-2, 0,-1,-3,-2,-2, 6,-2,-4,-4,-2,-3,-3,-2, 0,-2,-2,-3,-3,-1,-2,-1,-4,
 -2, 0, 1,-1,-3, 0, 0,-2, 8,-3,-3,-1,-2,-1,-2,-1,-2,-2, 2,-3, 0, 0,-1,-4,
 -1,-3,-3,-3,-1,-3,-3,-4,-3, 4, 2,-3, 1, 0,-3,-2,-1,-3,-1, 3,-3,-3,-1,-4,
 -1,-2,-3,-4,-1,-2,-3,-4,-3, 2, 4,-2, 2, 0,-3,-2,-1,-2,-1, 1,-4,-3,-1,-4,
 -1, 2, 0,-1,-3, 1, 1,-2,-1,-3,-2, 5,-1,-3,-1, 0,-1,-3,-2,-2, 0, 1,-1,-4,
 -1,-1,-2,-3,-1, 0,-2,-3,-2, 1, 2,-1, 5, 0,-2,-1,-1,-1,-1, 1,-3,-1,-1,-4,
 -2,-3,-3,-3,-2,-3,-3,-3,-1, 0, 0,-3, 0, 6,-4,-2,-2, 1, 3,-1,-3,-3,-1,-4,
 -1,-2,-2,-1,-3,-1,-1,-2,-2,-3,-3,-1,-2,-4, 7,-1,-1,-4,-3,-2,-2,-1,-2,-4,
  1,-1, 1, 0,-1, 0, 0, 0,-1,-2,-2, 0,-1,-2,-1, 4, 1,-3,-2,-2, 0, 0, 0,-4,
  0,-1, 0,-1,-1,-1,-1,-2,-2,-1,-1,-1,-1,-2,-1, 1, 5,-2,-2, 0,-1,-1, 0,-4,
 -3,-3,-4,-4,-2,-2,-3,-2,-2,-3,-2,-3,-1, 1,-4,-3,-2,11, 2,-3,-4,-3,-2,-4,
 -2,-2,-2,-3,-2,-1,-2,-3, 2,-1,-1,-2,-1, 3,-3,-2,-2, 2, 7,-1,-3,-2,-1,-4,
  0,-3,-3,-3,-1,-2,-2,-3,-3, 3, 1,-2, 1,-1,-2,-2, 0,-3,-1, 4,-3,-2,-1,-4,
 -2,-1, 3, 4,-3, 0, 1,-1, 0,-3,-4, 0,-3,-3,-2, 0,-1,-4,-3,-3, 4, 1,-1,-4,
 -1, 0, 0, 1,-3, 3, 4,-2, 0,-3,-3, 1,-1,-3,-1, 0,-1,-3,-2,-2, 1, 4,-1,-4,
  0,-1,-1,-1,-2,-1,-1,-1,-1,-1,-1,-1,-1,-1,-2, 0, 0,-2,-1,-1,-1,-1,-1,-4,
 -4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4, 1};

static const char dna_bases[] = "AaCcGgTt";

static const int sub_matrix[64] =
{ 2, 2,-4,-4,-4,-4,-4,-4,
  2, 2,-4,-4,-4,-4,-4,-4,
 -4,-4, 5, 5,-4,-4,-4,-4,
 -4,-4, 5, 5,-4,-4,-4,-4,
 -4,-4,-4,-4, 5, 5,-4,-4,
 -4,-4,-4,-4, 5, 5,-4,-4,
 -4,-4,-4,-4,-4,-4, 2, 2,
 -4,-4,-4,-4,-4,-4, 2, 2};

// Scoring for protein comparisons of length <35bp
void scoring_system_PAM30(scoring_t* scoring)
{
  // *->* match: 1
  // *->* mismatch: -17
  // Gap open -9, gap extend -1
  // no_start_gap_penalty, no_end_gap_penalty = 0
  // case_sensitive = 0
  scoring_init(scoring, 1, -17, -9, -1, 0, 0, 0, 0, 0, 0);

  // use_match_mismatch=1
  scoring_add_mutations(scoring, amino_acids, pam30, 1);
}

// Scoring for protein comparisons of length 35-50
void scoring_system_PAM70(scoring_t* scoring)
{
  // *->* match: 1
  // *->* mismatch: -11
  // Gap open -10, gap extend -1
  // no_start_gap_penalty, no_end_gap_penalty = 0
  // case_sensitive = 0
  scoring_init(scoring, 1, -11, -10, -1, 0, 0, 0, 0, 0, 0);

  // use_match_mismatch=1
  scoring_add_mutations(scoring, amino_acids, pam70, 1);
}

// Scoring for protein comparisons of length 50-85
void scoring_system_BLOSUM80(scoring_t* scoring)
{
  // *->* match: 1
  // *->* mismatch: -8
  // Gap open -10, gap extend -1
  // no_start_gap_penalty, no_end_gap_penalty = 0
  // case_sensitive = 0
  scoring_init(scoring, 1, -8, -10, -1, 0, 0, 0, 0, 0, 0);

  // use_match_mismatch=1
  scoring_add_mutations(scoring, amino_acids, blosum80, 1);
}

// Scoring for protein comparisons of length >85
void scoring_system_BLOSUM62(scoring_t* scoring)
{
  // *->* match: 1
  // *->* mismatch: -4
  // Gap open -10, gap extend -1
  // no_start_gap_penalty, no_end_gap_penalty = 0
  // case_sensitive = 0
  scoring_init(scoring, 1, -4, -10, -1, 0, 0, 0, 0, 0, 0);

  // use_match_mismatch=1
  scoring_add_mutations(scoring, amino_acids, blosum62, 1);
}

// Scoring system for predicting DNA hybridization
// "Optimization of the BLASTN substitution matrix for prediction of
//   non-specific DNA microarray hybridization" (2009)
// http://www.ncbi.nlm.nih.gov/pmc/articles/PMC2831327/
void scoring_system_DNA_hybridization(scoring_t* scoring)
{
  // match: 0
  // mismatch: 0
  // Gap open -10, gap extend -10
  // no_start_gap_penalty, no_end_gap_penalty = 0
  // case_sensitive = 0
  scoring_init(scoring, 0, 0, -10, -10, 0, 0, 0, 0, 0, 0);

  // use_match_mismatch=0
  scoring_add_mutations(scoring, dna_bases, sub_matrix, 0);
}

// Default
void scoring_system_default(scoring_t* scoring)
{
  int match_default = 1;
  int mismatch_default = -2;
  int gap_open_default = -4;
  int gap_extend_default = -1;

  // no_start_gap_penalty, no_end_gap_penalty = 0
  // case_sensitive = 0
  scoring_init(scoring, match_default, mismatch_default,
               gap_open_default, gap_extend_default,
               0, 0, 0, 0, 0, 0);
}


#endif


void align_scoring_load_matrix(gzFile file, const char* file_path,
                               scoring_t* scoring, char case_sensitive);

void align_scoring_load_pairwise(gzFile file, const char* file_path,
                                 scoring_t* scoring, char case_sensitive);

/*
 alignment_scoring_load.c
 url: https://github.com/noporpoise/seq-align
 maintainer: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain, no warranty
 date: Nov 2013
 */

// request decent POSIX version
#define _XOPEN_SOURCE 700
#define _BSD_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h> // tolower isspace

//#include "string_buffer/string_buffer.h"
/********************************/

/*
 stream_buffer.h
 project: string_buffer
 url: https://github.com/noporpoise/StringBuffer
 author: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain
 Jan 2015
*/

#ifndef STRING_BUFFER_FILE_SEEN
#define STRING_BUFFER_FILE_SEEN

#include <stdio.h> // needed for FILE
#include <zlib.h> // needed for gzFile
#include <stdarg.h> // needed for va_list

//#include "stream_buffer.h"


typedef struct
{
  char *b;
  size_t end; // end is index of \0
  size_t size; // size should be >= end+1 to allow for \0
} StrBuf;


//
// Creation, reset, free and memory expansion
//

// Constructors / Destructors
StrBuf* strbuf_new(size_t len);
static inline void strbuf_free(StrBuf *sb) { free(sb->b); free(sb); }

// Place a string buffer into existing memory. Example:
//   StrBuf buf;
//   strbuf_alloc(&buf, 100);
//   ...
//   strbuf_dealloc(&buf);
StrBuf* strbuf_alloc(StrBuf *sb, size_t len);

static inline void strbuf_dealloc(StrBuf *sb) {
  free(sb->b);
  memset(sb, 0, sizeof(*sb));
}

// Copy a string or existing string buffer
StrBuf* strbuf_create(const char *str);
StrBuf* strbuf_clone(const StrBuf *sb);

// Clear the content of an existing StrBuf (sets size to 0)
static inline void strbuf_reset(StrBuf *sb) {
  if(sb->b) { sb->b[sb->end = 0] = '\0'; }
}

//
// Resizing
//

// Ensure capacity for len characters plus '\0' character - exits on FAILURE
static inline void strbuf_ensure_capacity(StrBuf *sb, size_t len) {
  cbuf_capacity(&sb->b, &sb->size, len);
}

// Same as above, but update pointer if it pointed to resized array
void strbuf_ensure_capacity_update_ptr(StrBuf *sbuf, size_t size, const char **ptr);

// Resize the buffer to have capacity to hold a string of length new_len
// (+ a null terminating character).  Can also be used to downsize the buffer's
// memory usage.  Returns 1 on success, 0 on failure.
char strbuf_resize(StrBuf *sb, size_t new_len);

//
// Useful String functions
//

#define strbuf_len(sb)  ((sb)->end)

#define strbuf_char(sb,idx) (sb)->b[idx]

// Note: in MACROs we use local variables to avoid multiple evaluation of param

// Set string buffer to contain a given string
#define strbuf_set(__sb,__str) do         \
{                                         \
  StrBuf     *_sb  = (__sb);              \
  const char *_str = (__str);             \
  size_t _s = strlen(_str);               \
  strbuf_ensure_capacity(_sb,_s);         \
  memcpy(_sb->b, _str, _s);               \
  _sb->b[_sb->end = _s] = '\0';           \
} while(0)

// Set string buffer to match existing string buffer
#define strbuf_set_buff(__dst,__src) do      \
{                                            \
  StrBuf       *_dst = (__dst);              \
  const StrBuf *_src = (__src);              \
  strbuf_ensure_capacity(_dst, _src->end);   \
  memmove(_dst->b, _src->b, _src->end);      \
  _dst->b[_dst->end = _src->end] = '\0';     \
} while(0)

// Add a character to the end of this StrBuf
#define strbuf_append_char(__sb,__c) do     \
{                                           \
  StrBuf *_sb = (__sb);                     \
  char    _c  = (__c);                      \
  strbuf_ensure_capacity(_sb, _sb->end+1);  \
  _sb->b[_sb->end] = _c;                    \
  _sb->b[++_sb->end] = '\0';                \
} while(0)

// Copy N characters from a character array to the end of this StrBuf
// strlen(__str) must be >= __n
#define strbuf_append_strn(__sb,__str,__n) do                   \
{                                                               \
  StrBuf     *_sb  = (__sb);                                    \
  const char *_str = (__str);                                   \
  size_t      _n   = (__n);                                     \
  strbuf_ensure_capacity_update_ptr(_sb, _sb->end+_n, &_str);   \
  memcpy(_sb->b+_sb->end, _str, _n);                            \
  _sb->b[_sb->end = _sb->end+_n] = '\0';                        \
} while(0)

// Copy a character array to the end of this StrBuf
// name char* _str2 since strbuf_append_strn uses _str
#define strbuf_append_str(__sb,__str) do {        \
  const char *_str2 = (__str);                    \
  strbuf_append_strn(__sb, _str2, strlen(_str2)); \
} while(0)

#define strbuf_append_buff(__sb1,__sb2) do {     \
  const StrBuf *_sb2 = (__sb2);                  \
  strbuf_append_strn(__sb1, _sb2->b, _sb2->end); \
} while(0)

// Convert integers to string to append
void strbuf_append_int(StrBuf *buf, int value);
void strbuf_append_long(StrBuf *buf, long value);
void strbuf_append_ulong(StrBuf *buf, unsigned long value);

// Append a given string in lower or uppercase
void strbuf_append_strn_lc(StrBuf *buf, const char *str, size_t len);
void strbuf_append_strn_uc(StrBuf *buf, const char *str, size_t len);

// Append char `c` `n` times
void strbuf_append_charn(StrBuf *buf, char c, size_t n);

#define strbuf_shrink(__sb,__len) do {                    \
  StrBuf *_sb = (__sb); _sb->b[_sb->end = (__len)] = 0;   \
} while(0)

#define strbuf_dup_str(sb) strdup((sb)->b)

// Remove \r and \n characters from the end of this StrBuf
// Returns the number of characters removed
size_t strbuf_chomp(StrBuf *sb);

// Reverse a string
void strbuf_reverse(StrBuf *sb);

// Get a substring as a new null terminated char array
// (remember to free the returned char* after you're done with it!)
char* strbuf_substr(const StrBuf *sb, size_t start, size_t len);

// Change to upper or lower case
void strbuf_to_uppercase(StrBuf *sb);
void strbuf_to_lowercase(StrBuf *sb);

// Copy a string to this StrBuf, overwriting any existing characters
// Note: dst_pos + len can be longer the the current dst StrBuf
void strbuf_copy(StrBuf *dst, size_t dst_pos,
                 const char *src, size_t len);

// Insert: copy to a StrBuf, shifting any existing characters along
void strbuf_insert(StrBuf *dst, size_t dst_pos,
                   const char *src, size_t len);

// Overwrite dst_pos..(dst_pos+dst_len-1) with src_len chars from src
// if dst_len != src_len, content to the right of dst_len is shifted
// Example:
//   strbuf_set(sb, "aaabbccc");
//   char *data = "xxx";
//   strbuf_overwrite(sb,3,2,data,strlen(data));
//   // sb is now "aaaxxxccc"
//   strbuf_overwrite(sb,3,2,"_",1);
//   // sb is now "aaa_ccc"
void strbuf_overwrite(StrBuf *dst, size_t dst_pos, size_t dst_len,
                      const char *src, size_t src_len);

// Remove characters from the buffer
//   strbuf_set(sb, "aaaBBccc");
//   strbuf_delete(sb, 3, 2);
//   // sb is now "aaaccc"
void strbuf_delete(StrBuf *sb, size_t pos, size_t len);

//
// sprintf
//

// sprintf to the end of a StrBuf (adds string terminator after sprint)
int strbuf_sprintf(StrBuf *sb, const char *fmt, ...)
  __attribute__ ((format(printf, 2, 3)));

// Print at a given position (overwrite chars at positions >= pos)
int strbuf_sprintf_at(StrBuf *sb, size_t pos, const char *fmt, ...)
  __attribute__ ((format(printf, 3, 4)));

int strbuf_vsprintf(StrBuf *sb, size_t pos, const char *fmt, va_list argptr)
  __attribute__ ((format(printf, 3, 0)));

// sprintf without terminating character
// Does not prematurely end the string if you sprintf within the string
// (terminates string if sprintf to the end)
int strbuf_sprintf_noterm(StrBuf *sb, size_t pos, const char *fmt, ...)
  __attribute__ ((format(printf, 3, 4)));

//
// Reading files
//

// Reading a FILE
size_t strbuf_reset_readline(StrBuf *sb, FILE *file);
size_t strbuf_readline(StrBuf *sb, FILE *file);
size_t strbuf_skipline(FILE *file);
size_t strbuf_readline_buf(StrBuf *sb, FILE *file, StreamBuffer *in);
size_t strbuf_skipline_buf(FILE* file, StreamBuffer *in);
size_t strbuf_read(StrBuf *sb, FILE *file, size_t len);

// Reading a gzFile
size_t strbuf_reset_gzreadline(StrBuf *sb, gzFile gz_file);
size_t strbuf_gzreadline(StrBuf *sb, gzFile gz_file);
size_t strbuf_gzskipline(gzFile gz_file);
size_t strbuf_gzreadline_buf(StrBuf *sb, gzFile gz_file, StreamBuffer *in);
size_t strbuf_gzskipline_buf(gzFile file, StreamBuffer *in);
size_t strbuf_gzread(StrBuf *sb, gzFile gz_file, size_t len);

// Read a line that has at least one character that is not \r or \n
// these functions do not call reset before reading
// Returns the number of characters read
size_t strbuf_readline_nonempty(StrBuf *line, FILE *fh);
size_t strbuf_gzreadline_nonempty(StrBuf *line, gzFile gz);

//
// String functions
//

// Trim whitespace characters from the start and end of a string
void strbuf_trim(StrBuf *sb);

// Trim the characters listed in `list` from the left of `sb`
// `list` is a null-terminated string of characters
void strbuf_ltrim(StrBuf *sb, const char *list);

// Trim the characters listed in `list` from the right of `sb`
// `list` is a null-terminated string of characters
void strbuf_rtrim(StrBuf *sb, const char *list);

/**************************/
/* Other String functions */
/**************************/

// `n` is the maximum number of bytes to copy including the NULL byte
// copies at most n bytes from `src` to `dst`
// Always appends a NULL terminating byte, unless n is zero.
// Returns a pointer to dst
char* string_safe_ncpy(char *dst, const char *src, size_t n);

// Replaces `sep` with \0 in str
// Returns number of occurances of `sep` character in `str`
// Stores `nptrs` pointers in `ptrs`
size_t string_split_str(char *str, char sep, char **ptrs, size_t nptrs);

// Replace one char with another in a string. Return number of replacements made
size_t string_char_replace(char *str, char from, char to);

void string_reverse_region(char *str, size_t length);
char string_is_all_whitespace(const char *s);
char* string_next_nonwhitespace(char *s);
char* string_trim(char *str);
// Chomp a string, returns new length
size_t string_chomp(char *str, size_t len);
size_t string_count_char(const char *str, char c);
size_t string_split(const char *split, const char *txt, char ***result);

/*
 string_buffer.c
 project: string_buffer
 url: https://github.com/noporpoise/StringBuffer
 author: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain
 Jan 2014
*/

// POSIX required for kill signal to work
#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h> // kill on error
#include <ctype.h> // toupper() and tolower()

//#include "string_buffer.h"

#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define MAX(x,y) ((x) > (y) ? (x) : (y))

#ifndef ROUNDUP2POW
  #define ROUNDUP2POW(x) (0x1UL << (64 - __builtin_clzl(x)))
#endif

#define exit_on_error() do { abort(); exit(EXIT_FAILURE); } while(0)

/*********************/
/*  Bounds checking  */
/*********************/

// Bounds check when inserting (pos <= len are valid)
#define _bounds_check_insert(sbuf,pos) \
        _call_bounds_check_insert(sbuf,pos,__FILE__,__LINE__,__func__)
#define _bounds_check_read_range(sbuf,start,len) \
        _call_bounds_check_read_range(sbuf,start,len,__FILE__,__LINE__,__func__)

static inline
void _call_bounds_check_insert(const StrBuf *sbuf, size_t pos,
                               const char *file, int line, const char *func)
{
  if(pos > sbuf->end)
  {
    fprintf(stderr, "%s:%i:%s() - out of bounds error "
                    "[index: %zu, num_of_bits: %zu]\n",
            file, line, func, pos, sbuf->end);
    errno = EDOM;
    exit_on_error();
  }
}

// Bounds check when reading a range (start+len < strlen is valid)
static inline
void _call_bounds_check_read_range(const StrBuf *sbuf, size_t start, size_t len,
                                   const char *file, int line, const char *func)
{
  if(start + len > sbuf->end)
  {
    fprintf(stderr, "%s:%i:%s() - out of bounds error "
                    "[start: %zu; length: %zu; strlen: %zu; buf:%.*s%s]\n",
            file, line, func, start, len, sbuf->end,
            (int)MIN(5, sbuf->end), sbuf->b, sbuf->end > 5 ? "..." : "");
    errno = EDOM;
    exit_on_error();
  }
}

/******************************/
/*  Constructors/Destructors  */
/******************************/

StrBuf* strbuf_new(size_t len)
{
  StrBuf *sbuf = calloc(1, sizeof(StrBuf));
  if(!sbuf) return NULL;
  if(!strbuf_alloc(sbuf, len)) { free(sbuf); return NULL; }
  return sbuf;
}

StrBuf* strbuf_alloc(StrBuf *sbuf, size_t len)
{
  sbuf->end  = 0;
  sbuf->size = ROUNDUP2POW(len+1);
  sbuf->b    = malloc(sbuf->size);
  if(!sbuf->b) return NULL;
  sbuf->b[0] = '\0';
  return sbuf;
}

StrBuf* strbuf_create(const char *str)
{
  size_t str_len = strlen(str);
  StrBuf *sbuf = strbuf_new(str_len+1);
  if(!sbuf) return NULL;
  memcpy(sbuf->b, str, str_len);
  sbuf->b[sbuf->end = str_len] = '\0';
  return sbuf;
}

StrBuf* strbuf_clone(const StrBuf *sbuf)
{
  // One byte for the string end / null char \0
  StrBuf *cpy = strbuf_new(sbuf->end+1);
  if(!cpy) return NULL;
  memcpy(cpy->b, sbuf->b, sbuf->end);
  cpy->b[cpy->end = sbuf->end] = '\0';
  return cpy;
}

/******************************/
/*  Resize Buffer Functions   */
/******************************/

// Resize the buffer to have capacity to hold a string of length new_len
// (+ a null terminating character).  Can also be used to downsize the buffer's
// memory usage.  Returns 1 on success, 0 on failure.
char strbuf_resize(StrBuf *sbuf, size_t new_len)
{
  size_t capacity = ROUNDUP2POW(new_len+1);
  char *new_buff = realloc(sbuf->b, capacity * sizeof(char));
  if(new_buff == NULL) return 0;

  sbuf->b    = new_buff;
  sbuf->size = capacity;

  if(sbuf->end > new_len)
  {
    // Buffer was shrunk - re-add null byte
    sbuf->end = new_len;
    sbuf->b[sbuf->end] = '\0';
  }

  return 1;
}

void strbuf_ensure_capacity_update_ptr(StrBuf *sbuf, size_t size, const char **ptr)
{
  if(sbuf->size <= size+1)
  {
    size_t oldcap = sbuf->size;
    char *oldbuf  = sbuf->b;

    if(!strbuf_resize(sbuf, size))
    {
      fprintf(stderr, "%s:%i:Error: _ensure_capacity_update_ptr couldn't resize "
                      "buffer. [requested %zu bytes; capacity: %zu bytes]\n",
              __FILE__, __LINE__, size, sbuf->size);
      exit_on_error();
    }

    // ptr may have pointed to sbuf, which has now moved
    if(*ptr >= oldbuf && *ptr < oldbuf + oldcap) {
      *ptr = sbuf->b + (*ptr - oldbuf);
    }
  }
}

/*******************/
/* Append Integers */
/*******************/

/*
 * Integer to string functions adapted from:
 *   https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920
 */

#define P01 10
#define P02 100
#define P03 1000
#define P04 10000
#define P05 100000
#define P06 1000000
#define P07 10000000
#define P08 100000000
#define P09 1000000000
#define P10 10000000000
#define P11 100000000000
#define P12 1000000000000

/**
 * Return number of digits required to represent `num` in base 10.
 * Uses binary search to find number.
 * Examples:
 *   num_of_digits(0)   = 1
 *   num_of_digits(1)   = 1
 *   num_of_digits(10)  = 2
 *   num_of_digits(123) = 3
 */
static inline size_t num_of_digits(unsigned long v)
{
  if(v < P01) return 1;
  if(v < P02) return 2;
  if(v < P03) return 3;
  if(v < P12) {
    if(v < P08) {
      if(v < P06) {
        if(v < P04) return 4;
        return 5 + (v >= P05);
      }
      return 7 + (v >= P07);
    }
    if(v < P10) {
      return 9 + (v >= P09);
    }
    return 11 + (v >= P11);
  }
  return 12 + num_of_digits(v / P12);
}

void strbuf_append_ulong(StrBuf *buf, unsigned long value)
{
  // Append two digits at a time
  static const char digits[201] =
    "0001020304050607080910111213141516171819"
    "2021222324252627282930313233343536373839"
    "4041424344454647484950515253545556575859"
    "6061626364656667686970717273747576777879"
    "8081828384858687888990919293949596979899";

  size_t num_digits = num_of_digits(value);
  size_t pos = num_digits - 1;

  strbuf_ensure_capacity(buf, buf->end+num_digits);
  char *dst = buf->b + buf->end;

  while(value >= 100)
  {
    size_t v = value % 100;
    value /= 100;
    dst[pos] = digits[v * 2 + 1];
    dst[pos - 1] = digits[v * 2];
    pos -= 2;
  }

  // Handle last 1-2 digits
  if(value < 10) {
    dst[pos] = '0' + value;
  } else {
    dst[pos] = digits[value * 2 + 1];
    dst[pos - 1] = digits[value * 2];
  }

  buf->end += num_digits;
  buf->b[buf->end] = '\0';
}

void strbuf_append_int(StrBuf *buf, int value)
{
  // strbuf_sprintf(buf, "%i", value);
  strbuf_append_long(buf, value);
}

void strbuf_append_long(StrBuf *buf, long value)
{
  // strbuf_sprintf(buf, "%li", value);
  if(value < 0) { strbuf_append_char(buf, '-'); value = -value; }
  strbuf_append_ulong(buf, value);
}


/*
size_t strbuf_num_of_digits(unsigned long num)
{
  size_t digits = 1;
  while(1) {
    if(num < 10) return digits;
    if(num < 100) return digits+1;
    if(num < 1000) return digits+2;
    if(num < 10000) return digits+3;
    num /= 10000;
    digits += 4;
  }
  return digits;
}

void strbuf_append_ulong(StrBuf *buf, unsigned long value)
{
  // strbuf_sprintf(buf, "%lu", value);

  size_t i, num_digits = strbuf_num_of_digits(value);
  strbuf_ensure_capacity(buf, buf->end + num_digits);
  buf->end += num_digits;
  buf->b[buf->end] = '\0';

  for(i = 1; i <= num_digits; i++) {
    buf->b[buf->end - i] = '0' + (value % 10);
    value /= 10;
  }
}
*/

/********************/
/* Append functions */
/********************/

// Append string converted to lowercase
void strbuf_append_strn_lc(StrBuf *buf, const char *str, size_t len)
{
  strbuf_ensure_capacity(buf, buf->end + len);
  char *to = buf->b + buf->end;
  const char *end = str + len;
  for(; str < end; str++, to++) *to = tolower(*str);
  buf->end += len;
  buf->b[buf->end] = '\0';
}

// Append string converted to uppercase
void strbuf_append_strn_uc(StrBuf *buf, const char *str, size_t len)
{
  strbuf_ensure_capacity(buf, buf->end + len);
  char *to = buf->b + buf->end;
  const char *end = str + len;
  for(; str < end; str++, to++) *to = toupper(*str);
  buf->end += len;
  buf->b[buf->end] = '\0';
}

// Append char `c` `n` times
void strbuf_append_charn(StrBuf *buf, char c, size_t n)
{
  strbuf_ensure_capacity(buf, buf->end + n);
  memset(buf->b+buf->end, c, n);
  buf->end += n;
  buf->b[buf->end] = '\0';
}

// Remove \r and \n characters from the end of this StrBuf
// Returns the number of characters removed
size_t strbuf_chomp(StrBuf *sbuf)
{
  size_t old_len = sbuf->end;
  sbuf->end = string_chomp(sbuf->b, sbuf->end);
  return old_len - sbuf->end;
}

// Reverse a string
void strbuf_reverse(StrBuf *sbuf)
{
  string_reverse_region(sbuf->b, sbuf->end);
}

char* strbuf_substr(const StrBuf *sbuf, size_t start, size_t len)
{
  _bounds_check_read_range(sbuf, start, len);

  char *new_string = malloc((len+1) * sizeof(char));
  strncpy(new_string, sbuf->b + start, len);
  new_string[len] = '\0';

  return new_string;
}

void strbuf_to_uppercase(StrBuf *sbuf)
{
  char *pos, *end = sbuf->b + sbuf->end;
  for(pos = sbuf->b; pos < end; pos++) *pos = (char)toupper(*pos);
}

void strbuf_to_lowercase(StrBuf *sbuf)
{
  char *pos, *end = sbuf->b + sbuf->end;
  for(pos = sbuf->b; pos < end; pos++) *pos = (char)tolower(*pos);
}

// Copy a string to this StrBuf, overwriting any existing characters
// Note: dst_pos + len can be longer the the current dst StrBuf
void strbuf_copy(StrBuf *dst, size_t dst_pos, const char *src, size_t len)
{
  if(src == NULL || len == 0) return;

  _bounds_check_insert(dst, dst_pos);

  // Check if dst buffer can handle string
  // src may have pointed to dst, which has now moved
  size_t newlen = MAX(dst_pos + len, dst->end);
  strbuf_ensure_capacity_update_ptr(dst, newlen, &src);

  // memmove instead of strncpy, as it can handle overlapping regions
  memmove(dst->b+dst_pos, src, len * sizeof(char));

  if(dst_pos + len > dst->end)
  {
    // Extended string - add '\0' char
    dst->end = dst_pos + len;
    dst->b[dst->end] = '\0';
  }
}

// Insert: copy to a StrBuf, shifting any existing characters along
void strbuf_insert(StrBuf *dst, size_t dst_pos, const char *src, size_t len)
{
  if(src == NULL || len == 0) return;

  _bounds_check_insert(dst, dst_pos);

  // Check if dst buffer has capacity for inserted string plus \0
  // src may have pointed to dst, which will be moved in realloc when
  // calling ensure capacity
  strbuf_ensure_capacity_update_ptr(dst, dst->end + len, &src);

  char *insert = dst->b+dst_pos;

  // dst_pos could be at the end (== dst->end)
  if(dst_pos < dst->end)
  {
    // Shift some characters up
    memmove(insert + len, insert, (dst->end - dst_pos) * sizeof(char));

    if(src >= dst->b && src < dst->b + dst->size)
    {
      // src/dst strings point to the same string in memory
      if(src < insert) memmove(insert, src, len * sizeof(char));
      else if(src > insert) memmove(insert, src+len, len * sizeof(char));
    }
    else memmove(insert, src, len * sizeof(char));
  }
  else memmove(insert, src, len * sizeof(char));

  // Update size
  dst->end += len;
  dst->b[dst->end] = '\0';
}

// Overwrite dst_pos..(dst_pos+dst_len-1) with src_len chars from src
// if dst_len != src_len, content to the right of dst_len is shifted
// Example:
//   strbuf_set(sbuf, "aaabbccc");
//   char *data = "xxx";
//   strbuf_overwrite(sbuf,3,2,data,strlen(data));
//   // sbuf is now "aaaxxxccc"
//   strbuf_overwrite(sbuf,3,2,"_",1);
//   // sbuf is now "aaa_ccc"
void strbuf_overwrite(StrBuf *dst, size_t dst_pos, size_t dst_len,
                      const char *src, size_t src_len)
{
  _bounds_check_read_range(dst, dst_pos, dst_len);

  if(src == NULL) return;
  if(dst_len == src_len) strbuf_copy(dst, dst_pos, src, src_len);
  size_t newlen = dst->end + src_len - dst_len;

  strbuf_ensure_capacity_update_ptr(dst, newlen, &src);

  if(src >= dst->b && src < dst->b + dst->size)
  {
    if(src_len < dst_len) {
      // copy
      memmove(dst->b+dst_pos, src, src_len * sizeof(char));
      // resize (shrink)
      memmove(dst->b+dst_pos+src_len, dst->b+dst_pos+dst_len,
              (dst->end-dst_pos-dst_len) * sizeof(char));
    }
    else
    {
      // Buffer is going to grow and src points to this buffer

      // resize (grow)
      memmove(dst->b+dst_pos+src_len, dst->b+dst_pos+dst_len,
              (dst->end-dst_pos-dst_len) * sizeof(char));

      char *tgt = dst->b + dst_pos;
      char *end = dst->b + dst_pos + src_len;

      if(src < tgt + dst_len)
      {
        size_t len = MIN((size_t)(end - src), src_len);
        memmove(tgt, src, len);
        tgt += len;
        src += len;
        src_len -= len;
      }

      if(src >= tgt + dst_len)
      {
        // shift to account for resizing
        src += src_len - dst_len;
        memmove(tgt, src, src_len);
      }
    }
  }
  else
  {
    // resize
    memmove(dst->b+dst_pos+src_len, dst->b+dst_pos+dst_len,
            (dst->end-dst_pos-dst_len) * sizeof(char));
    // copy
    memcpy(dst->b+dst_pos, src, src_len * sizeof(char));
  }

  dst->end = newlen;
  dst->b[dst->end] = '\0';
}

void strbuf_delete(StrBuf *sbuf, size_t pos, size_t len)
{
  _bounds_check_read_range(sbuf, pos, len);
  memmove(sbuf->b+pos, sbuf->b+pos+len, sbuf->end-pos-len);
  sbuf->end -= len;
  sbuf->b[sbuf->end] = '\0';
}

/**************************/
/*         sprintf        */
/**************************/

int strbuf_vsprintf(StrBuf *sbuf, size_t pos, const char *fmt, va_list argptr)
{
  _bounds_check_insert(sbuf, pos);

  // Length of remaining buffer
  size_t buf_len = sbuf->size - pos;
  if(buf_len == 0 && !strbuf_resize(sbuf, sbuf->size << 1)) {
    fprintf(stderr, "%s:%i:Error: Out of memory\n", __FILE__, __LINE__);
    exit_on_error();
  }

  // Make a copy of the list of args incase we need to resize buff and try again
  va_list argptr_cpy;
  va_copy(argptr_cpy, argptr);

  int num_chars = vsnprintf(sbuf->b+pos, buf_len, fmt, argptr);
  va_end(argptr);

  // num_chars is the number of chars that would be written (not including '\0')
  // num_chars < 0 => failure
  if(num_chars < 0) {
    fprintf(stderr, "Warning: strbuf_sprintf something went wrong..\n");
    exit_on_error();
  }

  // num_chars does not include the null terminating byte
  if((size_t)num_chars+1 > buf_len)
  {
    strbuf_ensure_capacity(sbuf, pos+(size_t)num_chars);

    // now use the argptr copy we made earlier
    // Don't need to use vsnprintf now, vsprintf will do since we know it'll fit
    num_chars = vsprintf(sbuf->b+pos, fmt, argptr_cpy);
    if(num_chars < 0) {
      fprintf(stderr, "Warning: strbuf_sprintf something went wrong..\n");
      exit_on_error();
    }
  }
  va_end(argptr_cpy);

  // Don't need to NUL terminate, vsprintf/vnsprintf does that for us

  // Update length
  sbuf->end = pos + (size_t)num_chars;

  return num_chars;
}

// Appends sprintf
int strbuf_sprintf(StrBuf *sbuf, const char *fmt, ...)
{
  va_list argptr;
  va_start(argptr, fmt);
  int num_chars = strbuf_vsprintf(sbuf, sbuf->end, fmt, argptr);
  va_end(argptr);

  return num_chars;
}

int strbuf_sprintf_at(StrBuf *sbuf, size_t pos, const char *fmt, ...)
{
  _bounds_check_insert(sbuf, pos);

  va_list argptr;
  va_start(argptr, fmt);
  int num_chars = strbuf_vsprintf(sbuf, pos, fmt, argptr);
  va_end(argptr);

  return num_chars;
}

// Does not prematurely end the string if you sprintf within the string
// (vs at the end)
int strbuf_sprintf_noterm(StrBuf *sbuf, size_t pos, const char *fmt, ...)
{
  _bounds_check_insert(sbuf, pos);

  char last_char;
  size_t len = sbuf->end;

  // Call vsnprintf with NULL, 0 to get resulting string length without writing
  va_list argptr;
  va_start(argptr, fmt);
  int nchars = vsnprintf(NULL, 0, fmt, argptr);
  va_end(argptr);

  if(nchars < 0) {
    fprintf(stderr, "Warning: strbuf_sprintf something went wrong..\n");
    exit_on_error();
  }

  // Save overwritten char
  last_char = (pos+(size_t)nchars < sbuf->end) ? sbuf->b[pos+(size_t)nchars] : 0;

  va_start(argptr, fmt);
  nchars = strbuf_vsprintf(sbuf, pos, fmt, argptr);
  va_end(argptr);

  if(nchars < 0) {
    fprintf(stderr, "Warning: strbuf_sprintf something went wrong..\n");
    exit_on_error();
  }

  // Restore length if shrunk, null terminate if extended
  if(sbuf->end < len) sbuf->end = len;
  else sbuf->b[sbuf->end] = '\0';

  // Re-instate overwritten character
  sbuf->b[pos+(size_t)nchars] = last_char;

  return nchars;
}


/*****************/
/* File handling */
/*****************/

// Reading a FILE
size_t strbuf_readline(StrBuf *sbuf, FILE *file)
{
  return freadline(file, &(sbuf->b), &(sbuf->end), &(sbuf->size));
}

size_t strbuf_gzreadline(StrBuf *sbuf, gzFile file)
{
  return gzreadline(file, &sbuf->b, &sbuf->end, &sbuf->size);
}

// Reading a FILE
size_t strbuf_readline_buf(StrBuf *sbuf, FILE *file, StreamBuffer *in)
{
  return (size_t)freadline_buf(file, in, &sbuf->b, &sbuf->end, &sbuf->size);
}

size_t strbuf_gzreadline_buf(StrBuf *sbuf, gzFile file, StreamBuffer *in)
{
  return (size_t)gzreadline_buf(file, in, &sbuf->b, &sbuf->end, &sbuf->size);
}

size_t strbuf_skipline(FILE* file)
{
  return fskipline(file);
}

size_t strbuf_gzskipline(gzFile file)
{
  return gzskipline(file);
}

size_t strbuf_skipline_buf(FILE* file, StreamBuffer *in)
{
  return (size_t)fskipline_buf(file, in);
}

size_t strbuf_gzskipline_buf(gzFile file, StreamBuffer *in)
{
  return (size_t)gzskipline_buf(file, in);
}

#define _func_read_nonempty(name,type_t,__readline)                            \
  size_t name(StrBuf *line, type_t fh)                                         \
  {                                                                            \
    size_t i, origlen = line->end;                                             \
    while(__readline(line, fh) > 0) {                                          \
      i = origlen;                                                             \
      while(i < line->end && (line->b[i] == '\r' || line->b[i] == '\n'))       \
        i++;                                                                   \
      if(i < line->end) return line->end - origlen;                            \
      line->end = origlen;                                                     \
      line->b[line->end] = '\0';                                               \
    }                                                                          \
    return 0;                                                                  \
  }

_func_read_nonempty(strbuf_readline_nonempty,FILE*,strbuf_readline)
_func_read_nonempty(strbuf_gzreadline_nonempty,gzFile,strbuf_gzreadline)


#define _func_read(name,type_t,__read) \
  size_t name(StrBuf *sbuf, type_t file, size_t len)                           \
  {                                                                            \
    if(len == 0) return 0;                                                     \
    strbuf_ensure_capacity(sbuf, sbuf->end + len);                             \
    long nread;                                                                \
    if((nread = (long)__read(file,sbuf->b+sbuf->end,len)) <= 0) return 0;      \
    sbuf->end += (size_t)nread;                                                \
    return (size_t)nread;                                                      \
  }

_func_read(strbuf_gzread, gzFile, gzread2)
_func_read(strbuf_fread, FILE*, fread2)

// read FILE
// returns number of characters read
// or 0 if EOF
size_t strbuf_reset_readline(StrBuf *sbuf, FILE *file)
{
  strbuf_reset(sbuf);
  return strbuf_readline(sbuf, file);
}

// read gzFile
// returns number of characters read
// or 0 if EOF
size_t strbuf_reset_gzreadline(StrBuf *sbuf, gzFile file)
{
  strbuf_reset(sbuf);
  return strbuf_gzreadline(sbuf, file);
}

/**********/
/*  trim  */
/**********/

// Trim whitespace characters from the start and end of a string
void strbuf_trim(StrBuf *sbuf)
{
  if(sbuf->end == 0)
    return;

  // Trim end first
  while(sbuf->end > 0 && isspace(sbuf->b[sbuf->end-1]))
    sbuf->end--;

  sbuf->b[sbuf->end] = '\0';

  if(sbuf->end == 0)
    return;

  size_t start = 0;

  while(start < sbuf->end && isspace(sbuf->b[start]))
    start++;

  if(start != 0)
  {
    sbuf->end -= start;
    memmove(sbuf->b, sbuf->b+start, sbuf->end * sizeof(char));
    sbuf->b[sbuf->end] = '\0';
  }
}

// Trim the characters listed in `list` from the left of `sbuf`
// `list` is a null-terminated string of characters
void strbuf_ltrim(StrBuf *sbuf, const char *list)
{
  size_t start = 0;

  while(start < sbuf->end && (strchr(list, sbuf->b[start]) != NULL))
    start++;

  if(start != 0)
  {
    sbuf->end -= start;
    memmove(sbuf->b, sbuf->b+start, sbuf->end * sizeof(char));
    sbuf->b[sbuf->end] = '\0';
  }
}

// Trim the characters listed in `list` from the right of `sbuf`
// `list` is a null-terminated string of characters
void strbuf_rtrim(StrBuf *sbuf, const char *list)
{
  if(sbuf->end == 0)
    return;

  while(sbuf->end > 0 && strchr(list, sbuf->b[sbuf->end-1]) != NULL)
    sbuf->end--;

  sbuf->b[sbuf->end] = '\0';
}

/**************************/
/* Other String Functions */
/**************************/

// `n` is the maximum number of bytes to copy including the NULL byte
// copies at most n bytes from `src` to `dst`
// Always appends a NULL terminating byte, unless n is zero.
// Returns a pointer to dst
char* string_safe_ncpy(char *dst, const char *src, size_t n)
{
  if(n == 0) return dst;

  // From The Open Group:
  //   The memccpy() function copies bytes from memory area s2 into s1, stopping
  //   after the first occurrence of byte c is copied, or after n bytes are copied,
  //   whichever comes first. If copying takes place between objects that overlap,
  //   the behaviour is undefined.
  // Returns NULL if character c was not found in the copied memory
  if(memccpy(dst, src, '\0', n-1) == NULL)
    dst[n-1] = '\0';

  return dst;
}

// Replaces `sep` with \0 in str
// Returns number of occurances of `sep` character in `str`
// Stores `nptrs` pointers in `ptrs`
size_t string_split_str(char *str, char sep, char **ptrs, size_t nptrs)
{
  size_t n = 1;

  if(*str == '\0') return 0;
  if(nptrs > 0) ptrs[0] = str;

  while((str = strchr(str, sep)) != NULL) {
    *str = '\0';
    str++;
    if(n < nptrs) ptrs[n] = str;
    n++;
  }
  return n;
}

// Replace one char with another in a string. Return number of replacements made
size_t string_char_replace(char *str, char from, char to)
{
  size_t n = 0;
  for(; *str; str++) {
    if(*str == from) { n++; *str = to; }
  }
  return n;
}

// Reverse a string region
void string_reverse_region(char *str, size_t length)
{
  char *a = str, *b = str + length - 1;
  char tmp;
  while(a < b) {
    tmp = *a; *a = *b; *b = tmp;
    a++; b--;
  }
}

char string_is_all_whitespace(const char *s)
{
  int i;
  for(i = 0; s[i] != '\0' && isspace(s[i]); i++);
  return (s[i] == '\0');
}

char* string_next_nonwhitespace(char *s)
{
  while(*s != '\0' && isspace(*s)) s++;
  return (*s == '\0' ? NULL : s);
}

// Strip whitespace the the start and end of a string.  
// Strips whitepace from the end of the string with \0, and returns pointer to
// first non-whitespace character
char* string_trim(char *str)
{
  // Work backwards
  char *end = str+strlen(str);
  while(end > str && isspace(*(end-1))) end--;
  *end = '\0';

  // Work forwards: don't need start < len because will hit \0
  while(isspace(*str)) str++;

  return str;
}

// Removes \r and \n from the ends of a string and returns the new length
size_t string_chomp(char *str, size_t len)
{
  while(len > 0 && (str[len-1] == '\r' || str[len-1] == '\n')) len--;
  str[len] = '\0';
  return len;
}

// Returns count
size_t string_count_char(const char *str, char c)
{
  size_t count = 0;

  while((str = strchr(str, c)) != NULL)
  {
    str++;
    count++;
  }

  return count;
}

// Returns the number of strings resulting from the split
size_t string_split(const char *split, const char *txt, char ***result)
{
  size_t split_len = strlen(split);
  size_t txt_len = strlen(txt);

  // result is temporarily held here
  char **arr;

  if(split_len == 0)
  {
    // Special case
    if(txt_len == 0)
    {
      *result = NULL;
      return 0;
    }
    else
    {
      arr = malloc(txt_len * sizeof(char*));
    
      size_t i;

      for(i = 0; i < txt_len; i++)
      {
        arr[i] = malloc(2 * sizeof(char));
        arr[i][0] = txt[i];
        arr[i][1] = '\0';
      }

      *result = arr;
      return txt_len;
    }
  }
  
  const char *find = txt;
  size_t count = 1; // must have at least one item

  for(; (find = strstr(find, split)) != NULL; count++, find += split_len) {}

  // Create return array
  arr = malloc(count * sizeof(char*));
  
  count = 0;
  const char *last_position = txt;

  size_t str_len;

  while((find = strstr(last_position, split)) != NULL)
  {
    str_len = (size_t)(find - last_position);

    arr[count] = malloc((str_len+1) * sizeof(char));
    strncpy(arr[count], last_position, str_len);
    arr[count][str_len] = '\0';
    
    count++;
    last_position = find + split_len;
  }

  // Copy last item
  str_len = (size_t)(txt + txt_len - last_position);
  arr[count] = malloc((str_len+1) * sizeof(char));

  if(count == 0) strcpy(arr[count], txt);
  else           strncpy(arr[count], last_position, str_len);

  arr[count][str_len] = '\0';
  count++;
  
  *result = arr;
  
  return count;
}
#endif /* STRING_BUFFER_FILE_SEEN */

/**********************************/

// #include "alignment_cmdline.h"
/*
 alignment_cmdline.h
 url: https://github.com/noporpoise/seq-align
 maintainer: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain, no warranty
 date: Nov 2013
 */

#ifndef ALIGNMENT_CMDLINE_HEADER_SEEN
#define ALIGNMENT_CMDLINE_HEADER_SEEN

// request decent POSIX version
#define _XOPEN_SOURCE 700
#define _BSD_SOURCE

#include <stdarg.h> // required for va_list
#include <stdbool.h>

enum SeqAlignCmdType {SEQ_ALIGN_SW_CMD, SEQ_ALIGN_NW_CMD, SEQ_ALIGN_LCS_CMD};

typedef struct
{
  // file inputs
  size_t file_list_length, file_list_capacity;
  char **file_paths1, **file_paths2;

  // All values initially 0
  bool case_sensitive;
  int match, mismatch, gap_open, gap_extend;

  // SW specific
  score_t min_score;
  unsigned int print_context, max_hits_per_alignment;
  bool min_score_set, max_hits_per_alignment_set;
  bool print_seq;

  // NW specific
  bool freestartgap_set, freeendgap_set;
  bool print_matrices, print_scores;
  bool zam_stle_output;

  // Turns off zlib for stdin
  bool interactive;

  // General output
  bool print_fasta, print_pretty, print_colour;

  // Experimental
  bool no_gaps_in1, no_gaps_in2;
  bool no_mismatches;

  // Pair of sequences to align
  const char *seq1, *seq2;
} cmdline_t;

char parse_entire_int(char *str, int *result);
char parse_entire_uint(char *str, unsigned int *result);

cmdline_t* cmdline_new(int argc, char **argv, scoring_t *scoring,
                       enum SeqAlignCmdType cmd_type);
void cmdline_free(cmdline_t* cmd);

void cmdline_add_files(cmdline_t* cmd, char* p1, char* p2);
size_t cmdline_get_num_of_file_pairs(cmdline_t* cmd);
char* cmdline_get_file1(cmdline_t* cmd, size_t i);
char* cmdline_get_file2(cmdline_t* cmd, size_t i);

void align_from_file(const char *path1, const char *path2,
                     void (align)(read_t *r1, read_t *r2),
                     bool use_zlib);


/*
 alignment_cmdline.c
 url: https://github.com/noporpoise/seq-align
 maintainer: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain, no warranty
 date: Nov 2013
 */

// Turn on debugging output by defining SEQ_ALIGN_VERBOSE
//#define SEQ_ALIGN_VERBOSE

// request decent POSIX version
#define _XOPEN_SOURCE 700
#define _BSD_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <limits.h> // INT_MIN
#include <stdarg.h> // for va_list

// File loading
int file_list_length = 0;
int file_list_capacity = 0;
char **file_paths1 = NULL, **file_paths2 = NULL;

char parse_entire_int(char *str, int *result)
{
  size_t len = strlen(str);

  char *strtol_last_char_ptr = str;
  long tmp = strtol(str, &strtol_last_char_ptr, 10);

  if(tmp > INT_MAX || tmp < INT_MIN || strtol_last_char_ptr != str+len)
  {
    return 0;
  }
  else
  {
    *result = (int)tmp;
    return 1;
  }
}

char parse_entire_uint(char *str, unsigned int *result)
{
  size_t len = strlen(str);

  char *strtol_last_char_ptr = str;
  unsigned long tmp = strtoul(str, &strtol_last_char_ptr, 10);

  if(tmp > UINT_MAX || strtol_last_char_ptr != str+len)
  {
    return 0;
  }
  else
  {
    *result = (unsigned int)tmp;
    return 1;
  }
}

static void print_usage(enum SeqAlignCmdType cmd_type, score_t defaults[4],
                        const char *cmdstr, const char *errfmt, ...)
  __attribute__((format(printf, 4, 5)))
  __attribute__((noreturn));

static void print_usage(enum SeqAlignCmdType cmd_type, score_t defaults[4],
                        const char *cmdstr, const char *errfmt, ...)
{
  if(errfmt != NULL) {
    fprintf(stderr, "Error: ");
    va_list argptr;
    va_start(argptr, errfmt);
    vfprintf(stderr, errfmt, argptr);
    va_end(argptr);

    if(errfmt[strlen(errfmt)-1] != '\n') fprintf(stderr, "\n");
  }

  fprintf(stderr, "usage: %s [OPTIONS] [seq1 seq2]\n", cmdstr);

  fprintf(stderr,
"  %s optimal %s alignment (maximises score).  \n"
"  Takes a pair of sequences on the command line, or can read from a\n"
"  file and from sequence piped in.  Can read gzip files, FASTA and FASTQ.\n\n",
          cmd_type == SEQ_ALIGN_SW_CMD ? "Smith-Waterman" : "Needleman-Wunsch",
          cmd_type == SEQ_ALIGN_SW_CMD ? "local" : "global");

  fprintf(stderr,
"  OPTIONS:\n"
"    --file <file>        Sequence file reading with gzip support - read two\n"
"                         sequences at a time and align them\n"
"    --files <f1> <f2>    Read one sequence from each file to align at one time\n"
"    --stdin              Read from STDIN (same as '--file -')\n"
"\n"
"    --case_sensitive     Use case sensitive character comparison [default: off]\n"
"\n"
"    --match <score>      [default: %i]\n"
"    --mismatch <score>   [default: %i]\n"
"    --gapopen <score>    [default: %i]\n"
"    --gapextend <score>  [default: %i]\n"
"\n"
"    --scoring <PAM30|PAM70|BLOSUM80|BLOSUM62>\n"
"    --substitution_matrix <file>  see details for formatting\n"
"    --substitution_pairs <file>   see details for formatting\n"
"\n"
"    --wildcard <w> <s>   Character <w> matches all characters with score <s>\n\n",
          defaults[0], defaults[1],
          defaults[2], defaults[3]);

  if(cmd_type == SEQ_ALIGN_SW_CMD)
  {
    // SW specific
    fprintf(stderr,
"    --minscore <score>   Minimum required score\n"
"                         [default: match * MAX(0.2 * length, 2)]\n"
"    --maxhits <hits>     Maximum number of results per alignment\n"
"                         [default: no limit]\n"
"\n"
"    --context <n>        Print <n> bases of context\n"
"    --printseq           Print sequences before local alignments\n");
  }
  else
  {
    // NW specific
    fprintf(stderr,
"\n"
"    --freestartgap       No penalty for gap at start of alignment\n"
"    --freeendgap         No penalty for gap at end of alignment\n"
"\n"
"    --printscores        Print optimal alignment scores\n"
"    --zam                A funky type of output\n");
  }

  fprintf(stderr,
"    --printmatrices      Print dynamic programming matrices\n"
"    --printfasta         Print fasta header lines\n"
"    --pretty             Print with a descriptor line\n"
"    --colour             Print with colour\n"
"\n"
"  Experimental Options:\n"
"    --nogapsin1          No gaps allowed within the first sequence\n"
"    --nogapsin2          No gaps allowed within the second sequence\n"
"    --nogaps             No gaps allowed in either sequence\n");

  fprintf(stderr,
"    --nomismatches       No mismatches allowed%s\n",
          cmd_type == SEQ_ALIGN_SW_CMD ? "" : " (cannot be used with --nogaps..)");

  printf(
"\n"
" DETAILS:\n"
"  * For help choosing scoring, see the README file. \n"
"  * Gap (of length N) penalty is: (open+N*extend)\n"
"  * To do alignment without affine gap penalty, set '--gapopen 0'.\n"
"  * Scoring files should be matrices, with entries separated by a single\n"
"    character or whitespace. See files in the 'scores' directory for examples.\n"
"\n"
"  turner.isaac@gmail.com  (compiled: %s %s)\n", __DATE__, __TIME__);

  exit(EXIT_FAILURE);
}

void cmdline_free(cmdline_t *cmd)
{
  free(cmd->file_paths1);
  free(cmd->file_paths2);
  free(cmd);
}

#define usage(fmt,...) print_usage(cmd_type,defaults,argv[0],fmt, ##__VA_ARGS__)

cmdline_t* cmdline_new(int argc, char **argv, scoring_t *scoring,
                       enum SeqAlignCmdType cmd_type)
{
  cmdline_t* cmd = calloc(1, sizeof(cmdline_t));
  cmd->file_list_length = 0;
  cmd->file_list_capacity = 256;
  cmd->file_paths1 = malloc(sizeof(char*) * cmd->file_list_capacity);
  cmd->file_paths2 = malloc(sizeof(char*) * cmd->file_list_capacity);
  cmd->seq1 = cmd->seq2 = NULL;
  // All values initially 0

  // Store defaults
  score_t defaults[4] = {scoring->match, scoring->mismatch,
                         scoring->gap_open, scoring->gap_extend};

  if(argc == 1) usage(NULL);

  // First run through arguments to set up case_sensitive and scoring system

  // case sensitive needs to be dealt with first
  // (is is used to construct hash table for swap_scores)
  char scoring_set = 0, substitutions_set = 0, match_set = 0, mismatch_set = 0;

  int argi;
  for(argi = 1; argi < argc; argi++)
  {
    if(strcasecmp(argv[argi], "--help") == 0 ||
       strcasecmp(argv[argi], "-help") == 0 ||
       strcasecmp(argv[argi], "-h") == 0)
    {
      usage(NULL);
    }
    else if(strcasecmp(argv[argi], "--case_sensitive") == 0)
    {
      cmd->case_sensitive = 1;
    }
    else if(strcasecmp(argv[argi], "--scoring") == 0)
    {
      if(scoring_set)
      {
        usage("More than one scoring system specified - not permitted");
      }

      if(strcasecmp(argv[argi+1], "PAM30") == 0)
      {
        scoring_system_PAM30(scoring);
      }
      else if(strcasecmp(argv[argi+1], "PAM70") == 0)
      {
        scoring_system_PAM70(scoring);
      }
      else if(strcasecmp(argv[argi+1], "BLOSUM80") == 0)
      {
        scoring_system_BLOSUM80(scoring);
      }
      else if(strcasecmp(argv[argi+1], "BLOSUM62") == 0)
      {
        scoring_system_BLOSUM62(scoring);
      }
      else if(strcasecmp(argv[argi+1], "DNA_HYBRIDIZATION") == 0)
      {
        scoring_system_DNA_hybridization(scoring);
      }
      else {
        usage("Unknown --scoring choice, not one of "
              "PAM30|PAM70|BLOSUM80|BLOSUM62");
      }

      scoring_set = 1;
      argi++; // took an argument
    }
  }

  for(argi = 1; argi < argc; argi++)
  {
    if(argv[argi][0] == '-')
    {
      // strcasecmp does case insensitive comparison
      if(strcasecmp(argv[argi], "--freestartgap") == 0)
      {
        if(cmd_type != SEQ_ALIGN_NW_CMD)
          usage("--freestartgap only valid with Needleman-Wunsch");
        scoring->no_start_gap_penalty = true;
      }
      else if(strcasecmp(argv[argi], "--freeendgap") == 0)
      {
        if(cmd_type != SEQ_ALIGN_NW_CMD)
          usage("--freeendgap only valid with Needleman-Wunsch");
        scoring->no_end_gap_penalty = true;
      }
      else if(strcasecmp(argv[argi], "--nogaps") == 0)
      {
        scoring->no_gaps_in_a = true;
        scoring->no_gaps_in_b = true;
      }
      else if(strcasecmp(argv[argi], "--nogapsin1") == 0)
      {
        scoring->no_gaps_in_a = true;
      }
      else if(strcasecmp(argv[argi], "--nogapsin2") == 0)
      {
        scoring->no_gaps_in_b = true;
      }
      else if(strcasecmp(argv[argi], "--nomismatches") == 0)
      {
        scoring->no_mismatches = true;
      }
      else if(strcasecmp(argv[argi], "--case_sensitive") == 0)
      {
        // Already dealt with
        //case_sensitive = true;
      }
      else if(strcasecmp(argv[argi], "--printseq") == 0)
      {
        if(cmd_type != SEQ_ALIGN_SW_CMD)
          usage("--printseq only valid with Smith-Waterman");
        cmd->print_seq = true;
      }
      else if(strcasecmp(argv[argi], "--printmatrices") == 0)
      {
        cmd->print_matrices = true;
      }
      else if(strcasecmp(argv[argi], "--printscores") == 0)
      {
        if(cmd_type != SEQ_ALIGN_NW_CMD)
          usage("--printscores only valid with Needleman-Wunsch");
        cmd->print_scores = true;
      }
      else if(strcasecmp(argv[argi], "--printfasta") == 0)
      {
        cmd->print_fasta = true;
      }
      else if(strcasecmp(argv[argi], "--pretty") == 0)
      {
        cmd->print_pretty = true;
      }
      else if(strcasecmp(argv[argi], "--colour") == 0)
      {
        cmd->print_colour = true;
      }
      else if(strcasecmp(argv[argi], "--zam") == 0)
      {
        if(cmd_type != SEQ_ALIGN_NW_CMD)
          usage("--zam only valid with Needleman-Wunsch");
        cmd->zam_stle_output = true;
      }
      else if(strcasecmp(argv[argi], "--stdin") == 0)
      {
        // Similar to --file argument below
        cmdline_add_files(cmd, "", NULL);
        cmd->interactive = true;
      }
      else if(argi == argc-1)
      {
        // All the remaining options take an extra argument
        usage("Unknown argument without parameter: %s", argv[argi]);
      }
      else if(strcasecmp(argv[argi], "--scoring") == 0)
      {
        // This handled above
        argi++; // took an argument
      }
      else if(strcasecmp(argv[argi], "--substitution_matrix") == 0)
      {
        gzFile sub_matrix_file = gzopen(argv[argi+1], "r");
        if(sub_matrix_file == NULL) usage("Couldn't read: %s", argv[argi+1]);

        align_scoring_load_matrix(sub_matrix_file, argv[argi+1],
                                  scoring, cmd->case_sensitive);

        gzclose(sub_matrix_file);
        substitutions_set = true;

        argi++; // took an argument
      }
      else if(strcasecmp(argv[argi], "--substitution_pairs") == 0)
      {
        gzFile sub_pairs_file = gzopen(argv[argi+1], "r");
        if(sub_pairs_file == NULL) usage("Couldn't read: %s", argv[argi+1]);

        align_scoring_load_pairwise(sub_pairs_file, argv[argi+1],
                                    scoring, cmd->case_sensitive);

        gzclose(sub_pairs_file);
        substitutions_set = true;

        argi++; // took an argument
      }
      else if(strcasecmp(argv[argi], "--minscore") == 0)
      {
        if(cmd_type != SEQ_ALIGN_SW_CMD)
          usage("--minscore only valid with Smith-Waterman");

        if(!parse_entire_int(argv[argi+1], &cmd->min_score))
          usage("Invalid --minscore <score> argument (must be a +ve int)");

        cmd->min_score_set = true;

        argi++;
      }
      else if(strcasecmp(argv[argi], "--maxhits") == 0)
      {
        if(cmd_type != SEQ_ALIGN_SW_CMD)
          usage("--maxhits only valid with Smith-Waterman");

        if(!parse_entire_uint(argv[argi+1], &cmd->max_hits_per_alignment))
          usage("Invalid --maxhits <hits> argument (must be a +ve int)");

        cmd->max_hits_per_alignment_set = true;

        argi++;
      }
      else if(strcasecmp(argv[argi], "--context") == 0)
      {
        if(cmd_type != SEQ_ALIGN_SW_CMD)
          usage("--context only valid with Smith-Waterman");

        if(!parse_entire_uint(argv[argi+1], &cmd->print_context))
          usage("Invalid --context <c> argument (must be >= 0)");

        argi++;
      }
      else if(strcasecmp(argv[argi], "--match") == 0)
      {
        if(!parse_entire_int(argv[argi+1], &scoring->match))
        {
          usage("Invalid --match argument ('%s') must be an int", argv[argi+1]);
        }

        match_set = true;
        argi++; // took an argument
      }
      else if(strcasecmp(argv[argi], "--mismatch") == 0)
      {
        if(!parse_entire_int(argv[argi+1], &scoring->mismatch))
        {
          usage("Invalid --mismatch argument ('%s') must be an int", argv[argi+1]);
        }

        mismatch_set = true;
        argi++; // took an argument
      }
      else if(strcasecmp(argv[argi], "--gapopen") == 0)
      {
        if(!parse_entire_int(argv[argi+1], &scoring->gap_open))
        {
          usage("Invalid --gapopen argument ('%s') must be an int", argv[argi+1]);
        }

        argi++; // took an argument
      }
      else if(strcasecmp(argv[argi], "--gapextend") == 0)
      {
        if(!parse_entire_int(argv[argi+1], &scoring->gap_extend))
        {
          usage("Invalid --gapextend argument ('%s') must be an int",
                argv[argi+1]);
        }

        argi++; // took an argument
      }
      else if(strcasecmp(argv[argi], "--file") == 0)
      {
        cmdline_add_files(cmd, argv[argi+1], NULL);
        argi++; // took an argument
      }
      // Remaining options take two arguments but check themselves
      else if(strcasecmp(argv[argi], "--files") == 0)
      {
        if(argi >= argc-2)
        {
          usage("--files option takes 2 arguments");
        }
        else if(strcmp(argv[argi+1], "-") == 0 && strcmp(argv[argi+2], "-") == 0)
        {
          // Read both from stdin
          cmdline_add_files(cmd, argv[argi+1], NULL);
        }
        else
        {
          cmdline_add_files(cmd, argv[argi+1], argv[argi+2]);
        }

        argi += 2; // took two arguments
      }
      else if(strcasecmp(argv[argi], "--wildcard") == 0)
      {
        int wildscore = 0;

        if(argi == argc-2 || strlen(argv[argi+1]) != 1 ||
           !parse_entire_int(argv[argi+2], &wildscore))
        {
          usage("--wildcard <w> <s> takes a single character and a number");
        }

        scoring_add_wildcard(scoring, argv[argi+1][0], wildscore);

        argi += 2; // took two arguments
      }
      else usage("Unknown argument '%s'", argv[argi]);
    }
    else
    {
      if(argc - argi != 2) usage("Unknown options: '%s'", argv[argi]);
      break;
    }
  }

  if((match_set && !mismatch_set && !scoring->no_mismatches) ||
     (!match_set && mismatch_set))
  {
    usage("--match --mismatch must both be set or neither set");
  }
  else if(substitutions_set && !match_set)
  {
    // if substitution table set and not match/mismatch
    scoring->use_match_mismatch = 0;
  }

  if(scoring->use_match_mismatch && scoring->match < scoring->mismatch) {
    usage("Match value should not be less than mismatch penalty");
  }

  // Cannot guarantee that we can perform a global alignment if nomismatches
  // and nogaps is true
  if(cmd_type == SEQ_ALIGN_NW_CMD && scoring->no_mismatches &&
     (scoring->no_gaps_in_a || scoring->no_gaps_in_b))
  {
    usage("--nogaps.. --nomismatches cannot be used at together");
  }

  // Check for extra unused arguments
  // and set seq1 and seq2 if they have been passed
  if(argi < argc)
  {
    cmd->seq1 = argv[argi];
    cmd->seq2 = argv[argi+1];
  }

  if(cmd->seq1 == NULL && cmd->file_list_length == 0)
  {
    usage("No input specified");
  }

  if(cmd->zam_stle_output &&
     (cmd->print_pretty || cmd->print_scores ||
      cmd->print_colour || cmd->print_fasta))
  {
    usage("Cannot use --printscore, --printfasta, --pretty or --colour with "
          "--zam");
  }

  return cmd;
}


void cmdline_add_files(cmdline_t *cmd, char* p1, char* p2)
{
  if(cmd->file_list_length == cmd->file_list_capacity)
  {
    cmd->file_list_capacity *= 2;
    size_t mem = sizeof(char*) * cmd->file_list_capacity;
    cmd->file_paths1 = realloc(cmd->file_paths1, mem);
    cmd->file_paths2 = realloc(cmd->file_paths2, mem);

    if(cmd->file_paths1 == NULL || cmd->file_paths2 == NULL) {
      fprintf(stderr, "%s:%i: Out of memory\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }
  }

  cmd->file_paths1[cmd->file_list_length] = p1;
  cmd->file_paths2[cmd->file_list_length] = p2;
  cmd->file_list_length++;
}

size_t cmdline_get_num_of_file_pairs(cmdline_t *cmd)
{
  return cmd->file_list_length;
}

char* cmdline_get_file1(cmdline_t *cmd, size_t i)
{
  return cmd->file_paths1[i];
}

char* cmdline_get_file2(cmdline_t *cmd, size_t i)
{
  return cmd->file_paths2[i];
}

static seq_file_t* open_seq_file(const char *path, bool use_zlib)
{
  return (strcmp(path,"-") != 0 || use_zlib) ? seq_open(path)
                                             : seq_dopen(fileno(stdin), false, false, 0);
}

// If seq2 is NULL, read pair of entries from first file
// Otherwise read an entry from each
void align_from_file(const char *path1, const char *path2,
                     void (align)(read_t *r1, read_t *r2),
                     bool use_zlib)
{
  seq_file_t *sf1, *sf2;

  if((sf1 = open_seq_file(path1, use_zlib)) == NULL)
  {
    fprintf(stderr, "Alignment Error: couldn't open file %s\n", path1);
    fflush(stderr);
    return;
  }

  if(path2 == NULL)
  {
    sf2 = sf1;
  }
  else if((sf2 = open_seq_file(path2, use_zlib)) == NULL)
  {
    fprintf(stderr, "Alignment Error: couldn't open file %s\n", path1);
    fflush(stderr);
    return;
  }

  // fprintf(stderr, "File buffer %zu zlib: %i\n", sf1->in.size, seq_use_gzip(sf1));

  read_t read1, read2;
  seq_read_alloc(&read1);
  seq_read_alloc(&read2);

  // Loop while we can read a sequence from the first file
  unsigned long alignments;

  for(alignments = 0; seq_read(sf1, &read1) > 0; alignments++)
  {
    if(seq_read(sf2, &read2) <= 0)
    {
      fprintf(stderr, "Alignment Error: Odd number of sequences - "
                      "I read in pairs!\n");
      fflush(stderr);
      break;
    }

    (align)(&read1, &read2);
  }

  // warn if no bases read
  if(alignments == 0)
  {
    fprintf(stderr, "Alignment Warning: empty input\n");
    fflush(stderr);
  }

  // Close files
  seq_close(sf1);

  if(path2 != NULL)
    seq_close(sf2);

  // Free memory
  seq_read_dealloc(&read1);
  seq_read_dealloc(&read2);
}


#endif

/******************************/

static void _loading_error(const char* err_msg, const char* file_path,
                           int line_num, char is_matrix)
  __attribute__((noreturn));

static void _loading_error(const char* err_msg, const char* file_path,
                           int line_num, char is_matrix)
{
  if(is_matrix) fprintf(stderr, "Error: substitution matrix : %s\n", err_msg);
  else fprintf(stderr, "Error: substitution pairs : %s\n", err_msg);

  if(file_path != NULL) fprintf(stderr, "File: %s\n", file_path);
  if(line_num != -1) fprintf(stderr, "Line: %s\n", file_path);

  exit(EXIT_FAILURE);
}

void align_scoring_load_matrix(gzFile file, const char* file_path,
                               scoring_t* scoring, char case_sensitive)
{
  StrBuf* sbuf = strbuf_new(500);
  size_t read_length;
  int line_num = 0;

  // Read first line (column headings)
  while((read_length = strbuf_reset_gzreadline(sbuf, file)) > 0)
  {
    strbuf_chomp(sbuf);

    if(sbuf->end > 0 && sbuf->b[0] != '#' && // line is not empty, not comment
       !string_is_all_whitespace(sbuf->b)) // and not whitespace
    {
      // Read first line

      if(sbuf->end < 2)
      {
        _loading_error("Too few column headings", file_path, line_num, 1);
      }

      break;
    }

    line_num++;
  }

  if(line_num == 0 && sbuf->end <= 0)
  {
    _loading_error("Empty file", file_path, -1, 0);
  }

  // If the separator character is whitespace,
  // the set of whitespace characters is used
  char sep = sbuf->b[0];

  if((sep >= (int)'0' && sep <= (int)'9') || sep == '-')
  {
    _loading_error("Numbers (0-9) and dashes (-) do not make good separators",
                   file_path, line_num, 0);
  }

  char* characters = (char*)malloc(sbuf->end);
  int num_of_chars = 0;

  if(isspace(sep))
  {
    char* next = sbuf->b;

    while((next = string_next_nonwhitespace(next+1)) != NULL)
    {
      characters[num_of_chars++] = case_sensitive ? *next : tolower(*next);
    }

    // Now read lines below
    while((read_length = strbuf_reset_gzreadline(sbuf, file)) > 0)
    {
      strbuf_chomp(sbuf);

      char* from_char_pos = string_next_nonwhitespace(sbuf->b);

      if(from_char_pos == NULL || sbuf->b[0] == '#')
      {
        // skip this line
        continue;
      }

      char from_char = case_sensitive ? *from_char_pos : tolower(*from_char_pos);
      char to_char;

      char* score_txt = sbuf->b+1;
      int score;

      int i;
      for(i = 0; i < num_of_chars; i++)
      {
        to_char = characters[i];

        if(!isspace(*score_txt))
        {
          _loading_error("Expected whitespace between elements - found character",
                         file_path, line_num, 1);
        }

        score_txt = string_next_nonwhitespace(score_txt+1);

        char* strtol_last_char_ptr = score_txt;
        score = (int)strtol(strtol_last_char_ptr, &strtol_last_char_ptr, 10);

        // If pointer to end of number string hasn't moved -> error
        if(strtol_last_char_ptr == score_txt)
        {
          _loading_error("Missing number value on line", file_path, line_num, 1);
        }

        scoring_add_mutation(scoring, from_char, to_char, score);

        score_txt = strtol_last_char_ptr;
      }

      if(*score_txt != '\0' && !string_is_all_whitespace(score_txt))
      {
        _loading_error("Too many columns on row", file_path, line_num, 1);
      }

      line_num++;
    }
  }
  else
  {
    size_t i;

    for(i = 0; i < sbuf->end; i += 2)
    {
      if(sbuf->b[i] != sep)
      {
        _loading_error("Separator missing from line", file_path, line_num, 1);
      }

      char c = case_sensitive ? sbuf->b[i+1] : tolower(sbuf->b[i+1]);
      characters[num_of_chars++] = c;
    }

    int score;

    // Read rows
    while((read_length = strbuf_reset_gzreadline(sbuf, file)) > 0)
    {
      strbuf_chomp(sbuf);

      char from_char = case_sensitive ? sbuf->b[0] : tolower(sbuf->b[0]);

      if(from_char == '#' || string_is_all_whitespace(sbuf->b))
      {
        // skip this line
        continue;
      }

      char* str_pos = sbuf->b;

      int to_char_index = 0;
      char to_char;

      while(*str_pos != '\0')
      {
        to_char = characters[to_char_index++];

        if(*str_pos != sep)
        {
          _loading_error("Separator missing from line", file_path, line_num, 1);
        }

        // Move past separator
        str_pos++;

        char* after_num_str = str_pos;
        score = (int)strtol(str_pos, &after_num_str, 10);

        // If pointer to end of number string hasn't moved -> error
        if(str_pos == after_num_str)
        {
          _loading_error("Missing number value on line", file_path, line_num, 1);
        }

        if(to_char_index >= num_of_chars)
        {
          _loading_error("Too many columns on row", file_path, line_num, 1);
        }

        scoring_add_mutation(scoring, from_char, to_char, score);

        str_pos = after_num_str;
      }

      line_num++;
    }
  }

  free(characters);
  strbuf_free(sbuf);
}


void align_scoring_load_pairwise(gzFile file, const char* file_path,
                                 scoring_t* scoring, char case_sensitive)
{
  StrBuf* sbuf = strbuf_new(200);
  size_t read_length;
  int line_num = 0;

  char a, b;
  int score;

  int num_pairs_added = 0;

  while((read_length = strbuf_reset_gzreadline(sbuf, file)) > 0)
  {
    strbuf_chomp(sbuf);

    if(sbuf->end > 0 && sbuf->b[0] != '#' && // line is not empty, not comment
       !string_is_all_whitespace(sbuf->b)) // and not whitespace
    {
      if(read_length < 5)
      {
        _loading_error("Too few column headings", file_path, line_num, 0);
      }

      if(isspace(sbuf->b[1]))
      {
        // split by whitespace
        a = sbuf->b[0];

        size_t char2_pos;

        for(char2_pos = 1;
            sbuf->b[char2_pos] != '\0' && isspace(sbuf->b[char2_pos]);
            char2_pos++);

        if(char2_pos+2 >= sbuf->end || !isspace(sbuf->b[char2_pos+1]))
        {
          _loading_error("Line too short", file_path, line_num, 0);
        }

        b = sbuf->b[char2_pos];

        if(!parse_entire_int(sbuf->b+char2_pos+2, &score))
        {
          _loading_error("Invalid number", file_path, line_num, 0);
        }
      }
      else
      {
        if(sbuf->b[1] != sbuf->b[3])
        {
          _loading_error("Inconsistent separators used", file_path, line_num, 0);
        }

        a = sbuf->b[0];
        b = sbuf->b[2];

        if(!parse_entire_int(sbuf->b + 4, &score))
        {
          _loading_error("Invalid number", file_path, line_num, 0);
        }
      }

      if(!case_sensitive)
      {
        a = tolower(a);
        b = tolower(b);
      }

      scoring_add_mutation(scoring, a, b, score);
      num_pairs_added++;
    }

    line_num++;
  }

  strbuf_free(sbuf);

  if(num_pairs_added == 0)
  {
    _loading_error("No pairs added from file (file empty?)",
                   file_path, line_num, 0);
  }
}

#endif


#ifndef ROUNDUP2POW
  #define ROUNDUP2POW(x) _rndup2pow64(x)
  static inline size_t _rndup2pow64(unsigned long long x) {
    // long long >=64 bits guaranteed in C99
    --x; x|=x>>1; x|=x>>2; x|=x>>4; x|=x>>8; x|=x>>16; x|=x>>32; ++x;
    return x;
  }
#endif

typedef struct
{
  const scoring_t* scoring;
  const char *seq_a, *seq_b;
  size_t score_width, score_height; // width=len(seq_a)+1, height=len(seq_b)+1
  score_t *match_scores, *gap_a_scores, *gap_b_scores;
  size_t capacity;
} aligner_t;

// Store alignment result here
typedef struct
{
  char *result_a, *result_b;
  size_t capacity, length;
  size_t pos_a, pos_b; // position of first base (0-based)
  size_t len_a, len_b; // number of bases in alignment
  score_t score;
} alignment_t;

// Matrix names
enum Matrix { MATCH,GAP_A,GAP_B };

#define MATRIX_NAME(x) ((x) == MATCH ? "MATCH" : ((x) == GAP_A ? "GAP_A" : "GAP_B"))

// Printing colour codes
extern const char align_col_mismatch[], align_col_indel[], align_col_context[],
                  align_col_stop[];

#define aligner_init(a) (memset(a, 0, sizeof(aligner_t)))
void aligner_align(aligner_t *aligner,
                   const char *seq_a, const char *seq_b,
                   size_t len_a, size_t len_b,
                   const scoring_t *scoring, char is_sw);
void aligner_destroy(aligner_t *aligner);

// Constructors/Destructors for alignment
alignment_t* alignment_create(size_t capacity);
void alignment_ensure_capacity(alignment_t* result, size_t strlength);
void alignment_free(alignment_t* result);

void alignment_reverse_move(enum Matrix *curr_matrix, score_t *curr_score,
                            size_t *score_x, size_t *score_y,
                            size_t *arr_index, const aligner_t *aligner);

// Printing
void alignment_print_matrices(const aligner_t *aligner);

void alignment_colour_print_against(const char *alignment_a,
                                    const char *alignment_b,
                                    char case_sensitive);

void alignment_print_spacer(const char* alignment_a, const char* alignment_b,
                            const scoring_t* scoring);

/*
 alignment.c
 url: https://github.com/noporpoise/seq-align
 maintainer: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain, no warranty
 date: Feb 2015
 */

// Turn on debugging output by defining SEQ_ALIGN_VERBOSE
//#define SEQ_ALIGN_VERBOSE

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h> // tolower
#include <assert.h>

const char align_col_mismatch[] = "\033[92m"; // Mismatch (GREEN)
const char align_col_indel[] = "\033[91m"; // Insertion / deletion (RED)
// Pink used by SmithWaterman local alignment for printing surrounding bases
const char align_col_context[] = "\033[95m";
const char align_col_stop[] = "\033[0m";

// Fill in traceback matrix
static void alignment_fill_matrices(aligner_t *aligner, char is_sw)
{
  score_t *match_scores = aligner->match_scores;
  score_t *gap_a_scores = aligner->gap_a_scores;
  score_t *gap_b_scores = aligner->gap_b_scores;
  const scoring_t *scoring = aligner->scoring;
  size_t score_width = aligner->score_width;
  size_t score_height = aligner->score_height;
  size_t i, j;

  int gap_open_penalty = scoring->gap_extend + scoring->gap_open;
  int gap_extend_penalty = scoring->gap_extend;

  const score_t min = is_sw ? 0 : SCORE_MIN + abs(scoring->min_penalty);

  size_t seq_i, seq_j, len_i = score_width-1, len_j = score_height-1;
  size_t index, index_left, index_up, index_upleft;

  // [0][0]
  match_scores[0] = 0;
  gap_a_scores[0] = 0;
  gap_b_scores[0] = 0;

  if(is_sw)
  {
    for(i = 1; i < score_width; i++)
      match_scores[i] = gap_a_scores[i] = gap_b_scores[i] = 0;
    for(j = 1, index = score_width; j < score_height; j++, index += score_width)
      match_scores[index] = gap_a_scores[index] = gap_b_scores[index] = min;
  }
  else
  {
    // work along first row -> [i][0]
    for(i = 1; i < score_width; i++)
    {
      match_scores[i] = min;

      // Think carefully about which way round these are
      gap_a_scores[i] = min;
      gap_b_scores[i] = scoring->no_start_gap_penalty ? 0
                        : scoring->gap_open + (int)i * scoring->gap_extend;
    }

    // work down first column -> [0][j]
    for(j = 1, index = score_width; j < score_height; j++, index += score_width)
    {
      match_scores[index] = min;

      // Think carefully about which way round these are
      gap_a_scores[index] = scoring->no_start_gap_penalty ? 0
                            : scoring->gap_open + (int)j * scoring->gap_extend;
      gap_b_scores[index] = min;
    }
  }

  // start at position [1][1]
  index_upleft = 0;
  index_up = 1;
  index_left = score_width;
  index = score_width+1;

  for(seq_j = 0; seq_j < len_j; seq_j++)
  {
    for(seq_i = 0; seq_i < len_i; seq_i++)
    {

      // Update match_scores[i][j] with position [i-1][j-1]
      // substitution penalty
      bool is_match;
      int substitution_penalty;

      scoring_lookup(scoring, aligner->seq_a[seq_i], aligner->seq_b[seq_j],
                     &substitution_penalty, &is_match);

      if(scoring->no_mismatches && !is_match)
      {
        match_scores[index] = min;
      }
      else
      {
        // substitution
        // 1) continue alignment
        // 2) close gap in seq_a
        // 3) close gap in seq_b
        match_scores[index]
          = MAX4(match_scores[index_upleft] + substitution_penalty,
                 gap_a_scores[index_upleft] + substitution_penalty,
                 gap_b_scores[index_upleft] + substitution_penalty,
                 min);
      }

      // Long arithmetic since some INTs are set to min and penalty is -ve
      // (adding as ints would cause an integer overflow)

      // Update gap_a_scores[i][j] from position [i][j-1]
      if(seq_i == len_i-1 && scoring->no_end_gap_penalty)
      {
        gap_a_scores[index] = MAX3(match_scores[index_up],
                                   gap_a_scores[index_up],
                                   gap_b_scores[index_up]);
      }
      else if(!scoring->no_gaps_in_a || seq_i == len_i-1)
      {
        gap_a_scores[index]
          = MAX4(match_scores[index_up] + gap_open_penalty,
                 gap_a_scores[index_up] + gap_extend_penalty,
                 gap_b_scores[index_up] + gap_open_penalty,
                 min);
      }
      else
        gap_a_scores[index] = min;

      // Update gap_b_scores[i][j] from position [i-1][j]
      if(seq_j == len_j-1 && scoring->no_end_gap_penalty)
      {
        gap_b_scores[index] = MAX3(match_scores[index_left],
                                   gap_a_scores[index_left],
                                   gap_b_scores[index_left]);
      }
      else if(!scoring->no_gaps_in_b || seq_j == len_j-1)
      {
        gap_b_scores[index]
          = MAX4(match_scores[index_left] + gap_open_penalty,
                 gap_a_scores[index_left] + gap_open_penalty,
                 gap_b_scores[index_left] + gap_extend_penalty,
                 min);
      }
      else
        gap_b_scores[index] = min;

      index++;
      index_left++;
      index_up++;
      index_upleft++;
    }

    index++;
    index_left++;
    index_up++;
    index_upleft++;
  }
}

void aligner_align(aligner_t *aligner,
                   const char *seq_a, const char *seq_b,
                   size_t len_a, size_t len_b,
                   const scoring_t *scoring, char is_sw)
{
  aligner->scoring = scoring;
  aligner->seq_a = seq_a;
  aligner->seq_b = seq_b;
  aligner->score_width = len_a+1;
  aligner->score_height = len_b+1;

  size_t new_capacity = aligner->score_width * aligner->score_height;

  if(aligner->capacity < new_capacity)
  {
    aligner->capacity = ROUNDUP2POW(new_capacity);
    size_t mem = sizeof(score_t) * aligner->capacity;
    aligner->match_scores = realloc(aligner->match_scores, mem);
    aligner->gap_a_scores = realloc(aligner->gap_a_scores, mem);
    aligner->gap_b_scores = realloc(aligner->gap_b_scores, mem);
  }

  alignment_fill_matrices(aligner, is_sw);
}

void aligner_destroy(aligner_t *aligner)
{
  if(aligner->capacity > 0) {
    free(aligner->match_scores);
    free(aligner->gap_a_scores);
    free(aligner->gap_b_scores);
  }
}


alignment_t* alignment_create(size_t capacity)
{
  capacity = ROUNDUP2POW(capacity);
  alignment_t *result = malloc(sizeof(alignment_t));
  result->result_a = malloc(sizeof(char)*capacity);
  result->result_b = malloc(sizeof(char)*capacity);
  result->capacity = capacity;
  result->length = 0;
  result->result_a[0] = result->result_b[0] = '\0';
  result->pos_a = result->pos_b = result->len_a = result->len_b = 0;
  result->score = 0;
  return result;
}

void alignment_ensure_capacity(alignment_t* result, size_t strlength)
{
  size_t capacity = strlength+1;
  if(result->capacity < capacity)
  {
    capacity = ROUNDUP2POW(capacity);
    result->result_a = realloc(result->result_a, sizeof(char)*capacity);
    result->result_b = realloc(result->result_b, sizeof(char)*capacity);
    result->capacity = capacity;
    if(result->result_a == NULL || result->result_b == NULL) {
      fprintf(stderr, "%s:%i: Out of memory\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }
  }
}

void alignment_free(alignment_t* result)
{
  free(result->result_a);
  free(result->result_b);
  free(result);
}


// Backtrack through scoring matrices
void alignment_reverse_move(enum Matrix *curr_matrix, score_t *curr_score,
                            size_t *score_x, size_t *score_y,
                            size_t *arr_index, const aligner_t *aligner)
{
  size_t seq_x = (*score_x)-1, seq_y = (*score_y)-1;
  size_t len_i = aligner->score_width-1, len_j = aligner->score_height-1;

  bool is_match;
  int match_penalty;
  const scoring_t *scoring = aligner->scoring;

  scoring_lookup(scoring, aligner->seq_a[seq_x], aligner->seq_b[seq_y],
                 &match_penalty, &is_match);

  int gap_a_open_penalty, gap_b_open_penalty;
  int gap_a_extend_penalty, gap_b_extend_penalty;

  gap_a_open_penalty = gap_b_open_penalty = scoring->gap_extend + scoring->gap_open;
  gap_a_extend_penalty = gap_b_extend_penalty = scoring->gap_extend;

  // Free gaps at the ends
  if(scoring->no_end_gap_penalty) {
    if(*score_x == len_i) gap_a_open_penalty = gap_a_extend_penalty = 0;
    if(*score_y == len_j) gap_b_open_penalty = gap_b_extend_penalty = 0;
  }
  if(scoring->no_start_gap_penalty) {
    if(*score_x == 0) gap_a_open_penalty = gap_a_extend_penalty = 0;
    if(*score_y == 0) gap_b_open_penalty = gap_b_extend_penalty = 0;
  }

  int prev_match_penalty, prev_gap_a_penalty, prev_gap_b_penalty;

  switch(*curr_matrix)
  {
    case MATCH:
      prev_match_penalty = match_penalty;
      prev_gap_a_penalty = match_penalty;
      prev_gap_b_penalty = match_penalty;
      (*score_x)--;
      (*score_y)--;
      (*arr_index) -= aligner->score_width + 1;
      break;

    case GAP_A:
      prev_match_penalty = gap_a_open_penalty;
      prev_gap_a_penalty = gap_a_extend_penalty;
      prev_gap_b_penalty = gap_a_open_penalty;
      (*score_y)--;
      (*arr_index) -= aligner->score_width;
      break;

    case GAP_B:
      prev_match_penalty = gap_b_open_penalty;
      prev_gap_a_penalty = gap_b_open_penalty;
      prev_gap_b_penalty = gap_b_extend_penalty;
      (*score_x)--;
      (*arr_index)--;
      break;

    default:
      fprintf(stderr, "Program error: invalid matrix in get_reverse_move()\n");
      fprintf(stderr, "Please submit a bug report to: turner.isaac@gmail.com\n");
      exit(EXIT_FAILURE);
  }

  // *arr_index = ARR_2D_INDEX(aligner->score_width, *score_x, *score_y);

  if((!scoring->no_gaps_in_a || *score_x == 0 || *score_x == len_i) &&
     aligner->gap_a_scores[*arr_index] + prev_gap_a_penalty == *curr_score)
  {
    *curr_matrix = GAP_A;
    *curr_score = aligner->gap_a_scores[*arr_index];
  }
  else if((!scoring->no_gaps_in_b || *score_y == 0 || *score_y == len_j) &&
          aligner->gap_b_scores[*arr_index] + prev_gap_b_penalty == *curr_score)
  {
    *curr_matrix = GAP_B;
    *curr_score = aligner->gap_b_scores[*arr_index];
  }
  else if(aligner->match_scores[*arr_index] + prev_match_penalty == *curr_score)
  {
    *curr_matrix = MATCH;
    *curr_score = aligner->match_scores[*arr_index];
  }
  else
  {
    alignment_print_matrices(aligner);

    fprintf(stderr, "[%s:%zu,%zu]: %i [ismatch: %i] '%c' '%c'\n",
            MATRIX_NAME(*curr_matrix), *score_x, *score_y, *curr_score,
            is_match, aligner->seq_a[seq_x], aligner->seq_b[seq_y]);
    fprintf(stderr, " Penalties match: %i gap_open: %i gap_extend: %i\n",
            prev_match_penalty, prev_gap_a_penalty, prev_gap_b_penalty);
    fprintf(stderr, " Expected MATCH: %i GAP_A: %i GAP_B: %i\n",
            aligner->match_scores[*arr_index],
            aligner->gap_a_scores[*arr_index],
            aligner->gap_b_scores[*arr_index]);

    fprintf(stderr,
"Program error: traceback fail (get_reverse_move)\n"
"This may be due to an integer overflow if your sequences are long or scores\n"
"are large. If this is the case using smaller scores or shorter sequences may\n"
"work around this problem.  \n"
"  If you think this is a bug, please report it to: turner.isaac@gmail.com\n");
    exit(EXIT_FAILURE);
  }
}


void alignment_print_matrices(const aligner_t *aligner)
{
  const score_t* match_scores = aligner->match_scores;
  const score_t* gap_a_scores = aligner->gap_a_scores;
  const score_t* gap_b_scores = aligner->gap_b_scores;

  size_t i, j;

  printf("seq_a: %.*s\nseq_b: %.*s\n",
         (int)aligner->score_width-1, aligner->seq_a,
         (int)aligner->score_height-1, aligner->seq_b);

  printf("match_scores:\n");
  for(j = 0; j < aligner->score_height; j++)
  {
    printf("%3i:", (int)j);
    for(i = 0; i < aligner->score_width; i++)
    {
      printf("\t%3i", (int)ARR_LOOKUP(match_scores, aligner->score_width, i, j));
    }
    putc('\n', stdout);
  }
  printf("gap_a_scores:\n");
  for(j = 0; j < aligner->score_height; j++)
  {
    printf("%3i:", (int)j);
    for(i = 0; i < aligner->score_width; i++)
    {
      printf("\t%3i", (int)ARR_LOOKUP(gap_a_scores, aligner->score_width, i, j));
    }
    putc('\n', stdout);
  }
  printf("gap_b_scores:\n");
  for(j = 0; j < aligner->score_height; j++)
  {
    printf("%3i:", (int)j);
    for(i = 0; i < aligner->score_width; i++)
    {
      printf("\t%3i", (int)ARR_LOOKUP(gap_b_scores, aligner->score_width, i, j));
    }
    putc('\n', stdout);
  }

  printf("match: %i mismatch: %i gapopen: %i gapexend: %i\n",
         aligner->scoring->match, aligner->scoring->mismatch,
         aligner->scoring->gap_open, aligner->scoring->gap_extend);
  printf("\n");
}

void alignment_colour_print_against(const char *alignment_a,
                                    const char *alignment_b,
                                    char case_sensitive)
{
  int i;
  char red = 0, green = 0;

  for(i = 0; alignment_a[i] != '\0'; i++)
  {
    if(alignment_b[i] == '-')
    {
      if(!red)
      {
        fputs(align_col_indel, stdout);
        red = 1;
      }
    }
    else if(red)
    {
      red = 0;
      fputs(align_col_stop, stdout);
    }

    if(((case_sensitive && alignment_a[i] != alignment_b[i]) ||
        (!case_sensitive && tolower(alignment_a[i]) != tolower(alignment_b[i]))) &&
       alignment_a[i] != '-' && alignment_b[i] != '-')
    {
      if(!green)
      {
        fputs(align_col_mismatch, stdout);
        green = 1;
      }
    }
    else if(green)
    {
      green = 0;
      fputs(align_col_stop, stdout);
    }

    putc(alignment_a[i], stdout);
  }

  if(green || red)
  {
    // Stop all colours
    fputs(align_col_stop, stdout);
  }
}

// Order of alignment_a / alignment_b is not important
void alignment_print_spacer(const char* alignment_a, const char* alignment_b,
                            const scoring_t* scoring)
{
  int i;

  for(i = 0; alignment_a[i] != '\0'; i++)
  {
    if(alignment_a[i] == '-' || alignment_b[i] == '-')
    {
      putc(' ', stdout);
    }
    else if(alignment_a[i] == alignment_b[i] ||
            (!scoring->case_sensitive &&
             tolower(alignment_a[i]) == tolower(alignment_b[i])))
    {
      putc('|', stdout);
    }
    else
    {
      putc('*', stdout);
    }
  }
}


#endif


typedef struct sw_aligner_t sw_aligner_t;

sw_aligner_t *smith_waterman_new();
void smith_waterman_free(sw_aligner_t *sw_aligner);

aligner_t* smith_waterman_get_aligner(sw_aligner_t *sw);

/*
 Do not alter seq_a, seq_b or scoring whilst calling this method
 or between calls to smith_waterman_get_hit
*/
void smith_waterman_align(const char *seq_a, const char *seq_b,
                          const scoring_t *scoring, sw_aligner_t *sw);

void smith_waterman_align2(const char *seq_a, const char *seq_b,
                           size_t len_a, size_t len_b,
                           const scoring_t *scoring, sw_aligner_t *sw);

// An alignment to read from, and a pointer to memory to store the result
// returns 1 if an alignment was read, 0 otherwise
int smith_waterman_fetch(sw_aligner_t *sw, alignment_t *result);


/*
 smith_waterman.c
 url: https://github.com/noporpoise/seq-align
 maintainer: Isaac Turner <turner.isaac@gmail.com>
 license: Public Domain, no warranty
 date: Nov 2013
 */

// Turn on debugging output by defining SEQ_ALIGN_VERBOSE
//#define SEQ_ALIGN_VERBOSE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// #include "sort_r/sort_r.h"
/* Isaac Turner 29 April 2014 Public Domain */
#ifndef SORT_R_H_
#define SORT_R_H_

#include <stdlib.h>
#include <string.h>

/*

sort_r function to be exported.

Parameters:
  base is the array to be sorted
  nel is the number of elements in the array
  width is the size in bytes of each element of the array
  compar is the comparison function
  arg is a pointer to be passed to the comparison function

void sort_r(void *base, size_t nel, size_t width,
            int (*compar)(const void *_a, const void *_b, void *_arg),
            void *arg);

*/

#define _SORT_R_INLINE inline

#if (defined __APPLE__ || defined __MACH__ || defined __DARWIN__ || \
     defined __FreeBSD__ || defined __DragonFly__)
#  define _SORT_R_BSD
#elif (defined _GNU_SOURCE || defined __gnu_hurd__ || defined __GNU__ || \
       defined __linux__ || defined __MINGW32__ || defined __GLIBC__)
#  define _SORT_R_LINUX
#elif (defined _WIN32 || defined _WIN64 || defined __WINDOWS__)
#  define _SORT_R_WINDOWS
#  undef _SORT_R_INLINE
#  define _SORT_R_INLINE __inline
#else
  /* Using our own recursive quicksort sort_r_simple() */
#endif

#if (defined NESTED_QSORT && NESTED_QSORT == 0)
#  undef NESTED_QSORT
#endif

/* swap a, b iff a>b */
/* __restrict is same as restrict but better support on old machines */
static _SORT_R_INLINE int sort_r_cmpswap(char *__restrict a, char *__restrict b, size_t w,
                                         int (*compar)(const void *_a, const void *_b,
                                                       void *_arg),
                                         void *arg)
{
  char tmp, *end = a+w;
  if(compar(a, b, arg) > 0) {
    for(; a < end; a++, b++) { tmp = *a; *a = *b; *b = tmp; }
    return 1;
  }
  return 0;
}

/* Implement recursive quicksort ourselves */
/* Note: quicksort is not stable, equivalent values may be swapped */
static _SORT_R_INLINE void sort_r_simple(void *base, size_t nel, size_t w,
                                         int (*compar)(const void *_a, const void *_b,
                                                       void *_arg),
                                         void *arg)
{
  char *b = (char *)base, *end = b + nel*w;
  if(nel < 7) {
    /* Insertion sort for arbitrarily small inputs */
    char *pi, *pj;
    for(pi = b+w; pi < end; pi += w) {
      for(pj = pi; pj > b && sort_r_cmpswap(pj-w,pj,w,compar,arg); pj -= w) {}
    }
  }
  else
  {
    /* nel > 6; Quicksort */

    /* Use median of first, middle and last items as pivot */
    char *x, *y, *xend, ch;
    char *pl, *pr;
    char *last = b+w*(nel-1), *tmp;
    char *l[3];
    l[0] = b;
    l[1] = b+w*(nel/2);
    l[2] = last;

    if(compar(l[0],l[1],arg) > 0) { tmp=l[0]; l[0]=l[1]; l[1]=tmp; }
    if(compar(l[1],l[2],arg) > 0) {
      tmp=l[1]; l[1]=l[2]; l[2]=tmp; /* swap(l[1],l[2]) */
      if(compar(l[0],l[1],arg) > 0) { tmp=l[0]; l[0]=l[1]; l[1]=tmp; }
    }

    /* swap l[id], l[2] to put pivot as last element */
    for(x = l[1], y = last, xend = x+w; x<xend; x++, y++) {
      ch = *x; *x = *y; *y = ch;
    }

    pl = b;
    pr = last;

    while(pl < pr) {
      for(; pl < pr; pl += w) {
        if(sort_r_cmpswap(pl, pr, w, compar, arg)) {
          pr -= w; /* pivot now at pl */
          break;
        }
      }
      for(; pl < pr; pr -= w) {
        if(sort_r_cmpswap(pl, pr, w, compar, arg)) {
          pl += w; /* pivot now at pr */
          break;
        }
      }
    }

    sort_r_simple(b, (pl-b)/w, w, compar, arg);
    sort_r_simple(pl+w, (end-(pl+w))/w, w, compar, arg);
  }
}


#if defined NESTED_QSORT

  static _SORT_R_INLINE void sort_r(void *base, size_t nel, size_t width,
                                    int (*compar)(const void *_a, const void *_b,
                                                  void *aarg),
                                    void *arg)
  {
    int nested_cmp(const void *a, const void *b)
    {
      return compar(a, b, arg);
    }

    qsort(base, nel, width, nested_cmp);
  }

#else /* !NESTED_QSORT */

  /* Declare structs and functions */

  #if defined _SORT_R_BSD

    /* Ensure qsort_r is defined */
    extern void qsort_r(void *base, size_t nel, size_t width, void *thunk,
                        int (*compar)(void *_thunk, const void *_a, const void *_b));

  #endif

  #if defined _SORT_R_BSD || defined _SORT_R_WINDOWS

    /* BSD (qsort_r), Windows (qsort_s) require argument swap */

    struct sort_r_data
    {
      void *arg;
      int (*compar)(const void *_a, const void *_b, void *_arg);
    };

    static _SORT_R_INLINE int sort_r_arg_swap(void *s, const void *a, const void *b)
    {
      struct sort_r_data *ss = (struct sort_r_data*)s;
      return (ss->compar)(a, b, ss->arg);
    }

  #endif

  #if defined _SORT_R_LINUX

    typedef int(* __compar_d_fn_t)(const void *, const void *, void *);
    extern void qsort_r(void *base, size_t nel, size_t width,
                        __compar_d_fn_t __compar, void *arg)
      __attribute__((nonnull (1, 4)));

  #endif

  /* implementation */

  static _SORT_R_INLINE void sort_r(void *base, size_t nel, size_t width,
                                    int (*compar)(const void *_a, const void *_b, void *_arg),
                                    void *arg)
  {
    #if defined _SORT_R_LINUX

      #if defined __GLIBC__ && ((__GLIBC__ < 2) || (__GLIBC__ == 2 && __GLIBC_MINOR__ < 8))

        /* no qsort_r in glibc before 2.8, need to use nested qsort */
        sort_r_simple(base, nel, width, compar, arg);

      #else

        qsort_r(base, nel, width, compar, arg);

      #endif

    #elif defined _SORT_R_BSD

      struct sort_r_data tmp;
      tmp.arg = arg;
      tmp.compar = compar;
      qsort_r(base, nel, width, &tmp, sort_r_arg_swap);

    #elif defined _SORT_R_WINDOWS

      struct sort_r_data tmp;
      tmp.arg = arg;
      tmp.compar = compar;
      qsort_s(base, nel, width, sort_r_arg_swap, &tmp);

    #else

      /* Fall back to our own quicksort implementation */
      sort_r_simple(base, nel, width, compar, arg);

    #endif
  }

#endif /* !NESTED_QSORT */

#undef _SORT_R_INLINE
#undef _SORT_R_WINDOWS
#undef _SORT_R_LINUX
#undef _SORT_R_BSD

#endif /* SORT_R_H_ */


typedef struct {
  uint32_t *b; size_t l, s; // l is bits, s in uint32_t
} BitSet;

static inline BitSet* bitset_alloc(BitSet *bs, size_t l) {
  bs->s = (l+31)/32;
  bs->l = l;
  bs->b = calloc(sizeof(bs->b[0]), bs->s);
  return bs->b ? bs : NULL;
}

static inline void bitset_dealloc(BitSet *bs) {
  free(bs->b);
  memset(bs, 0, sizeof(*bs));
}

static inline BitSet* bitset_set_length(BitSet *bs, size_t l) {
  size_t ss = (l+31)/32;
  if(ss > bs->s) {
    bs->b = realloc(bs->b, ss*sizeof(bs->b[0]));
    memset(bs->b+bs->s, 0, (ss-bs->s)*sizeof(bs->b[0])); // zero new memory
    bs->s = ss;
  }
  bs->l = l;
  return bs->b ? bs : NULL;
}

// For iterating through local alignments
typedef struct
{
  BitSet match_scores_mask;
  size_t *sorted_match_indices, hits_capacity, num_of_hits, next_hit;
} sw_history_t;

// Store alignment here
struct sw_aligner_t
{
  aligner_t aligner;
  sw_history_t history;
};

// Sort indices by their matrix values
// Struct used to pass data to sort_match_indices
typedef struct
{
  score_t *match_scores;
  unsigned int score_width;
} MatrixSort;

// Function passed to sort_r
int sort_match_indices(const void *aa, const void *bb, void *arg)
{
  const size_t *a = aa;
  const size_t *b = bb;
  const MatrixSort *tmp = arg;

  // Recover variables from the struct
  const score_t *match_scores = tmp->match_scores;
  size_t score_width = tmp->score_width;

  long diff = (long)match_scores[*b] - match_scores[*a];

  // Sort by position (from left to right) on seq_a
  if(diff == 0) return (*a % score_width) - (*b % score_width);
  else return diff > 0 ? 1 : -1;
}

static void _init_history(sw_history_t *hist)
{
  bitset_alloc(&hist->match_scores_mask, 256);
  hist->hits_capacity = 256;
  size_t mem = hist->hits_capacity * sizeof(*(hist->sorted_match_indices));
  hist->sorted_match_indices = malloc(mem);
}

static void _ensure_history_capacity(sw_history_t *hist, size_t arr_size)
{
  if(arr_size > hist->hits_capacity) {
    hist->hits_capacity = ROUNDUP2POW(arr_size);
    bitset_set_length(&hist->match_scores_mask, hist->hits_capacity);

    size_t mem = hist->hits_capacity * sizeof(*(hist->sorted_match_indices));
    hist->sorted_match_indices = realloc(hist->sorted_match_indices, mem);
    if(!hist->match_scores_mask.b || !hist->sorted_match_indices) {
      fprintf(stderr, "%s:%i: Out of memory\n", __FILE__, __LINE__);
      exit(EXIT_FAILURE);
    }
  }
}

sw_aligner_t* smith_waterman_new()
{
  sw_aligner_t *sw = calloc(1, sizeof(sw_aligner_t));
  _init_history(&(sw->history));
  return sw;
}

void smith_waterman_free(sw_aligner_t *sw)
{
  aligner_destroy(&(sw->aligner));
  bitset_dealloc(&sw->history.match_scores_mask);
  free(sw->history.sorted_match_indices);
  free(sw);
}

aligner_t* smith_waterman_get_aligner(sw_aligner_t *sw)
{
  return &sw->aligner;
}

void smith_waterman_align(const char *a, const char *b,
                          const scoring_t *scoring, sw_aligner_t *sw)
{
  smith_waterman_align2(a, b, strlen(a), strlen(b), scoring, sw);
}

void smith_waterman_align2(const char *a, const char *b,
                           size_t len_a, size_t len_b,
                           const scoring_t *scoring, sw_aligner_t *sw)
{
  aligner_t *aligner = &sw->aligner;
  sw_history_t *hist = &sw->history;
  aligner_align(aligner, a, b, len_a, len_b, scoring, 1);

  size_t arr_size = aligner->score_width * aligner->score_height;
  _ensure_history_capacity(hist, arr_size);

  // Set number of hits
  memset(hist->match_scores_mask.b, 0, (hist->match_scores_mask.l+31)/32);
  hist->num_of_hits = hist->next_hit = 0;

  size_t pos;
  for(pos = 0; pos < arr_size; pos++) {
    if(aligner->match_scores[pos] > 0)
      hist->sorted_match_indices[hist->num_of_hits++] = pos;
  }

  // Now sort matched hits
  MatrixSort tmp_struct = {aligner->match_scores, aligner->score_width};
  sort_r(hist->sorted_match_indices, hist->num_of_hits,
         sizeof(size_t), sort_match_indices, &tmp_struct);
}

// Return 1 if alignment was found, 0 otherwise
static char _follow_hit(sw_aligner_t* sw, size_t arr_index,
                        alignment_t* result)
{
  const aligner_t *aligner = &(sw->aligner);
  const sw_history_t *hist = &(sw->history);

  // Follow path through matrix
  size_t score_x = (size_t)ARR_2D_X(arr_index, aligner->score_width);
  size_t score_y = (size_t)ARR_2D_Y(arr_index, aligner->score_width);

  // Local alignments always (start and) end with a match
  enum Matrix curr_matrix = MATCH;
  score_t curr_score = aligner->match_scores[arr_index];

  // Store end arr_index and (x,y) coords for later
  size_t end_arr_index = arr_index;
  size_t end_score_x = score_x;
  size_t end_score_y = score_y;
  score_t end_score = curr_score;

  size_t length;

  for(length = 0; ; length++)
  {
    if(bitset32_get(hist->match_scores_mask.b, arr_index)) return 0;
    bitset32_set(hist->match_scores_mask.b, arr_index);

    if(curr_score == 0) break;

    //printf(" %i (%i, %i)\n", length, score_x, score_y);

    // Find out where to go next
    alignment_reverse_move(&curr_matrix, &curr_score,
                           &score_x, &score_y, &arr_index, aligner);
  }

  // We got a result!
  // Allocate memory for the result
  result->length = length;

  alignment_ensure_capacity(result, length);

  // Jump back to the end of the alignment
  arr_index = end_arr_index;
  score_x = end_score_x;
  score_y = end_score_y;
  curr_matrix = MATCH;
  curr_score = end_score;

  // Now follow backwards again to create alignment!
  unsigned int i;

  for(i = length-1; curr_score > 0; i--)
  {
    switch(curr_matrix)
    {
      case MATCH:
        result->result_a[i] = aligner->seq_a[score_x-1];
        result->result_b[i] = aligner->seq_b[score_y-1];
        break;

      case GAP_A:
        result->result_a[i] = '-';
        result->result_b[i] = aligner->seq_b[score_y-1];
        break;

      case GAP_B:
        result->result_a[i] = aligner->seq_a[score_x-1];
        result->result_b[i] = '-';
        break;

      default:
      fprintf(stderr, "Program error: invalid matrix in _follow_hit()\n");
      fprintf(stderr, "Please submit a bug report to: turner.isaac@gmail.com\n");
      exit(EXIT_FAILURE);
    }

    alignment_reverse_move(&curr_matrix, &curr_score,
                           &score_x, &score_y, &arr_index, aligner);
  }

  result->result_a[length] = '\0';
  result->result_b[length] = '\0';

  result->score = end_score;

  result->pos_a = score_x;
  result->pos_b = score_y;

  result->len_a = end_score_x - score_x;
  result->len_b = end_score_y - score_y;

  return 1;
}

int smith_waterman_fetch(sw_aligner_t *sw, alignment_t *result)
{
  sw_history_t *hist = &(sw->history);

  while(hist->next_hit < hist->num_of_hits)
  {
    size_t arr_index = hist->sorted_match_indices[hist->next_hit++];
    // printf("hit %lu/%lu\n", hist->next_hit, hist->num_of_hits);

    if(!bitset32_get(hist->match_scores_mask.b, arr_index) &&
       _follow_hit(sw, arr_index, result))
    {
      return 1;
    }
  }

  return 0;
}


#endif


cmdline_t *cmd;
scoring_t scoring;

// Alignment results stored here
sw_aligner_t *sw;
alignment_t *result;

size_t alignment_index = 0;
bool wait_on_keystroke = 0;

static void sw_set_default_scoring()
{
  scoring_system_default(&scoring);

  // Change slightly
  scoring.match = 2;
  scoring.mismatch = -2;
  scoring.gap_open = -2;
  scoring.gap_extend = -1;
}

// Print one line of an alignment
void print_alignment_part(const char* seq1, const char* seq2,
                          size_t pos, size_t len,
                          const char* context_str,
                          size_t spaces_left, size_t spaces_right,
                          size_t context_left, size_t context_right)
{
  size_t i;
  printf("  ");

  for(i = 0; i < spaces_left; i++) printf(" ");

  if(context_left > 0)
  {
    if(cmd->print_colour) fputs(align_col_context, stdout);
    printf("%.*s", (int)context_left, context_str+pos-context_left);
    if(cmd->print_colour) fputs(align_col_stop, stdout);
  }

  if(cmd->print_colour)
    alignment_colour_print_against(seq1, seq2, scoring.case_sensitive);
  else
    fputs(seq1, stdout);

  if(context_right > 0)
  {
    if(cmd->print_colour) fputs(align_col_context, stdout);
    printf("%.*s", (int)context_right, context_str+pos+len);
    if(cmd->print_colour) fputs(align_col_stop, stdout);
  }

  for(i = 0; i < spaces_right; i++) putc(' ', stdout);

  printf("  [pos: %li; len: %lu]\n", pos, len);
}

static char get_next_hit()
{
  if(!wait_on_keystroke)
    return 1;

  int r = 0;

  char response = 0;
  char next_hit = 0;

  while(!response)
  {
    printf("next [h]it or [a]lignment: ");
    fflush(stdout);

    while((r = getc(stdin)) != -1 && r != '\n' && r != '\r')
    {
      if(r == 'h' || r == 'H')
      {
        next_hit = 1;
        response = 1;
      }
      else if(r == 'a' || r == 'A')
      {
        next_hit = 0;
        response = 1;
      }
    }

    if(r == -1)
    {
      // We're done here
      putc('\n', stdout);
      exit(EXIT_SUCCESS);
    }
  }

  return next_hit;
}

// Align two sequences against each other to find local alignments between them
void align(const char *seq_a, const char *seq_b,
           const char *seq_a_name, const char *seq_b_name)
{
  if((seq_a_name != NULL || seq_b_name != NULL) && wait_on_keystroke)
  {
    fprintf(stderr, "Error: Interactive input takes seq only "
                    "(no FASTA/FASTQ) '%s:%s'\n", seq_a_name, seq_b_name);
    fflush(stderr);
    exit(EXIT_FAILURE);
  }

  // Check both arguments have length > 0
  if(seq_a[0] == '\0' || seq_b[0] == '\0')
  {
    fprintf(stderr, "Error: Sequences must have length > 0\n");
    fflush(stderr);

    if(cmd->print_fasta && seq_a_name != NULL && seq_b_name != NULL)
    {
      fprintf(stderr, "%s\n%s\n", seq_a_name, seq_b_name);
    }

    fflush(stderr);

    return;
  }

  smith_waterman_align(seq_a, seq_b, &scoring, sw);

  aligner_t *aligner = smith_waterman_get_aligner(sw);
  size_t len_a = aligner->score_width-1, len_b = aligner->score_height-1;

  printf("== Alignment %zu lengths (%lu, %lu):\n", alignment_index, len_a, len_b);

  if(cmd->print_matrices)
  {
    alignment_print_matrices(aligner);
  }

  // seqA
  if(cmd->print_fasta && seq_a_name != NULL)
  {
    fputs(seq_a_name, stdout);
    putc('\n', stdout);
  }

  if(cmd->print_seq)
  {
    fputs(seq_a, stdout);
    putc('\n', stdout);
  }

  // seqB
  if(cmd->print_fasta && seq_b_name != NULL)
  {
    fputs(seq_b_name, stdout);
    putc('\n', stdout);
  }

  if(cmd->print_seq)
  {
    fputs(seq_b, stdout);
    putc('\n', stdout);
  }

  putc('\n', stdout);

  if(!cmd->min_score_set)
  {
    // If min_score hasn't been set, set a limit based on the lengths of seqs
    // or zero if we're running interactively
    cmd->min_score = wait_on_keystroke ? 0
                       : scoring.match * MAX2(0.2 * MIN2(len_a, len_b), 2);

    #ifdef SEQ_ALIGN_VERBOSE
    printf("min_score: %i\n", cmd->min_score);
    #endif
  }

  fflush(stdout);

  size_t hit_index = 0;

  // For print context
  size_t context_left = 0, context_right = 0;
  size_t left_spaces_a = 0, left_spaces_b = 0;
  size_t right_spaces_a = 0, right_spaces_b = 0;


  while(get_next_hit() &&
        smith_waterman_fetch(sw, result) && result->score >= cmd->min_score &&
        (!cmd->max_hits_per_alignment_set ||
         hit_index < cmd->max_hits_per_alignment))
  {
    printf("hit %zu.%zu score: %i\n", alignment_index, hit_index++, result->score);

    if(cmd->print_context)
    {
      // Calculate number of characters of context to print either side
      context_left = MAX2(result->pos_a, result->pos_b);
      context_left = MIN2(context_left, cmd->print_context);

      size_t rem_a = len_a - (result->pos_a + result->len_a);
      size_t rem_b = len_b - (result->pos_b + result->len_b);

      context_right = MAX2(rem_a, rem_b);
      context_right = MIN2(context_right, cmd->print_context);

      left_spaces_a = (context_left > result->pos_a)
                      ? context_left - result->pos_a : 0;

      left_spaces_b = (context_left > result->pos_b)
                      ? context_left - result->pos_b : 0;

      right_spaces_a = (context_right > rem_a) ? context_right - rem_a : 0;
      right_spaces_b = (context_right > rem_b) ? context_right - rem_b : 0;
    }

    #ifdef SEQ_ALIGN_VERBOSE
    printf("context left = %lu; right = %lu spacing: [%lu,%lu] [%lu,%lu]\n",
           context_left, context_right,
           left_spaces_a, right_spaces_a,
           left_spaces_b, right_spaces_b);
    #endif

    // seq a
    print_alignment_part(result->result_a, result->result_b,
                         result->pos_a, result->len_a,
                         seq_a,
                         left_spaces_a, right_spaces_a,
                         context_left-left_spaces_a,
                         context_right-right_spaces_a);

    if(cmd->print_pretty)
    {
      fputs("  ", stdout);

      size_t max_left_spaces = MAX2(left_spaces_a, left_spaces_b);
      size_t max_right_spaces = MAX2(right_spaces_a, right_spaces_b);
      size_t spacer;

      // Print spaces for lefthand spacing
      for(spacer = 0; spacer < max_left_spaces; spacer++)
      {
        putc(' ', stdout);
      }

      // Print dots for lefthand context sequence
      for(spacer = 0; spacer < context_left-max_left_spaces; spacer++)
      {
        putc('.', stdout);
      }

      alignment_print_spacer(result->result_a, result->result_b, &scoring);

      // Print dots for righthand context sequence
      for(spacer = 0; spacer < context_right-max_right_spaces; spacer++)
      {
        putc('.', stdout);
      }

      // Print spaces for righthand spacing
      for(spacer = 0; spacer < max_right_spaces; spacer++)
      {
        putc(' ', stdout);
      }

      putc('\n', stdout);
    }

    // seq b
    print_alignment_part(result->result_b, result->result_a,
                         result->pos_b, result->len_b,
                         seq_b,
                         left_spaces_b, right_spaces_b,
                         context_left-left_spaces_b,
                         context_right-right_spaces_b);

    printf("\n");

    // Flush output here
    fflush(stdout);
  }

  fputs("==\n", stdout);
  fflush(stdout);

  // Increment sequence alignment counter
  alignment_index++;
}

void align_pair_from_file(read_t *read1, read_t *read2)
{
  align(read1->seq.b, read2->seq.b,
       (read1->name.end == 0 ? NULL : read1->name.b),
       (read2->name.end == 0 ? NULL : read2->name.b));
}

int main(int argc, char* argv[])
{
  #ifdef SEQ_ALIGN_VERBOSE
  printf("VERBOSE: on\n");
  #endif

  sw_set_default_scoring();
  cmd = cmdline_new(argc, argv, &scoring, SEQ_ALIGN_SW_CMD);

  // Align!
  sw = smith_waterman_new();
  result = alignment_create(256);

  if(cmd->seq1 != NULL)
  {
    // Align seq1 and seq2
    align(cmd->seq1, cmd->seq2, NULL, NULL);
  }

  // Align from files
  size_t i, num_of_file_pairs = cmdline_get_num_of_file_pairs(cmd);
  for(i = 0; i < num_of_file_pairs; i++)
  {
    const char *file1 = cmdline_get_file1(cmd, i);
    const char *file2 = cmdline_get_file2(cmd, i);
    if(file1 != NULL && *file1 == '\0' && file2 == NULL) {
      wait_on_keystroke = 1;
      file1 = "-";
    }
    align_from_file(file1, file2, &align_pair_from_file, !cmd->interactive);
  }

  // Free memory for storing alignment results
  smith_waterman_free(sw);
  alignment_free(result);

  cmdline_free(cmd);

  return EXIT_SUCCESS;
}
