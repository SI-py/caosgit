#pragma once
#include <string>
#include <map>
#include <vector>

class Repository {
public:
    Repository() = default;
    void init(const std::string& path);
    void status();
    void commit(const std::string& message);
    void new_branch(const std::string& branch_name);
    void jump(const std::string& branch_name);
    void merge(const std::string& branch_name);
    void revert(const std::string& commit_hash);
    void reset(const std::string& commit_hash);
    void tag(const std::string& tag_name, const std::string& commit_hash);
    void tags();
    void log();

private:
    std::string repo_path;
    bool isInitialized();

    std::string getHeadRefPath();
    std::string getHeadBranchName();
    std::string getBranchRefPath(const std::string& branch_name);
    std::string readCurrentCommit();
    void writeCurrentCommit(const std::string& commit_hash);

    std::string createCommitObject(const std::string& parent, const std::string& message);
    std::string getCommitContent(const std::string& commit_hash);
    std::string getParentCommit(const std::string& commit_hash);
    std::vector<std::string> getCommittedFiles(const std::string& commit_hash);
    void writeBranch(const std::string& branch_name, const std::string& commit_hash);
    bool branchExists(const std::string& branch_name);

    void writeTag(const std::string& tag_name, const std::string& commit_hash);
    std::string readTag(const std::string& tag_name);
    std::vector<std::string> listTags();

    std::map<std::string, std::string> getWorkingDirectoryState();
    std::map<std::string, std::string> getCommitSnapshot(const std::string& commit_hash);

    std::string generateCommitHash(const std::string& content);
    void ensureInitialized();
};
