#pragma once

#include <windows.h>
#include <fstream>
#include <set>

#include "Bloomfilter.h"
#include "hash_function.h"

using namespace std;

#define TRACE_READ 100000
#define _NOT_DELETE	0
#define _INSERT 1
#define _DELETE	2
#define _CHANGE	3

#define FIBLEN				sizeof(struct FibTrie)		//size of each Trie node
#define EMPTYHOP			0							//Trie node doesnt't has a next hop
#define HIGHTBIT			2147483648					//Binary: 10000000000000000000000000000000
/**************************BF**************************/
#define PORT_MAX 	255

// There are 3 levels
#define TABLE1		20
#define TABLE2		24
#define TABLE3		32

#define TABLE1_ELE_NUM	((1<<TABLE1) - 1)

#define SeBF_HF			14
#define DDBF24_HF		17
#define DDBF32_HF		17
/******************************************************/

struct FibTrie
{
	FibTrie*			parent;					//point to father node
	FibTrie*			lchild;					//point to the left child(0)
	FibTrie*			rchild;					//point to the right child(1)
	int					newPort;					
	int					oldPort;
	int					nodeLevel;
	bool				ifpushed;				//if the missing node.
};

class CFib
{
public:
	FibTrie* m_pTrie;				//root node of FibTrie
	
	int allNodeCount;				//to count all the nodes in Trie tree, including empty node
	int solidNodeCount;				//to count all the solid nodes in Trie tree

	/*********************************SentryBF*************************************/
	set<unsigned char> port_set;
	unsigned int port_number;

	unsigned int collisionNum;
	unsigned int WL24_Len;
	unsigned int WL32_Len;

	unsigned int interNode24;
	unsigned int level24_prefix_num[PORT_MAX+1];
	unsigned int level32_prefix_num[PORT_MAX+1];

	unsigned char *level16_table;

	SBF *ebf;
	SBF *level24_bf[PORT_MAX+1];
	SBF *level32_bf[PORT_MAX+1];
	/********************************SentryBF End************************************/


	/********************************CDF stat************************************/
	unsigned int CBFInsertNum;
	unsigned int CBFDelNum;
	unsigned int trueUpdateNum;						// total update time
	unsigned int invalid;
	unsigned int invalid0;
	unsigned int invalid1;
	unsigned int invalid2;

	unsigned int deepest;

	unsigned int CBFInsertArray[25000];
	unsigned int CBFDeleteArray[25000];
	/********************************End************************************/

	CFib(void);
	~CFib(void);

	// creat a new FIBTRIE ndoe 
	void CreateNewNode(FibTrie* &pTrie);
	//get the total number of nodes in FibTrie  
	void ytGetNodeCounts();
	void Pretraversal(FibTrie* pTrie);
	//output the result
	void OutputTrie(FibTrie* pTrie,string sFileName,string oldPortfile);
	void CFib::OutputTrie_32(FibTrie* pTrie);
	bool IsLeaf(FibTrie * pNode);
private:
	//get and output all the nexthop in Trie
	void CFib::GetTrieHops(FibTrie* pTrie,unsigned int iVal,int iBitLen,ofstream* fout,bool ifnewPort);
	void CFib::GetTrieHops_32(FibTrie* pTrie,unsigned int iVal,int iBitLen,ofstream* fout,bool ifnewPort);
public:
	unsigned int BuildFibFromFile(string sFileName);
	void AddNode(unsigned long lPrefix,unsigned int iPrefixLen,unsigned int iNextHop);
	void CFib::LeafPush(FibTrie* pTrie, int depth);

	void BFLevelPushing(FibTrie* pTrie, unsigned int level,unsigned int default_port);
	void CFib::ytLevelPushing(FibTrie* pTrie, unsigned int level,unsigned int default_port);
	void Update(int insertport, char *insert_C, int operation_type);

	int num_level[50];

	void CFib::LevelStatistic(FibTrie* pTrie, unsigned int level);
	unsigned int CFib::btod(char *bstr);

	/*******************************BF Lookup***************************************/
	void buildLookupTable(FibTrie* pTrie, unsigned int iPrefix);
	int FibTrieLookup(unsigned int IPInteger, unsigned int &level);
	int bfLookup(unsigned int IPInteger);
	void DDBFLevelStatistic(FibTrie* pTrie);
	void initLookupTable();
	unsigned int getSize(unsigned int prefix_num, unsigned int hfn);
	void OutputBFs(char *filename);
	void CFib::OutputBFsBasic(char *filename);
	/***********************************End*******************************************/

	/*******************************BFstat*******************************************/
	unsigned int GetAncestorHop(FibTrie* pTrie);
	void subTrieUpdate(FibTrie* root, unsigned int default_port, int operation);
	void subTrieLevelPushing(FibTrie* root, unsigned int default_port);
	void ytTriePortTest(FibTrie* pTrie);
	/********************************End*********************************************/
};