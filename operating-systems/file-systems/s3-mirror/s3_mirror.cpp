#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/ListObjectsV2Request.h>
#include <aws/s3/model/CommonPrefix.h>
#include <iostream>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include "s3_mirror.h"
using namespace Aws;
using namespace Aws::Auth;
using namespace std::chrono;

Aws::SDKOptions options;

extern "C" {
    void s3m_init_api();

    void s3m_shutdown_api();

    s3m_object_list_t *s3m_list_objects(const char *bucket, const char *prefix);    
}

void s3m_init_api() {
    Aws::InitAPI(options);
}

void s3m_shutdown_api() {
    Aws::ShutdownAPI(options);
}

s3m_object_list_t *s3m_list_objects(const char *bucket, const char *prefix) {
    std::cout << "    cpp prefix: " << prefix << std::endl;

    static Aws::Client::ClientConfiguration clientConfig;
    static Aws::S3::S3Client s3Client(clientConfig);
    
    Aws::S3::Model::ListObjectsV2Request request;

    request.WithBucket(bucket).WithPrefix(prefix).WithDelimiter("/");

    Aws::String continuationToken; // Used for pagination.
    Aws::Vector<Aws::S3::Model::Object> allObjects;
    Aws::Vector<Aws::S3::Model::CommonPrefix> allPrefixes;
    
    do {
        if (!continuationToken.empty()) {
            request.SetContinuationToken(continuationToken);
        }

        auto outcome = s3Client.ListObjectsV2(request);

        if (!outcome.IsSuccess()) {
            std::cerr << "Error: listObjects: " <<
                      outcome.GetError().GetMessage() << std::endl;
            return nullptr;
        } else {
            Aws::Vector<Aws::S3::Model::Object> objects =
                    outcome.GetResult().GetContents();
            Aws::Vector<Aws::S3::Model::CommonPrefix> prefixes =
                    outcome.GetResult().GetCommonPrefixes();

            allObjects.insert(allObjects.end(), objects.begin(), objects.end());
            allPrefixes.insert(allPrefixes.end(), prefixes.begin(), prefixes.end());
            continuationToken = outcome.GetResult().GetNextContinuationToken();
        }
    } while (!continuationToken.empty());

    s3m_object_list_t *list = (s3m_object_list_t *)malloc(sizeof(s3m_object_list_t *));
    list->num_objs = allObjects.size();
    list->num_dirs = allPrefixes.size();
    list->objs = (s3m_object_t*)malloc(allObjects.size() * sizeof(s3m_object_t *));
    list->dirs = (char**)malloc(allPrefixes.size() * sizeof(char *));
 
    int i = 0;

    for (const auto &object: allObjects) {
        std::cout << "    cpp file: " << object.GetKey() << std::endl;
        
        auto tp = object.GetLastModified().UnderlyingTimestamp();
        auto secs = time_point_cast<seconds>(tp);

        list->objs[i].size = object.GetSize();
        list->objs[i].last_modified = secs.time_since_epoch().count();
        list->objs[i].key = strdup(object.GetKey().c_str());

        i++;
    }

    int j = 0;

    for (const auto &prfx: allPrefixes) {
        std::cout << "    cpp dir: " << prfx.GetPrefix() << std::endl;
        list->dirs[j++] = strdup(prfx.GetPrefix().c_str());
    }

    return list;
}