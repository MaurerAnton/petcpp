// petcpp - C++ port of pet (CLI snippet manager)
// Snippets stored in simple key-value format in ~/.config/petcpp/snippets.txt

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

static const char* VERSION = "0.1.0";

struct Snippet {
    std::string description;
    std::string command;
    std::vector<std::string> tags;
    std::string output;
};

static std::string configDir() {
    const char* home = getenv("HOME");
    if (!home) home = "/tmp";
    return std::string(home) + "/.config/petcpp";
}

static std::string snippetFile() {
    return configDir() + "/snippets.txt";
}

static std::string configFile() {
    return configDir() + "/config.txt";
}

/* Simple config: key=value format */
static std::string getConfig(const std::string& key, const std::string& deflt = "") {
    std::ifstream f(configFile());
    if (!f) return deflt;
    std::string line;
    while (std::getline(f, line)) {
        if (line.find(key + "=") == 0) {
            return line.substr(key.size() + 1);
        }
    }
    return deflt;
}

/* Load snippets from file */
static std::vector<Snippet> loadSnippets() {
    std::vector<Snippet> snippets;
    std::ifstream f(snippetFile());
    if (!f) return snippets;

    std::string line;
    Snippet cur;
    bool inSnippet = false;
    std::string multiline;

    while (std::getline(f, line)) {
        if (line.back() == '\r') line.pop_back();

        if (line == "---") {
            if (inSnippet && !cur.command.empty()) {
                if (!multiline.empty()) cur.command = multiline;
                snippets.push_back(cur);
            }
            cur = Snippet();
            multiline.clear();
            inSnippet = true;
            continue;
        }

        if (!inSnippet) continue;

        if (line.find("desc:") == 0) {
            cur.description = line.substr(5);
            /* trim */
            size_t s = 0; while (s < cur.description.size() && cur.description[s] == ' ') s++;
            cur.description = cur.description.substr(s);
        } else if (line.find("cmd:") == 0) {
            cur.command = line.substr(4);
            size_t s = 0; while (s < cur.command.size() && cur.command[s] == ' ') s++;
            cur.command = cur.command.substr(s);
        } else if (line.find("tags:") == 0) {
            std::string t = line.substr(5);
            size_t pos = 0;
            while (pos < t.size()) {
                size_t comma = t.find(',', pos);
                std::string tag = (comma == std::string::npos) ? t.substr(pos) : t.substr(pos, comma - pos);
                size_t s = 0; while (s < tag.size() && tag[s] == ' ') s++;
                size_t e = tag.size(); while (e > s && tag[e-1] == ' ') e--;
                tag = tag.substr(s, e - s);
                if (!tag.empty()) cur.tags.push_back(tag);
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
        } else if (line.find("out:") == 0) {
            cur.output = line.substr(4);
            size_t s = 0; while (s < cur.output.size() && cur.output[s] == ' ') s++;
            cur.output = cur.output.substr(s);
        } else if (line.find("  ") == 0 || line.find('\t') == 0) {
            /* continuation line for multi-line commands */
            if (!multiline.empty()) multiline += "\n";
            multiline += line;
        }
    }
    if (inSnippet && !cur.command.empty()) {
        if (!multiline.empty()) cur.command = multiline;
        snippets.push_back(cur);
    }
    return snippets;
}

/* Save snippets to file */
static void saveSnippets(const std::vector<Snippet>& snippets) {
    mkdir(configDir().c_str(), 0700);
    std::ofstream f(snippetFile());
    if (!f) return;
    for (auto& s : snippets) {
        f << "---\n";
        f << "desc: " << s.description << "\n";
        if (s.command.find('\n') != std::string::npos) {
            f << "cmd: |\n";
            std::istringstream ss(s.command);
            std::string l;
            while (std::getline(ss, l)) f << "  " << l << "\n";
        } else {
            f << "cmd: " << s.command << "\n";
        }
        if (!s.tags.empty()) {
            f << "tags: ";
            for (size_t i = 0; i < s.tags.size(); i++) {
                if (i > 0) f << ",";
                f << s.tags[i];
            }
            f << "\n";
        }
        if (!s.output.empty()) f << "out: " << s.output << "\n";
    }
}

/* Initialize config */
static void cmdConfigure() {
    mkdir(configDir().c_str(), 0700);
    std::ofstream cf(configFile());
    cf << "editor=" << (getenv("EDITOR") ? getenv("EDITOR") : "vim") << "\n";
    cf << "column=40\n";
    cf << "selectcmd=fzf --ansi --layout=reverse --border --height=90%\n";
    cf << "sortby=recency\n";
    cf.close();

    /* Create empty snippets file */
    std::ofstream sf(snippetFile());
    sf << "# petcpp snippets\n";
    sf.close();

    printf("Config created at %s\n", configDir().c_str());
}

/* List snippets */
static void cmdList(const std::vector<Snippet>& snippets, bool oneLine) {
    for (size_t i = 0; i < snippets.size(); i++) {
        auto& s = snippets[i];
        if (oneLine) {
            printf("[%zu] %s: %s\n", i, s.description.c_str(), s.command.c_str());
        } else {
            printf("\033[1;36m[%zu] %s\033[0m\n", i, s.description.c_str());
            printf("  \033[33m%s\033[0m\n", s.command.c_str());
            if (!s.tags.empty()) {
                printf("  tags: ");
                for (auto& t : s.tags) printf("%s ", t.c_str());
                printf("\n");
            }
            printf("\n");
        }
    }
}

/* Search snippets */
static void cmdSearch(const std::vector<Snippet>& snippets, const std::string& query, bool color) {
    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    for (size_t i = 0; i < snippets.size(); i++) {
        auto& s = snippets[i];
        std::string desc = s.description;
        std::string cmd = s.command;
        std::transform(desc.begin(), desc.end(), desc.begin(), ::tolower);
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

        if (desc.find(q) != std::string::npos || cmd.find(q) != std::string::npos) {
            if (color) {
                printf("\033[1;36m[%zu] %s\033[0m\n", i, s.description.c_str());
                printf("  \033[33m%s\033[0m\n\n", s.command.c_str());
            } else {
                printf("[%zu] %s: %s\n", i, s.description.c_str(), s.command.c_str());
            }
        }
    }
}

/* New snippet */
static void cmdNew(std::vector<Snippet>& snippets) {
    Snippet s;
    printf("Description: ");
    std::getline(std::cin, s.description);
    printf("Command: ");
    std::getline(std::cin, s.command);

    /* Use editor for multiline */
    printf("Use editor for command? [y/N]: ");
    std::string ans;
    std::getline(std::cin, ans);
    if (ans == "y" || ans == "Y") {
        std::string editor = getConfig("editor", "vim");
        std::string tmpfile = "/tmp/petcpp_cmd_" + std::to_string(getpid());
        {
            std::ofstream tf(tmpfile);
            tf << s.command;
        }
        std::string cmd = editor + " " + tmpfile;
        system(cmd.c_str());
        std::ifstream tf(tmpfile);
        std::stringstream buf;
        buf << tf.rdbuf();
        s.command = buf.str();
        /* trim trailing newline */
        while (!s.command.empty() && s.command.back() == '\n') s.command.pop_back();
        unlink(tmpfile.c_str());
    }

    printf("Tags (comma-sep): ");
    std::string tags;
    std::getline(std::cin, tags);
    /* parse tags */
    size_t pos = 0;
    while (pos < tags.size()) {
        size_t comma = tags.find(',', pos);
        std::string tag = (comma == std::string::npos) ? tags.substr(pos) : tags.substr(pos, comma - pos);
        size_t s2 = 0; while (s2 < tag.size() && tag[s2] == ' ') s2++;
        tag = tag.substr(s2);
        if (!tag.empty()) s.tags.push_back(tag);
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }

    snippets.push_back(s);
    saveSnippets(snippets);
    printf("Snippet added.\n");
}

/* Edit snippet */
static void cmdEdit(std::vector<Snippet>& snippets, int idx) {
    if (idx < 0 || idx >= (int)snippets.size()) {
        fprintf(stderr, "Invalid snippet index\n");
        return;
    }
    std::string editor = getConfig("editor", "vim");
    std::string tmpfile = "/tmp/petcpp_edit_" + std::to_string(getpid());
    {
        std::ofstream tf(tmpfile);
        auto& s = snippets[idx];
        tf << "description: " << s.description << "\n";
        tf << "command:\n" << s.command << "\n";
        tf << "tags: ";
        for (size_t i = 0; i < s.tags.size(); i++) {
            if (i > 0) tf << ",";
            tf << s.tags[i];
        }
        tf << "\n";
    }
    std::string cmd = editor + " " + tmpfile;
    system(cmd.c_str());

    /* Parse edited file */
    std::ifstream tf(tmpfile);
    std::string line;
    auto& s = snippets[idx];
    s.tags.clear();
    s.command.clear();
    bool inCmd = false;
    while (std::getline(tf, line)) {
        if (line.find("description:") == 0) {
            s.description = line.substr(12);
            size_t ss = 0; while (ss < s.description.size() && s.description[ss] == ' ') ss++;
            s.description = s.description.substr(ss);
        } else if (line.find("command:") == 0) {
            inCmd = true;
        } else if (line.find("tags:") == 0) {
            inCmd = false;
            std::string t = line.substr(5);
            size_t pos = 0;
            while (pos < t.size()) {
                size_t comma = t.find(',', pos);
                std::string tag = (comma == std::string::npos) ? t.substr(pos) : t.substr(pos, comma - pos);
                size_t ss = 0; while (ss < tag.size() && tag[ss] == ' ') ss++;
                tag = tag.substr(ss);
                if (!tag.empty()) s.tags.push_back(tag);
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
        } else if (inCmd) {
            if (!s.command.empty()) s.command += "\n";
            s.command += line;
        }
    }
    unlink(tmpfile.c_str());
    saveSnippets(snippets);
    printf("Snippet updated.\n");
}

/* Execute snippet */
static void cmdExec(const std::vector<Snippet>& snippets, int idx) {
    if (idx < 0 || idx >= (int)snippets.size()) {
        fprintf(stderr, "Invalid snippet index\n");
        return;
    }
    auto& s = snippets[idx];
    printf("\033[90m$ %s\033[0m\n", s.command.c_str());
    int ret = system(s.command.c_str());
    if (ret != 0) printf("Exit: %d\n", WEXITSTATUS(ret));
}

/* Sync via curl (to GitHub Gist) */
static void cmdSync() {
    std::string gistID = getConfig("gist_id");
    std::string token = getConfig("access_token");
    if (gistID.empty() || token.empty()) {
        fprintf(stderr, "Configure gist_id and access_token in %s\n", configFile().c_str());
        return;
    }
    /* Upload snippet file to Gist */
    std::string sf = snippetFile();
    std::string cmd = "curl -s -X PATCH -H 'Authorization: token " + token + "'"
        " -H 'Content-Type: application/json'"
        " -d '{\"files\":{\"pet-snippet.txt\":{\"content\":\"'\"$(cat '" + sf + "' | sed 's/\"/\\\\\"/g' | tr '\\n' '\\n')\"'\"}}'"
        " https://api.github.com/gists/" + gistID;
    printf("Syncing...\n");
    system(cmd.c_str());
}

static void printUsage() {
    fprintf(stderr,
        "petcpp - CLI snippet manager (C++ port of pet)\n"
        "Usage: petcpp [command] [flags]\n\n"
        "Commands:\n"
        "  new           Create a new snippet\n"
        "  list          List all snippets\n"
        "  search QUERY  Search snippets\n"
        "  edit N        Edit snippet #N\n"
        "  exec N        Execute snippet #N\n"
        "  configure     Initialize config\n"
        "  sync          Sync to GitHub Gist\n"
        "  version       Print version\n"
        "  help          Show this help\n\n"
        "Flags:\n"
        "  -1, --oneline  Compact list output\n"
        "  -c, --color    Colorize output\n"
        "  -t, --tag TAG  Filter by tag\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2) { printUsage(); return 0; }

    std::string cmd = argv[1];

    if (cmd == "version") {
        printf("petcpp %s\n", VERSION);
        return 0;
    }
    if (cmd == "help" || cmd == "-h" || cmd == "--help") {
        printUsage();
        return 0;
    }
    if (cmd == "configure") {
        cmdConfigure();
        return 0;
    }
    if (cmd == "sync") {
        cmdSync();
        return 0;
    }

    /* Commands that need snippets loaded */
    auto snippets = loadSnippets();

    if (cmd == "list") {
        bool oneLine = false;
        for (int i = 2; i < argc; i++)
            if (strcmp(argv[i], "-1") == 0 || strcmp(argv[i], "--oneline") == 0) oneLine = true;
        cmdList(snippets, oneLine);
    } else if (cmd == "search") {
        if (argc < 3) { fprintf(stderr, "Usage: petcpp search <query>\n"); return 1; }
        bool color = false;
        for (int i = 2; i < argc; i++)
            if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--color") == 0) color = true;
        cmdSearch(snippets, argv[2], color);
    } else if (cmd == "new") {
        cmdNew(snippets);
    } else if (cmd == "edit") {
        if (argc < 3) { fprintf(stderr, "Usage: petcpp edit <index>\n"); return 1; }
        cmdEdit(snippets, atoi(argv[2]));
    } else if (cmd == "exec") {
        if (argc < 3) { fprintf(stderr, "Usage: petcpp exec <index>\n"); return 1; }
        cmdExec(snippets, atoi(argv[2]));
    } else {
        fprintf(stderr, "Unknown command: %s\n", cmd.c_str());
        printUsage();
        return 1;
    }

    return 0;
}
