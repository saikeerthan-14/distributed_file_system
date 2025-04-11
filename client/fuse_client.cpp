#define FUSE_USE_VERSION 35
#include <fuse3/fuse.h>
#include <string.h>
#include <unistd.h>
#include <iostream>
#include <grpcpp/grpcpp.h>
#include "../build/dfs.grpc.pb.h"

using grpc::Channel;
using dfs::DFS;
using dfs::GetAttrRequest;
using dfs::GetAttrResponse;
using dfs::ReadRequest;
using dfs::ReadResponse;

// Global gRPC stub
std::unique_ptr<DFS::Stub> stub_;

static int dfs_getattr(const char *path, struct stat *st, struct fuse_file_info *) {
    memset(st, 0, sizeof(struct stat));
    GetAttrRequest request;
    request.set_path(path + 1); // remove leading "/"

    GetAttrResponse response;
    grpc::ClientContext context;
    auto status = stub_->GetAttr(&context, request, &response);

    if (!status.ok() || !response.exists()) return -ENOENT;

    st->st_mode = S_IFREG | 0666;
    st->st_nlink = 1;
    st->st_size = response.size();
    st->st_mtime = response.mtime();

    return 0;
}

static int dfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *) {
    ReadRequest request;
    request.set_path(path + 1);
    request.set_offset(offset);
    request.set_size(size);

    ReadResponse response;
    grpc::ClientContext context;
    auto status = stub_->Read(&context, request, &response);

    if (!status.ok()) return -EIO;

    memcpy(buf, response.data().c_str(), response.bytes_read());
    return response.bytes_read();
}

static int dfs_create(const char *path, mode_t, struct fuse_file_info *fi) {
    std::string empty_data = "";
    dfs::WriteRequest request;
    request.set_path(path + 1); // strip leading "/"
    request.set_offset(0);
    request.set_data(empty_data);
    request.set_mtime(std::time(nullptr)); // send current time

    dfs::WriteResponse response;
    grpc::ClientContext context;

    auto status = stub_->Write(&context, request, &response);
    return status.ok() ? 0 : -EIO;
}

static int dfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *) {
    dfs::WriteRequest request;
    request.set_path(path + 1);
    request.set_offset(offset);
    request.set_data(std::string(buf, size));
    request.set_mtime(std::time(nullptr));

    dfs::WriteResponse response;
    grpc::ClientContext context;

    auto status = stub_->Write(&context, request, &response);
    if (!status.ok()) return -EIO;

    return response.bytes_written();
}

static int dfs_unlink(const char *path) {
    dfs::UnlinkRequest request;
    request.set_path(path + 1);
    dfs::UnlinkResponse response;
    grpc::ClientContext context;

    auto status = stub_->Unlink(&context, request, &response);
    return (status.ok() && response.success()) ? 0 : -ENOENT;
}

static struct fuse_operations dfs_ops = {};


int main(int argc, char *argv[]) {
    stub_ = DFS::NewStub(grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));
    dfs_ops.getattr = dfs_getattr;
    dfs_ops.read = dfs_read;
    dfs_ops.write = dfs_write;
    dfs_ops.create = dfs_create;
    dfs_ops.unlink = dfs_unlink;
    return fuse_main(argc, argv, &dfs_ops, nullptr);
}