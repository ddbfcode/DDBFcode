/*
 * Fib.cpp
 *
 */

#include "Fib.h"
#include <iostream>

#define NONROUTE_PORT -1

#define FIBLEN				sizeof(struct FibTrie)		//size of each Trie node
#define EMPTYHOP			0							//Trie node doesnt't has a next hop
#define HIGHTBIT			2147483648					//Binary: 10000000000000000000000000000000


char * result_compare="result_compare.txt";
char * hop_count="hop_count.txt";
CFib::CFib(void)
{
	//initial the root of the Trie tree
	CreateNewNode(m_pTrie);
	/*
	ZhaoAccuCount=0;
	MyAccuCount=0;
	MyAccC_OUTOF=0;
	*/
#ifdef TCAM_COMPUTE
	changeNum=0;
	leafnum=0;
	memorycount=0;
#endif
	allNodeCount=0;			//to count all the nodes in Trie tree, including empty node
	solidNodeCount=0;		//to count all the solid nodes in Trie tree
	nonRouteNum=0;

	EmptyEmCount=0;

	BSCount=0;
	memset(BZdepth,0,sizeof(BZdepth));
	BZAve_depth=0.0;
	Max_depth=0;
	BZSize=0;
	memset(num_level, 0, sizeof(num_level));
	memset(stop_level, 0, sizeof(stop_level));

	memset(levelNumber,0,sizeof(levelNumber));
}

CFib::~CFib(void)
{
}

//clear the Fib Trie
void CFib::ClearTrie(FibTrie* pTrie)
{

}
//creat new node 
void CFib::CreateNewNode(FibTrie* &pTrie)
{
	
	pTrie= (struct FibTrie*)malloc(FIBLEN);

	//initial
	pTrie->parent = NULL;
	pTrie->lchild = NULL;
	pTrie->rchild = NULL;
	pTrie->newPort = EMPTYHOP;
	pTrie->oldPort = EMPTYHOP;
	pTrie->ifblind = false;
	pTrie->ifpushed= false;
}


void CFib::PassTwo(FibTrie* pTrie, int default_NewPort)
{
	if(pTrie->lchild==NULL && pTrie->rchild==NULL)return;

	if(0!=pTrie->newPort)default_NewPort=pTrie->newPort;

	if(pTrie->lchild!=NULL)PassTwo(pTrie->lchild,default_NewPort);
	if(pTrie->rchild!=NULL)PassTwo(pTrie->rchild,default_NewPort);

	if (NULL==pTrie->rchild)//don't have right child and replace left child with default_newport
	{
		if (0==pTrie->newPort)
		{
			pTrie->newPort=default_NewPort;
		}
		delegateProc(pTrie->lchild,default_NewPort);
	}
	//don't have left child and replace right child with default_newport
	else if (NULL==pTrie->lchild)
	{
		if (0==pTrie->newPort)
		{
			pTrie->newPort=default_NewPort;
		}
		delegateProc(pTrie->rchild,default_NewPort);
	}
	else if (pTrie->lchild->newPort==pTrie->rchild->newPort)//two children of a node have the same newPort
	{
		pTrie->newPort=pTrie->lchild->newPort;
		pTrie->lchild->newPort=0;
		pTrie->rchild->newPort=0;
	}
	//else if (0==pTrie->lchild->newPort && 0==pTrie->rchild->newPort)
	//{
	//	//EmptyEmCount++;
	//	//if (delegateProc(pTrie,default_NewPort)) pTrie->newPort=default_NewPort;
	//	//else
	//		
	//}
	else if (0==pTrie->lchild->newPort)
	{
	//right child is elected, so it is emptyed. Left child are delegated.
		if (delegateProc(pTrie->lchild->lchild,pTrie->rchild->newPort)+delegateProc(pTrie->lchild->rchild,pTrie->rchild->newPort))
		{
			pTrie->newPort=pTrie->rchild->newPort;
			pTrie->rchild->newPort=0;
		}
		else
			pTrie->newPort=0;
	}
	else if (0==pTrie->rchild->newPort)
	{
		if (delegateProc(pTrie->rchild->lchild,pTrie->lchild->newPort)+delegateProc(pTrie->rchild->rchild,pTrie->lchild->newPort))
		{
			pTrie->newPort=pTrie->lchild->newPort;
			pTrie->lchild->newPort=0;
		}
		else
			pTrie->newPort=0;
	}
	else//both children are solid ones, so no result
	{
		pTrie->newPort=0;
	}
}
// second round handling towards Trie
void CFib::PassTwo_forupdate(FibTrie* pTrie, int default_NewPort)
{
	if (NULL==pTrie)return;
	pTrie->newPort=pTrie->oldPort;

	if(0!=pTrie->oldPort)default_NewPort=pTrie->oldPort;

	if(pTrie->lchild!=NULL)PassTwo_forupdate(pTrie->lchild,default_NewPort);
	if(pTrie->rchild!=NULL)PassTwo_forupdate(pTrie->rchild,default_NewPort);


	if (NULL==pTrie->rchild)//if no right child, used default_newport instead of left child
	{
		if (0==pTrie->newPort)
		{
			pTrie->newPort=default_NewPort;
		}
		delegateProc(pTrie->lchild,default_NewPort);
	}
	else if (NULL==pTrie->lchild)//if no left child, used default_newport instead of right child
	{
		if (0==pTrie->newPort)
		{
			pTrie->newPort=default_NewPort;
		}
		delegateProc(pTrie->rchild,default_NewPort);
	}
	else if (pTrie->lchild->newPort==pTrie->rchild->newPort)//two children of a node have the same newPort
	{
		pTrie->newPort=pTrie->lchild->newPort;
		pTrie->lchild->newPort=0;
		pTrie->rchild->newPort=0;
	}
	//else if (0==pTrie->lchild->newPort && 0==pTrie->rchild->newPort)
	//{
	//	//EmptyEmCount++;
	//	//if (delegateProc(pTrie,default_NewPort)) pTrie->newPort=default_NewPort;
	//	//else
	//		
	//}
	/*
	*the left node is empty and the right one is solid, 
	*right child is elected, so it is emptyed. Left child are delegated.
	*/
	else if (0==pTrie->lchild->newPort)
	{
		if (delegateProc(pTrie->lchild->lchild,pTrie->rchild->newPort)+delegateProc(pTrie->lchild->rchild,pTrie->rchild->newPort))
		{
			pTrie->newPort=pTrie->rchild->newPort;
			pTrie->rchild->newPort=0;
		}
		else
			pTrie->newPort=0;
	}
	else if (0==pTrie->rchild->newPort)
	{
		if (delegateProc(pTrie->rchild->lchild,pTrie->lchild->newPort)+delegateProc(pTrie->rchild->rchild,pTrie->lchild->newPort))
		{
			pTrie->newPort=pTrie->lchild->newPort;
			pTrie->lchild->newPort=0;
		}
		else
			pTrie->newPort=0;
	}
	else//both children are solid ones, so no result
	{
		pTrie->newPort=0;
	}
}
bool CFib::delegateProc(FibTrie* pTrie,int hopNow)
{
	if (NULL==pTrie)return 0;

	bool leftnum=0;
	bool rightnum=0;

	if (0==pTrie->newPort)
	{
		//return delegateProc(pTrie->lchild,hopNow)+delegateProc(pTrie->rchild,hopNow);
		leftnum=delegateProc(pTrie->lchild,hopNow);
		rightnum=delegateProc(pTrie->rchild,hopNow);
		return leftnum+rightnum;
	}
	else if (pTrie->newPort==hopNow)
	{
		pTrie->newPort=0;
		return true;
	}
	return false;
}

bool CFib::IsLeaf(FibTrie * pNode)
{
	if (pNode->lchild==NULL && pNode->rchild==NULL)return true;
	else return false;	
}

//get the nexthop of nearest ancestor
unsigned int CFib::GetAncestorHop(FibTrie* pTrie)
{
	unsigned int iHop = EMPTYHOP;
	if(pTrie != NULL){
		pTrie=pTrie->parent;
		if(pTrie!=NULL){
			iHop = pTrie->newPort;
			if(iHop==EMPTYHOP){
				iHop=GetAncestorHop(pTrie);
			}
		}
	}
	return iHop;
}
// EAR_ORTC
unsigned int CFib::CompressTrie(void)
{
	
	if(0==m_pTrie->oldPort)
	{
		m_pTrie->oldPort=NONROUTE_PORT;
		m_pTrie->newPort=NONROUTE_PORT;
	}
	PassTwo(m_pTrie,NONROUTE_PORT);
	//SimpleUpdate(m_pTrie,m_pTrie->oldPort);

	return 0;
}

int CFib::GetBZSize(FibTrie* pTrie)
{
	if (NULL==pTrie)return 0;
	int lsize=GetBZSize(pTrie->lchild);
	int rsize=GetBZSize(pTrie->rchild);

	if (pTrie->newPort!=0)return lsize+rsize+1;
	else 	return lsize+rsize;
}

int CFib::GetBZdepth(FibTrie* pTrie)
{
	if (NULL==pTrie)return 0;
	if (NULL==pTrie->lchild && NULL==pTrie->rchild)return 0;

	int lheight=GetBZdepth(pTrie->lchild);
	int rheight=GetBZdepth(pTrie->rchild);

	return lheight > rheight ? lheight : rheight+1;
}

void CFib::Pretraversal(FibTrie* pTrie)
{
	if (NULL==pTrie)return;

	allNodeCount++;
	if (pTrie->newPort!=0)solidNodeCount++;
	if (-1==pTrie->newPort)nonRouteNum++;
	if (pTrie->oldPort!=0)oldNodeCount++;	

	if (true==pTrie->ifblind)
	{
		BZdepth[BSCount]=GetBZdepth(pTrie);
		BSCount++;
		BZSize+=GetBZSize(pTrie)-1;
	}

	Pretraversal(pTrie->lchild);
	Pretraversal(pTrie->rchild);
}
void CFib::ytGetNodeCounts()
{
	allNodeCount=0;
	solidNodeCount=0;
	nonRouteNum=0;
	oldNodeCount=0;
	BSCount=0;
	BZSize=0;
	
	memset(BZdepth,0,sizeof(BZdepth));
	Pretraversal(m_pTrie);

	float depthSum=0.0;
	for (int i=0;i<BSCount;i++)
	{
		depthSum+=BZdepth[i];
		if (BZdepth[i]>Max_depth)Max_depth=BZdepth[i];
	}
	
	BZAve_depth=depthSum/BSCount;
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

//get and output all the nexthop in Trie
void CFib::GetTrieHops(FibTrie* pTrie,unsigned int iVal,int iBitLen,ofstream* fout,bool ifnewPort)
{
	
	int portOut=-1;
	if (true==ifnewPort)
		portOut=pTrie->newPort;
	else				
		portOut=pTrie->oldPort;
	
	if(portOut!=EMPTYHOP)
	{
		char strVal[50];
		memset(strVal,0,sizeof(strVal));
		sprintf(strVal,"%d.%d.%d.%d/%d\t%d\n",(iVal>>24),(iVal<<8)>>24,(iVal<<16)>>24,(iVal<<24)>>24,iBitLen,portOut);
		*fout<<strVal;
	}

	if (-1==pTrie->newPort)
	{
		nonRouteNum++;
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


//reign algorithm
void CFib::reign(FibTrie *insertnode,int defaultold,int newregim)
{
	if (insertnode==NULL)return;

	//----0/0----，recursive
	if (insertnode->oldPort==0&&insertnode->newPort==0)
	{
		reign(insertnode->lchild,defaultold,newregim);
		reign(insertnode->rchild,defaultold,newregim);
	}
	else if (insertnode->oldPort!=0 && insertnode->newPort==0)
	{
		if (insertnode->oldPort==defaultold)
		{
			insertnode->newPort=insertnode->oldPort;
		}
		else
		{
			insertnode->newPort=insertnode->oldPort;
			//Attention! must be insertnode->iNewPort
			reign(insertnode->lchild,defaultold,insertnode->newPort);
			reign(insertnode->rchild,defaultold,insertnode->newPort);
		}
	}
	//----0/1----
	else if (insertnode->oldPort==0&&insertnode->newPort!=0)
	{
		

		insertnode->newPort=0;
		reign(insertnode->lchild,defaultold,newregim);
		reign(insertnode->rchild,defaultold,newregim);
	}
	else if (insertnode->oldPort!=0&&insertnode->newPort!=0)
	{
		if (insertnode->newPort==newregim)
		{
			insertnode->newPort=0;
			//static int ii=0;
			//printf("\rdelete--%d..",ii++);
		}
	}
}
void CFib::outofOffice(FibTrie *insertnode,int insertport)
{
	if (insertnode==NULL)return;

	//----1/1 && 1/0----return
	if (insertnode->oldPort!=0)return;

	//oldport=0 &&  current node is leaf node, 
	//which probably means: incomplete node or common node
	//so assign insertport to current node
	if (IsLeaf(insertnode))
	{
		//printf("yes");
		insertnode->newPort=insertport;
		return;
	}
	//0/1
	else if (insertnode->newPort!=0 && (NULL==insertnode->lchild || NULL==insertnode->rchild) )
	{
		insertnode->newPort=insertport;
		return;
	}
	
	//0/0 ，and not leaf---recursive
	else 
	{
		outofOffice(insertnode->lchild,insertport);
		outofOffice(insertnode->rchild,insertport);
	}
}

void CFib::Update(int insertport, char *insert_C, int operation_type)
{
	int operation;
	
	FibTrie *insertNode=m_pTrie;
	int default_oldport=0;
	int default_newport=0;

	bool IfNewBornNode=false;
	//look up the location of the current node
	for (int i=0;i<(int)strlen(insert_C);i++)
	{
		if ('0'==insert_C[i])
		{
			if (NULL==insertNode->lchild)
			{//turn left, if left child is empty, create new node
				if(_DELETE==operation_type)	
				{
					return;
				}
				IfNewBornNode=true;
				FibTrie* pNewNode;
				CreateNewNode(pNewNode);
				pNewNode->parent=insertNode;
				insertNode->lchild=pNewNode;

				if (_NOT_DELETE==operation_type)insertNode->ifblind=true;
			}

			insertNode=insertNode->lchild;
		}
		else
		{
		//turn right, if left child is empty, create new node
			if (NULL==insertNode->rchild)
			{
				if(_DELETE==operation_type)	
				{
					
					return;
				}
				IfNewBornNode=true;
				FibTrie* pNewNode;
				CreateNewNode(pNewNode);
				pNewNode->parent=insertNode;
				insertNode->rchild=pNewNode;

				if (_NOT_DELETE==operation_type)insertNode->ifblind=true;
			}

			insertNode=insertNode->rchild;
		}
		if (insertNode->parent->oldPort!=0)
			default_oldport=insertNode->parent->oldPort;
		if (insertNode->newPort!=0)
			default_newport=insertNode->newPort;
	}


	if (_NOT_DELETE==operation_type && true==IfNewBornNode)
	{
		insertNode->oldPort=insertport;
		insertNode->newPort=insertport;
	}
	return;


	if(_DELETE!=operation_type) 
	{
		if (0==insertNode->oldPort)operation=_INSERT;
		else if (insertNode->oldPort==insertport)
		{
			
			return;
		}
		else operation=_CHANGE;
	}
	else if (0==insertNode->oldPort)//Withdraw
	{
		
		return;
	}
	else//W
		operation=_DELETE;

	//insert operation
	if (_INSERT==operation)
	{
		//only one node need to modify
		if (insertport==default_oldport)
		{
			insertNode->oldPort=insertport;
			return;
		}
		
		//insert leaf node
		if (IsLeaf(insertNode))
		{
			insertNode->oldPort=insertport;
			insertNode->newPort=insertport;
			return;
		}

	}

	
	else if (_CHANGE==operation)
	{
		
		if (true==IsLeaf(insertNode))
		{
			insertNode->oldPort=insertport;
			insertNode->newPort=insertport;
			//static int rund=0;
			//printf("\r%d更改叶子...",rund++);
			return;
		}

	}
	
	else if (_DELETE==operation)
	{
		insertNode->oldPort=0;
		//update operation only effect a single node
		if (insertNode->oldPort==default_oldport)return;

		
		if (NULL==insertNode->lchild && NULL==insertNode->rchild)
		{
			insertNode->newPort=default_oldport;
			return;
		}
	}
}


bool CFib::superDelete(FibTrie *deleteNode)
{
	FibTrie *fatherNode=deleteNode->parent;
	if (	0==fatherNode->lchild->newPort && 0==fatherNode->lchild->oldPort
	  && NULL==fatherNode->lchild->lchild && NULL==fatherNode->lchild->rchild
		 && 0==fatherNode->rchild->newPort  && 0==fatherNode->rchild->oldPort
	  && NULL==fatherNode->rchild->lchild && NULL==fatherNode->rchild->rchild
	 )
	{
		free(fatherNode->lchild);
		free(fatherNode->rchild);
		fatherNode->lchild=fatherNode->rchild=NULL;
		superDelete(fatherNode);
		return true;
	}
	return false;
}

void CFib::UpdateTreeNodeCount(FibTrie *insertNode)
{
	if (NULL==insertNode)return;
	else
	{
		//FibUpdateCount++;
		UpdateTreeNodeCount(insertNode->rchild);
		UpdateTreeNodeCount(insertNode->lchild);
	}
}
	/*
	*PURPOSE: convert file in binary format into IP ones'
	*RETURN VALUES: number of converted items 
	*/
unsigned int CFib::ConvertBinToIP(string sBinFile,string sIpFile)
{

	char			sBinPrefix[32];		//PREFIX in binary format
	string			strIpPrefix;		//PREFIX in binary format
	unsigned int	iPrefixLen;			//the length of PREFIX
	unsigned int	iNextHop;			//to store NEXTHOP in RIB file
	unsigned int	iEntryCount=0;		//the number of items that is transformed sucessfully

	ofstream fout(sIpFile.c_str());

	
	ifstream fin(sBinFile.c_str());
	while (!fin.eof()) {
		iNextHop = 0;
		
		memset(sBinPrefix,0,sizeof(sBinPrefix));
		fin >> sBinPrefix>> iNextHop;

		//empty lines are ignored
		if(iNextHop != 0){
			string strBin(sBinPrefix);
			iPrefixLen=strBin.length();
			strBin.append(32-iPrefixLen,'0');

			//transform routing infomation from binary format into IP format
			strIpPrefix="";
			for(int i=0; i<32; i+=8){				//include 4 sub parts
				int iVal=0;
				string strVal=strBin.substr(i,8);

				//turn into integer
				for(int j=7;j>=0;j--){
					if(strVal.substr(j,1)=="1"){
						iVal+=(1<<(7-j));
					}
				}

				//turn into decimal
				char buffer[5];
				memset(buffer,0,sizeof(buffer));
				sprintf(buffer,"%d",iVal);
				//itoa(iVal,buffer,10);
				strVal=string(buffer);


				//IP format
				strIpPrefix += strVal;
				if(i<24)
				{
					strIpPrefix += ".";
				}
				strVal="";
			}
			fout<<strIpPrefix<<"/"<<iPrefixLen<<" "<<iNextHop<<endl;
		}
	}
	
	fin.close();

	fout<<flush;
	fout.close();

	return iEntryCount;
}

	/*
	*PURPOSE: convert file in IP format into binary ones'
	*RETURN VALUES: number of converted items 
	*/
unsigned int CFib::ConvertIpToBin(string sIpFile,string sBinFile)
{
	char			sBinPrefix[32];		//PREFIX in binary format
	string			strIpPrefix;		//PREFIX in binary format
	unsigned int	iPrefixLen;			//the length of PREFIX
	unsigned int	iNextHop;			//to store NEXTHOP in RIB file
	unsigned int	iEntryCount=0;		//the number of items that is transformed sucessfully

	char			sPrefix[20];		//store the prefix from rib file

	//open the file and prepare to store the routing information in binary format
	ofstream fout(sBinFile.c_str());

	//open the RIB file in IP format
	ifstream fin(sIpFile.c_str());
	while (!fin.eof()) {

		
		iPrefixLen = 0;
		iNextHop = EMPTYHOP;

		memset(sPrefix,0,sizeof(sPrefix));
	
		fin >> sPrefix>> iNextHop;

		int iLen=strlen(sPrefix);

		if(iLen>0)
		{
			iEntryCount++;
			for ( int i=0; i<iLen; i++ )
			{
				if ( sPrefix[i] == '/' )
				{
					//extract the forth sub-part
					string strVal(sPrefix,i);
					strIpPrefix=strVal;

					//extract the length of prefix
					strVal= string(sPrefix+i+1,iLen-1);
					iPrefixLen=atoi(strVal.c_str());
					break;
				}
			}

			memset(sBinPrefix,0,sizeof(sBinPrefix));
			//convert IP data into binary data
			IpToBinary(strIpPrefix,sBinPrefix);
			
			//to handle the root whose the length of prefix is 0
			if(iPrefixLen>0)
			{
				strIpPrefix=string(sBinPrefix,iPrefixLen);
			}
			else
			{
				strIpPrefix="*";
			}
			
			fout<<strIpPrefix<<"\t"<<iNextHop<<endl;
		}
	}

	
	fin.close();

	
	fout<<flush;
	fout.close();

	return iEntryCount;
}

//PURPOSE: convert IP data into binary data
void CFib::IpToBinary(string sIP,char saBin[32]){
	int iStart=0;				//the start point of IP
	int iEnd=0;					//the end point of IP
	int iFieldIndex = 3;		
	int iLen=sIP.length();		//the Length of IP
	unsigned long	lPrefix=0;	//IP in integer format

	
	//turn IP into an integer
	for ( int i=0; i<iLen; i++ ){
		//extract the first three sub-parts
		if ( sIP.substr(i,1)== "." ){
			iEnd = i;
			string strVal=sIP.substr(iStart,iEnd-iStart);
			lPrefix += atol(strVal.c_str()) << (8 * iFieldIndex);
			iFieldIndex--;
			iStart = i+1;
			i++;
		}
		if ( iFieldIndex == 0 ){
			
			//extract the forth sub-part
			iEnd = iLen;
			string strVal=sIP.substr(iStart,iEnd-iStart);
			lPrefix += atol(strVal.c_str());
			iStart = i+1;
		}
	}

	//turn into binary format stored in a array
	unsigned long	lVal=0x80000000;
	for(int i=0;i<32;i++){
		if(lPrefix&lVal){
			saBin[i]='1';
		}
		else{
			saBin[i]='0';
		}
		lVal=lVal>>1;
	}
}

//add a node in Rib tree
void CFib::AddNode(unsigned long lPrefix,unsigned int iPrefixLen,unsigned int iNextHop)
{
	
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
				pTChild->ifblind=false;
				pTChild->ifpushed=false;

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
				pTChild->oldPort=0;
				pTChild->newPort=0;
				pTChild->ifblind=false;
				pTChild->ifpushed=false;

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

	char			sPrefix[20];		//prefix from rib file
	unsigned long	lPrefix;			//the value of Prefix
	unsigned int	iPrefixLen;			//the length of PREFIX
	unsigned int	iNextHop;			//to store NEXTHOP in RIB file

	
	ifstream fin(sFileName.c_str());
	while (!fin.eof()) {

		
		lPrefix = 0;
		iPrefixLen = 0;
		iNextHop = EMPTYHOP;

		memset(sPrefix,0,sizeof(sPrefix));
		
		fin >> sPrefix>> iNextHop;

		int iStart=0;				//the start point of PREFIX
		int iEnd=0;					//the start point of PREFIX
		int iFieldIndex = 3;		
		int iLen=strlen(sPrefix);	//The length of PREFIX

		
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

void CFib::SimpleUpdate(FibTrie* pTrie,int defaultoldport)
{
	if (pTrie==NULL)return;
	int nowPort=defaultoldport;
	if (pTrie->oldPort!=0)defaultoldport=pTrie->oldPort;

	SimpleUpdate(pTrie->lchild,defaultoldport);
	SimpleUpdate(pTrie->rchild,defaultoldport);

	pTrie->newPort=pTrie->oldPort;

	if (nowPort==pTrie->newPort)pTrie->newPort=0;

	if (pTrie->lchild!=NULL && pTrie->rchild!=NULL && pTrie->lchild->newPort!=0 && pTrie->lchild->newPort==pTrie->rchild->newPort)
	{
		pTrie->newPort=pTrie->lchild->newPort;
		pTrie->lchild->newPort=0;
		pTrie->rchild->newPort=0;
	}
}


bool CFib::superFree(FibTrie* pTrie)
{
	if (NULL==pTrie->lchild && NULL==pTrie->rchild)
	{
		free(pTrie);
		return true;
	}

	return false;
}

void CFib::TestLeafPush(FibTrie* pTrie, int depth)
{
	if (IsLeaf(pTrie))
	{
		if (pTrie->newPort<=0)
		{
			printf("wrong!!!!\n");
			return;
		}
	}
	else
	{
		if (NULL==pTrie->lchild || NULL==pTrie->rchild || pTrie->newPort>0)
		{
			printf("wrong2222!!!!\n");
			return;
		}

		TestLeafPush(pTrie->lchild,depth+1);
		TestLeafPush(pTrie->rchild,depth+1);
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
		pTChild->ifblind= false;
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
		pTChild->ifblind=false;
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

void CFib::outputLevelNum(FibTrie* pTrie,unsigned int level)
{

	if (pTrie->newPort>0)levelNumber[level]++;

	outputLevelNum(pTrie->lchild,level+1);
	outputLevelNum(pTrie->rchild,level+1);
}

void CFib::LevelPushing(FibTrie* pTrie, unsigned int level)
{
     if(NULL == pTrie) {
          return;
     }
     else {
          if(pTrie->newPort == 0) {
             LevelPushing(pTrie->lchild, level+1);
             LevelPushing(pTrie->rchild, level+1);
          }
          else {
               //after leaf pushing, all the concrete nodes are leaf nodes,
               //therefore we should create new nodes.
			  bool at_stop_level = false;
			  for(int i=0; i<10; i++) {
				  if(stop_level[i] != 0 && level == stop_level[i]) {
					  at_stop_level = true;
					  break;
				  }
			  }

			  //not at stop level, do push
			  if(!at_stop_level) {
                          //left child
                          FibTrie* pTChild = (struct FibTrie*)malloc(FIBLEN);
		                  pTChild->parent = pTrie;
		                  pTChild->lchild = NULL;
                    	  pTChild->rchild = NULL;
                      	  pTChild->oldPort=0;
                          pTChild->newPort=pTrie->newPort;
                  		  pTChild->ifblind=false;
                     	  pTChild->ifpushed= true;

                          pTrie->lchild=pTChild;
                          //right child
                          pTChild = (struct FibTrie*)malloc(FIBLEN);
		                  pTChild->parent = pTrie;
		                  pTChild->lchild = NULL;
                    	  pTChild->rchild = NULL;
                      	  pTChild->oldPort=0;
                          pTChild->newPort=pTrie->newPort;
                  		  pTChild->ifblind=false;
                     	  pTChild->ifpushed= true;

                          pTrie->rchild=pTChild;
                          //disable the current node
                          pTrie->newPort = 0;
                          
                          LevelPushing(pTrie->lchild, level+1);
                          LevelPushing(pTrie->rchild, level+1);
               }
               else {
					num_level[level]++;
					return;
               }
          }
     }
}
