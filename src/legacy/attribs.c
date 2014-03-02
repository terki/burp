/*
 *  Encode and decode standard Unix attributes and
 *   Extended attributes for Win32 and
 *   other non-Unix systems, or Unix systems with ACLs, ...
 */

#include "include.h"

/*
 * Encode a stat structure into a base64 character string
 *   All systems must create such a structure.
 */
int encode_stat(struct sbufl *sb)
{
   char *p;
   struct stat *statp=&sb->statp;

   if(!sb->attr.buf)
   {
	sb->attr.cmd=CMD_ATTRIBS; // should not be needed
	if(!(sb->attr.buf=(char *)malloc(128)))
	{
		log_out_of_memory(__FUNCTION__);
		return -1;
	}
   }
   p=sb->attr.buf;

   p += to_base64(statp->st_dev, p);
   *p++ = ' ';                        /* separate fields with a space */
   p += to_base64(statp->st_ino, p);
   *p++ = ' ';
   p += to_base64(statp->st_mode, p);
   *p++ = ' ';
   p += to_base64(statp->st_nlink, p);
   *p++ = ' ';
   p += to_base64(statp->st_uid, p);
   *p++ = ' ';
   p += to_base64(statp->st_gid, p);
   *p++ = ' ';
   p += to_base64(statp->st_rdev, p);
   *p++ = ' ';
   p += to_base64(statp->st_size, p);
   *p++ = ' ';
#ifdef HAVE_WIN32
   p += to_base64(0, p); /* output place holder */
   *p++ = ' ';
   p += to_base64(0, p); /* output place holder */
#else
   p += to_base64(statp->st_blksize, p);
   *p++ = ' ';
   p += to_base64(statp->st_blocks, p);
#endif
   *p++ = ' ';
   p += to_base64(statp->st_atime, p);
   *p++ = ' ';
   p += to_base64(statp->st_mtime, p);
   *p++ = ' ';
   p += to_base64(statp->st_ctime, p);
   *p++ = ' ';

#ifdef HAVE_CHFLAGS
   /* FreeBSD function */
   p += to_base64(statp->st_flags, p);  /* output st_flags */
#else
   p += to_base64(0, p);     /* output place holder */
#endif
   *p++ = ' ';

#ifdef HAVE_WIN32
   p += to_base64(sb->winattr, p);
#else
   p += to_base64(0, p);     /* output place holder */
#endif
   *p++ = ' ';

   p += to_base64(sb->compression, p);

   *p = 0;

   sb->attr.len=p-sb->attr.buf;

   return 0;
}


/* Do casting according to unknown type to keep compiler happy */
#ifdef HAVE_TYPEOF
  #define plug(st, val) st = (typeof st)val
#else
  #if !HAVE_GCC & HAVE_SUN_OS
    /* Sun compiler does not handle templates correctly */
    #define plug(st, val) st = val
  #elif __sgi
    #define plug(st, val) st = val
  #else
    /* Use templates to do the casting */
    template <class T> void plug(T &st, uint64_t val)
      { st = static_cast<T>(val); }
  #endif
#endif


/* Decode a stat packet from base64 characters */
void decode_stat(struct sbufl *sb)
{
   const char *p=sb->attr.buf;
   struct stat *statp=&sb->statp;
   int64_t val;

   p += from_base64(&val, p);
   plug(statp->st_dev, val);
   p++;
   p += from_base64(&val, p);
   plug(statp->st_ino, val);
   p++;
   p += from_base64(&val, p);
   plug(statp->st_mode, val);
   p++;
   p += from_base64(&val, p);
   plug(statp->st_nlink, val);
   p++;
   p += from_base64(&val, p);
   plug(statp->st_uid, val);
   p++;
   p += from_base64(&val, p);
   plug(statp->st_gid, val);
   p++;
   p += from_base64(&val, p);
   plug(statp->st_rdev, val);
   p++;
   p += from_base64(&val, p);
   plug(statp->st_size, val);
   p++;
   p += from_base64(&val, p);
#ifdef HAVE_WIN32
//   plug(statp->st_blksize, val);
   p++;
   p += from_base64(&val, p);
//   plug(statp->st_blocks, val);
#else
   plug(statp->st_blksize, val);
   p++;
   p += from_base64(&val, p);
   plug(statp->st_blocks, val);
#endif
   p++;
   p += from_base64(&val, p);
   plug(statp->st_atime, val);
   p++;
   p += from_base64(&val, p);
   plug(statp->st_mtime, val);
   p++;
   p += from_base64(&val, p);
   plug(statp->st_ctime, val);

   /* FreeBSD user flags */
   if (*p == ' ' || (*p != 0 && *(p+1) == ' ')) {
      p++;
      if(!*p) return;
      p += from_base64(&val, p);
#ifdef HAVE_CHFLAGS
      plug(statp->st_flags, val);
   } else {
      statp->st_flags  = 0;
#endif
   }

   /* Look for winattr */
   if (*p == ' ' || (*p != 0 && *(p+1) == ' ')) {
      p++;
      p += from_base64(&val, p);
   } else {
      val = 0;
   }
   sb->winattr=val;

   /* Compression */
   if (*p == ' ' || (*p != 0 && *(p+1) == ' ')) {
      p++;
      if(!*p) return;
      p += from_base64(&val, p);
      sb->compression=val;
   } else {
      sb->compression=-1;
   }
}