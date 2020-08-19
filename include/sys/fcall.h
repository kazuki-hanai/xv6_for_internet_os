
#define VERSION9P "9P2000"
#define MAXWELEM  16

struct Fcall
{
	uint8	type;
	uint32	fid;
	uint16	tag;
	union {
		struct {
			uint16	msize;		/* Tversion, Rversion */
			uint8	*version;	/* Tversion, Rversion */
		};
		struct {
			uint16	oldtag;		/* Tflush */
		};
		struct {
			uint8	*ename;		/* Rerror */
		};
		struct {
			Qid	qid;		/* Rattach, Ropen, Rcreate */
			uint32	iounit;		/* Ropen, Rcreate */
		};
		struct {
			Qid	aqid;		/* Rauth */
		};
		struct {
			uint32	afid;		/* Tauth, Tattach */
			uint8	*uname;		/* Tauth, Tattach */
			uint8	*aname;		/* Tauth, Tattach */
		};
		struct {
			uint32	perm;		/* Tcreate */ 
			uint8	*name;		/* Tcreate */
			uuint8	mode;		/* Tcreate, Topen */
		};
		struct {
			uint32	newfid;		/* Twalk */
			uint16	nwname;		/* Twalk */
			uint8	*wname[MAXWELEM];	/* Twalk */
		};
		struct {
			uint16	nwqid;		/* Rwalk */
			Qid	wqid[MAXWELEM];		/* Rwalk */
		};
		struct {
			uint64	offset;		/* Tread, Twrite */
			uint32	count;		/* Tread, Twrite, Rread */
			uint8	*data;		/* Twrite, Rread */
		};
		struct {
			uint16	nstat;		/* Twstat, Rstat */
			uuint8	*stat;		/* Twstat, Rstat */
		};
	};
};

#define	GBIT8(p)	((p)[0])
#define	GBIT16(p)	((p)[0]|((p)[1]<<8))
#define	GBIT32(p)	((p)[0]|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24))
#define	GBIT64(p)	((uint32_t)((p)[0]|((p)[1]<<8)|((p)[2]<<16)|((p)[3]<<24)) |\
				((uint64_t)((p)[4]|((p)[5]<<8)|((p)[6]<<16)|((p)[7]<<24)) << 32))

#define	PBIT8(p,v)	(p)[0]=(v)
#define	PBIT16(p,v)	(p)[0]=(v);(p)[1]=(v)>>8
#define	PBIT32(p,v)	(p)[0]=(v);(p)[1]=(v)>>8;(p)[2]=(v)>>16;(p)[3]=(v)>>24
#define	PBIT64(p,v)	(p)[0]=(v);(p)[1]=(v)>>8;(p)[2]=(v)>>16;(p)[3]=(v)>>24;\
			(p)[4]=(v)>>32;(p)[5]=(v)>>40;(p)[6]=(v)>>48;(p)[7]=(v)>>56

#define	BIT8SZ		1
#define	BIT16SZ		2
#define	BIT32SZ		4
#define	BIT64SZ		8
#define	QIDSZ	(BIT8SZ+BIT32SZ+BIT64SZ)

/* STATFIXLEN includes leading 16-bit count */
/* The count, however, excludes itself; total size is BIT16SZ+count */
#define STATFIXLEN	(BIT16SZ+QIDSZ+5*BIT16SZ+4*BIT32SZ+1*BIT64SZ)	/* amount of fixed length data in a stat buffer */

#define	NOTAG		(uint16_t)~0U	/* Dummy tag */
#define	NOFID		(uint32_t)~0U	/* Dummy fid */
#define	IOHDRSZ		24	/* ample room for Twrite/Rread header (iounit) */

enum
{
	Tversion =	100,
	Rversion,
	Tauth =		102,
	Rauth,
	Tattach =	104,
	Rattach,
	Terror =	106,	/* illegal */
	Rerror,
	Tflush =	108,
	Rflush,
	Twalk =		110,
	Rwalk,
	Topen =		112,
	Ropen,
	Tcreate =	114,
	Rcreate,
	Tread =		116,
	Rread,
	Twrite =	118,
	Rwrite,
	Tclunk =	120,
	Rclunk,
	Tremove =	122,
	Rremove,
	Tstat =		124,
	Rstat,
	Twstat =	126,
	Rwstat,
	Tmax,
};
