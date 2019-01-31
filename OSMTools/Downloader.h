//
// Created by Xun Li on 12/20/18.
//

#ifndef OSMTOOLSPROJECT_DOWNLOADER_H
#define OSMTOOLSPROJECT_DOWNLOADER_H

#include <curl/curl.h>

namespace OSMTools {
    struct myprogress {
        double lastruntime; /* type depends on version, see above */
        CURL *curl;
    };

    class Downloader {
    public:
        Downloader();
        ~Downloader();

        bool Get(const char* url, const char* post, const char* file_name);

    protected:
        static int xferinfo(void *p,
                            curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t ultotal, curl_off_t ulnow);

        static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream);

    protected:
        // MINIMAL_PROGRESS_FUNCTIONALITY_INTERVAL
        static double m_progress_report_interval;

        static double m_progress;
    };
}

#endif //OSMTOOLSPROJECT_DOWNLOADER_H
