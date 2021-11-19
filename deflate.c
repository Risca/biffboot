
#include <stdio.h>
#include <zlib.h>


#include "munge.c"

int main(int argc, char *argv[])
{
	FILE *fp;
	static char inbuf[0x30000];
	static char outbuf[0x10000];
	size_t inlen;
	size_t outlen;
	int i;
	z_stream stream;

	if (argc != 3) {
		fprintf(stderr,
			"Compresses a file in the DEFLATE format.\n"
			"Usage: deflate input_file deflated_file\n"
		);
		return 1;
	}
	
	fp = fopen(argv[1], "rb");
	if (fp == NULL) {
		perror(argv[1]);
		return 1;
	}
	inlen = fread(inbuf, 1, sizeof(inbuf), fp);
	fclose(fp);
		
	stream.next_in = inbuf;
	stream.avail_in = inlen;
	stream.next_out = outbuf;
	stream.avail_out = sizeof(outbuf);
	stream.zalloc = (alloc_func) 0;
	stream.zfree = (free_func) 0;
	if (deflateInit2(&stream, Z_BEST_COMPRESSION, Z_DEFLATED,
		-MAX_WBITS, 9, Z_DEFAULT_STRATEGY) != Z_OK) {
		fprintf(stderr, "deflater: deflateInit2 failed\n");
		return 1;
	}
	if (deflate(&stream, Z_FINISH) != Z_STREAM_END) {
		fprintf(stderr, "deflater: deflate failed\n");
		return 1;
	}
	if (deflateEnd(&stream) != Z_OK) {
		fprintf(stderr, "deflater: deflateEnd failed\n");
		return 1;
	}

	fp = fopen(argv[2], "wb");
	if (fp == NULL) {
		perror(argv[2]);
		return 1;
	}

	for (i=0;i<stream.total_out;i++)
		outbuf[i] = MUNGE(i, outbuf[i]);	
	
	outlen = fwrite(outbuf, 1, stream.total_out, fp);
	fclose(fp);
	if (outlen != stream.total_out) {
		perror(argv[3]);
		return 1;
	}

	printf("Compressed %s (%d bytes) to %s (%d bytes)\n",
		argv[1], inlen, argv[2], outlen);
	return 0;
}

