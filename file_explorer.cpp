#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstring>
#include <iomanip>
#include <sstream>

// Linux-specific headers
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <fcntl.h>
#include <utime.h>

using namespace std;

// Color codes for better UI
#define RESET   "\033[0m"
#define BOLD    "\033[1m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"

class FileExplorer {
private:
    string currentPath;
    
    // Helper function to get file permissions as string
    string getPermissions(mode_t mode) {
        string perms;
        perms += (S_ISDIR(mode)) ? 'd' : '-';
        perms += (mode & S_IRUSR) ? 'r' : '-';
        perms += (mode & S_IWUSR) ? 'w' : '-';
        perms += (mode & S_IXUSR) ? 'x' : '-';
        perms += (mode & S_IRGRP) ? 'r' : '-';
        perms += (mode & S_IWGRP) ? 'w' : '-';
        perms += (mode & S_IXGRP) ? 'x' : '-';
        perms += (mode & S_IROTH) ? 'r' : '-';
        perms += (mode & S_IWOTH) ? 'w' : '-';
        perms += (mode & S_IXOTH) ? 'x' : '-';
        return perms;
    }
    
    // Helper function to format file size
    string formatSize(off_t size) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unitIndex = 0;
        double dSize = size;
        
        while (dSize >= 1024 && unitIndex < 4) {
            dSize /= 1024;
            unitIndex++;
        }
        
        stringstream ss;
        ss << fixed << setprecision(2) << dSize << " " << units[unitIndex];
        return ss.str();
    }
    
    // Helper function to copy file contents
    bool copyFileContents(const string& src, const string& dest) {
        int srcFd = open(src.c_str(), O_RDONLY);
        if (srcFd < 0) {
            cerr << RED << "Error: Cannot open source file" << RESET << endl;
            return false;
        }
        
        int destFd = open(dest.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (destFd < 0) {
            close(srcFd);
            cerr << RED << "Error: Cannot create destination file" << RESET << endl;
            return false;
        }
        
        char buffer[4096];
        ssize_t bytesRead;
        
        while ((bytesRead = read(srcFd, buffer, sizeof(buffer))) > 0) {
            if (write(destFd, buffer, bytesRead) != bytesRead) {
                close(srcFd);
                close(destFd);
                return false;
            }
        }
        
        close(srcFd);
        close(destFd);
        return bytesRead == 0;
    }
    
    // Recursive search function
    void searchInDirectory(const string& path, const string& pattern, vector<string>& results) {
        DIR* dir = opendir(path.c_str());
        if (!dir) return;
        
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            string name = entry->d_name;
            if (name == "." || name == "..") continue;
            
            string fullPath = path + "/" + name;
            
            // Check if name matches pattern
            if (name.find(pattern) != string::npos) {
                results.push_back(fullPath);
            }
            
            // Recursively search in subdirectories
            if (entry->d_type == DT_DIR) {
                searchInDirectory(fullPath, pattern, results);
            }
        }
        closedir(dir);
    }
    
public:
    FileExplorer() {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            currentPath = cwd;
        } else {
            currentPath = "/";
        }
    }
    
    // DAY 1: List files in current directory
    void listFiles(bool detailed = false) {
        DIR* dir = opendir(currentPath.c_str());
        if (!dir) {
            cerr << RED << "Error: Cannot open directory" << RESET << endl;
            return;
        }
        
        cout << BOLD << CYAN << "\nCurrent Directory: " << currentPath << RESET << endl;
        cout << string(80, '=') << endl;
        
        if (detailed) {
            cout << left << setw(12) << "Permissions" 
                 << setw(10) << "Owner" 
                 << setw(10) << "Group"
                 << setw(12) << "Size" 
                 << setw(20) << "Modified" 
                 << "Name" << endl;
            cout << string(80, '-') << endl;
        }
        
        struct dirent* entry;
        vector<string> files, directories;
        
        while ((entry = readdir(dir)) != nullptr) {
            string name = entry->d_name;
            if (name == ".") continue;
            
            string fullPath = currentPath + "/" + name;
            struct stat fileStat;
            
            if (stat(fullPath.c_str(), &fileStat) == 0) {
                if (detailed) {
                    // Get owner and group names
                    struct passwd* pw = getpwuid(fileStat.st_uid);
                    struct group* gr = getgrgid(fileStat.st_gid);
                    
                    // Format modified time
                    char timeStr[20];
                    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M", localtime(&fileStat.st_mtime));
                    
                    string color = S_ISDIR(fileStat.st_mode) ? BLUE : (fileStat.st_mode & S_IXUSR ? GREEN : RESET);
                    
                    cout << left << setw(12) << getPermissions(fileStat.st_mode)
                         << setw(10) << (pw ? pw->pw_name : to_string(fileStat.st_uid))
                         << setw(10) << (gr ? gr->gr_name : to_string(fileStat.st_gid))
                         << setw(12) << (S_ISDIR(fileStat.st_mode) ? "<DIR>" : formatSize(fileStat.st_size))
                         << setw(20) << timeStr
                         << color << name << RESET << endl;
                } else {
                    if (S_ISDIR(fileStat.st_mode)) {
                        directories.push_back(name);
                    } else {
                        files.push_back(name);
                    }
                }
            }
        }
        
        if (!detailed) {
            // Sort and display
            sort(directories.begin(), directories.end());
            sort(files.begin(), files.end());
            
            for (const auto& d : directories) {
                cout << BLUE << "[DIR]  " << d << RESET << endl;
            }
            for (const auto& f : files) {
                cout << "       " << f << endl;
            }
        }
        
        closedir(dir);
        cout << string(80, '=') << endl;
    }
    
    // DAY 2: Navigate to directory
    bool changeDirectory(const string& path) {
        string newPath;
        
        if (path == "..") {
            size_t pos = currentPath.find_last_of('/');
            if (pos != string::npos && pos != 0) {
                newPath = currentPath.substr(0, pos);
            } else {
                newPath = "/";
            }
        } else if (path[0] == '/') {
            newPath = path;
        } else {
            newPath = currentPath + "/" + path;
        }
        
        if (chdir(newPath.c_str()) == 0) {
            currentPath = newPath;
            cout << GREEN << "Changed to: " << currentPath << RESET << endl;
            return true;
        } else {
            cerr << RED << "Error: Cannot change to directory" << RESET << endl;
            return false;
        }
    }
    
    string getCurrentPath() const {
        return currentPath;
    }
    
    // DAY 3: Create directory
    bool createDirectory(const string& name) {
        string fullPath = currentPath + "/" + name;
        if (mkdir(fullPath.c_str(), 0755) == 0) {
            cout << GREEN << "Directory created: " << name << RESET << endl;
            return true;
        } else {
            cerr << RED << "Error: Cannot create directory" << RESET << endl;
            return false;
        }
    }
    
    // DAY 3: Create file
    bool createFile(const string& name) {
        string fullPath = currentPath + "/" + name;
        int fd = open(fullPath.c_str(), O_CREAT | O_WRONLY, 0644);
        if (fd >= 0) {
            close(fd);
            cout << GREEN << "File created: " << name << RESET << endl;
            return true;
        } else {
            cerr << RED << "Error: Cannot create file" << RESET << endl;
            return false;
        }
    }
    
    // DAY 3: Delete file or directory
    bool deleteItem(const string& name) {
        string fullPath = currentPath + "/" + name;
        struct stat fileStat;
        
        if (stat(fullPath.c_str(), &fileStat) != 0) {
            cerr << RED << "Error: Item not found" << RESET << endl;
            return false;
        }
        
        if (S_ISDIR(fileStat.st_mode)) {
            if (rmdir(fullPath.c_str()) == 0) {
                cout << GREEN << "Directory deleted: " << name << RESET << endl;
                return true;
            } else {
                cerr << RED << "Error: Cannot delete directory (may not be empty)" << RESET << endl;
                return false;
            }
        } else {
            if (unlink(fullPath.c_str()) == 0) {
                cout << GREEN << "File deleted: " << name << RESET << endl;
                return true;
            } else {
                cerr << RED << "Error: Cannot delete file" << RESET << endl;
                return false;
            }
        }
    }
    
    // DAY 3: Copy file
    bool copyFile(const string& src, const string& dest) {
        string srcPath = currentPath + "/" + src;
        string destPath = currentPath + "/" + dest;
        
        struct stat srcStat;
        if (stat(srcPath.c_str(), &srcStat) != 0 || S_ISDIR(srcStat.st_mode)) {
            cerr << RED << "Error: Source is not a file or doesn't exist" << RESET << endl;
            return false;
        }
        
        if (copyFileContents(srcPath, destPath)) {
            chmod(destPath.c_str(), srcStat.st_mode);
            cout << GREEN << "File copied: " << src << " -> " << dest << RESET << endl;
            return true;
        } else {
            cerr << RED << "Error: Cannot copy file" << RESET << endl;
            return false;
        }
    }
    
    // DAY 3: Move/Rename file
    bool moveFile(const string& src, const string& dest) {
        string srcPath = currentPath + "/" + src;
        string destPath = currentPath + "/" + dest;
        
        if (rename(srcPath.c_str(), destPath.c_str()) == 0) {
            cout << GREEN << "Moved/Renamed: " << src << " -> " << dest << RESET << endl;
            return true;
        } else {
            cerr << RED << "Error: Cannot move/rename" << RESET << endl;
            return false;
        }
    }
    
    // DAY 4: Search for files
    void searchFiles(const string& pattern) {
        cout << YELLOW << "\nSearching for '" << pattern << "' in " << currentPath << "..." << RESET << endl;
        vector<string> results;
        searchInDirectory(currentPath, pattern, results);
        
        if (results.empty()) {
            cout << "No files found matching pattern." << endl;
        } else {
            cout << GREEN << "Found " << results.size() << " result(s):" << RESET << endl;
            for (const auto& result : results) {
                cout << "  " << result << endl;
            }
        }
    }
    
    // DAY 5: Change permissions
    bool changePermissions(const string& name, const string& perms) {
        string fullPath = currentPath + "/" + name;
        
        // Convert permission string (e.g., "755") to mode_t
        mode_t mode = 0;
        if (perms.length() == 3) {
            mode = ((perms[0] - '0') << 6) | ((perms[1] - '0') << 3) | (perms[2] - '0');
        } else {
            cerr << RED << "Error: Invalid permission format (use 3 digits, e.g., 755)" << RESET << endl;
            return false;
        }
        
        if (chmod(fullPath.c_str(), mode) == 0) {
            cout << GREEN << "Permissions changed: " << name << " -> " << perms << RESET << endl;
            return true;
        } else {
            cerr << RED << "Error: Cannot change permissions" << RESET << endl;
            return false;
        }
    }
    
    // DAY 5: View file information
    void viewFileInfo(const string& name) {
        string fullPath = currentPath + "/" + name;
        struct stat fileStat;
        
        if (stat(fullPath.c_str(), &fileStat) != 0) {
            cerr << RED << "Error: File not found" << RESET << endl;
            return;
        }
        
        struct passwd* pw = getpwuid(fileStat.st_uid);
        struct group* gr = getgrgid(fileStat.st_gid);
        
        cout << BOLD << CYAN << "\nFile Information: " << name << RESET << endl;
        cout << string(60, '=') << endl;
        cout << "Type:        " << (S_ISDIR(fileStat.st_mode) ? "Directory" : "File") << endl;
        cout << "Size:        " << formatSize(fileStat.st_size) << " (" << fileStat.st_size << " bytes)" << endl;
        cout << "Permissions: " << getPermissions(fileStat.st_mode) << " (";
        cout << oct << (fileStat.st_mode & 0777) << dec << ")" << endl;
        cout << "Owner:       " << (pw ? pw->pw_name : to_string(fileStat.st_uid)) << endl;
        cout << "Group:       " << (gr ? gr->gr_name : to_string(fileStat.st_gid)) << endl;
        
        char timeStr[100];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&fileStat.st_mtime));
        cout << "Modified:    " << timeStr << endl;
        
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&fileStat.st_atime));
        cout << "Accessed:    " << timeStr << endl;
        
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", localtime(&fileStat.st_ctime));
        cout << "Changed:     " << timeStr << endl;
        cout << string(60, '=') << endl;
    }
};

void displayMenu() {
    cout << BOLD << MAGENTA << "\n╔═══════════════════════════════════════╗" << endl;
    cout << "║     LINUX FILE EXPLORER MENU         ║" << endl;
    cout << "╚═══════════════════════════════════════╝" << RESET << endl;
    cout << CYAN << "Navigation & Listing:" << RESET << endl;
    cout << "  1.  List files (simple)" << endl;
    cout << "  2.  List files (detailed)" << endl;
    cout << "  3.  Change directory" << endl;
    cout << "  4.  Show current path" << endl;
    cout << CYAN << "\nFile/Directory Operations:" << RESET << endl;
    cout << "  5.  Create directory" << endl;
    cout << "  6.  Create file" << endl;
    cout << "  7.  Delete file/directory" << endl;
    cout << "  8.  Copy file" << endl;
    cout << "  9.  Move/Rename file" << endl;
    cout << CYAN << "\nSearch & Information:" << RESET << endl;
    cout << "  10. Search files" << endl;
    cout << "  11. View file information" << endl;
    cout << CYAN << "\nPermissions:" << RESET << endl;
    cout << "  12. Change permissions" << endl;
    cout << CYAN << "\nOther:" << RESET << endl;
    cout << "  0.  Exit" << endl;
    cout << string(40, '-') << endl;
}

int main() {
    FileExplorer explorer;
    int choice;
    string input, src, dest;
    
    cout << BOLD << GREEN << "╔══════════════════════════════════════════╗" << endl;
    cout << "║  Welcome to Linux File Explorer v1.0    ║" << endl;
    cout << "╚══════════════════════════════════════════╝" << RESET << endl;
    
    while (true) {
        displayMenu();
        cout << YELLOW << "Enter choice: " << RESET;
        cin >> choice;
        cin.ignore();
        
        switch (choice) {
            case 1:
                explorer.listFiles(false);
                break;
                
            case 2:
                explorer.listFiles(true);
                break;
                
            case 3:
                cout << "Enter directory path (or .. for parent): ";
                getline(cin, input);
                explorer.changeDirectory(input);
                break;
                
            case 4:
                cout << GREEN << "Current path: " << explorer.getCurrentPath() << RESET << endl;
                break;
                
            case 5:
                cout << "Enter directory name: ";
                getline(cin, input);
                explorer.createDirectory(input);
                break;
                
            case 6:
                cout << "Enter file name: ";
                getline(cin, input);
                explorer.createFile(input);
                break;
                
            case 7:
                cout << "Enter file/directory name: ";
                getline(cin, input);
                explorer.deleteItem(input);
                break;
                
            case 8:
                cout << "Enter source file name: ";
                getline(cin, src);
                cout << "Enter destination file name: ";
                getline(cin, dest);
                explorer.copyFile(src, dest);
                break;
                
            case 9:
                cout << "Enter source name: ";
                getline(cin, src);
                cout << "Enter destination name: ";
                getline(cin, dest);
                explorer.moveFile(src, dest);
                break;
                
            case 10:
                cout << "Enter search pattern: ";
                getline(cin, input);
                explorer.searchFiles(input);
                break;
                
            case 11:
                cout << "Enter file/directory name: ";
                getline(cin, input);
                explorer.viewFileInfo(input);
                break;
                
            case 12:
                cout << "Enter file/directory name: ";
                getline(cin, src);
                cout << "Enter permissions (e.g., 755): ";
                getline(cin, dest);
                explorer.changePermissions(src, dest);
                break;
                
            case 0:
                cout << BOLD << GREEN << "Thank you for using File Explorer!" << RESET << endl;
                return 0;
                
            default:
                cout << RED << "Invalid choice. Please try again." << RESET << endl;
        }
        
        cout << "\nPress Enter to continue...";
        cin.get();
    }
    
    return 0;
}
