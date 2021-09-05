#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/types.h>
#include	<string.h>
#include	<errno.h>
#include	<curl/curl.h>
#include	<libxml/xmlerror.h>
#include	<libxml/parser.h>
#include	<libxml/tree.h>
#include	<libxml/xpath.h>

#define	MAX_BUF		65536

#define	RSS_FEED	"https://news.yahoo.co.jp/rss/topics/top-picks.xml"
#define	XPATH_ITEM	"/rss/channel/item"

char	wr_buf[MAX_BUF+1];
int	wr_index;

#define TAG_RSS			"rss"
#define TAG_CHANNEL		"channel"
#define	TAG_TITLE		"title"
#define	TAG_LINK		"link"
#define	TAG_DESCRIPTION		"description"
#define	TAG_PUBDATE		"pubDate"
#define	TAG_DCDATA		"dc:data"
#define	TAG_ITEM		"item"
#define	TAG_GUID		"guid"

/*
 * Write data callback function (called within the context of curl_easy_perform.
 */

size_t
write_data( void *buffer, size_t size, size_t nmemb, void *userp )
{
	int	segsize = size * nmemb;

	/*
	 * Check to see if this data exceeds the size of our buffer. If so, 
	 * set the user-defined context value and return 0 to indicate a
	 * problem to curl.
	 */
	if ( wr_index + segsize > MAX_BUF ) {
		*(int *)userp = 1;
		return 0;
	}

	/* Copy the data from the curl buffer into our buffer */
	memcpy( (void *)&wr_buf[wr_index], buffer, (size_t)segsize );

	/* Update the write index */
	wr_index += segsize;

	/* Null terminate the buffer */
	wr_buf[wr_index] = 0;

	/* Return the number of bytes received, indicating to curl that
	 * all is okay */
	return segsize;
}

int
print_xml(char* text) {
	int			i;
	int			size;
	xmlDoc			*doc;
	xmlXPathContextPtr	xpathCtx;
	xmlXPathObjectPtr	xpathObj;
	xmlNodeSetPtr		nodes;
	xmlNode			*node;
	xmlChar			*textcont;

	doc = xmlReadMemory(text, strlen(text), "noname.xml", NULL, 0);
	if (doc == NULL) {
		perror("xmlReadFile");
	}

	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return -1;
	}
	xpathObj = xmlXPathEvalExpression(BAD_CAST XPATH_ITEM, xpathCtx);
	nodes = xpathObj->nodesetval;
	if (nodes == NULL) {
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return -1;
	}

	printf("\n");
	size = nodes->nodeNr;
	for (i = 0; i < size; i++) {
		node = xmlFirstElementChild(nodes->nodeTab[i]);
		if (node == NULL) {
			continue;
		}
		for (; node; node = xmlNextElementSibling(node)) {
			if (strcmp((char*)node->name,TAG_TITLE) != 0 &&
			    strcmp((char*)node->name,TAG_LINK) != 0) {
				continue;
			}
			textcont = xmlNodeGetContent(node);
			if (textcont) {
				printf(" %s:\t%s\n", node->name, textcont);
				xmlFree(textcont);
			}
		}
		printf("\n");
	}

	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	xmlCleanupParser();
	return 0;
}

int
main(int argc, char *argv[]) {
	CURL		*curl;
	CURLcode	ret;
	int		wr_error;

	wr_error = 0;
	wr_index = 0;

	/* First step, init curl */
	curl = curl_easy_init();
	if (!curl) {
		printf("couldn't init curl\n");
		exit(1);
	}

	/* Tell curl the URL of the file we're going to retrieve */
	curl_easy_setopt( curl, CURLOPT_URL, RSS_FEED );

	/* Tell curl that we'll receive data to the function write_data, and
	 * also provide it with a context pointer for our error return.
	 */
	curl_easy_setopt( curl, CURLOPT_WRITEDATA, (void *)&wr_error );
	curl_easy_setopt( curl, CURLOPT_WRITEFUNCTION, write_data );

	/* Allow curl to perform the action */
	ret = curl_easy_perform( curl );

	//printf( "ret = %d (write_error = %d)\n\n", ret, wr_error );

	/* Emit the page if curl indicates that no errors occurred */
	if ( ret == 0 ) {
		// printf("%s\n\n", wr_buf );
		print_xml( wr_buf );
	} else {
		printf( "ret = %d (write_error = %d)\n\n", ret, wr_error );
		curl_easy_cleanup( curl );
		exit(1);
	}

	curl_easy_cleanup( curl );
	return 0;
}

