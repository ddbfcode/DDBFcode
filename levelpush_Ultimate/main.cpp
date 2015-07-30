#include "Fib.h"
#include <iostream>
#include <fstream>
#include <windows.h>
#include <conio.h>

extern int num_level_16;
extern int num_level_24;
extern int num_level_32;

char * ribFile		  ="rib.txt";				//original Rib file
char * ribFile_IP     ="rib_IP.txt";				//transformed rib file in IP format

char * outputFile     ="[5]SubFast_Result.txt";		//output rib file in IP format
char * outputFile_bin ="[5]SubFast_Result_bin.txt";	//output rib file in binary format

char * updateFile     ="updates.txt"; 			//update file in IP format
char * updateFile_IP  ="updates_IP.txt";			//transformed updates file in IP format

char * oldPortfile    ="[5]oldport.txt";
char * oldPortfile_bin="[5]oldport_bin.txt";

char * result_bat	  ="leafpush_bat.txt";		

#define LINESIZE	75
using namespace std;
bool	bIpFormat=true;								//mark of Rib file,IP format initially e.g."160.120.228/20 5356"。

int stop_level_console[10];
#define DEFAULT_PORT	255

//Threshold_rate can vary from 0.5 to 0.7, also can be a constant value
#define Threshold_rate 1
int     Threshold_Value=0;
#define UpdateFileCount	6

#ifdef USE_REIGN
#define UPDATE_TIME "REIGN_update_time.txt"
#define UPDATE_ALG	_REIGN
#else
#define UPDATE_TIME "[5]mini_redundancy_twotras_time.txt"
#define UPDATE_ALG	_MINI_REDUANDANCY_TWOTRAS
#endif


//get a string whose the number of duplicate element is assigned
string DupString(char cElement,int iLen,bool bNewLine){
	string strRet;
	for (int i=1;i<=iLen;i++){
		if(i==iLen && bNewLine==true){
			strRet += "\n";
		}
		else{
			strRet += cElement;
		}
	}
	return strRet;
}
//to verify if the file exist
bool Exist(char * FileName)
{
	FILE*   fp;
	if((fp=fopen(FileName, "r "))==NULL)
	{
		return false;
	}
	else
	{
		fclose(fp);
		return true;
	}
}

//Interation, get input information from user
bool ShowTipInfo()
{
	
	printf("The Implementation of Blind spot algorithm\n");
	printf("%s",DupString('=',LINESIZE,true).c_str());

	printf("Rib file can be in binary format(B) or IP format(I), Please choose(B/I):");

	//get input from user
	char cResponse=getch();
	printf("%c",cResponse);
	while(cResponse!='B'||cResponse!='b'||cResponse!='I'||cResponse!='i')
	{
		if(cResponse=='B'||cResponse=='b')
		{
			//to verify if the file exist
			if(Exist(ribFile_IP)==false)
			{
				printf("\nRIb file in IP format:\t%s\n",ribFile);
				printf("File Not Exist. \n");
				printf("%s",DupString('=',LINESIZE,true).c_str());
				printf("\t\t    Press any key to exist...");
				getch();
				return false;
			}
			else
			{
				bIpFormat=false;
				break;
			}
		}
		if(cResponse=='I'||cResponse=='i')
		{
			//to verify if the file exist
			if(Exist(ribFile)==false)
			{
				printf("\nRIb file in IP format::\t%s\n",ribFile_IP);
				printf("File in IP format Not Exist.\n");
				printf("%s",DupString('=',LINESIZE,true).c_str());
				printf("\t\t    Press any key to quit...");
				getch();
				return false;
			}
			else
			{
				break;
			}
		}
		printf("\nInput Error! File can in binary format(B) or IP format(I), Please choose(B/I):");
		cResponse=getch();
		printf("%c",cResponse);
	}
	return true;
}

long time_update=0;

void leafpushatfile(char * ribf)
{
	CFib tFib= CFib();		//build FibTrie

	for(int i=0; i<10; i++) {
		tFib.stop_level[i] = stop_level_console[i];
	}

	int origi_count=0;
	int compressed_count=0;
	//Tips
	//if(ShowTipInfo() == true)
	{
		unsigned int iEntryCount = 0;

		//read rib file and build ribtrie
		if(bIpFormat)
		{//read the orignal routing information
			iEntryCount=tFib.BuildFibFromFile(ribf);
		}
		else
		{//read binary routing information
			tFib.ConvertBinToIP(ribf,ribFile_IP);
			iEntryCount=tFib.BuildFibFromFile(ribFile_IP);
		}

		//original Trie node
		tFib.ytGetNodeCounts();
		origi_count=tFib.solidNodeCount;
		printf("\nThe total number of routing items in FRib file is \t%u, \nThe total number of solid Trie node is :\t%u,\ntFib.allNodeCount=%d\n",iEntryCount,tFib.solidNodeCount,tFib.allNodeCount);
	
		if (tFib.m_pTrie->oldPort<=0)
		{
			tFib.m_pTrie->oldPort=DEFAULT_PORT;
			tFib.m_pTrie->newPort=DEFAULT_PORT;
		}
		tFib.LeafPush(tFib.m_pTrie,0);
		//tFib.LeafPush(tFib.m_pTrie->lchild,0);
		//tFib.LeafPush(tFib.m_pTrie->rchild,0);


		//tFib.TestLeafPush(tFib.m_pTrie,0);
		//printf("Test passed!\n");

		tFib.LevelPushing(tFib.m_pTrie,0);
		//tFib.LevelPushing(tFib.m_pTrie->rchild,1);

		tFib.ytGetNodeCounts();
		compressed_count=tFib.solidNodeCount;
		printf("\nAfter Compression:\t%u solid nodes in the Trie,\nallNodeCount=\t%d\n",tFib.solidNodeCount,tFib.allNodeCount);
		printf("memory cost is: \t%d\n",tFib.allNodeCount*FIBLEN);

		FILE *fp=fopen(result_bat, "a");
		//# of original nodes	# of compressed nodes
		fprintf(fp, "%d\t%d\n", origi_count,compressed_count);
		fclose(fp);
	
		//enable the output part
		
		//printf("Output the compression result...\n");

		//char outfile_change[40];
		//memset(outfile_change,0,sizeof(outfile_change));
		//sprintf(outfile_change,"%s.levelpush",ribf);
		//tFib.OutputTrie(tFib.m_pTrie,outfile_change,oldPortfile);
	}

	//output the number of each level
	ofstream out("pushing_result.txt", ios::app);
	int sum = 0;
	for(int i=0; i<10; i++) {
		if(tFib.stop_level[i] != 0) {
			sum += tFib.num_level[tFib.stop_level[i]];
			out << "level\t" << tFib.stop_level[i] << "\t" << tFib.num_level[tFib.stop_level[i]] << "\t";
		}
	}
	out << "sum\t" << sum << endl;
	return;
}


void main(int argc, char * argv[])
{
	memset(stop_level_console, 0, sizeof(stop_level_console));
	//1~10 level push are supported
	if(argc < 2 || argc > 11) {
		cout << "Only 1~10 level push are supported!" << endl;
		return;
	}
	for(int i=1; i<argc; i++) {
		stop_level_console[i-1] = atoi(argv[i]);
	}
     

	FILE *fp=fopen(result_bat, "w");
	//char str_tmp[100];
	fprintf(fp, "orgi\tcompressed\n");
	fclose(fp);

	leafpushatfile(argv[5]);
	//for (int i=0;i<20;i++)
	//{
	//	char ribf[20];
	//	memset(ribf,0,sizeof(ribf));
	//	sprintf(ribf,"rrc%d.txt",i);

	//	if(Exist(ribf)==false)
	//	{
	//		//printf("%s\tdoesn't exist...\n",ribf);
	//		continue;
	//	}

	//	printf("*****************************%s**************************\n",ribf);
	//	leafpushatfile(ribf);
	//}
}
