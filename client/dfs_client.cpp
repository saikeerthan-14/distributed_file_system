#include <iostream>
#include <fstream>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>
#include "../build/dfs.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using dfs::DFS;
using dfs::ReadRequest;
using dfs::ReadResponse;

class DFSClient {
public:
    DFSClient(std::shared_ptr<Channel> channel)
        : stub_(DFS::NewStub(channel)) {}

    void ReadFile(const std::string& path, int64_t offset, int64_t size) {
        ReadRequest request;
        request.set_path(path);
        request.set_offset(offset);
        request.set_size(size);

        ReadResponse response;
        ClientContext context;

        Status status = stub_->Read(&context, request, &response);

        if (status.ok()) {
            std::cout << "Read " << response.bytes_read() << " bytes:\n";
            std::cout << response.data() << std::endl;
        } else {
            std::cerr << "Read failed: " << status.error_message() << std::endl;
        }
    }

    void WriteFile(const std::string& path, const std::string& content, int64_t offset = 0) {
        dfs::WriteRequest request;
        request.set_path(path);
        request.set_offset(offset);
        request.set_data(content);
        // request.set_mtime(std::time(nullptr));
        request.set_mtime(std::time(nullptr));
    
        dfs::WriteResponse response;
        grpc::ClientContext context;
    
        grpc::Status status = stub_->Write(&context, request, &response);
    
        if (status.ok()) {
            std::cout << "Wrote " << response.bytes_written() << " bytes.\n";
        } else {
            std::cerr << "Write failed: " << status.error_message() << std::endl;
        }
    }

    void DeleteFile(const std::string& path) {
        dfs::UnlinkRequest request;
        request.set_path(path);
        dfs::UnlinkResponse response;
        grpc::ClientContext context;
    
        auto status = stub_->Unlink(&context, request, &response);
        if (status.ok() && response.success()) {
            std::cout << "File deleted: " << path << std::endl;
        } else {
            std::cerr << "Delete failed: " << status.error_message() << std::endl;
        }
    }

    void GetFileAttr(const std::string& path) {
        dfs::GetAttrRequest request;
        request.set_path(path);
        dfs::GetAttrResponse response;
        grpc::ClientContext context;
    
        auto status = stub_->GetAttr(&context, request, &response);
        if (status.ok() && response.exists()) {
            std::cout << "File size: " << response.size()
                      << ", Modified: " << response.mtime() << std::endl;
        } else {
            std::cerr << "GetAttr failed: " << status.error_message() << std::endl;
        }
    }

private:
    std::unique_ptr<DFS::Stub> stub_;
};

int main(int argc, char** argv) {
    std::string target_str = "localhost:50051";
    DFSClient client(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

    std::string file = argc > 1 ? argv[1] : "test.txt";
    client.ReadFile(file, 0, 1024);  // Read first 1KB of the file

    client.WriteFile("test.txt", "Modified content\n");
    client.ReadFile("test.txt", 0, 1024);

    client.WriteFile("temp.txt", "Temporary file");
    client.GetFileAttr("temp.txt");
    client.DeleteFile("temp.txt");
    client.GetFileAttr("temp.txt");
    return 0;
}