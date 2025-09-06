# Distributed File System (DFS) with AFS Semantics

![C++](https://img.shields.io/badge/C++17-00599C?style=flat&logo=c%2B%2B&logoColor=white)
![gRPC](https://img.shields.io/badge/gRPC-4A90E2?style=flat&logo=google)
![FUSE](https://img.shields.io/badge/FUSE-linux-blue)
![CMake](https://img.shields.io/badge/build-CMake-blue)
![Ubuntu](https://img.shields.io/badge/ubuntu-tested-orange?logo=ubuntu)
![License](https://img.shields.io/badge/license-MIT-yellow)
![Status](https://img.shields.io/badge/status-working-brightgreen)

---

This project implements a mountable **Distributed File System (DFS)** in C++ using:

- 📡 **gRPC** for client-server communication  
- 🧠 **AFS-style versioning** with Last Writer Wins semantics  
- 💾 **FUSE (Filesystem in Userspace)** for seamless Linux file system integration  

Interact with it just like a regular file system — `echo`, `cat`, `rm` etc. work exactly as expected!

---

## 🧠 Key Features

- Mountable virtual file system (via FUSE)
- Remote read/write/delete operations via gRPC
- Automatic version conflict resolution using timestamps
- Designed with AFS-like caching and update consistency
- Clean modular C++ design

---

## 📁 Folder Structure

```
dfs_project/
├── proto/              # gRPC protobuf definitions
│   └── dfs.proto
├── client/             # gRPC + FUSE client
│   ├── dfs_client.cpp  # CLI test client
│   └── fuse_client.cpp # Mountable FUSE client
├── server/             # DFS gRPC server
│   └── dfs_server.cpp
├── build/              # Build artifacts (created after cmake)
├── CMakeLists.txt      # Project build configuration
```

---

## ⚙️ Dependencies

### 📦 Install Required Packages (Ubuntu)

```bash
sudo apt update
sudo apt install -y build-essential cmake git libfuse3-dev pkg-config \
                    protobuf-compiler grpc-tools libgrpc++-dev
```

---

## 🛠️ Build Instructions

Clone and build the project:

```bash
git clone https://github.com/YOUR_USERNAME/dfs_project.git
cd dfs_project

# Generate gRPC code
cd proto
protoc -I=. --cpp_out=../build --grpc_out=../build \
  --plugin=protoc-gen-grpc=$(which grpc_cpp_plugin) dfs.proto
cd ..

# Build the entire project
cmake -S . -B build
cmake --build build
```

---

## 🚀 Run the DFS

### Step 1: Start the DFS Server

```bash
./build/server
```

> Keeps running in the background, serving file requests.

---

### Step 2: Mount the DFS with FUSE

```bash
mkdir -p /tmp/dfs_mount
./build/fuse_client /tmp/dfs_mount -f
```

> This mounts your DFS at `/tmp/dfs_mount`

---

## ✅ Testing the File System

Open another terminal and run the following:

```bash
# Write to a file in the DFS
echo "hello world" > /tmp/dfs_mount/test.txt

# Read the file
cat /tmp/dfs_mount/test.txt

# Delete the file
rm /tmp/dfs_mount/test.txt

# Confirm it's gone
ls /tmp/dfs_mount
```

✅ You’ll see all these operations are forwarded over gRPC to the DFS server.

---

## 🧠 Versioning Logic: "Last Writer Wins"

Each `WriteRequest` includes a **timestamp**. The server tracks the last modified time of each file. If a client tries to write an older version:

```txt
Write from outdated client rejected (Last Writer Wins)
```

This implements the AFS-style consistency model.

---

## 🛑 Unmounting the File System

From another terminal:

```bash
fusermount3 -u /tmp/dfs_mount
```

If that fails:

```bash
sudo umount /tmp/dfs_mount
```

---

## 💬 Project Explanation

> I built a mountable distributed file system using gRPC and FUSE in C++. The client communicates with the DFS server over gRPC to perform remote file operations. To maintain consistency across clients, I implemented AFS-style "Last Writer Wins" semantics using per-file timestamps. The file system is mounted via FUSE, allowing standard Linux tools to interact with it.

---

## 📌 Future Improvements

- Client-side caching + callback invalidation
- Directory support (`readdir`, `mkdir`)
- Multi-server replication
- Authentication & access control

---

## 🧑‍💻 Author

Sai Keerthan Palavarapu • [LinkedIn](https://www.linkedin.com/in/saikeerthan) • [GitHub](https://github.com/saikeerthan-14)

---

## 🪪 License

This project is licensed under the [MIT License](./LICENSE).
