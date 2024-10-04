# S3 Mirror

Small application for mounting and exploring a S3 Bucket in your favourite terminal.

Uses [FUSE] (https://github.com/libfuse/libfuse) and AWS C++ SDK under the covers.

## Building the Application

You will first need to install the [AWS C++ SDK](https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/getting-started.html).

Application can then be compiled using the `Makefile`.

## Running the Application

To run in the background:

```./s3m <bucket name> <mount point>```

To close the application, unmount the file system:

```umount -l <s3m mount point>```

To run in the foreground, use the FUSE ```-f``` option:

```./s3m -f <bucket name> <mount point>```

### AWS Authentication

Refer to [SDK authentication with AWS](https://docs.aws.amazon.com/sdk-for-cpp/v1/developer-guide/credentials.html).

