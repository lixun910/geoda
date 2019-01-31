//
// Created by Xun Li on 12/20/18.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <curl/curl.h>

#include "Downloader.h"

using namespace OSMTools;

double Downloader::m_progress_report_interval = 3;
double Downloader::m_progress = 0;

Downloader::Downloader()
{
}

Downloader::~Downloader()
{
}

int Downloader::xferinfo(void *p,
                         curl_off_t dltotal, curl_off_t dlnow,
                         curl_off_t ultotal, curl_off_t ulnow)
{
    struct myprogress *myp = (struct myprogress *)p;
    CURL *curl = myp->curl;
    double curtime = 0;

    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME, &curtime);

    if (dltotal > 0) m_progress = (double)dlnow / (double)dltotal;

    if((curtime - myp->lastruntime) >= m_progress_report_interval) {
        myp->lastruntime = curtime;
        fprintf(stderr, "TOTAL TIME: %f \r\n", curtime);
    }

    fprintf(stderr, "Progress: %f \r\n", m_progress);
    //fprintf(stderr, "UP: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
    //                "  DOWN: %" CURL_FORMAT_CURL_OFF_T " of %" CURL_FORMAT_CURL_OFF_T
    //                "\r\n",
    //        ulnow, ultotal, dlnow, dltotal);
    //if(dlnow > STOP_DOWNLOAD_AFTER_THIS_MANY_BYTES) return 1;
    return 0;
}

size_t Downloader::write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
    size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
    return written;
}

bool Downloader::Get(const char *url, const char* post, const char *file_name)
{
    if (url == 0 || file_name == 0) return false;
    bool flag = true;
    CURL *curl;
    CURLcode res = CURLE_OK;
    struct myprogress prog;

    curl = curl_easy_init();
    if(curl) {
        prog.lastruntime = 0;
        prog.curl = curl;

        curl_easy_setopt(curl, CURLOPT_URL, url);
        if (post != 0) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post);
        }
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        // xferinfo was introduced in 7.32.0
        curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, Downloader::xferinfo);
        /* pass the struct pointer into the xferinfo function, note that this is
        an alias to CURLOPT_PROGRESSDATA */
        curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &prog);
        /* enable progress meter, set to 1L to disable and enable debug output */
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
        /* send all data to this function  */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Downloader::write_data);
        /* open the file */
        FILE *pagefile;
        pagefile = fopen(file_name, "wb");
        if(pagefile) {
            /* write the page body to this file handle */
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);
            /* get it! */
            res = curl_easy_perform(curl);
            if(res != CURLE_OK) {
                flag = false;
                fprintf(stderr, "%s\n", curl_easy_strerror(res));
            }
            /* close the header file */
            fclose(pagefile);
        }
        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    return flag;
}



