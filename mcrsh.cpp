#include <iostream>
#include <unistd.h>
#include "microsha.h"
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <vector>
#include <sys/wait.h>
#include <dirent.h>

using namespace std;

#define _WAYOFTILDA_ "/home/apxapa"

typedef struct parsArgv
{
    vector<vector<string>> argValue;
    string in;
    string out;
    string *currentDir;
} parsArgv, *pparsArgv;

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
            return buf;
        }
    }
}

string fullWay(string w, string *currentDir)
{
    string fullway;
    string ptr = w.substr(0, (w.find_first_of("/") == -1) ? w.size() : w.find_first_of("/"));
    if (ptr == "~")
    {
        fullway = _WAYOFTILDA_;
        fullway = fullway.append(w, 1, w.size() - 1);
        return fullway;
    }

    if (ptr == ".")
    {
        fullway = *currentDir;
        fullway = fullway.append(w, 1, w.size() - 1);
        return fullway;
    }

    fullway = w;
    return fullway;
}

string wayWithTilda(string fullway, string *currentDir)
{
    string way = _WAYOFTILDA_;
    string ptr = fullway.substr(0, (fullway.find_first_of("/") == -1) ? fullway.size() : fullway.find_first_of("/"));
    if (ptr == ".")
        fullway = fullWay(fullway, currentDir);

    if (fullway.size() >= way.size() && fullway.substr(0, way.size()) == way)
    {
        string res("~");
        return res.append(fullway.substr(way.size(), fullway.size() - way.size()));
    }

    if (fullway.size() < way.size() && way.substr(0, fullway.size()) == fullway)
        return fullway;
    return "errror";
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

void execProg(pparsArgv pparsArgv, int i)
{
    if (pparsArgv->argValue.at(i).at(0) == "cd")
    {
        mycd(pparsArgv, i);
        return;
    }
    if (pparsArgv->argValue.at(i).at(0) == "pwd")
    {
        cout << "mcrsha:" << (*pparsArgv->currentDir) << endl;
        return;
    }
    vector<char *> myArgvc;
    string fullway = fullWay(pparsArgv->argValue.at(i).at(0), pparsArgv->currentDir);
    vecStrToChar(&(pparsArgv->argValue.at(i)), &myArgvc);
    printf("executable file \"%s\" does not exist (exec retutned %d)\n", myArgvc[0], execv((const char *)fullway.c_str(), &myArgvc[0]));
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
    printf("Microcha is started, enter the command with \"do\" for execute, To exit enter \"close_mcrsh\"\n");
    string currentDir(_WAYOFTILDA_);
    for (;;)
    {
        cout << wayWithTilda(currentDir, &currentDir) << ">";
        struct parsArgv parsArgv;
        parsArgv.currentDir = &currentDir;
        vector<string> myArgvs;
        getArgvs(&myArgvs);
        parsArgvs(&myArgvs, &parsArgv);
        execArgv(&parsArgv);
    }
    printf("Microcha is ended\n");
    return 0;
}
