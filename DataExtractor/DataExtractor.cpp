// DataExtractor.cpp: определ€ет точку входа дл€ консольного приложени€.
//

// ConsoleApplication1.cpp: определ€ет точку входа дл€ консольного приложени€.
//


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

#include <map>
#include <string>
#include <iostream>


using namespace std;
using namespace xercesc;


std::multimap <std::string, DOMNode *> dictionary;


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


void start_dictionary ( DOMNode  * Node )
{




	const XMLCh * x_str = Node->getNodeValue();
	if ( x_str)
	{

		dictionary.insert( std::pair <std::string, DOMNode *>( std::string(XMLString::transcode(x_str)),Node));

	}


	DOMNodeList * list = Node->getChildNodes();

	for ( int i =0 ; i < list->getLength(); i ++)
	{
		start_dictionary ( list->item(i));
	}

}


void get_bounding_box ( std::string first_key, std::string second_key,std::multimap <std::string, DOMNode *> dic,int & x1_res,int & x2_res,int & y1_res,int & y2_res )
{
	std::pair <std::multimap <std::string, DOMNode *>::iterator, std::multimap <std::string, DOMNode *>::iterator> ret_first,ret_second;

	ret_first = dic.equal_range (first_key);

	ret_second = dic.equal_range (second_key);


	for (std::multimap <std::string, DOMNode *>::iterator it1 = ret_first.first; it1 != ret_first.second ; it1++)
	{

		for (std::multimap <std::string, DOMNode *>::iterator it2 = ret_second.first; it2 != ret_second.second ; it2++)
		{

			DOMNode* parent1 =  it1->second->getParentNode()->getParentNode();

			DOMNode* parent2 =  it2->second->getParentNode()->getParentNode();


			if ((unsigned int) parent1  == (unsigned int)parent2)
			{


				show_element (it1->second->getParentNode()  );
				show_element ( it2->second->getParentNode());

				int x1,y1,x2,y2;
				int x3,y3,x4,y4;
				int x_center, y_center;

				extract_bb (it1->second->getParentNode() ,x1, y1, x2, y2);

				extract_bb (it2->second->getParentNode() ,x3, y3, x4, y4);

				x_center = (x1+x2+x3+x4)/4;
				y_center = (y1+y2+y3+y4)/4;

				// TODO more complicated logic is needed

				x1_res= x2;

				y1_res = y1;

				x2_res = x3;

				y2_res = y4;



			}





		}
	}

}

#define PROCEED_TESSAPI 1

int main(int argc, char* argv[]) {
	XMLPlatformUtils::Initialize();


	char* text1,*text;

#if !PROCEED_TESSAPI
	text1 = (char * )malloc (2 << 20);
	// just to be quicker we can use previous parsed data that the program have done before
	FILE * fd = fopen ("DWC1Test.txt", "r");


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



		text = tessApi.GetHOCRText(0);



		//char *text = tessApi.GetUTF8Text();//распознаЄм


		//---генерируем им€ файла в который будет записан распознанный текст
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
		parser->setValidationScheme(XercesDOMParser::Val_Always);
		parser->setDoNamespaces(true);    // optional


		ErrorHandler* errHandler = (ErrorHandler*) new HandlerBase();
		parser->setErrorHandler(errHandler);

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

		DOMNodeList * list = doc->getChildNodes();

		for ( int i =0 ; i < list->getLength(); i ++)
		{

			printf ("node name is %s\n", XMLString::transcode (doc->getChildNodes()->item(i)->getNodeName()));

			printf ("node value is %s\n", XMLString::transcode (doc->getChildNodes()->item(i)->getNodeValue()));

			//	show_element ( list->item(i));
			start_dictionary ( list->item(i));


			// delete [] text;
		}

		int x1,y1,x2,y2;

		get_bounding_box ("Nrmt!3re_" , "__j_TodayТs", dictionary, x1,y1,x2,y2 );

		printf (" x1 is %d y1 is %d x2 is %d y2 is %d\n", x1,y1,x2,y2);
	
	return 0;
}

