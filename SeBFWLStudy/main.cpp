#include "Fib.h"
#include <iostream>

#include <stdio.h>
#include <fstream>
#include <math.h>
#include <conio.h>

#define IP_LEN		32

char * ribFile			=	"rrc04(2013080808).txt.port.onrtc";					//original Fib file
char * updateFile   =	"updates.txt"; 			//update file in IP format

char * oldPortfile    = "oldport-BF.txt";
char * oldPortfile_bin= "oldport_bin.txt";
char * newPortfile    = "newport-BF.txt";
char * newPortfile_bin= "newport_bin.txt";

char * trace_path	=	"trace(100000).integer";	//"rand_trace(100000).integer";//
char * ribfileName	=	"rib.txt.port";

char * BFsFile = "sebfs.txt";
char * BFsBasicFile = "SeBFsBasic.txt";

#define UpdateFileCount		6
#define UPDATE_TIME		"update.stat"

char ret[IP_LEN+1];

//given a ip in binary---str and its length---len, return the next ip in binary
char * GetStringIP(char *str, int len)
{
	memset(ret,0,sizeof(ret));
	memcpy(ret,str,IP_LEN);
	int i;
	for (i=0;i<len;i++)
	{
		if ('0'==ret[i])
		{
			ret[i]='1';
			break;
		}
		else if ('1'==ret[i])
		{
			ret[i]='0';
		}
	}
	//printf("%s*\n",ret);
	return ret;
}

unsigned int btod(char *bstr)
{
	unsigned int d = 0;
	unsigned int len = (unsigned int)strlen(bstr);
	if (len > 32)
	{
		printf("too long\n");
		return -1; 
	}
	len--;
	for (unsigned int i = 0; i <= len; i++)
	{
		d += (bstr[i] - '0') * (1 << (len - i));
	}

	return d;
}

void help () {
	printf ("#######################################\n");
	printf ("##  *-*-*-*-OH algorithm-*-*-*-*-*   ##\n");
	printf ("#   {para} = [trace name] [rib name]  #\n");
	printf ("##       trace_path   ribfileName    ##\n");
	printf ("#######################################\n");
	system ("pause");
}


// Levelpushing Trie Update
unsigned int BFLevelPushingTrieUpdate(string sFileName,CFib *tFib)
{
	unsigned int		iEntryCount = 0;						//the number of items from file
	char						sPrefix[20];								//prefix from rib file
	unsigned long	lPrefix;										//the value of Prefix
	unsigned int		iPrefixLen;								//the length of PREFIX
	int						iNextHop;								//to store NEXTHOP in RIB file

	char			operate_type_read;
	int 			operate_type;
	int			readlines = 0;
	long			updatetimeused = 0;

	long			yearmonthday=0;							//an integer to record year, month, day
	long			hourminsec=0;								//an integer to record hour, minute, second
	long			yearmonthday_old=0;					//an integer to record year, month, day
	long			hourminsec_old=0;						//an integer to record hour, minute, second
	
	long			outputCount=0;
	long			insertNum_old=0;
	long			DelNum_old=0;
	long			readlines_old=0;

	LARGE_INTEGER frequence,privious,privious1;
	
	if(!QueryPerformanceFrequency(&frequence)) return 0;

	FILE * fp_u;
	fp_u=fopen(UPDATE_TIME, "w");
	fprintf(fp_u,"Time	#update	#update_in_minute	#insertion_in_minute	#deletion_in_minute		\n");

	for (int jjj = 1; jjj <= UpdateFileCount; jjj++)
	{
		char strName[20];
		memset(strName, 0, sizeof(strName));
		sprintf(strName, "updates%d.txt", jjj);

		ifstream fin(strName);
		if (!fin)
		{
			//printf("!!!error!!!!  no file named:%s\n",strName);
			continue;
		}

		printf("\nParsing %s\n", strName);

		while (!fin.eof()) 
		{
		
			lPrefix = 0;
			iPrefixLen = 0;
			iNextHop = -9;

			memset(sPrefix,0,sizeof(sPrefix));
			
			//read data from rib file, iNextHop attention !!!
			fin >> yearmonthday >> hourminsec >> operate_type_read >> sPrefix;	//>> iNextHop;

			if('W' == operate_type_read) {
				operate_type = _DELETE;
			}
			else if ('A' == operate_type_read)
			{
				fin >> iNextHop;
				operate_type = _NOT_DELETE;
			}
			else
			{
				printf("Format of update file Error, quit....\n");
				getchar();
				return 0;
			}

			int iStart = 0;								//the end point of IP
			int iEnd = 0;								//the end point of IP
			int iFieldIndex = 3;		
			int iLen = strlen(sPrefix);			//the length of Prefix

		
			if(iLen > 0)
			{
				if (yearmonthday - yearmonthday_old > 0 || hourminsec - hourminsec_old >= 100)
				{	
					yearmonthday_old = yearmonthday;
					hourminsec_old = hourminsec;

					int hour_format = hourminsec/100;
					char hour_string[20];
					memset(hour_string, 0, sizeof(hour_string));
					if (0 == hour_format)					sprintf(hour_string, "0000");
					if (hour_format < 10)					sprintf(hour_string, "000%d", hour_format);
					else if (hour_format < 100)		sprintf(hour_string, "00%d", hour_format);
					else if (hour_format < 1000)		sprintf(hour_string, "0%d", hour_format);
					else												sprintf(hour_string, "%d", hour_format);

					if (readlines - readlines_old < 10000)
					{
						//printf("%d%s\t%u\t%u\t%u\t%d\n",yearmonthday,hour_string,readlines,readlines-readlines_old,tFib->CBFInsertNum-insertNum_old,tFib->CBFDelNum-DelNum_old);
						//fprintf(fp_u,"%d%s\t%u\t%u\t%u\t%d\n",yearmonthday,hour_string,readlines,readlines-readlines_old,tFib->CBFInsertNum-insertNum_old,tFib->CBFDelNum-DelNum_old);
					}

					insertNum_old = tFib->CBFInsertNum;
					DelNum_old = tFib->CBFDelNum;
					readlines_old = readlines;
					//printf("%d%s\t%d\t%u\t%d\t%d\t%d\n",yearmonthday,hour_string,readlines,tFib->trueUpdateNum,tFib->CBFInsertNum,tFib->CBFDelNum,tFib->invalid);

					//fprintf(fp_u,"%d%s\t%d\t%u\t%d\t%d\t%d\n",yearmonthday,hour_string,readlines,tFib->trueUpdateNum,tFib->CBFInsertNum,tFib->CBFDelNum,tFib->invalid);

					//fprintf(fp_u,"%d%d\t%d\t%d\t%d\t%d\n",yearmonthday,hourminsec,readlines,tFib->solidNodeCount,tFib->oldNodeCount,updatetimeused);
				}

				readlines++;
				for ( int i=0; i<iLen; i++ )
				{
					//extract the first 3 sub-part
					if ( sPrefix[i] == '.' )
					{
						iEnd = i;
						string strVal(sPrefix + iStart, iEnd - iStart);
						lPrefix += atol(strVal.c_str()) << (8 * iFieldIndex); //向左移位到高位
						iFieldIndex--;
						iStart = i + 1;
						i++;
					}

					if ( sPrefix[i] == '/' ) {
						//extract the 4th sub-part
						iEnd = i;
						string strVal(sPrefix + iStart, iEnd - iStart);
						lPrefix += atol(strVal.c_str());
						iStart = i + 1;

						//extract the length of prefix
						i++;
						strVal = string(sPrefix + iStart, iLen - 1);
						iPrefixLen = atoi(strVal.c_str());
					}
				}

				char insert_C[50];
				memset(insert_C,0,sizeof(insert_C));
				//insert the current node into Trie tree
				for (unsigned int yi = 0; yi < iPrefixLen; yi++)
				{
					//turn right
					if(((lPrefix << yi) & HIGHTBIT) == HIGHTBIT) insert_C[yi]='1';
					else insert_C[yi]='0';
				}
				//printf("%s\/%d\t%d\n",insert_C,iPrefixLen,iNextHop);

				if (iPrefixLen < 8) {
					printf("%d-%d; ", iPrefixLen, iNextHop);
				}
				else
				{
					QueryPerformanceCounter(&privious); 
					
					tFib->Update(iNextHop, insert_C, operate_type);

					QueryPerformanceCounter(&privious1);
					updatetimeused+=1000000*(privious1.QuadPart-privious.QuadPart)/frequence.QuadPart;
				}
			}
		}
		fin.close();
	}

	printf("trueUpdateNum = %u\tinvalid = %u\t invalid0 = %u\t invalid1 = %u\t invalid2 = %u\n", tFib->trueUpdateNum, tFib->invalid, tFib->invalid0, tFib->invalid1, tFib->invalid2);

	fclose(fp_u);

	FILE *fpp = fopen("CDF.stat","w");
	fprintf(fpp,"CBFInsert\tCBFDelete\t%u\n",readlines);
	for (int mm = 0; mm < 25000; mm++)
	{
		fprintf(fpp,"%u\t%u\n", tFib->CBFInsertArray[mm], tFib->CBFDeleteArray[mm]);
	}
	
	fclose(fpp);

	return readlines;
}

void bfTrieDetectForFullIp(CFib *tFib) {
	int nonRouteStatic=0;

	int hop1=0;
	int hop2=0;

	unsigned int level = 0;

	char strIP00[IP_LEN + 1];
	memset(strIP00, 0, sizeof(strIP00));
	
	for (int tmp=0; tmp < IP_LEN; tmp++)
	{
		strIP00[tmp]='0';
	}

	int len88 = strlen(strIP00);

	char new_tmp[IP_LEN + 1];
	char old_tmp[IP_LEN + 1];

	memset(new_tmp, 0, sizeof(new_tmp));
	memset(new_tmp, 0, sizeof(new_tmp));
	memcpy(new_tmp, strIP00, IP_LEN);

	double zhishuI = pow((double)2,(double)IP_LEN);

	bool ifhalved = false;
	printf("\t\ttotal\t%.0f\t\n", zhishuI);
	printf("\t\tlength\tcycles\t\tpercent\tnexthop\tCollision\n");

	for (long long k=0; k < zhishuI; k++)
	{
		memcpy(old_tmp, new_tmp, IP_LEN);
		memcpy(new_tmp, GetStringIP(old_tmp, IP_LEN), IP_LEN);
	
		unsigned int IPInteger = btod(new_tmp);

		hop1 = tFib->FibTrieLookup(IPInteger, level);
		hop2 = tFib->bfLookup(IPInteger);

		if (hop1== -1 && hop2 != hop1)
		{
			nonRouteStatic++;
			continue;
		}

		double ratio=0;
		double collision_ratio = 0.0;
		
		if (hop2 != hop1)
		{
			printf("%d:%d", hop1, hop2);
			printf("\n\n\t\tNot Equal!!!\n");
			_getch();
		}
		else 
		{
			//if (-1==hop1)nonRouteNum++;

			if (k%100000 == 0)
			{
				ratio=k/(double)(zhishuI/100);
				collision_ratio= ((tFib->WL24_Len*1.0/256+tFib->WL32_Len*1.0))/(double)(zhishuI/100);
				//printf("\r\t\t%d\t%lld\t%.2f%%\t%d             ", IP_LEN, k, ratio, hop1);
				printf("\r\t\t%d\t%lld\t%.2f%%\t%d\t%.6f %%            ", IP_LEN, k, ratio, hop1, collision_ratio);
			}
		}
	}

	printf("\n\t\tTotal number of garbage roaming route：%d",nonRouteStatic);
	//printf("\n\t\tTotal number of Non-Route: %d\n",nonRouteNum);
	printf("\n\t\tEqual!!!!\n");
	//_getch();
}

void initLookup(CFib *tFib, char * filename)
{
	
	unsigned int iEntryCount = tFib->BuildFibFromFile(filename);
	int default_port = PORT_MAX;
	//int default_port = EMPTYHOP;

	if (tFib->m_pTrie->newPort > 0) default_port = tFib->m_pTrie->newPort;
	tFib->BFLevelPushing(tFib->m_pTrie, 0, default_port);
	tFib->initLookupTable();
	tFib->buildLookupTable(tFib->m_pTrie, 0);
}

int EBFLookup(CFib *tFib, unsigned int IP)
{
	return tFib->bfLookup(IP);
}


unsigned int * TrafficRead(char *traffic_file)
{
	unsigned int *traffic=new unsigned int[TRACE_READ];
	int return_value=-1;
	unsigned int traceNum=0;

	//first read the trace...
	ifstream fin(traffic_file);
	if (!fin)return 0;
	fin>>traceNum;

	int TraceLine=0;
	unsigned int IPtmp=0;
	while (!fin.eof() && TraceLine<TRACE_READ )
	{
		fin>>IPtmp;
		traffic[TraceLine]=IPtmp;
		TraceLine++;
	}
	fin.close();
	printf("    trace read complete...\n");

	if (TraceLine<TRACE_READ)
	{
		printf("not enough\n",TraceLine);
	}

	return traffic;
}

void test (int argc, char** argv)
{
	if (argc<2)return;

	CFib tFib = CFib();
	unsigned int *traffi=TrafficRead(argv[1]);
	initLookup(&tFib, argv[2]);
	int LPMPort=-1;

	for (int i=0;i<TRACE_READ;i++)
	{
		LPMPort=EBFLookup(&tFib, traffi[i]);
	}

	printf("LPMPort is %d\n",LPMPort);

}

void main (int argc, char** argv) {

	if (argc < 2) return;

	CFib tFib = CFib();		//build FibTrie
	unsigned int iEntryCount = 0;
	//iEntryCount = tFib.BuildFibFromFile(ribFile);
	iEntryCount = tFib.BuildFibFromFile(argv[1]);
	tFib.ytGetNodeCounts();
	printf("\nThe total number of routing items in FRib file is \t%u, \nThe total number of solid Trie node is :\t%u,\ntFib.allNodeCount=%d\n",iEntryCount,tFib.solidNodeCount,tFib.allNodeCount);
	
	//level-pushing
	int default_port = PORT_MAX;

	if (tFib.m_pTrie->newPort > 0) {
		default_port = tFib.m_pTrie->newPort;
	}

	tFib.BFLevelPushing(tFib.m_pTrie, 0, default_port);
	tFib.ytGetNodeCounts();
	printf("\nAfter BF level pushing:\n\tThe total number of routing items in FRib file is \t%u, \n\tThe total number of solid Trie node is :\t%u,\n\ttFib.allNodeCount=%d\n", iEntryCount, tFib.solidNodeCount, tFib.allNodeCount);


	//tFib.OutputTrie(tFib.m_pTrie, newPortfile, oldPortfile);

	/*printf("To test the port correctness!\n");
	tFib.ytTriePortTest(tFib.m_pTrie);
	printf("End of the test!\n");*/

	BFLevelPushingTrieUpdate(updateFile, &tFib);

	tFib.ytGetNodeCounts();
	printf("\nAfter Update:\t%u solid nodes in the Trie,\nallNodeCount=\t%d\n",tFib.solidNodeCount, tFib.allNodeCount);

	printf("\nTo test the port correctness!\n");
	tFib.ytTriePortTest(tFib.m_pTrie);
	printf("End of the test!\n");

	//tFib.OutputTrie(tFib.m_pTrie, newPortfile, oldPortfile);

	printf("\nBegin to init the lookup tables ......\n");
	tFib.initLookupTable();

	printf("\nBegin to build the lookup tables ......\n");
	tFib.buildLookupTable(tFib.m_pTrie, 0);

	//tFib.OutputBFs(BFsFile);
	//tFib.OutputBFsBasic(BFsBasicFile);

	bfTrieDetectForFullIp(&tFib);

	cout << endl;
	cout << "The total hash collision is:\t" << tFib.collisionNum << endl;
	cout << "The total hash collision in level 24 is:\t" << tFib.WL24_Len << endl;
	cout << endl;
	cout << "The length of Whitelist in level 24 is:\t" << (unsigned int)(tFib.WL24_Len/256) << endl;
	cout << "The length of Whitelist in level 32 is:\t" << tFib.WL32_Len << endl;

	printf("\nMission Complete, Press any key to continue...\n");
	system("pause");
}
