//
//  test.cpp
//  Linker
//
//  Created by Haoran Jia on 2/16/15.
//  Copyright (c) 2015 Haoran Jia. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <cstdio>
#include <set>
#include <cstdlib>

using namespace std;

class Symbol
{
public:
    int moduleNum;
    int moduleBase;
    int rAddr;
    int addr;
    bool multiDef;
    bool used;
    
    Symbol(int _moduleNum, int _moduleBase, int _rAddr)
    {
        moduleNum = _moduleNum;
        moduleBase = _moduleBase;
        rAddr = _rAddr;
        addr = 0;
        multiDef = false;
        used = false;
    }
};

ifstream inFile;
int lineNum, lineOffset, nextLineNum, nextLineOffset, lastLineEnd, instrCount, moduleCount;
map<string, Symbol*> symbolTable;
map<int, int> moduleSize;
vector<string> symbolList;

void PrintParseError(int errCode)
{
    static string errStr[] =
    {
        "NUM_EXPECTED",             // 0. Number Expect
        "SYM_EXPECTED",             // 1. Symbol Expected
        "ADDR_EXPECTED",            // 2. Addressing Expected
        "SYM_TOLONG",               // 3. Symbol Name is Too Long
        "TO_MANY_DEF_IN_MODULE",    // 4. > 16
        "TO_MANY_USE_IN_MODULE",    // 5. > 16
        "TO_MANY_INSTR"             // 6. total num_instr exceeds memory size (512)
    };
    
    printf("Parse Error line %d offset %d: %s\n", lineNum, lineOffset, errStr[errCode].c_str());
    
    exit(0);
}

void ReviseCursor()
{
    if (lineOffset == 1)
    {
        lineNum--;
        lineOffset = lastLineEnd;
    }
    else
        lineOffset--;
}

void Init()
{
    lineNum = 1;
    lineOffset = 1;
    nextLineNum = 1;
    nextLineOffset = 1;
    lastLineEnd = 0;
    instrCount = 0;
    moduleCount = 0;
}

int GetNextString(string &str)
{
    char ch;
    
    str = "";
    lineNum = nextLineNum;
    lineOffset = nextLineOffset;
    
    while (true)
    {
        ch = inFile.get();
        if (inFile.eof()) break;
        
        if (ch == ' ' || ch == '\t')
            lineOffset++;
        else
            if (ch == '\n')
            {
                lineNum++;
                lastLineEnd = lineOffset;
                lineOffset = 1;
            }
            else
            {
                str += ch;
                break;
            }
    }
    
    if (str == "")
        return 0;
    
    nextLineNum = lineNum;
    nextLineOffset = lineOffset + 1;
    
    while (true)
    {
        ch = inFile.get();
        if (inFile.eof())
            break;
        
        if (ch == ' ' || ch == '\t')
        {
            nextLineOffset++;
            return 1;
        }
        else
            if (ch == '\n')
            {
                nextLineNum++;
                lastLineEnd = nextLineOffset;
                nextLineOffset = 1;
                return 2;
            }
            else
            {
                str += ch;
                nextLineOffset++;
            }
    }
    
    return 3;
}

bool IsNumber(char ch)
{
    if (ch >= '0' && ch <= '9')
        return true;
    
    return false;
}

int StrToNum(string str)
{
    int ret = 0;
    
    for (int i = 0; i < str.length(); i++)
        if (!IsNumber(str[i]))
            PrintParseError(0);
        else
            ret = ret * 10 + (str[i] - '0');
    
    return ret;
}

bool IsLetter(char ch)
{
    if (ch >= 'a' && ch <= 'z')
        return true;
    if (ch >= 'A' && ch <= 'Z')
        return true;
    
    return false;
}

bool IsSymbol(string str)
{
    if (!IsLetter(str[0]))
        return false;
    for (int i = 0; i < str.length(); i++)
        if (!IsLetter(str[i]) && !IsNumber(str[i]))
            return false;
    
    return true;
}

string GetSymbol()
{
    string str;
    
    if (GetNextString(str) == 0)
    {
        ReviseCursor();
        PrintParseError(1);
    }
    
    if (!IsSymbol(str))
        PrintParseError(1);
    
    if (str.length() > 16)
        PrintParseError(3);
    
    return str;
}

int GetRAddr()
{
    string str;
    
    if (GetNextString(str) == 0)
    {
        ReviseCursor();
        PrintParseError(0);
    }
    
    return StrToNum(str);
}

bool IsType(string str)
{
    if (str != "I" && str != "A" && str != "R" && str != "E")
        return false;
    
    return true;
}

string GetType()
{
    string str;
    
    if (GetNextString(str) == 0)
    {
        ReviseCursor();
        PrintParseError(2);
    }
    
    if (!IsType(str))
        PrintParseError(2);
    
    return str;
}

bool IsInstr(string str)
{
    for (int i = 0; i < str.length(); i++)
        if (!IsNumber(str[i]))
            return false;
    
    return true;
}

string GetInstr()
{
    string str;
    
    if (GetNextString(str) == 0)
    {
        ReviseCursor();
        PrintParseError(2);
    }
    
    if (!IsInstr(str))
        PrintParseError(2);
    
    return str;
}

int ScanDefList()
{
    string str;
    
    if (GetNextString(str) == 0)
        return 0;
    
    int defCount = StrToNum(str);
    
    if (defCount > 16)
        PrintParseError(4);
    
    moduleCount++;
    
    for (int i = 0; i < defCount; i++)
    {
        string symbol = GetSymbol();
        int rAddr = GetRAddr();
        
        if (symbolTable.count(symbol) == 0)
        {
            symbolTable[symbol] = new Symbol(moduleCount, instrCount, rAddr);
            symbolList.push_back(symbol);
        }
        else
            symbolTable[symbol]->multiDef = true;
    }
    
    return 1;
}

void ScanUseList()
{
    string str;
    
    if (GetNextString(str) == 0)
    {
        ReviseCursor();
        PrintParseError(0);
    }
    
    int useCount = StrToNum(str);
    
    if (useCount > 16)
        PrintParseError(5);
    
    for (int i = 0; i < useCount; i++)
        GetSymbol();
}

void ScanProgText()
{
    string str;
    
    if (GetNextString(str) == 0)
    {
        ReviseCursor();
        PrintParseError(0);
    }
    
    int codeCount = StrToNum(str);
    moduleSize[moduleCount] = codeCount;
    
    instrCount += codeCount;
    
    if (instrCount > 512)
        PrintParseError(6);
    
    for (int i = 0; i < codeCount; i++)
    {
        GetType();
        GetInstr();
    }
}

void Pass1()
{
    while (true)
    {
        if (ScanDefList() == 0)
            break;
        ScanUseList();
        ScanProgText();
    }
}

void Rule5()
{
    for (vector<string>::iterator i = symbolList.begin(); i != symbolList.end(); i++)
    {
        Symbol *pSymbol = symbolTable[*i];
        int moduleNum = pSymbol->moduleNum;
        
        if (pSymbol->rAddr >= moduleSize[moduleNum])
        {
            printf("Warning: Module %d: %s to big %d (max=%d) assume zero relative\n", moduleNum, i->c_str(), pSymbol->rAddr, moduleSize[moduleNum] - 1);
            pSymbol->rAddr = 0;
        }
    }
}

void PrintSymbolTable()
{
    printf("Symbol Table\n");
    
    for (vector<string>::iterator i = symbolList.begin(); i != symbolList.end(); i++)
    {
        Symbol *pSymbol = symbolTable[*i];
        
        pSymbol->addr = pSymbol->moduleBase + pSymbol->rAddr;
        printf("%s=%d", i->c_str(), pSymbol->addr);
        if (pSymbol->multiDef)
            printf(" Error: This variable is multiple times defined; first value used");
        printf("\n");
    }
}

void Pass2()
{
    int defCount, useCount, codeCount, currentBase;
    
    moduleCount = 0;
    instrCount = 0;
    printf("Memory Map\n");
    
    while (inFile >> defCount)
    {
        moduleCount++;
        currentBase = instrCount;
        
        for (int i = 0; i < defCount; i++)
        {
            string s1, s2;
            inFile >> s1 >> s2;
        }
        
        inFile >> useCount;
        
        vector<string> useList;
        useList.clear();
        vector<bool> useListFlag;
        useListFlag.clear();
        
        for (int i = 0; i < useCount; i++)
        {
            string s;
            inFile >> s;
            useList.push_back(s);
            useListFlag.push_back(false);
        }
        
        inFile >> codeCount;
        for (int i = 0; i < codeCount; i++)
        {
            string ss;
            char type;
            string instr;
            int operand;
            
            printf("%03d: ", instrCount);
            inFile >> ss >> instr;
            type = ss[0];
            
            while (instr.length() < 4)
                instr = '0' + instr;
            operand = atoi(instr.substr(1, 3).c_str());
            
            if (instr.length() > 4)
                if (type == 'I')
                    printf("9999 Error: Illegal immediate value; treated as 9999\n");
                else
                    printf("9999 Error: Illegal opcode; treated as 9999\n");
                else
                    if (type == 'I')
                        printf("%s\n", instr.c_str());
                    else
                        if (type == 'A')
                            if (operand >= 512)
                                printf("%c000 Error: Absolute address exceeds machine size; zero used\n", instr[0]);
                            else
                                printf("%s\n", instr.c_str());
                            else
                                if (type == 'R')
                                    if (operand >= codeCount)
                                        printf("%c%03d Error: Relative address exceeds module size; zero used\n", instr[0], currentBase);
                                    else
                                        printf("%c%03d\n", instr[0], operand + currentBase);
                                    else
                                        if (operand >= useCount)
                                            printf("%s Error: External address exceeds length of uselist; treated as immediate\n", instr.c_str());
                                        else
                                        {
                                            string symbol = useList[operand];
                                            useListFlag[operand] = true;
                                            
                                            if (symbolTable.count(symbol) == 0)
                                                printf("%c000 Error: %s is not defined; zero used\n", instr[0], symbol.c_str());
                                            else
                                            {
                                                Symbol *pSymbol = symbolTable[symbol];
                                                pSymbol->used = true;
                                                printf("%c%03d\n", instr[0], pSymbol->addr);
                                            }
                                        }
            instrCount++;
        }
        
        for (int i = 0; i < useCount; i++)
            if (!useListFlag[i])
                printf("Warning: Module %d: %s appeared in the uselist but was not actually used\n", moduleCount, useList[i].c_str());
    }
    
    for (vector<string>::iterator i = symbolList.begin(); i != symbolList.end(); i++)
    {
        Symbol *pSymbol = symbolTable[*i];
        
        if (!pSymbol->used)
            printf("Warning: Module %d: %s was defined but never used\n", pSymbol->moduleNum, i->c_str());
    }
}

int main(int argc, const char * argv[])
{
    inFile.open(argv[1]);
    Init();
    Pass1();
    Rule5();
    PrintSymbolTable();
    inFile.close();
    inFile.open(argv[1]);
    Pass2();
    inFile.close();
    
    return 0;
}

