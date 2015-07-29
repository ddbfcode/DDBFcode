/*
 * Fib.h
 *
 */

#ifndef FIB_H_
#define FIB_H_

//#pragma once

#define		FIBLEN			sizeof(struct FibTrie)		//size of each node in FibTrie
#define		NEXTHOPLEN		sizeof(struct NextHop)		//size of struct Nexthop
#define		MAX_BLINDSPORT	20000						

#define _NOT_DELETE	0
#define _INSERT 1
#define _DELETE	2
#define _CHANGE	3
#define _REIGN	4
#define _OUTOFOFFICE	5
#define _MINI_REDUANDANCY_TWOTRAS	6
#define HIGHTBIT				2147483648				//Binary: 10000000000000000000000000000000

//#define TCAM_COMPUTE	9

//#define USE_REIGN		222
//#define USE_OUTOFOFFICE	333
//#define USE_MINI_SPACE	444

//#include	"Rib.h"
#include	<string>
#include	<fstream>
using namespace std;

//node in FibTrie
struct FibTrie
{
	FibTrie*			parent;					//point to father node
	FibTrie*			lchild;					//point to the left child(0)
	FibTrie*			rchild;					//point to the right child(1)
	int					newPort;					
	int					oldPort;	
	bool				ifblind;				//initial-false, true means blind pot.
	bool				ifpushed;				//if the missing node.
	//int					depth;
};

class CFib
{
public:
	FibTrie* m_pTrie;								//FibTrie

#ifdef TCAM_COMPUTE
	int changeNum;
	int leafnum;
#endif

	int allNodeCount;			//to count all the nodes in Trie tree, including empty node
	int solidNodeCount;			//to count all the solid nodes in Trie tree
	int nonRouteNum;			//the number of nonRoute Space node 
	int oldNodeCount;			//the size of initial routing table
	int EmptyEmCount;			//Couth the situation: left node(empty), right node(empyt)
								//to judge whether Election is needed.
	int BSCount;				//blind spot count.
	int BZdepth[MAX_BLINDSPORT];					//blind zone size.
	float BZAve_depth;
	int   Max_depth;
	int BZSize;

	int levelNumber[33];

	CFib(void);
	~CFib(void);

	//clear the FibTrie
	void CFib::ClearTrie(FibTrie* pTrie);

	// creat a new FIBTRIE ndoe 
	void CreateNewNode(FibTrie* &pTrie);

	// EAR_ORTC algorithm 
	unsigned int CompressTrie(void);

	//get the total number of nodes in RibTrie  
	void ytGetNodeCounts();
	void Pretraversal(FibTrie* pTrie);

	//output the result
	void OutputTrie(FibTrie* pTrie,string sFileName,string oldPortfile);

	//update algorithm
	void Update(int insertport, char *insert_C, int operation_type);

	//reign algorithm
	void reign(FibTrie *insertnode,int defaultold,int newregim);

	//out of office algorithm
	void outofOffice(FibTrie *insertnode,int insertport);

	//one round sub-tree update alorithm
	void SimpleUpdate(FibTrie* pTrie,int defaultoldport);

	bool superDelete(FibTrie *deleteNode);

	void UpdateTreeNodeCount(FibTrie *insertNode);

	bool IsLeaf(FibTrie * pNode);
private:

	// the second procedure to Trie
	void PassTwo(FibTrie* pTrie, int default_NewPort);
	void PassTwo_forupdate(FibTrie* pTrie, int default_NewPort);
	
	bool delegateProc(FibTrie* pTrie,int hopNow);

	//get the nexthop of nearest ancestor
	unsigned int GetAncestorHop(FibTrie* pTrie);

	//get and output all the nexthop in Trie
	void GetTrieHops(FibTrie* pTrie,unsigned int iVal,int iBitLen,ofstream* fout,bool ifnewPort);


public:
	/*
	*PURPOSE: convert file in binary format into IP ones'
	*RETURN VALUES: number of converted items 
	*/
	unsigned int ConvertBinToIP(string sBinFile,string sIpFile);

	/*
	*PURPOSE: convert file in IP format into binary ones'
	*RETURN VALUES: number of converted items 
	*/
	unsigned int ConvertIpToBin(string sIpFile,string sBinFile);

	//PURPOSE: convert IP data into binary data
	void IpToBinary(string sIP,char sBin[32]);

	/*
	*PURPOSE: construct RIB tree from file
	*RETURN VALUES: number of items in rib file
	*/
	unsigned int BuildFibFromFile(string sFileName);

	//add a node in Rib tree
	void AddNode(unsigned long lPrefix,unsigned int iPrefixLen,unsigned int iNextHop);

	bool CFib::superFree(FibTrie* pTrie);
	void CFib::LeafPush(FibTrie* pTrie, int depth);
	void CFib::TestLeafPush(FibTrie* pTrie, int depth);

	int CFib::GetBZdepth(FibTrie* pTrie);
	int CFib::GetBZSize(FibTrie* pTrie);

	void CFib::outputLevelNum(FibTrie* pTrie,unsigned int level);

	void CFib::LevelPushing(FibTrie* pTrie, unsigned int level);
	int num_level[50];
	int stop_level[10];
};


#endif /* FIB_H_ */
