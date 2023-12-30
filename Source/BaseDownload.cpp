#pragma warning(disable: 4996)

#include "BaseDownload.h"
#include "StringProc.h"
#include "PathProc.h"
#include <curl/curl.h>

#define WRITEBUFSIZE	((1u << 20) * 8) // 8MB

struct download_struct
{
	CURL* curl;
	AFC_DownloadCB callback;
	char* buffer;
	uintmax_t bufcache;
	FILE* filepointer;
};

static int progress(download_struct* dlstruct, curl_off_t dltotal, curl_off_t dlnow, curl_off_t, curl_off_t)
{
	if (!dlstruct->callback) return 0;
	curl_off_t dlspeed = 0;
	curl_easy_getinfo(dlstruct->curl, CURLINFO_SPEED_DOWNLOAD_T, &dlspeed);
	return dlstruct->callback(dltotal, dlnow, dlspeed) ? 0 : -1;
}

static void flushbuffer(download_struct* dlstruct)
{
	if (!dlstruct->bufcache) return;
	fwrite(dlstruct->buffer, 1, dlstruct->bufcache, dlstruct->filepointer);
	dlstruct->bufcache = 0;
}

static size_t writedata(char* buffer, size_t size, size_t nitems, download_struct* dlstruct)
{
	size_t bufsize = size * nitems;
	if (dlstruct->bufcache > (WRITEBUFSIZE - bufsize)) flushbuffer(dlstruct);
	memcpy(&(dlstruct->buffer[dlstruct->bufcache]), buffer, bufsize);
	dlstruct->bufcache += bufsize;
	return bufsize;
}

int AFC_DownloadA(const char* Url, const char* Path, AFC_DownloadCB Callback)
{
	download_struct dlstruct = { 0 };
	dlstruct.curl = curl_easy_init();
	dlstruct.callback = Callback;
	dlstruct.buffer = new char[WRITEBUFSIZE];
	dlstruct.filepointer = fopen(Path, "wb");
	if (!dlstruct.filepointer) return false;
	curl_easy_setopt(dlstruct.curl, CURLOPT_URL, Url);
	curl_easy_setopt(dlstruct.curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(dlstruct.curl, CURLOPT_XFERINFODATA, &dlstruct);
	curl_easy_setopt(dlstruct.curl, CURLOPT_XFERINFOFUNCTION, progress);
	curl_easy_setopt(dlstruct.curl, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(dlstruct.curl, CURLOPT_WRITEDATA, &dlstruct);
	curl_easy_setopt(dlstruct.curl, CURLOPT_WRITEFUNCTION, writedata);
	CURLcode code = curl_easy_perform(dlstruct.curl);
	curl_easy_cleanup(dlstruct.curl);
	flushbuffer(&dlstruct);
	fclose(dlstruct.filepointer);
	delete[] dlstruct.buffer;
	return code == CURLE_OK;
}

int AFC_DownloadW(const wchar_t* Url, const wchar_t* Path, AFC_DownloadCB Callback)
{
	return AFC_DownloadA(afc::convert_string(Url).c_str(), afc::convert_string(Path).c_str(), Callback);
}
