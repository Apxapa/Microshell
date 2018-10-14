#include <iostream>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <vector>
#include <sys/wait.h>
#include <dirent.h>
#include <algorithm>

using namespace std;

#define _HOMEDIRECTORY_ "/home/apxapa"

typedef struct parsArgv
{
    vector<vector<string>> argValue;
    string in;
    string out;
    string *currentDir;
} parsArgv, *pparsArgv;

bool predSortString(string a, string b)
{
    return a < b;
}

void getArgvs(vector<string> *myArgvs)
{
    for (;;)
    {
        string s;
        cin >> s;
        if (s == "do")
            break;
        (*myArgvs).push_back(s);
    }
    return;
}

void parsArgvs(vector<string> *myArgvs, pparsArgv pparsArgv)
{
    int i;
    for (i = 0; i < (*myArgvs).size() - 1; ++i)
    {
        if ((*myArgvs).at(i) == "<")
        {
            pparsArgv->in = (*myArgvs).at(i + 1);
        }
        if ((*myArgvs).at(i) == ">")
        {
            pparsArgv->out = (*myArgvs).at(i + 1);
        }
    }

    vector<string> vec;
    i = 0;
    if (pparsArgv->in.size() > 0 || pparsArgv->out.size() > 0)
    {
        while ((*myArgvs).at(i) != "<" && (*myArgvs).at(i) != ">")
        {
            vec.push_back((*myArgvs).at(i));
            ++i;
        }
        pparsArgv->argValue.push_back(vec);
        return;
    }
    while (i < (*myArgvs).size())
    {
        while (i < (*myArgvs).size() && (*myArgvs).at(i) != "|")
        {
            vec.push_back((*myArgvs).at(i));
            ++i;
        }
        pparsArgv->argValue.push_back(vec);
        vec.clear();
        ++i;
    }
    return;
}

void vecStrToChar(vector<string> *myArgvs, vector<char *> *myArgvc)
{
    for (int i = 0; i < (*myArgvs).size(); i++)
    {
        (*myArgvc).push_back((char *)(*myArgvs).at(i).c_str());
    }
    (*myArgvc).push_back(NULL);
    return;
}

string mcrshDirectory()
{
    int n = 100;
    char *buf = (char *)malloc(sizeof(char) * n);
    while (1)
    {
        if (n == readlink("/proc/self/exe", buf, n))
        {
            n = n * 2;
            char *buf = (char *)realloc(buf, n);
        }
        else
        {
            string res(buf);
            free(buf);
            res = res.substr(0, res.find_last_of("/"));
            return res;
        }
    }
}

string fullWay(string w, string *currentDir)
{
    string fullway;
    string ptr = w.substr(0, (w.find_first_of("/") == -1) ? w.size() : w.find_first_of("/"));
    if (ptr == "~")
    {
        fullway = _HOMEDIRECTORY_ + w.substr(1, w.size());
        return fullway;
    }

    if (ptr == ".")
    {
        fullway = *currentDir + w.substr(1, w.size());
        return fullway;
    }
    fullway = w;
    return fullway;
}

string wayWithTilda(string fullway, string *currentDir)
{
    string way = _HOMEDIRECTORY_;
    string ptr = fullway.substr(0, (fullway.find_first_of("/") == -1) ? fullway.size() : fullway.find_first_of("/"));
    if (ptr == ".")
        fullway = fullWay(fullway, currentDir);

    if (fullway.size() >= way.size() && fullway.substr(0, way.size()) == way)
    {
        string res("~");
        return res + fullway.substr(way.size(), fullway.size() - way.size());
    }

    if (fullway.size() < way.size() && way.substr(0, fullway.size()) == fullway)
        return fullway;
    return fullway;
}

void mycd(pparsArgv pparsArgv, int i)
{
    DIR *dir = NULL;
    struct stat s = {0};
    if (pparsArgv->argValue.at(i).size() == 1)
        pparsArgv->argValue.at(i).push_back("~");
    string fullway = fullWay(pparsArgv->argValue.at(i).at(1), pparsArgv->currentDir);
    if (!stat((const char *)(fullway.c_str()), &s) && s.st_mode & S_IFDIR)
    {
        *(pparsArgv->currentDir) = fullway;
    }
    else
    {
        cout << "\"" << pparsArgv->argValue.at(i).at(1) << "\" is not directory" << endl;
    }
}

void myls(pparsArgv pparsArgv, int i)
{

    vector<char *> argvc;
    vector<string> argvs;
    argvs.push_back("ls");
    if (pparsArgv->argValue.at(i).at(0) != "ls")
        return;
    if (pparsArgv->argValue.at(i).size() == 1)
        argvs.push_back(*pparsArgv->currentDir);

    for (int k = 1; k < pparsArgv->argValue.at(i).size(); ++k)
        argvs.push_back(fullWay(pparsArgv->argValue.at(i).at(k), pparsArgv->currentDir));
    vecStrToChar(&argvs, &argvc);

    struct stat st;
    vector<string> objects;
    for (int p = 1; p < argvs.size(); ++p)
    {
        if (argvc[p][0] == '-')
            continue;
        if (stat(argvc[p], &st) < 0)
        {
            perror(argvc[p]);
            return;
        }
        if (S_ISDIR(st.st_mode))
        {
            printf("directory '%s':\n", argvc[p]);
            DIR *d = opendir(argvc[p]);
            if (d == NULL)
            {
                perror(argvc[p]);
                return;
            }
            for (dirent *de = readdir(d); de != NULL; de = readdir(d))
            {
                if (string(de->d_name) == ".")
                    continue;
                if (string(de->d_name) == "..")
                    continue;
                if (string(de->d_name)[0] == '.')
                    continue;
                objects.push_back(string(de->d_name));
            }
            sort(objects.begin(), objects.end(), predSortString);

            for (int l = 0; l < objects.size(); ++l)
                cout << "'" << objects.at(l) << "' ";
            cout << endl;
            closedir(d);
        }
    }
    return;
}

void execProg(pparsArgv pparsArgv, int i)
{
    if (pparsArgv->argValue.at(i).at(0) == "cd")
    {
        mycd(pparsArgv, i);
        return;
    }
    if (pparsArgv->argValue.at(i).at(0) == "ls")
    {
        myls(pparsArgv, i);
        return;
    }
    if (pparsArgv->argValue.at(i).at(0) == "pwd")
    {
        cout << "mcrsha:" << (*pparsArgv->currentDir) << endl;
        return;
    }
    vector<char *> myArgvc;
    vecStrToChar(&(pparsArgv->argValue.at(i)), &myArgvc);
    if (pparsArgv->argValue.at(i).at(0)[0] == '/')
    {
        execv((const char *)pparsArgv->argValue.at(i).at(0).c_str(), &myArgvc[0]);
    }
    string way;
    /*if (pparsArgv->argValue.at(i).at(0)[0] != '.' && pparsArgv->argValue.at(i).at(0)[0] != '~')
    {
        way = mcrshDirectory();
        way = way + "/" + pparsArgv->argValue.at(i).at(0);
        execv((const char *)way.c_str(), &myArgvc[0]);
    }*/

    way = fullWay(pparsArgv->argValue.at(i).at(0), pparsArgv->currentDir);
    printf("executable file \"%s\" does not exist (exec retutned %d)\n", myArgvc[0], execv((const char *)way.c_str(), &myArgvc[0]));
}

void execPipeProg(pparsArgv pparsArgv)
{
    int fd[pparsArgv->argValue.size() - 1][2];
    int i = 0;
    pid_t pid;
    for (i = 0; i < pparsArgv->argValue.size() - 1; ++i)
    {
        pipe(fd[i]);
        pid = fork();
        if (pid == -1)
        {
            perror("fork");
            return;
        }
        if (pid == 0)
        {
            close(fd[i][0]);   /* close unused in */
            dup2(fd[i][1], 1); /* stdout to pipe out */
            if (i > 0)
            {
                dup2(fd[i - 1][0], 0);
            }
            execProg(pparsArgv, i);
        }
        if (pid > 0)
        {
            close(fd[i][1]);
        }
    }
    if (i > 0)
        dup2(fd[i - 1][0], 0);
    execProg(pparsArgv, i);
    return;
}

void execChStreamProg(pparsArgv pparsArgv)
{
    if (pparsArgv->out.size() > 0)
    {
        int out = open((char *)(pparsArgv->out.c_str()), O_WRONLY | O_CREAT);
        dup2(out, 1);
    }
    if (pparsArgv->in.size() > 0)
    {
        int in = open((char *)(pparsArgv->in.c_str()), O_RDONLY | O_CREAT);
        dup2(in, 0);
    }
    execProg(pparsArgv, 0);
}

void execArgv(pparsArgv pparsArgv)
{
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork");
        return;
    }
    if (pid > 0)
    {
        waitpid(pid, NULL, 0);
        return;
    }
    if (pparsArgv->argValue.size() > 0)
    {
        if (pparsArgv->out.size() > 0 || pparsArgv->in.size() > 0)
            execChStreamProg(pparsArgv);

        if (pparsArgv->in.size() == 0 && pparsArgv->out.size() == 0)
            execPipeProg(pparsArgv);
    }
}

int main()
{
    printf("Microshell is started, enter the command with \"do\" for execute, To exit enter \"break_mcrsh\"\n");
    string currentDir(_HOMEDIRECTORY_);
    for (;;)
    {
        cout << wayWithTilda(currentDir, &currentDir) << ">";
        struct parsArgv parsArgv;
        parsArgv.currentDir = &currentDir;
        vector<string> myArgvs;
        getArgvs(&myArgvs);
        if (myArgvs.empty())
            continue;
        if (myArgvs.at(0) == "break_mcrsh")
            break;
        parsArgvs(&myArgvs, &parsArgv);
        execArgv(&parsArgv);
    }
    cout << "Microshell is ended" << endl;
    return 0;
}
