#include "../korsvg.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
	static const char source[] =
		"<svg width=\"2\" height=\"2\"><rect width=\"2\" height=\"2\" "
		"fill=\"#ff0000\"/></svg>";
	static const char original[] = "keep this file";
	char path[4096];
	char bytes[sizeof(original)] = { 0 };
	FILE *file;
	KorSVGDataRef data = NULL;
	KorSVGDataRef serialized = NULL;
	KorSVGDocumentRef document = NULL;
	KorSVGContextRef context = NULL;
	KorSVGURLRef url = NULL;
	int result = 1;

	if (argc != 2 || snprintf(path, sizeof(path), "%s/wasi-write.svg",
				 argv[1]) >= (int)sizeof(path))
		return 1;
	file = fopen(path, "wb");
	if (!file)
		return 1;
	if (fwrite(original, 1, sizeof(original), file) != sizeof(original)) {
		fclose(file);
		return 1;
	}
	if (fclose(file) != 0)
		return 1;
	data = KorSVGDataCreate(source, sizeof(source) - 1);
	document = KorSVGDocumentCreateFromData(data, NULL);
	serialized = KorSVGDataCreateMutable();
	context = KorSVGContextCreate(2, 2);
	url = KorSVGURLCreate(path);
	if (!data || !document || !serialized || !context || !url)
		goto done;
	if (KorSVGDocumentWriteToURL(document, url, NULL) != 0 ||
	    !strstr(KorSVGGetLastError(), "WASI"))
		goto done;
	file = fopen(path, "rb");
	if (!file)
		goto done;
	if (fread(bytes, 1, sizeof(bytes), file) != sizeof(bytes) ||
	    memcmp(bytes, original, sizeof(original)) != 0) {
		fclose(file);
		goto done;
	}
	if (fclose(file) != 0)
		goto done;
	if (!KorSVGContextDrawDocument(context, document) ||
	    KorSVGContextGetData(context)[0] != 255 ||
	    KorSVGContextGetData(context)[3] != 255 ||
	    !KorSVGDocumentWriteToData(document, serialized, NULL) ||
	    KorSVGDataGetLength(serialized) != sizeof(source) - 1 ||
	    memcmp(KorSVGDataGetBytes(serialized), source,
		   sizeof(source) - 1) != 0)
		goto done;
	result = 0;
done:
	if (result)
		fprintf(stderr, "WASI contract failed: %s\n", KorSVGGetLastError());
	KorSVGURLRelease(url);
	KorSVGContextRelease(context);
	KorSVGDataRelease(serialized);
	KorSVGDocumentRelease(document);
	KorSVGDataRelease(data);
	return result;
}
