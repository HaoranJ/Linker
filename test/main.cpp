//
//  main.cpp
//  Lab-1: Linker
//
//  Created by Haoran Jia on 2/4/15.
//  Copyright (c) 2015 Haoran Jia. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <cstdio>
#include <set>
#include <cstdlib>
#include <cmath>

using namespace::std;

ifstream inputFile;
map<int, int> moduleLength;
vector<string> symbolList;
int instrCounter, lineNum, lineOffset, lineNumNext, offsetNext, endOffset, moduleCounter;
string types[] = {"A", "I", "R", "E"};


void PrintParseError(int errCode)
{
    static string errStr[] =
    {
        "NUM_EXPECTED",             // 0. Number Expected
        "SYM_EXPECTED",             // 1. Symbol Expected
        "ADDR_EXPECTED",            // 2. Addressing Expected
        "SYM_TOLONG",               // 3. Symbol Name is Too Long
        "TO_MANY_DEF_IN_MODULE",    // 4. > 16
        "TO_MANY_USE_IN_MODULE",    // 5. > 16
        "TO_MANY_INSTR"             // 6. total num_instr exceeds memory size (512)
    };
    
    printf("Parse Error line %i offset %i: %s\n", lineNum, lineOffset, errStr[errCode].c_str());
    
    exit(0);
}

class Symbol
{
public:
    string symbolName;
    int relativeAddress;
    int baseAddress;
    int absoluteAddress;
    int moduleNum;
    bool reDef;
    bool isUsed;
    
    Symbol(string sName, int rAddress, int _moduleNum, int _baseAddress)
    {
        symbolName = sName;
        relativeAddress = rAddress;
        moduleNum = _moduleNum;
        reDef = false;
        isUsed = false;
        absoluteAddress = _baseAddress + rAddress;
    }
};

map<string, Symbol*> symbolTable;

void setUp(){
    lineNumNext = 1;
    offsetNext = 1;
    lineOffset = 1;
    lineNum = 1;
    endOffset = 0;
    instrCounter = 0;
    moduleCounter = 0;
}

void resetCursor(){
    if (lineOffset != 1) {
        lineOffset--;
    } else {
        lineNum--;
        lineOffset = endOffset;
    }
}

int getNextString(string &str)
{
    str = "";
    char ch;
    lineOffset = offsetNext;
    lineNum = lineNumNext;
    
    while (true) {
        
        ch = inputFile.get();
        if(inputFile.eof()){
            break;
        }
        
        if(ch == '\t' || ch == ' '){
            lineOffset++;
        }
        else if(ch == '\n'){
            lineNum++;
            endOffset = lineOffset;
            lineOffset = 1;
        }
        else {
            str += ch;
            break;
        }
    }
    
    if(str == "") return 0;
    
    lineNumNext = lineNum;
    offsetNext = lineOffset + 1;
    
    while (true) {
        ch = inputFile.get();
        if(inputFile.eof()) break;
        if (ch == ' ' || ch == '\t') {
            offsetNext++;
            return 1;
        }
        else if(ch == '\n'){
            lineNumNext++;
            endOffset = offsetNext;
            offsetNext = 1;
            return 2;
        }
        else{
            str += ch;
            offsetNext++;
        }
    }
    return 7;
}

bool isAlpha(char ch){
    char ch1 = 'a';
    char ch2 = 'A';
    for (int i = 0; i < 26; ++i)
    {
        if(ch == ch1+i || ch == ch2+i){
            return true;
        }
    }
    return false;
}

bool isSymbol(string s){
    int len = s.length();
    if(!isAlpha(s[0])){
        return false;
    }
    else{
        for (int i = 1; i < len; ++i)
        {
            if(!isdigit(s[i]) && !isAlpha(s[i])){
                return false;
            }
        }
        return true;
    }
}

int StrToNum(string str)
{
    int len = str.length();
    int a = 0;
    for (int i = 0; i < len; ++i)
    {
        if (!isdigit(str[i])) {
            PrintParseError(0);
        }
        a += (str[i]-'0')*pow(10,len-i-1);
    }
    return a;
}

string getSymbol()
{
    string str;
    if(getNextString(str) == 0)
    {
        resetCursor();
        PrintParseError(1);
    }
    
    if (!isSymbol(str)) {
        PrintParseError(1);
    }
    
    if(str.length() > 16){
        PrintParseError(3);
    }
    return str;
    
}

int getRelativeAddress()
{
    string str;
    if(getNextString(str) == 0) {
        resetCursor();
        PrintParseError(0);
    }
    return StrToNum(str);
    
}

bool isType(string s){
    for (int i=0; i<4; i++) {
        if (s == types[i]) {
            return true;
        }
    }
    return false;
}

string getType(){
    string s;
    if (getNextString(s) == 0) {
        resetCursor();
        PrintParseError(2);
    }
    
    if (!isType(s)) {
        PrintParseError(2);
    }
    return s;
}

string getInstruction(){
    string str;
    if (getNextString(str) ==0) {
        resetCursor();
        PrintParseError(2);
    }
    
    if (!isdigit(str[0])) {
        PrintParseError(2);
    }
    return str;
}

int scanDefList()
{
    string str;
    int a = getNextString(str);
    if (a == 0) {
        return 0;
    }
    int defCount = StrToNum(str);
    
    if (defCount > 16) {
        PrintParseError(4);
    }
    
    moduleCounter++;
    int N = defCount;
    while(N--) {
        string symbol = getSymbol();
        int relative_address =getRelativeAddress();
        if (symbolTable.find(symbol) == symbolTable.end()) {
            symbolTable[symbol] = new Symbol(symbol, relative_address, moduleCounter, instrCounter);
            symbolList.push_back(symbol);
        }
        else { symbolTable[symbol]->reDef = true; }
    }
    return 1;
    
}

void scanUseList()
{
    string str;
    if(getNextString(str) == 0){
        resetCursor();
        PrintParseError(0);
    }
    int useCount = StrToNum(str);
    
    if (useCount > 16) {
        PrintParseError(5);
    }
    
    for (int i = 0; i<useCount; i++) {
        getSymbol();
    }
}

void scanProgramText()
{
    string str;
    if(getNextString(str) == 0){
        resetCursor();
        PrintParseError(0);
    }
    int codeCount = StrToNum(str);
    moduleLength.insert(pair<int,int>(moduleCounter,codeCount));
    instrCounter += codeCount;
    
    if (instrCounter > 512) {
        PrintParseError(6);
    }
    
    int i = codeCount;
    while (i--) {
        getType();
        getInstruction();
    }
}

void checkRule5(){
    for (vector<string>::iterator it=symbolList.begin() ; it != symbolList.end(); it++) {
        int ra = symbolTable[*it]->relativeAddress;
        int mnum = symbolTable[*it]->moduleNum;
        int mn =  moduleLength[mnum];
        if (ra >= mn) {
            printf("Warning: Module %d: %s to big %d (max=%d) assume zero relative\n",mnum, it->c_str(), ra, mn-1);
            symbolTable[*it]->absoluteAddress = 0;
        }
        
    }
    
}

void printSymbolTable()
{
    cout<<"Symbol Table"<<"\n";
    for (vector<string>::iterator it = symbolList.begin() ; it != symbolList.end(); ++it){
        Symbol *ptr = symbolTable[*it];
        int aa = ptr->absoluteAddress;
        printf("%s=%d", it->c_str(), aa);
        if (ptr->reDef)
            printf(" Error: This variable is multiple times defined; first value used");
        cout<<endl;
    }
}

void passOne(){
    while (true) {
        if(scanDefList()==0) {
            break;
        }
        scanUseList();
        scanProgramText();
    }
}

enum aTypes {A, E, I, R};
map<string, aTypes> table;

void initialize(){
    table["A"] = A;
    table["E"] = E;
    table["I"] = I;
    table["R"] = R;
}


void passTwo()
{
    cout<<"Memory Map"<<"\n";
    int defCount, useCount, codeCount, baseAddress;
    int mapCounter = 0;;
    moduleCounter = 0;
    instrCounter = 0;
    
    
    while (inputFile >> defCount)
    {
        moduleCounter++;
        baseAddress = instrCounter;
        
        //read def list
        while (defCount--) {
            string s1;
            int n1;
            inputFile>>s1>>n1;
        }
        
        //read use list
        inputFile >> useCount;
        map<int, pair<string, bool> > useList;
        useList.clear();
        for (int i = 0; i < useCount; i++)
        {
            string s;
            inputFile >> s;
            useList[i]=pair<string, bool>(s, false);
        }
        
        //read program text
        inputFile >> codeCount;
        int c = codeCount;
        while(c--)
        {
            string type, instr;
            
            printf("%03d: ", mapCounter++);
            inputFile >> type >> instr;
            
            while (instr.length() < 4)
                instr = '0' + instr;
            
            string str;
            for (int j = 1; j<4; j++) {
                str += instr[j];
            }
            int operand = StrToNum(str);
            
            if (instr.length() > 4){
                if (type[0] == 'I')
                    printf("9999 Error: Illegal immediate value; treated as 9999\n");
                else
                    printf("9999 Error: Illegal opcode; treated as 9999\n");
            }else{
                switch (table[type]) {
                    case I:{
                        cout<<instr<<endl;
                        break;
                    }
                    case A:{
                        if (operand >= 512){
                            printf("%c000 Error: Absolute address exceeds machine size; zero used\n", instr[0]);
                        }else{
                            printf("%s\n", instr.c_str());
                        }
                        break;
                    }
                    case R:{
                        if (operand >= codeCount){
                            printf("%c%03d Error: Relative address exceeds module size; zero used\n", instr[0], baseAddress);
                        }else{
                            printf("%c%03d\n", instr[0], operand + baseAddress);
                        }
                        break;
                    }
                    case E:{
                        if (operand >= useCount){
                            printf("%s Error: External address exceeds length of uselist; treated as immediate\n", instr.c_str());
                        }else{
                            ;
                            string symbol = useList[operand].first;
                            useList[operand].second =  true;
                            if (symbolTable.count(symbol) == 0)
                                printf("%c000 Error: %s is not defined; zero used\n", instr[0], symbol.c_str());
                            else
                            {
                                Symbol *pSymbol = symbolTable[symbol];
                                pSymbol->isUsed = true;
                                printf("%c%03d\n", instr[0], pSymbol->absoluteAddress);
                            }
                        }
                        break;
                    }
                }
            }
            instrCounter++;
        }
        
        for (int i = 0; i < useCount; i++){
            if(useList[i].second == false){
                printf("Warning: Module %d: %s appeared in the uselist but was not actually used\n", moduleCounter, useList[i].first.c_str());
            }
        }
    }
    
    for (vector<string>::iterator i = symbolList.begin(); i != symbolList.end(); i++)
    {
        Symbol *pSymbol = symbolTable[*i];
        
        if (!pSymbol->isUsed)
            printf("Warning: Module %d: %s was defined but never used\n", pSymbol->moduleNum, i->c_str());
    }
}

int main(int argc, const char * argv[])
{
    inputFile.open(argv[1]);
    setUp();
    passOne();
    checkRule5();
    printSymbolTable();
    inputFile.close();
    inputFile.open(argv[1]);
    initialize();
    passTwo();
    inputFile.close();
    
    return 0;
}






