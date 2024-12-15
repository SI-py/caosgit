#include "repository.h"
#include "utils.h"
#include <filesystem>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

void Repository::init(const std::string& path) {
    if (!fs::exists(path)) {
        fs::create_directories(path);
    }

    repo_path = fs::absolute(path).string();
    fs::path caosgit = fs::path(repo_path) / ".caosgit";
    if (fs::exists(caosgit)) {
        throw std::runtime_error("Repository already initialized here.");
    }

    fs::create_directories(caosgit / "objects");
    fs::create_directories(caosgit / "refs");
    // Создаем ветку main
    {
        std::ofstream head((caosgit / "HEAD").string());
        head << "ref: refs/main" << std::endl;
    }

    {
        std::ofstream main_ref((caosgit / "refs" / "main").string());
        main_ref << "" << std::endl; // пустой, нет ещё коммитов
    }

    std::cout << "Initialized empty caosgit repository in " << repo_path << "/.caosgit\n";
}

bool Repository::isInitialized() {
    if (repo_path.empty()) {
        // Пытаемся найти .caosgit от текущей директории вверх
        std::string cur = fs::current_path().string();
        fs::path p = cur;
        while (!p.empty()) {
            if (fs::exists(p / ".caosgit")) {
                repo_path = p.string();
                return true;
            }
            if (p.has_parent_path())
                p = p.parent_path();
            else
                break;
        }
        return false;
    } else {
        return fs::exists(fs::path(repo_path) / ".caosgit");
    }
}

void Repository::ensureInitialized() {
    if (!isInitialized()) {
        throw std::runtime_error("Not a caosgit repository (or any of the parent directories). Run 'init' first.");
    }
}

std::string Repository::getHeadRefPath() {
    ensureInitialized();
    fs::path head = fs::path(repo_path) / ".caosgit" / "HEAD";
    if (!fs::exists(head)) {
        throw std::runtime_error("HEAD not found - corrupted repository.");
    }
    std::ifstream in(head.string());
    std::string line;
    std::getline(in, line);
    if (line.rfind("ref: ", 0) == 0) {
        return line.substr(5);
    }
    throw std::runtime_error("HEAD is detached or corrupted.");
}

std::string Repository::getHeadBranchName() {
    std::string refPath = getHeadRefPath(); // something like refs/main
    fs::path p = refPath;
    return p.filename().string();
}

std::string Repository::getBranchRefPath(const std::string& branch_name) {
    return (fs::path(repo_path) / ".caosgit" / "refs" / branch_name).string();
}

std::string Repository::readCurrentCommit() {
    ensureInitialized();
    std::string refPath = getHeadRefPath();
    fs::path fullRef = fs::path(repo_path) / ".caosgit" / refPath;
    if (!fs::exists(fullRef)) {
        return "";
    }
    std::ifstream in(fullRef.string());
    std::string commit;
    std::getline(in, commit);
    return commit;
}

void Repository::writeCurrentCommit(const std::string& commit_hash) {
    ensureInitialized();
    std::string refPath = getHeadRefPath();
    fs::path fullRef = fs::path(repo_path) / ".caosgit" / refPath;
    std::ofstream out(fullRef.string());
    out << commit_hash << std::endl;
}

std::map<std::string, std::string> Repository::getWorkingDirectoryState() {
    std::map<std::string, std::string> files;
    for (auto &entry: std::filesystem::recursive_directory_iterator(repo_path, std::filesystem::directory_options::skip_permission_denied)) {
        auto rel = std::filesystem::relative(entry.path(), repo_path);

        // Если путь указывает на .caosgit или находится внутри него — пропускаем
        // Проверяем не только точное совпадение ".caosgit", но и любые вложенные пути (".caosgit/...").
        const std::string relStr = rel.string();
        if (relStr == ".caosgit" || relStr.rfind(".caosgit/", 0) == 0) {
            continue;
        }

        if (entry.is_regular_file()) {
            std::ifstream in(entry.path().string(), std::ios::binary);
            std::ostringstream buf;
            buf << in.rdbuf();
            files[relStr] = buf.str();
        }
    }
    return files;
}

std::string Repository::createCommitObject(const std::string& parent, const std::string& message) {
    // Создаём снимок текущего состояния
    auto state = getWorkingDirectoryState();

    // Сериализуем содержимое коммита
    std::ostringstream commit_content;
    commit_content << "parent " << parent << "\n";
    commit_content << "message " << message << "\n";
    for (auto &f: state) {
        commit_content << "file " << f.first << "\n";
    }
    commit_content << "\n";

    // Генерируем хэш
    std::string hash = generateCommitHash(commit_content.str());

    // Записываем объект
    fs::path commit_path = fs::path(repo_path) / ".caosgit" / "objects" / hash;
    std::ofstream out(commit_path.string(), std::ios::binary);
    out << commit_content.str();
    // Записываем содержимое файлов (snapshot)
    out << "\n--FILES--\n";
    for (auto &f: state) {
        out << "##" << f.first << "\n";
        out << f.second;
        out << "\n--END--\n";
    }

    return hash;
}

std::string Repository::getCommitContent(const std::string& commit_hash) {
    fs::path commit_path = fs::path(repo_path) / ".caosgit" / "objects" / commit_hash;
    if (!fs::exists(commit_path)) {
        throw std::runtime_error("Commit not found: " + commit_hash);
    }
    std::ifstream in(commit_path.string(), std::ios::binary);
    std::ostringstream buf;
    buf << in.rdbuf();
    return buf.str();
}

std::string Repository::getParentCommit(const std::string& commit_hash) {
    if (commit_hash.empty()) return "";
    std::string content = getCommitContent(commit_hash);
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.rfind("parent ", 0) == 0) {
            return line.substr(7);
        }
        if (line.empty()) break;
    }
    return "";
}

std::vector<std::string> Repository::getCommittedFiles(const std::string& commit_hash) {
    std::vector<std::string> result;
    if (commit_hash.empty()) return result;
    std::string content = getCommitContent(commit_hash);
    std::istringstream iss(content);
    std::string line;
    while (std::getline(iss, line)) {
        if (line.rfind("file ",0) == 0) {
            result.push_back(line.substr(5));
        }
        if (line.empty()) break; 
    }
    return result;
}

void Repository::writeBranch(const std::string& branch_name, const std::string& commit_hash) {
    ensureInitialized();
    std::ofstream out(getBranchRefPath(branch_name));
    out << commit_hash << "\n";
}

bool Repository::branchExists(const std::string& branch_name) {
    fs::path refPath = fs::path(repo_path) / ".caosgit" / "refs" / branch_name;
    return fs::exists(refPath);
}

std::string Repository::generateCommitHash(const std::string& content) {
    return simpleHash(content);
}

void Repository::status() {
    ensureInitialized();
    std::string current_commit = readCurrentCommit();

    auto wd_state = getWorkingDirectoryState();
    auto commit_state = getCommitSnapshot(current_commit);

    // Сравниваем состояния
    bool changes = false;
    // Проверяем изменённые/добавленные
    for (auto &f: wd_state) {
        if (commit_state.find(f.first) == commit_state.end()) {
            std::cout << "Added: " << f.first << "\n";
            changes = true;
        } else {
            if (commit_state[f.first] != f.second) {
                std::cout << "Modified: " << f.first << "\n";
                changes = true;
            }
        }
    }
    // Проверяем удалённые
    for (auto &f: commit_state) {
        if (wd_state.find(f.first) == wd_state.end()) {
            std::cout << "Deleted: " << f.first << "\n";
            changes = true;
        }
    }

    if (!changes) {
        std::cout << "No changes.\n";
    }
}

void Repository::commit(const std::string& message) {
    ensureInitialized();
    std::string parent = readCurrentCommit();
    std::string new_commit = createCommitObject(parent, message);
    writeCurrentCommit(new_commit);
    std::cout << "Committed as " << new_commit << " with message: " << message << std::endl;
}

void Repository::new_branch(const std::string& branch_name) {
    ensureInitialized();
    if (branchExists(branch_name)) {
        throw std::runtime_error("Branch already exists.");
    }
    std::string current_commit = readCurrentCommit();
    writeBranch(branch_name, current_commit);
    std::cout << "New branch created: " << branch_name << std::endl;
}

void Repository::jump(const std::string& branch_name) {
    ensureInitialized();
    if (!branchExists(branch_name)) {
        throw std::runtime_error("Branch not found.");
    }
    fs::path head = fs::path(repo_path) / ".caosgit" / "HEAD";
    {
        std::ofstream out(head.string());
        out << "ref: refs/" << branch_name << "\n";
    }
    std::cout << "Switched to branch " << branch_name << std::endl;
}

void Repository::merge(const std::string& branch_name) {
    ensureInitialized();
    if (!branchExists(branch_name)) {
        throw std::runtime_error("Branch not found.");
    }
    // Упрощённое слияние: просто берём состояние ветки и делаем коммит
    std::string target_commit;
    {
        std::ifstream in(getBranchRefPath(branch_name));
        std::getline(in, target_commit);
    }

    if (target_commit.empty()) {
        std::cout << "Nothing to merge." << std::endl;
        return;
    }

    // Для простоты: не проверяем конфликтов, просто делаем снимок текущего состояния и мержим
    // Идея: применить состояние target_commit к рабочей директории (но для упрощения ничего не меняем)
    // Просто создадим новый коммит с сообщением "Merge branch X"
    std::string parent = readCurrentCommit();
    std::string new_commit = createCommitObject(parent, "Merge branch " + branch_name);
    writeCurrentCommit(new_commit);
    std::cout << "Merged branch " << branch_name << " into " << getHeadBranchName() << std::endl;
}

void Repository::revert(const std::string& commit_hash) {
    ensureInitialized();
    // Упрощенно: создадим новый коммит с сообщением "Revert ..." без реальной отмены изменений
    // Настоящая реализация требует анализа состояния файлов.
    std::string parent = readCurrentCommit();
    std::string new_commit = createCommitObject(parent, "Revert commit " + commit_hash);
    writeCurrentCommit(new_commit);
    std::cout << "Reverted commit " << commit_hash << std::endl;
}

void Repository::reset(const std::string& commit_hash) {
    ensureInitialized();
    // Устанавливаем текущую ветку на указанный коммит
    // Предполагаем, что commit_hash существует, иначе исключение
    fs::path commit_path = fs::path(repo_path) / ".caosgit" / "objects" / commit_hash;
    if (!fs::exists(commit_path)) {
        throw std::runtime_error("Commit not found: " + commit_hash);
    }
    writeCurrentCommit(commit_hash);
    std::cout << "Reset current branch to " << commit_hash << std::endl;
}

void Repository::tag(const std::string& tag_name, const std::string& commit_hash) {
    ensureInitialized();
    fs::path tag_path = fs::path(repo_path) / ".caosgit" / "refs" / ("tag_" + tag_name);
    std::ofstream out(tag_path.string());
    out << commit_hash << "\n";
    std::cout << "Tag " << tag_name << " created for commit " << commit_hash << std::endl;
}

void Repository::tags() {
    ensureInitialized();
    for (auto &entry: fs::directory_iterator(fs::path(repo_path) / ".caosgit" / "refs")) {
        if (entry.is_regular_file()) {
            std::string name = entry.path().filename().string();
            if (name.rfind("tag_",0)==0) {
                std::ifstream in(entry.path().string());
                std::string commit;
                std::getline(in, commit);
                std::cout << name.substr(4) << " -> " << commit << "\n";
            }
        }
    }
}

void Repository::log() {
    ensureInitialized();
    // Проходим по цепочке родительских коммитов от текущего
    std::string commit = readCurrentCommit();
    while (!commit.empty()) {
        std::string content = getCommitContent(commit);
        std::istringstream iss(content);
        std::string line;
        std::string parent;
        std::string message;
        while (std::getline(iss,line)) {
            if (line.rfind("parent ",0)==0) {
                parent = line.substr(7);
            } else if (line.rfind("message ",0)==0) {
                message = line.substr(8);
            }
            if (line.empty()) break;
        }
        std::cout << commit << " " << message << "\n";
        commit = parent;
    }
}

std::map<std::string, std::string> Repository::getCommitSnapshot(const std::string& commit_hash) {
    std::map<std::string, std::string> snapshot;
    if (commit_hash.empty()) return snapshot;

    std::string content = getCommitContent(commit_hash);
    auto pos = content.find("\n--FILES--\n");
    if (pos == std::string::npos) {
        return snapshot; 
    }

    std::string files_part = content.substr(pos + 11); // пропускаем \n--FILES--\n
    std::istringstream iss(files_part);
    std::string line;
    std::string current_file;
    std::ostringstream file_content;
    bool reading_file = false;
    while (std::getline(iss, line)) {
        if (line.rfind("##",0)==0) {
            if (reading_file && !current_file.empty()) {
                snapshot[current_file] = file_content.str();
                file_content.str("");
                file_content.clear();
            }
            current_file = line.substr(2);
            reading_file = true;
        } else if (line == "--END--") {
            if (!current_file.empty()) {
                snapshot[current_file] = file_content.str();
                file_content.str("");
                file_content.clear();
                current_file.clear();
                reading_file = false;
            }
        } else {
            if (reading_file) {
                file_content << line << "\n";
            }
        }
    }

    return snapshot;
}
