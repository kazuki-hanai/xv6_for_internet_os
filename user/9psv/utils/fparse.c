#include "user.h"
#include "../p9.h"

uint8_t*  parse_tversion(struct p9_fcall*, uint8_t*, int);
uint8_t*  parse_tattach(struct p9_fcall*, uint8_t*, int);
uint8_t*  parse_tflush(struct p9_fcall*, uint8_t*, int);
uint8_t*  parse_twalk(struct p9_fcall*, uint8_t*, int);
uint8_t*  parse_topen(struct p9_fcall*, uint8_t*, int);
uint8_t*  parse_tcreate(struct p9_fcall*, uint8_t*, int);
uint8_t*  parse_tread(struct p9_fcall*, uint8_t*, int);
uint8_t*  parse_twrite(struct p9_fcall*, uint8_t*, int);
uint8_t*  parse_tclunk(struct p9_fcall*, uint8_t*, int);
uint8_t*  parse_tremove(struct p9_fcall*, uint8_t*, int);
uint8_t*  parse_tstat(struct p9_fcall*, uint8_t*, int);
uint8_t*  parse_twstat(struct p9_fcall*, uint8_t*, int);

static uint8_t* parse_ignore(struct p9_fcall* f, uint8_t* buf, int len) {
	return 0;
}

static uint8_t* (*parsefunc[]) (struct p9_fcall* f, uint8_t* buf, int len) = {
	[P9_TVERSION]     parse_tversion,
	[P9_TAUTH]        parse_ignore,
	[P9_TATTACH]      parse_tattach,
	[P9_TERROR]       parse_ignore,
	[P9_TFLUSH]       parse_tflush,
	[P9_TWALK]        parse_twalk,
	[P9_TOPEN]        parse_topen,
	[P9_TCREATE]      parse_tcreate,
	[P9_TREAD]        parse_tread,
	[P9_TWRITE]       parse_twrite,
	[P9_TCLUNK]       parse_tclunk,
	[P9_TREMOVE]      parse_tremove,
	[P9_TSTAT]        parse_tstat,
	[P9_TWSTAT]       parse_twstat,
};

static uint8_t* parse_hdr(struct p9_fcall *fcall, uint8_t* buf) {
	fcall->size = GBIT32(buf);
	buf += 4;
	fcall->type = GBIT8(buf);
	buf += 1;
	fcall->tag = GBIT16(buf);
	buf += 2;
	return buf;
}

struct p9_req* parsefcall(uint8_t* buf, int size) {
	if (buf == 0) {
		return 0;
	}
	if (size < P9_HDR_SIZE) {
		return 0;
	}

	struct p9_req *req = p9_allocreq();
	if (req == 0) {
		return 0;
	}
	memset(req, 0, sizeof *req);
	struct p9_fcall *ifcall = &req->ifcall;

	buf = parse_hdr(ifcall, buf);
	int mlen = ifcall->size - P9_HDR_SIZE;

	buf = parsefunc[ifcall->type](ifcall, buf, mlen);
	if (buf == 0) {
		goto fail;
	}
	return req;

fail:
	if (req)
		p9_freereq(req);
	return 0;
}
