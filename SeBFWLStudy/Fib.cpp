#include "Fib.h"

CFib::CFib(void)
{
	//initial the root of the Trie tree
	CreateNewNode(m_pTrie);

	allNodeCount=0;			//to count all the nodes in Trie tree, including empty node
	solidNodeCount=0;		//to count all the solid nodes in Trie tree

	memset(num_level, 0, sizeof(num_level));

	/*********************************BF Init*************************************/
	port_set.clear();

	WL24_Len = 0;
	WL32_Len = 0;
	collisionNum = 0;

	port_number = 0;
	interNode24 = 0;

	level16_table=new unsigned char[TABLE1_ELE_NUM];
	memset(level16_table, 0, TABLE1_ELE_NUM);

	memset(level24_prefix_num, 0, sizeof(level24_prefix_num));
	memset(level32_prefix_num, 0, sizeof(level32_prefix_num));

	try {
		ebf = NULL;
		for (int i = 0; i <= PORT_MAX; i++) {
			level24_bf[i] = NULL;
			level32_bf[i] = NULL;
		}
	} catch(bad_alloc &memExp) {
		cerr<< memExp.what() <<endl;
		system("pause");
	}
	/**********************************End**************************************/


	/*****************************CDF stat Init*************************/
	CBFInsertNum=0;
	CBFDelNum=0;
	trueUpdateNum=0;

	invalid = 0;
	invalid0 = 0;
	invalid1 = 0;
	invalid2 = 0;

	memset(CBFInsertArray,0,sizeof(CBFInsertArray));
	memset(CBFDeleteArray,0,sizeof(CBFDeleteArray));

	m_pTrie->oldPort = PORT_MAX;
	m_pTrie->newPort = PORT_MAX;
	/********************************End******************************/
}

CFib::~CFib(void)
{
}

//creat new node 
void CFib::CreateNewNode(FibTrie* &pTrie)
{
	pTrie= (struct FibTrie*)malloc(FIBLEN);

	if (!pTrie)
		return;

	//initial
	pTrie->parent = NULL;
	pTrie->lchild = NULL;
	pTrie->rchild = NULL;
	pTrie->newPort = EMPTYHOP;
	pTrie->oldPort = EMPTYHOP;
	pTrie->nodeLevel = 0;
	pTrie->ifpushed= false;
}



unsigned int CFib::btod(char *bstr)
{
	unsigned int d = 0;
	unsigned int len = strlen(bstr);
	if (len > 32)
	{
		printf("too long\n");
		return -1; 
	}
	len--;

	unsigned int i = 0;
	for (i = 0; i <= len; i++)
	{
		d += (bstr[i] - '0') * (1 << (len - i));
	}

	return d;
}
 
bool CFib::IsLeaf(FibTrie * pNode)
{
	if (pNode->lchild==NULL && pNode->rchild==NULL) return true;
	else return false;	
}


void CFib::Pretraversal(FibTrie* pTrie)
{
	if (NULL==pTrie) return;

	allNodeCount++;
	if (pTrie->newPort != 0) solidNodeCount++;


	Pretraversal(pTrie->lchild);
	Pretraversal(pTrie->rchild);
}
void CFib::ytGetNodeCounts()
{
	allNodeCount = 0;
	solidNodeCount = 0;

	Pretraversal(m_pTrie);
}

void CFib::OutputTrie(FibTrie* pTrie,string sFileName,string oldPortfile)
{
	ofstream fout(sFileName.c_str());
	GetTrieHops(pTrie,0,0,&fout,true);
	fout<<flush;
	fout.close();

	ofstream fout1(oldPortfile.c_str());
	GetTrieHops(pTrie,0,0,&fout1,false);
	fout1<<flush;
	fout1.close();
}

void CFib::OutputTrie_32(FibTrie* pTrie)
{
	ofstream fout("Prefixes_32.txt");
	GetTrieHops_32(pTrie,0,0,&fout,true);
	fout<<flush;
	fout.close();
}



void CFib::GetTrieHops_32(FibTrie* pTrie,unsigned int iVal,int iBitLen,ofstream* fout,bool ifnewPort)
{
	unsigned short portOut=PORT_MAX;

	if (-1!=pTrie->newPort)
	{
		portOut=pTrie->newPort;
	}

	if(portOut!=EMPTYHOP  && 32==iBitLen )
	{
		*fout<<iVal<<"\t"<<portOut<<endl;
	}

	iBitLen++;

	//try to handle the left sub-tree
	if(pTrie->lchild!=NULL)
	{
		GetTrieHops_32(pTrie->lchild,iVal,iBitLen,fout,ifnewPort);
	}
	//try to handle the right sub-tree
	if(pTrie->rchild!=NULL)
	{
		iVal += 1<<(32-iBitLen);
		GetTrieHops_32(pTrie->rchild,iVal,iBitLen,fout,ifnewPort);
	}
}
//get and output all the nexthop in Trie
void CFib::GetTrieHops(FibTrie* pTrie,unsigned int iVal,int iBitLen,ofstream* fout,bool ifnewPort)
{
	
	int portOut=-1;
	if (true==ifnewPort)
		portOut=pTrie->newPort;
	else				
		portOut=pTrie->oldPort;
	

	//1 00000000  00010000   00000000
	if(portOut!=EMPTYHOP)
	{
		char strVal[50];
		memset(strVal,0,sizeof(strVal));
		//printf("%d.%d.%d.%d/%d\t%d\n",(iVal>>24),(iVal<<8)>>24,(iVal<<16)>>24,(iVal<<24)>>24,iBitLen,portOut);

		sprintf(strVal,"%d.%d.%d.%d/%d\t%d\n",(iVal>>24),(iVal<<8)>>24,(iVal<<16)>>24,(iVal<<24)>>24,iBitLen,portOut);
		*fout<<strVal;
	}
	
	iBitLen++;

	//try to handle the left sub-tree
	if(pTrie->lchild!=NULL)
	{
		GetTrieHops(pTrie->lchild,iVal,iBitLen,fout,ifnewPort);
	}
	//try to handle the right sub-tree
	if(pTrie->rchild!=NULL)
	{
		iVal += 1<<(32-iBitLen);
		GetTrieHops(pTrie->rchild,iVal,iBitLen,fout,ifnewPort);
	}
}

//add a node in Rib tree
void CFib::AddNode(unsigned long lPrefix,unsigned int iPrefixLen,unsigned int iNextHop)
{
	if (iNextHop > PORT_MAX)
		return;

	//get the root of rib
	FibTrie* pTrie = m_pTrie;
	//locate every prefix in the rib tree
	for (unsigned int i=0; i<iPrefixLen; i++){
		//turn right
		if(((lPrefix<<i) & HIGHTBIT)==HIGHTBIT){
			//creat new node
			if(pTrie->rchild == NULL){
				FibTrie* pTChild = (struct FibTrie*)malloc(FIBLEN);
				//insert new node
				pTChild->parent = pTrie;
				pTChild->lchild = NULL;
				pTChild->rchild = NULL;
				pTChild->oldPort=0;
				pTChild->newPort=0;

				pTChild->nodeLevel = pTrie->nodeLevel + 1; // the level of the node
				pTChild->ifpushed = false;

				pTrie->rchild = pTChild;
			}
			//change the pointer
			pTrie = pTrie->rchild;
		}
		//turn left
		else{
			//if left node is empty, creat a new node
			if(pTrie->lchild == NULL){
				FibTrie* pTChild = (struct FibTrie*)malloc(FIBLEN);
				//insert new node
				pTChild->parent = pTrie;
				pTChild->lchild = NULL;
				pTChild->rchild = NULL;
				pTChild->oldPort = 0;
				pTChild->newPort = 0;

				pTChild->nodeLevel = pTrie->nodeLevel + 1;
				pTChild->ifpushed = false;

				pTrie->lchild = pTChild;
			}
			//change the pointer
			pTrie = pTrie->lchild;
		}
	}

	pTrie->newPort = iNextHop;
	pTrie->oldPort = iNextHop;
}

	/*
	*PURPOSE: construct RIB tree from file
	*RETURN VALUES: number of items in rib file
	*/
unsigned int CFib::BuildFibFromFile(string sFileName)
{
	unsigned int	iEntryCount=0;		//the number of items from file

	char			sPrefix[100];		//prefix from rib file
	unsigned long	lPrefix;			//the value of Prefix
	unsigned int	iPrefixLen;			//the length of PREFIX
	unsigned int	iNextHop;			//to store NEXTHOP in RIB file

	
	ifstream fin(sFileName.c_str());

	if (!fin) {
		fprintf(stderr, "Open file %s error!\n", sFileName.c_str());
		return 0;
	}

	while (!fin.eof()) {

		
		lPrefix = 0;
		iPrefixLen = 0;
		iNextHop = EMPTYHOP;

		memset(sPrefix,0,sizeof(sPrefix));
		
		fin >> sPrefix>> iNextHop;

		int iStart=0;				//the start point of PREFIX
		int iEnd=0;					//the start point of PREFIX
		int iFieldIndex = 3;		
		int iLen=(int)strlen(sPrefix);	//The length of PREFIX

		if (iLen>20)
		{
			continue;//maybe IPv6 address
		}
		
		if(iLen>0){
			iEntryCount++;
			for ( int i=0; i<iLen; i++ ){
				//get the first three sub-items
				if ( sPrefix[i] == '.' ){
					iEnd = i;
					string strVal(sPrefix+iStart,iEnd-iStart);
					lPrefix += atol(strVal.c_str()) << (8 * iFieldIndex);
					iFieldIndex--;
					iStart = i+1;
					i++;
				}
				if ( sPrefix[i] == '/' ){
					//get the prefix length
					iEnd = i;
					string strVal(sPrefix+iStart,iEnd-iStart);
					lPrefix += atol(strVal.c_str());
					iStart = i+1;

					
					i++;
					strVal= string(sPrefix+iStart,iLen-1);
					iPrefixLen=atoi(strVal.c_str());
				}
			}
		
			AddNode(lPrefix,iPrefixLen,iNextHop);
		}
	}
	fin.close();
	return iEntryCount;
}

unsigned int CFib::GetAncestorHop(FibTrie* pTrie)
{
	unsigned int iHop = EMPTYHOP;
	if(pTrie != NULL){
		pTrie=pTrie->parent;
		if(pTrie!=NULL){
			iHop = pTrie->oldPort;
			if(iHop==EMPTYHOP){
				iHop=GetAncestorHop(pTrie);
			}
		}
	}
	return iHop;
}

void CFib::subTrieUpdate(FibTrie* root, unsigned int default_port, int operation) {
	int level = root->nodeLevel;

	if((level == TABLE1 || level == TABLE2 || level == TABLE3) && IsLeaf(root)) {
		if (operation == _INSERT) {
			CBFInsertNum ++;
		}
		else if (operation == _CHANGE) {
			CBFDelNum ++;
			CBFInsertNum ++;
		}
		else {
			CBFDelNum ++;
		}

		return;
	}

	root->newPort = 0;

	if (root->lchild != NULL && root->lchild->oldPort == 0) {
		root->lchild->newPort = default_port;
		subTrieUpdate(root->lchild, default_port, operation);
	}

	if (root->rchild != NULL &&  root->rchild->oldPort == 0) {
		root->rchild->newPort = default_port;
		subTrieUpdate(root->rchild, default_port, operation);
	}
}

void CFib::subTrieLevelPushing(FibTrie* pTrie, unsigned int default_port) {

	int level = 0;
	if(NULL == pTrie) return;

	level = pTrie->nodeLevel;

	if((level == TABLE1 || level == TABLE2 || level == TABLE3) && IsLeaf(pTrie)) {
		CBFInsertNum ++;
		return;
	}

	if (pTrie->newPort > 0) {
		default_port = pTrie->newPort;
	}

	//left child
	if (NULL== pTrie->lchild)
	{
		FibTrie* pTChild;//  = new FibTrie();//= (struct FibTrie*)malloc(FIBLEN);
		CreateNewNode(pTChild);

		if (NULL==pTChild)
		{
			printf("malloc faild");
		}

		pTChild->parent = pTrie;
		pTChild->lchild = NULL;
		pTChild->rchild = NULL;
		pTChild->oldPort = 0;
		pTChild->newPort = pTrie->newPort;
		pTChild->nodeLevel = pTrie->nodeLevel + 1;
		pTChild->ifpushed = true;
		pTrie->lchild = pTChild;
	}
	else if (0 == pTrie->lchild->newPort) {
		pTrie->lchild->newPort = default_port;
	}

	//right child
	if (NULL == pTrie->rchild)
	{
		FibTrie* pTChild;// = new FibTrie();//(struct FibTrie*)malloc(FIBLEN);

		CreateNewNode(pTChild);

		if (NULL==pTChild)
		{
			printf("malloc faild");
		}

		pTChild->parent = pTrie;
		pTChild->lchild = NULL;
		pTChild->rchild = NULL;
		pTChild->oldPort = 0;
		pTChild->newPort = pTrie->newPort;
		pTChild->nodeLevel = pTrie->nodeLevel + 1;
		pTChild->ifpushed = true;
		pTrie->rchild = pTChild;
	}
	else if (0 == pTrie->rchild->newPort) {
		pTrie->rchild->newPort = default_port;
	}

	pTrie->newPort = 0;

	subTrieLevelPushing(pTrie->lchild, default_port);
	subTrieLevelPushing(pTrie->rchild, default_port);
}

void CFib::Update(int insertport, char *insert_C, int operation_type)
{

	if (insertport > PORT_MAX)
		return;

	int operation = -9; 
		
	FibTrie *insertNode= m_pTrie;

	FibTrie *levelNode16 = NULL;
	FibTrie *levelNode24 = NULL;

	int default_oldport = 0;

	int prefixLen = strlen(insert_C);

	if (prefixLen < 8) {
		return;
	}

	bool IfNewBornNode = false;
	//look up the location of the current node
	for (int i=0;i< prefixLen; i++)	//0.0.0.0/0
	{
		if ('0' == insert_C[i])
		{
			if (NULL== insertNode->lchild)
			{//turn left, if left child is empty, create new node
				if(_DELETE == operation_type)
				{
					invalid++;
					invalid0++;
					return;
				}

				IfNewBornNode = true;
				FibTrie* pNewNode;
				CreateNewNode(pNewNode);
				pNewNode->parent = insertNode;
				pNewNode->nodeLevel = insertNode->nodeLevel + 1;
				insertNode->lchild = pNewNode;
			}

			if (insertNode->oldPort != 0) default_oldport = insertNode->oldPort;
			insertNode = insertNode->lchild;
		}
		else
		{
			//turn right, if left child is empty, create new node
			if (NULL== insertNode->rchild)
			{
				if(_DELETE == operation_type)	
				{
					invalid++;
					invalid0++;
					return;
				}

				IfNewBornNode = true;
				FibTrie* pNewNode;
				CreateNewNode(pNewNode);
				pNewNode->parent = insertNode;
				pNewNode->nodeLevel = insertNode->nodeLevel + 1;
				insertNode->rchild = pNewNode;
			}

			if (insertNode->oldPort !=0 ) default_oldport = insertNode->oldPort;
			insertNode = insertNode->rchild;
		}

		if (insertNode->nodeLevel == TABLE1) {
			levelNode16 = insertNode;
		}

		if (insertNode->nodeLevel == TABLE2) {
			levelNode24 = insertNode;
		}
	}
	
	if(_DELETE != operation_type)		// _NOT_DELETE
	{
		if (0 == insertNode->oldPort) {	
			operation = _INSERT;
		}
		else if (insertNode->oldPort == insertport)
		{
			invalid++;
			invalid1++;	// TODO nothing
			return;
		}
		else {	//修改操作 
			operation = _CHANGE;
		}
	}
	else if (0 == insertNode->oldPort)	{	//Withdraw
		invalid++;
		invalid2++;	// TODO nothing
		return;
	}
	else	{	//W
		operation = _DELETE;
	}
		
	CBFInsertNum = 0;
	CBFDelNum = 0;
	//deepest = 0;

	trueUpdateNum++;

	if (operation == _INSERT) {
		if (IfNewBornNode) {
			if (levelNode16 && levelNode16->newPort != 0) {
				subTrieLevelPushing(levelNode16, levelNode16->newPort);
			}
			else if (levelNode24 && levelNode24->newPort != 0) {
				subTrieLevelPushing(levelNode24, levelNode24->newPort);
			}
		}
		else if (IsLeaf(insertNode) && (insertNode->nodeLevel == TABLE1 || insertNode->nodeLevel == TABLE2 || insertNode->nodeLevel == TABLE3))  {//叶子节点
			insertNode->oldPort = insertport;
			insertNode->newPort = insertport;
			CBFInsertNum++;			
		}
		else if (!IsLeaf(insertNode)) {		
			insertNode->oldPort = insertport;
			insertNode->newPort = insertport;
			subTrieUpdate(insertNode, insertport, operation);
		}
	}
	else if (operation == _CHANGE) {	
		insertNode->oldPort = insertport;
		insertNode->newPort = insertport;
		subTrieUpdate(insertNode, insertport, operation);	
	}
	else if (operation == _DELETE) {	
		insertNode->oldPort = 0;
		insertNode->newPort = default_oldport;
		subTrieUpdate(insertNode, default_oldport, operation);		
	}

	CBFInsertArray[CBFInsertNum]++;
	CBFDeleteArray[CBFDelNum]++;
}

void CFib::ytTriePortTest(FibTrie* pTrie)
{
	int level = pTrie->nodeLevel;

	if((level == TABLE1 || level == TABLE2 || level == TABLE3) && IsLeaf(pTrie) && pTrie->newPort == 0) {
		printf("The leaf node with newport = 0\n");
	}

	if (pTrie->oldPort != 0 && pTrie->newPort != 0 && pTrie->oldPort != pTrie->newPort) {
		printf("Error:\toldport = %d\t newport = %d\n", pTrie->oldPort, pTrie->newPort);
	}

	if (pTrie->nodeLevel != TABLE1 && pTrie->nodeLevel != TABLE2 && pTrie->nodeLevel != TABLE3 && pTrie->newPort != 0) {
		printf("Error:\tThe newport of an Internal node is %d\n", pTrie->newPort);
	}

	if (!IsLeaf(pTrie)) {
		if (pTrie->lchild == NULL || pTrie->rchild == NULL) {
			printf("Error:\t The node has only one child!!!\n");
		}
	}

	if (pTrie->lchild) {
		ytTriePortTest(pTrie->lchild);
	}

	if (pTrie->rchild) {
		ytTriePortTest(pTrie->rchild);
	}
}

void CFib::LeafPush(FibTrie* pTrie, int depth)
{

	if (NULL==pTrie)return;

	if (NULL==pTrie->lchild && NULL==pTrie->rchild)return;

	if (0==pTrie->newPort)
	{
		LeafPush(pTrie->lchild,depth);
		LeafPush(pTrie->rchild,depth);
		return;
	}

	if (NULL!=pTrie->lchild && NULL==pTrie->rchild)
	{
		FibTrie* pTChild = (struct FibTrie*)malloc(FIBLEN);
		pTChild->parent = pTrie;
		pTChild->lchild = NULL;
		pTChild->rchild = NULL;
		pTChild->oldPort= 0;
		pTChild->newPort= pTrie->newPort;
		pTChild->ifpushed= true;

		pTrie->rchild=pTChild;

		if (0==pTrie->lchild->newPort)pTrie->lchild->newPort=pTrie->newPort;

		LeafPush(pTrie->lchild,depth);

		pTrie->newPort=0;
	}

	else if (NULL!=pTrie->rchild && NULL==pTrie->lchild)
	{
		FibTrie* pTChild = (struct FibTrie*)malloc(FIBLEN);
		pTChild->parent = pTrie;
		pTChild->lchild = NULL;
		pTChild->rchild = NULL;
		pTChild->oldPort=0;
		pTChild->newPort=pTrie->newPort;
		pTChild->ifpushed= true;

		pTrie->lchild=pTChild;

		if (0==pTrie->rchild->newPort)pTrie->rchild->newPort=pTrie->newPort;

		LeafPush(pTrie->rchild,depth);

		pTrie->newPort=0;
	}

	else 
	{
		if (0==pTrie->rchild->newPort)pTrie->rchild->newPort=pTrie->newPort;
		if (0==pTrie->lchild->newPort)pTrie->lchild->newPort=pTrie->newPort;

		LeafPush(pTrie->lchild,depth);
		LeafPush(pTrie->rchild,depth);

		pTrie->newPort=0;
	}
}


void CFib::LevelStatistic(FibTrie* pTrie, unsigned int level)
{
	if(NULL == pTrie)return;
	if(pTrie->newPort != 0) num_level[level]++;

	LevelStatistic(pTrie->lchild, level+1);
	LevelStatistic(pTrie->rchild, level+1);
}

void CFib::BFLevelPushing(FibTrie* pTrie, unsigned int level,unsigned int default_port)
{
	if(NULL == pTrie) return;

	if((level == TABLE1 || level == TABLE2 || level == TABLE3) && IsLeaf(pTrie)) return;

	if (pTrie->newPort > 0) {
		default_port = pTrie->newPort;
	}

	//left child
	if (NULL== pTrie->lchild)
	{
		FibTrie* pTChild;//  = new FibTrie();//= (struct FibTrie*)malloc(FIBLEN);
		CreateNewNode(pTChild);

		if (NULL==pTChild)
		{
			printf("malloc faild");
		}
		pTChild->parent = pTrie;
		pTChild->lchild = NULL;
		pTChild->rchild = NULL;
		pTChild->oldPort = 0;
		pTChild->newPort = pTrie->newPort;
		pTChild->nodeLevel = pTrie->nodeLevel + 1;
		pTChild->ifpushed = true;
		pTrie->lchild = pTChild;
	}
	else if (0 == pTrie->lchild->newPort) {
		pTrie->lchild->newPort = default_port;
	}

	//right child
	if (NULL == pTrie->rchild)
	{
		FibTrie* pTChild;// = new FibTrie();//(struct FibTrie*)malloc(FIBLEN);
		CreateNewNode(pTChild);

		if (NULL==pTChild)
		{
			printf("malloc faild");
		}

		pTChild->parent = pTrie;
		pTChild->lchild = NULL;
		pTChild->rchild = NULL;
		pTChild->oldPort = 0;
		pTChild->newPort = pTrie->newPort;
		pTChild->nodeLevel = pTrie->nodeLevel + 1;
		pTChild->ifpushed = true;
		pTrie->rchild = pTChild;
	}
	else if (0 == pTrie->rchild->newPort) {
		pTrie->rchild->newPort = default_port;
	}

	pTrie->newPort = 0;

	BFLevelPushing(pTrie->lchild, level+1,default_port);
	BFLevelPushing(pTrie->rchild, level+1,default_port);
}

void CFib::ytLevelPushing(FibTrie* pTrie, unsigned int level,unsigned int default_port)
{
	if(NULL == pTrie)return;

	if((level == 16 || level == 24 || level == 32) && IsLeaf(pTrie)) return;

	if (pTrie->newPort>0) default_port=pTrie->newPort;

	//left child
	if (NULL==pTrie->lchild)
	{
		FibTrie* pTChild  = new FibTrie();//= (struct FibTrie*)malloc(FIBLEN);
		if (NULL==pTChild)
		{
			printf("malloc faild");
		}
		pTChild->parent = pTrie;
		pTChild->lchild = NULL;
		pTChild->rchild = NULL;
		pTChild->oldPort=0;
		pTChild->newPort = pTrie->newPort;
		pTChild->ifpushed= true;
		pTrie->lchild=pTChild;
	}
	else if (0==pTrie->lchild->newPort)pTrie->lchild->newPort=default_port;

	//right child
	if (NULL==pTrie->rchild)
	{
		FibTrie* pTChild = new FibTrie();//(struct FibTrie*)malloc(FIBLEN);
		if (NULL==pTChild)
		{
			printf("malloc faild");
		}
		pTChild->parent = pTrie;
		pTChild->lchild = NULL;
		pTChild->rchild = NULL;
		pTChild->oldPort=0;
		pTChild->newPort=pTrie->newPort;
		pTChild->ifpushed= true;
		pTrie->rchild=pTChild;
	}
	else if (0==pTrie->rchild->newPort)pTrie->rchild->newPort=default_port;
	
	pTrie->newPort = 0;

	ytLevelPushing(pTrie->lchild, level+1,default_port);
	ytLevelPushing(pTrie->rchild, level+1,default_port);
}

void CFib::DDBFLevelStatistic(FibTrie* pTrie) {
	if(NULL == pTrie) return;
	
	if (pTrie->newPort > 0 && pTrie->newPort <= PORT_MAX) {
		port_set.insert((unsigned char)pTrie->newPort);
		
		if (pTrie->nodeLevel == TABLE2) {
			level24_prefix_num[pTrie->newPort]++;
		}
		else if (pTrie->nodeLevel == TABLE3) {
			level32_prefix_num[pTrie->newPort]++;
		}
	}
	else if (pTrie->newPort > PORT_MAX || pTrie->newPort < 0) {
		cout << "Error port!" << endl;
	}

	if (pTrie->nodeLevel == TABLE2 && !IsLeaf(pTrie)) {
		interNode24++;
	}

	DDBFLevelStatistic(pTrie->lchild);
	DDBFLevelStatistic(pTrie->rchild);
}

unsigned int CFib::getSize(unsigned int prefix_num, unsigned int hfn)
{
	return (unsigned int)(1.0 * hfn * prefix_num/0.7);
}

void CFib::initLookupTable()
{
	DDBFLevelStatistic(m_pTrie);

	port_number = port_set.size();

	cout << "The total number of ports is : " << port_number << endl;
	cout << "The total number of internal nodes in level 24 is : " << interNode24 << endl;

	try {
		ebf = new SBF(getSize(interNode24, SeBF_HF), SeBF_HF);

		for (int i = 1; i <= PORT_MAX; i++) {
			//printf("The %dth:\t%d\t%d\n", i, level24_prefix_num[i], level32_prefix_num[i]);

			if (level24_prefix_num[i] > 0) {
				level24_bf[i] = new SBF(getSize(level24_prefix_num[i], DDBF24_HF), DDBF24_HF);
			}

			if (level32_prefix_num[i] > 0) {
				level32_bf[i] = new SBF(getSize(level32_prefix_num[i], DDBF32_HF), DDBF32_HF);
			}
		}
	} catch(bad_alloc &memExp) {
		cerr<< memExp.what() <<endl;
		system("pause");
	}
}

void CFib::buildLookupTable(FibTrie* pTrie, unsigned int iPrefix) {
	if (pTrie == NULL) return;

	unsigned int sPrefix;
	unsigned int lPrefix = iPrefix << 1;
	unsigned int rPrefix = (iPrefix << 1)+1;
	unsigned char val_str[4];

	int level = pTrie->nodeLevel;

	if (level == TABLE1) {
		level16_table[iPrefix] = pTrie->newPort;
	}
	else if (level == TABLE2) {
		
		sPrefix = iPrefix << (32 - level);
		val_str[3] = (sPrefix & 255);
		val_str[2] = ((sPrefix>>8) & 255);
		val_str[1] = ((sPrefix>>16) & 255);
		val_str[0] = ((sPrefix>>24) & 255);

		if ((pTrie->newPort > 0) && (level24_bf[pTrie->newPort] != NULL)) {
			level24_bf[pTrie->newPort]->insert_k((const unsigned char *)val_str, (level>>3));
		}
		else if ((pTrie->newPort > 0) && !level24_bf[pTrie->newPort]) {
			printf("Error: No such bf!!!\n");
		}

		if (!IsLeaf(pTrie)) {
			ebf->insert_k((const unsigned char *)val_str, (level>>3));
			//ebf->insert_k((const unsigned char *)&sPrefix, (level>>3));
		}
	}
	else if ((level == TABLE3) && (pTrie->newPort > 0)) {
		if (level32_bf[pTrie->newPort] != NULL) {
			
			val_str[3] = (iPrefix & 255);
			val_str[2] = ((iPrefix>>8) & 255);
			val_str[1] = ((iPrefix>>16) & 255);
			val_str[0] = ((iPrefix>>24) & 255);

			level32_bf[pTrie->newPort]->insert_k((const unsigned char *)val_str, (level>>3));
		}
		else printf("Error: No such bf!!!\n");
	}

	buildLookupTable(pTrie->lchild,  lPrefix);
	buildLookupTable(pTrie->rchild,  rPrefix);
}

int CFib::FibTrieLookup(unsigned int IPInteger, unsigned int &level)
{
	int nextHop =  -1; //init the return value
	int i = 0;
	
	FibTrie *insertNode = m_pTrie;

	if (insertNode->oldPort != 0) {
		nextHop = insertNode->oldPort;
	}

	for (i = 0; i < 32;i++)
	{		
		if (((IPInteger << i) & HIGHTBIT) == 0)
		{//if 0, turn left
			if (NULL != insertNode->lchild)	
			{
				insertNode = insertNode->lchild;
			}
			else {
				break;
			}
		}
		else
		{//if 1, turn right
			if (NULL != insertNode->rchild) {
				insertNode = insertNode->rchild;
			}
			else {
				break;
			}
		}

		if (insertNode->newPort != 0)	{
			nextHop = insertNode->newPort;
		}
	}
	
	level = i;

	return	nextHop;
}

int CFib::bfLookup(unsigned int IPInteger) {
	int nexthop = PORT_MAX;
	int find_num = 0;
	unsigned int level = 0;
	unsigned char itos[4];

	itos[3] = (IPInteger & 255);
	itos[2] = ((IPInteger>>8) & 255);
	itos[1] = ((IPInteger>>16) & 255);
	itos[0] = ((IPInteger>>24) & 255);
	
	if (nexthop = level16_table[IPInteger >> (32 - TABLE1)]) {}
	else {	
		for (int i = 1; i <= PORT_MAX; i++) {

			if (nexthop > PORT_MAX) break;

			if (level24_bf[i] && level24_bf[i]->query_k((const unsigned char *)itos, (TABLE2 >> 3))) {
				find_num++;
				if (nexthop) nexthop = i + PORT_MAX + 1;
				else	nexthop = i;
			}
		}

		if ((find_num <= 1) && ebf->query_k((const unsigned char *)itos, (TABLE2 >> 3))) {
			for (int i = 1; i <= PORT_MAX; i++) {

				if (nexthop > PORT_MAX) break;

				if (level32_bf[i] && level32_bf[i]->query_k((const unsigned char *)itos, (TABLE3 >> 3))) {
					
					if (nexthop) {
						if (nexthop != i) {
							find_num++;
							nexthop = i + PORT_MAX + 1;
						}
					}
					else	{
						find_num++;
						nexthop = i;
					}
				}
			}
		}
	}

	if (find_num > 1) {
		collisionNum++;
		nexthop = FibTrieLookup(IPInteger, level);

		if (level == 24) {
			WL24_Len++;
		}
		else if (level == 32) {
			WL32_Len++;
		}
		else if (level != 16) {
			cout << "Error level!" << endl;
		}
	}
	
	if (nexthop == 0) {
		cout << "Error next hop!!!" << endl;
	}

	return nexthop;
}

void CFib::OutputBFs(char *filename) {
	if (!filename) return;

	unsigned int i = 0;
	unsigned int j = 0;
	ofstream fout(filename);

	fout << "16:\t" << endl;

	for (i = 0; i < TABLE1_ELE_NUM; i++) {
		fout << (int)level16_table[i] << "\t";
	}
	fout << endl;

	fout << "24:\t" << endl;
	for (i = 1; i <= PORT_MAX; i++) {
		if (level24_bf[i]) {
			unsigned int elementNum = (level24_bf[i]->bf_m >> 6) + 1;
			fout << i << "\t" << elementNum << endl;
			for (j = 0; j < elementNum; j++) {
				fout << level24_bf[i]->bf_k64[j] << "\t";
			}
			fout << endl;
		}
	}

	fout << "32:\t" << endl;
	for (i = 1; i <= PORT_MAX; i++) {
		if (level32_bf[i]) {
			unsigned int elementNum = (level32_bf[i]->bf_m >> 6) + 1;
			fout << i << "\t" << elementNum << endl;
			for (j = 0; j < elementNum; j++) {
				fout << level32_bf[i]->bf_k64[j] << "\t";
			}
			fout << endl;
		}
	}

	fout << "SeBF:\t" << endl;
	unsigned int ebfEleNum = (ebf->bf_m >> 6) + 1;
	fout << ebfEleNum << endl;
	for (j = 0; j < ebfEleNum; j++) {
		fout << ebf->bf_k64[j] << "\t";
	}
	fout << endl;

	fout<<flush;
	fout.close();
}

void CFib::OutputBFsBasic(char *filename) {
	if (!filename) return;

	unsigned int i = 0;
	ofstream fout(filename);

	for (i = 1; i <= port_number; i++) {
		if (level24_bf[i]) {
			unsigned int elementNum = level24_bf[i]->bf_m;
			fout << 24 << "\t" << i << "\t" << elementNum << endl;
		}
		else
		{
			fout << 24 << "\t" << i << "\t" << 0 << endl;
		}
	}

	if (level24_bf[PORT_MAX]) {
		fout << 24 << "\t" << PORT_MAX << "\t" << level24_bf[PORT_MAX]->bf_m << endl;
	}

	for (i = 1; i <= port_number; i++) {
		if (level32_bf[i]) {
			unsigned int elementNum = level32_bf[i]->bf_m;
			fout << 32 << "\t" << i << "\t" << elementNum << endl;
		}
		else
		{
			fout << 32 << "\t" << i << "\t" << 0 << endl;
		}
	}

	if (level32_bf[PORT_MAX]) {
		fout << 32 << "\t" << PORT_MAX << "\t" << level32_bf[PORT_MAX]->bf_m << endl;
	}

	unsigned int ebfEleNum = ebf->bf_m;
	fout << 25 << "\t" << 0 << "\t" << ebfEleNum << endl;

	fout<<flush;
	fout.close();
}
