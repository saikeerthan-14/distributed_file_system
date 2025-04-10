#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unordered_map>
#include <mutex>

#include <grpcpp/grpcpp.h>
#include "../build/dfs.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using dfs::DFS;
using dfs::ReadRequest;
using dfs::ReadResponse;

std::unordered_map<std::string, time_t> file_versions;
std::mutex version_mutex;

class DFSServerImpl final : public DFS::Service
{
public:
    Status Read(ServerContext *context, const ReadRequest *request, ReadResponse *response) override
    {
        std::string path = request->path();
        int64_t offset = request->offset();
        int64_t size = request->size();

        std::ifstream file(path, std::ios::binary);
        if (!file)
        {
            std::cerr << "Failed to open file: " << path << std::endl;
            return Status(grpc::NOT_FOUND, "File not found");
        }

        file.seekg(offset);
        std::string buffer(size, 0);
        file.read(&buffer[0], size);

        response->set_data(buffer);
        response->set_bytes_read(file.gcount());

        return Status::OK;
    }

    grpc::Status Write(grpc::ServerContext *context, const dfs::WriteRequest *request, dfs::WriteResponse *response) override
    {
        std::string path = request->path();
        int64_t offset = request->offset();
        const std::string &data = request->data();

        // Check current timestamp
        time_t client_mtime = request->mtime();  // <-- We'll add this field to proto
        time_t server_mtime = 0;

        {
            std::lock_guard<std::mutex> lock(version_mutex);
            if (file_versions.find(path) != file_versions.end()) {
                server_mtime = file_versions[path];
            }
        }

        if (client_mtime < server_mtime) {
            std::cerr << "[REJECTED] Write from older client. Last Writer Wins.\n";
            return grpc::Status(grpc::FAILED_PRECONDITION, "Outdated file version");
        }


        std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open())
        {
            // Create if not exists
            file.open(path, std::ios::out | std::ios::binary);
            file.close();
            file.open(path, std::ios::in | std::ios::out | std::ios::binary);
        }

        file.seekp(offset);
        file.write(data.data(), data.size());
        response->set_bytes_written(data.size());

        {
            std::lock_guard<std::mutex> lock(version_mutex);
            file_versions[path] = std::time(nullptr);
        }    

        return grpc::Status::OK;
    }

    grpc::Status Unlink(grpc::ServerContext *context, const dfs::UnlinkRequest *request, dfs::UnlinkResponse *response) override
    {
        std::string path = request->path();
        int result = std::remove(path.c_str());

        if (result == 0)
        {
            response->set_success(true);
            return grpc::Status::OK;
        }
        else
        {
            response->set_success(false);
            return grpc::Status(grpc::NOT_FOUND, "File not found");
        }
    }

    grpc::Status GetAttr(grpc::ServerContext *context, const dfs::GetAttrRequest *request, dfs::GetAttrResponse *response) override
    {
        std::string path = request->path();
        struct stat statbuf;

        if (stat(path.c_str(), &statbuf) == 0)
        {
            response->set_exists(true);
            response->set_size(statbuf.st_size);
            response->set_mtime(statbuf.st_mtime);
            return grpc::Status::OK;
        }
        else
        {
            response->set_exists(false);
            return grpc::Status(grpc::NOT_FOUND, "File not found");
        }
    }
};

void RunServer()
{
    std::string server_address("0.0.0.0:50051");
    DFSServerImpl service;

    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "DFS Server listening on " << server_address << std::endl;
    server->Wait();
}

int main(int argc, char **argv)
{
    RunServer();
    return 0;
}