

#include "stdafx.h"
#include <stdio.h>
#include <string.h>

#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>

#include <xercesc/sax2/SAX2XMLReader.hpp>
#include <xercesc/sax2/XMLReaderFactory.hpp>
#include <xercesc/sax2/DefaultHandler.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOMDocument.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <xercesc\framework\MemBufInputSource.hpp>
#include <xercesc\util\regx\RegxParser.hpp>

#include <map>
#include <string>
#include <iostream>
#include <vector>
#include <tuple>


using namespace std;
using namespace xercesc;




struct keyword_candidate
{
	std::string keyword;
	DOMNode * Node;
	double probability;
};

struct markerbox
{	std::string KeyPhrase1;
std::string KeyPhrase2;
int index;
int x1,y1,x2,y2;
};

struct search_entry
{
	std::string KeyPhrase1;
	std::string KeyPhrase2;
	int index;
	int deltaX1,deltaY1,deltaX2,deltaY2;
	int PrevDataXOffset,PrevDataYOffset,NextDataXOffset,NextDataYOffset;
	int NumberOfLines;
	std::vector <markerbox>  boxes;
	bool process_flag;
};

void show_element ( DOMNode *  Node)
{

	printf ("node name is %s\n", XMLString::transcode (Node->getNodeName()));

	const XMLCh * x_str = Node->getNodeValue();
	if ( x_str)
	{
		printf ("node value is %s\n",XMLString::transcode (x_str) );

		if (! strcmp ( XMLString::transcode (Node->getNodeValue()),"Nombre"))
		{
			printf ("Oops\n");
		}
	}

	DOMNamedNodeMap * map = Node->getAttributes();

	if (map)
		for ( int i =0 ; i < map->getLength(); i ++)
		{

			const XMLCh * x_str = map->item(i)->getNodeName();
			if (x_str)

				printf ("Atribute name is %s\n", XMLString::transcode (x_str));



			x_str = map->item(i)->getNodeValue();
			if (x_str)

				printf ("Attribute value is %s\n", XMLString::transcode (x_str));



		}



		DOMNodeList * list = Node->getChildNodes();

		for ( int i =0 ; i < list->getLength(); i ++)
		{
			show_element ( list->item(i));


		}


}

bool extract_bb (DOMNode * Node ,int& x1,int& y1,int& x2, int &y2)
{

	DOMNamedNodeMap * map = Node->getAttributes();

	if (map)
		for ( int i =0 ; i < map->getLength(); i ++)
		{

			const XMLCh * x_str = map->item(i)->getNodeName();
			if (x_str)

				if (!strcmp (  XMLString::transcode (x_str),"title"))
				{
					x_str = map->item(i)->getNodeValue();
					sscanf ( XMLString::transcode (x_str),"%*s %d %d %d %d", &x1,&y1,&x2,&y2);


					return true;

				}

		}
}

void get_key_word_candidates (std::string keyword, std::multimap <std::string, DOMNode *> dictionary ,std::vector <keyword_candidate > & KeyWord_candidates)
{

	std::pair <std::multimap <std::string, DOMNode *>::iterator, std::multimap <std::string, DOMNode *>::iterator> ret_first,ret_second;

	ret_first = dictionary.equal_range ( keyword);

	if ( (ret_first.first != dictionary.begin()) && (ret_first.second != dictionary.begin()) )
	{
		for ( std::multimap <std::string, DOMNode *>::iterator it = ret_first.first; it != ret_first.second; it++)
		{
			keyword_candidate key;

			key.keyword = it->first;
			key.Node = it->second;
			key.probability = 1;
			KeyWord_candidates.push_back(key);
		}
	}
}

double custom_compare (std::string str1, std::string str2)
{
	if ( str1.size() == str2.size ())
	{
		if ( !str1.compare(str2))
		{
			return 1.0;	
		}
		else
		{
			int k = 0;
			for ( int i = 0; i < str1.size(); i++)
			{
				if ( str1.at(i) == str2.at(i) )
					k++;
			}
			if ( ((1.0*k)/str1.size()) > 0.5 )
				return ((1.0*k)/str1.size());
		}
	}
}

void get_key_word_candidates_v2(std::string keyword, std::vector < std::pair <std::string, DOMNode *> > & vec ,std::vector <keyword_candidate > & KeyWord_candidates)
{

	std::string regularexp;

	//regularexp.append(".*");
	regularexp.append(keyword);
	//regularexp.append(".*");

	RegularExpression exp(regularexp.c_str());

	for ( auto it = vec.begin(); it != vec.end(); it++)
	{
/*
		double prob;
		if (prob = custom_compare(it->first,keyword) > 0.5)
		{
			keyword_candidate key;
			key.keyword = it->first;
			key.Node = it->second;
			key.probability = prob;
			KeyWord_candidates.push_back(key);

			continue;
		}*/
		if ( exp.matches(it->first.c_str()))
		{

			keyword_candidate key;
			key.keyword = it->first;
			key.Node = it->second;
			key.probability = 0.8;
			KeyWord_candidates.push_back(key);
			//std::cout << "reg exp";
		}
	}

}

void construct_multimap ( DOMNode  * Node, std::multimap < std::string, DOMNode * > & map )
{
	const XMLCh * x_str = Node->getNodeValue();
	if ( x_str)
	{
		if ((XMLString::transcode(x_str))[0] !='\n' )
		map.insert( std::pair <std::string, DOMNode *>( std::string(XMLString::transcode(x_str)),Node));
	}

	DOMNodeList * list = Node->getChildNodes();

	for ( int i =0 ; i < list->getLength(); i ++)
	{
		construct_multimap ( list->item(i), map);
	}

}

void construct_vector ( DOMNode  * Node, std::vector < std::pair <std::string, DOMNode * > > & vec )
{
	const XMLCh * x_str = Node->getNodeValue();
	if ( x_str)
	{
		if ((XMLString::transcode(x_str))[0] !='\n' )
		vec.push_back( std::pair <std::string, DOMNode *>( std::string(XMLString::transcode(x_str)),Node));
	}

	DOMNodeList * list = Node->getChildNodes();

	for ( int i =0 ; i < list->getLength(); i ++)
	{
		construct_vector ( list->item(i), vec);
	}

}

bool find_keyword_matching_on_line (std::vector <keyword_candidate > keys1,std::vector <keyword_candidate > keys2, keyword_candidate & key1, keyword_candidate & key2)
{
	for ( auto it1 = keys1.begin() ; it1 != keys1.end(); it1++)
	{
		for ( auto it2 = keys2.begin() ; it2 != keys2.end(); it2++)
		{
			DOMNode* parent1 =  it1->Node->getParentNode()->getParentNode();
			DOMNode* parent2 =  it2->Node->getParentNode()->getParentNode();

			if ((unsigned int) parent1  == (unsigned int)parent2)
			{

				//show_element ( it1->Node->getParentNode());
				//show_element ( it2->Node->getParentNode());



				key1 = (*it1);
				key2 = (*it2);

				return true;

			}
		}

	}
	return false;
}

markerbox get_marker_box ( keyword_candidate & key1, keyword_candidate & key2,search_entry & entry)
{
	markerbox box;

	box.KeyPhrase1 = key1.keyword;
	box.KeyPhrase2 = key2.keyword;

	box.index = entry.index;

	int x1,y1,x2,y2;
	int x3,y3,x4,y4;

	extract_bb(key1.Node->getParentNode(),x1,y1,x2,y2);
	extract_bb(key2.Node->getParentNode(),x3,y3,x4,y4);

	box.x1 = x2+entry.deltaX1;
	box.y1 = y1+entry.deltaY1;

	box.x2 = x3+entry.deltaX2;
	box.y2 = y4+entry.deltaY2;

	return box;
}

markerbox get_marker_box_left ( keyword_candidate & key1,search_entry & entry)
{
	markerbox box;

	box.KeyPhrase1 = key1.keyword;
	
	box.index = entry.index;

	int x1,y1,x2,y2;
	int x3,y3,x4,y4;

	extract_bb(key1.Node->getParentNode(),x1,y1,x2,y2);
	extract_bb(key1.Node->getParentNode(),x3,y3,x4,y4);

	box.x1 = x2+entry.deltaX1;
	box.y1 = y1+entry.deltaY1;

	box.x2 = x2+entry.deltaX2+entry.NextDataXOffset;
	box.y2 = y4+entry.deltaY2+entry.NextDataYOffset;

	return box;
}

markerbox get_marker_box_right ( keyword_candidate & key2 ,search_entry & entry)
{
	markerbox box;

	//box.KeyPhrase1 = key1.keyword;
	box.KeyPhrase2 = key2.keyword;

	box.index = entry.index;

	int x1,y1,x2,y2;
	int x3,y3,x4,y4;

	extract_bb(key2.Node->getParentNode(),x1,y1,x2,y2);
	extract_bb(key2.Node->getParentNode(),x3,y3,x4,y4);

	box.x1 = x2+entry.deltaX1+entry.NextDataXOffset;
	box.y1 = y1+entry.deltaY1+entry.NextDataYOffset;

	box.x2 = x3+entry.deltaX2;
	box.y2 = y4+entry.deltaY2;

	return box;
}

bool get_bounding_boxes ( std::vector <search_entry> & vec ,xercesc_3_1::DOMDocument * doc  )
{

	std::multimap <std::string, DOMNode *> dictionary;
	std::vector < std::pair < std::string,  DOMNode *> > vector_nodes;

	DOMNodeList * list = doc->getChildNodes();

	for ( int i =0 ; i < list->getLength(); i ++)
	{

		//printf ("node name is %s\n", XMLString::transcode (doc->getChildNodes()->item(i)->getNodeName()));
		//printf ("node value is %s\n", XMLString::transcode (doc->getChildNodes()->item(i)->getNodeValue()));
		
		construct_multimap ( list->item(i),dictionary);
		construct_vector ( list->item(i),vector_nodes);

	}



	for ( std::vector <search_entry> ::iterator search_entry_it = vec.begin(); search_entry_it != vec.end(); search_entry_it++)
	{

		// for earch boundbox we have to search
		// case if both words is

			std::vector <keyword_candidate > KeyWord1_candidates,KeyWord2_candidates;

			if (! search_entry_it->KeyPhrase1.empty())
			{
			get_key_word_candidates (search_entry_it->KeyPhrase1 ,dictionary ,KeyWord1_candidates);		
			get_key_word_candidates_v2 (search_entry_it->KeyPhrase1 ,vector_nodes ,KeyWord1_candidates);
			
			}
			
			if (! search_entry_it->KeyPhrase2.empty())
			{
			get_key_word_candidates (search_entry_it->KeyPhrase2 ,dictionary ,KeyWord2_candidates);	
			get_key_word_candidates_v2 (search_entry_it->KeyPhrase2 ,vector_nodes ,KeyWord2_candidates);	
			}

			keyword_candidate key1,key2;


			if (find_keyword_matching_on_line (KeyWord1_candidates,KeyWord2_candidates, key1,  key2))
			{
				markerbox box = get_marker_box ( key1, key2,*(search_entry_it));

				search_entry_it->boxes.push_back(box);
				search_entry_it->process_flag = true;
			}
			else
			{
				if (KeyWord1_candidates.empty() && !KeyWord2_candidates.empty())
				{
					
					markerbox box = get_marker_box_right (  KeyWord2_candidates.at(0),*(search_entry_it));
					search_entry_it->boxes.push_back(box);
					if (search_entry_it->KeyPhrase1.size())
						search_entry_it->process_flag = false;
				}


				if (KeyWord2_candidates.empty() && !KeyWord1_candidates.empty())
				{
					
					markerbox box = get_marker_box_left (  KeyWord1_candidates.at(0),*(search_entry_it));
					search_entry_it->boxes.push_back(box);
						if (search_entry_it->KeyPhrase2.size())
						search_entry_it->process_flag = false;
				}
			}
		

			if ( search_entry_it->NumberOfLines > 1 )
			{
		//		show_element ( KeyWord1_candidates.at(0).Node);
	
			}
		}
	return true;
}

void clear_search_entry (search_entry & entry)
{
	entry.KeyPhrase1.clear();
	entry.KeyPhrase2.clear();

	entry.index = -1;
	entry.deltaX1 = -1;
	entry.deltaY1 = -1;
	entry.deltaX2 = -1;

}

bool push_value_search_entry ( std::string str_value,int arg_count, search_entry & entry)
{

	switch (arg_count)
	{
	case 0:
		entry.KeyPhrase1 = str_value;
		break;
	case 1:
		entry.KeyPhrase2 = str_value;
		break;
	case 2:
		sscanf (str_value.c_str(), "%d",&entry.index);
		break;

	case 3:
		sscanf (str_value.c_str(), "%d",&entry.deltaX1);
		break;

	case 4:
		sscanf (str_value.c_str(), "%d",&entry.deltaY1);
		break;

	case 5:
		sscanf (str_value.c_str(), "%d",&entry.deltaX2);
		break;

	case 6:
		sscanf (str_value.c_str(), "%d",&entry.deltaY2);
		break;

	case 7:
		sscanf (str_value.c_str(), "%d",&entry.NextDataXOffset);
		break;

	case 8:
		sscanf (str_value.c_str(), "%d",&entry.NextDataYOffset);
		break;

	case 9:
		sscanf (str_value.c_str(), "%d",&entry.PrevDataXOffset);
		break;

	case 10:
		sscanf (str_value.c_str(), "%d",&entry.PrevDataYOffset);
		break;

	case 11:
		sscanf (str_value.c_str(), "%d",&entry.NumberOfLines);
		break;

	}
	return true;
}

bool custom_parser ( std::string filename, std::vector <search_entry> & vec_entry)
{
	FILE * config_fd = fopen (filename.c_str(), "r");

	void * buffer = malloc (2 << 20);
	size_t sz;

	sz = fread (buffer,1, 2 << 20, config_fd);

	((unsigned char*)buffer) [sz] = 0;


	bool quotes_flag = 0;
	int argument_count = 0;
	search_entry entry;
	std::string temp_str;

	for (  char * pointer = ( char *)buffer;  *pointer != 0; pointer ++)
	{


		if ( *pointer == '\n' )
		{
			push_value_search_entry(temp_str,argument_count,entry);
			temp_str.clear();
			argument_count++;

			vec_entry.push_back (entry);

			quotes_flag = false;
			argument_count = 0;
			clear_search_entry(entry);

			continue;
		}



		if ( *pointer == '"')
		{
			quotes_flag = !quotes_flag;
			continue;
		}

			if ( *pointer == '\t')
		{
			continue;
		}

		if (*pointer < 0)
		{
			continue;
		}

		if (*pointer == ',')
		{
			if (!quotes_flag)
			{
				push_value_search_entry(temp_str,argument_count,entry);
				temp_str.clear();
				argument_count++;
				continue;
			}
			else
			{
				temp_str.push_back(*pointer);
				continue;

			}
		}

		if (*pointer == ' ' && temp_str.empty())
			continue;

		temp_str.push_back(*pointer);

	}
	return true;
}


#define PROCEED_TESSAPI 0
#define USE_WINAPI 0
#define USE_SCANF 0
#define USE_CUSTOM_PARSE 0





int main(int argc, char* argv[]) {

	// Try to parse config file
#if USE_WINAPI
	DWORD rc;
	char * config_str = (char *)malloc ( 2<<20);
	wchar_t * pMsg;

	WCHAR   cfg_IniName[256];         

	GetCurrentDirectory (MAX_PATH, cfg_IniName );    

	wcscat ( cfg_IniName, L"\\DWC.config" );  


	rc = GetPrivateProfileString ( NULL,NULL,NULL,(LPWSTR)config_str, 2<<20,cfg_IniName);

	DWORD  error = GetLastError();

	FormatMessageW(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		error,
		0,
		(LPWSTR)&pMsg,
		0,
		NULL
		);

	std::cout  << pMsg << std::endl;

#endif

#if 0
	FILE * config_fd =  fopen ("DWC.config", "r");
	int count =12 ;
	std::vector < search_entry > vec_entry;


	while (count >= 12)
	{
		char str1 [256];
		char str2 [256];
		search_entry entry;
		count = fscanf ( config_fd, "%[^,] %*[,]  %[^,] %*[,] %u %*[,] %u %*[,] %u %*[,] %u %*[,] %u %*[,] %u %*[,] %u %*[,] %u %*[,] %u %*[,] %u",str1,str2,&(entry.index),
			&(entry.deltaX1),&(entry.deltaY1),&(entry.deltaX2),&(entry.deltaY2),	
			&entry.PrevDataXOffset,&entry.PrevDataYOffset,&entry.NextDataXOffset,&entry.NextDataYOffset,
			&entry.NumberOfLines
			);

		if ( count >=12)
		{
			entry.KeyPhrase1.append(str1);	
			entry.KeyPhrase2.append(str2);

			vec_entry.push_back (entry);
		}
	}

	fclose(config_fd);


#endif

#if 1

	std::vector < search_entry > vec_entry;
	custom_parser ( "DWC.config",  vec_entry);
#endif

	XMLPlatformUtils::Initialize();

	char* text1,*text;

#if !PROCEED_TESSAPI
	text1 = (char * )malloc (2 << 20);
	// just to be quicker we can use previous parsed data that the program have done before
	FILE * fd = fopen ("DWC1Test.hocr", "r");

	memset (text1,0,2 << 20);

	size_t sz = fread (text1,1,(2 << 20),fd);
	// make str null termenating
	text1[sz] = 0;

	fclose (fd);
#endif

#if PROCEED_TESSAPI

	tesseract::TessBaseAPI tessApi;
	tessApi.Init("./", "eng+spa");// loading language data

	if(argc > 1) {
		PIX *pix = pixRead(argv[1]);// reading image with leptonica lin, argv[1] - name of the file to read

		tessApi.SetImage(pix);// 



		text = tessApi.GetHOCRText(0)
			//char *text = tessApi.GetUTF8Text();


			char *fileName = NULL;
		long prefixLength;
		const char* lastDotPosition = strrchr(argv[1], '.');
		if(lastDotPosition != NULL) {
			prefixLength = lastDotPosition - argv[1];
			fileName = new char[prefixLength + 5];
			strncpy(fileName, argv[1], prefixLength);
			strcpy(fileName + prefixLength, ".txt\0");
		} else {
			exit(1);
		}

		FILE *outF = fopen(fileName, "w");
		delete [] fileName;
		fprintf(outF, "%s", text);
		fclose(outF);




		pixDestroy(&pix);

	}
	else
	{
		return 0;
	}
#endif

	XercesDOMParser* parser = new XercesDOMParser();
	//parser->setValidationScheme(XercesDOMParser::Val_Always);
	//parser->setDoNamespaces(true);    // optional

	ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
	parser->setErrorHandler(errHandler);

	parser->setIgnoreCachedDTD(true);
	parser->setLoadExternalDTD(false);
	parser->setLoadSchema(false);
	parser->setLowWaterMark(false);
#if !PROCEED_TESSAPI
	MemBufInputSource myxml_buf((const  XMLByte* const)text1,strlen(text1),
		"myxml (in memory)");
#endif
#if PROCEED_TESSAPI
	MemBufInputSource myxml_buf((const  XMLByte* const)text,strlen(text),
		"myxml (in memory)");
#endif

	parser->parse(myxml_buf);

	xercesc_3_1::DOMDocument * doc = parser->getDocument();

	get_bounding_boxes (vec_entry , doc );

	for ( auto it = vec_entry.begin(); it != vec_entry.end(); it++)
	{
		for ( std::vector <markerbox>::iterator it2 = it->boxes.begin(); it2 != it->boxes.end() ; it2++)
			printf ("Box found with key1 %s, key2 %s, index - %d x1 - %d y1 - %d x2 - %d y2 i- %d\n", (*it).KeyPhrase1.c_str(),(*it).KeyPhrase2.c_str(),(*it2).index, 
			(*it2).x1,(*it2).y1,(*it2).x2,(*it2).y2)  ;	
	}

	return 0;
}

