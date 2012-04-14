#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "result.h"
#include <tinyxml.h>

int ParseXMLSearch(const char * xml, struct SearchResult * p)
{
	bool ret;
	TiXmlDocument doc(xml);
	ret = doc.LoadFile();
	if (!ret) {
		return -1;
	}
	TiXmlHandle docHandle(&doc);
	TiXmlElement* movies;
	TiXmlElement* id;
	TiXmlElement* title;
	TiXmlElement* year;
	movies = docHandle.FirstChild("torec").ToElement();
	if (movies == NULL)
		return -1;
	int n = 0;
	id = movies->FirstChildElement("id");
	title = movies->FirstChildElement("title");
	year = movies->FirstChildElement("year");
	while (id != NULL && title != NULL && year != NULL && n < MAX_SEARCH_RESULT) {
		int res = 0;
		const char * str;
		TiXmlElement * item;

		str = id->GetText();
		if (str) {
			p->results[n].id = strdup(str);
			res ++;
		}
		str = title->GetText();
		if (str) {
			p->results[n].name = strdup(str);
			res ++;
		}
		str = year->GetText();
		if (str) {
			p->results[n].year = atoi(str);
		}
		if (res != 2) {
			if (p->results[n].name)
				free(p->results[n].name);
			if (p->results[n].id)
				free(p->results[n].id);
			printf("[ TOREC ] Parse failed\n");
			continue;
		}

		n++;
		id = id->NextSiblingElement("id");
		title = title->NextSiblingElement("title");
		year = year->NextSiblingElement("year");
	}
	p->nResults = n;
	return n;
}

int ParseXMLInfo(const char * xml, struct InfoResult * p)
{
	bool ret;
	const char * str_notfound = "Not Found";
	const char * str;
	TiXmlDocument doc(xml);
	ret = doc.LoadFile();
	if (!ret) {
		return -1;
	}
	TiXmlHandle docHandle(&doc);
	TiXmlElement* movie;
	TiXmlElement* ele;
	movie = docHandle.FirstChild("torec").ToElement();
	if (movie == NULL)
		return -1;

#define INFO_SET_STR(var, item) do { \
	str = NULL; \
	ele = movie->FirstChildElement(item); \
	if (ele) str = ele->GetText(); \
	if (str == NULL) str = str_notfound; \
	var = strdup(str); } while(0)
#define INFO_SET_STR2(var, item0, item1, attr) do { \
	str = NULL; \
	ele = movie->FirstChildElement(item0); \
	if (ele) ele = ele->FirstChildElement(item1); \
	if (ele) str = ele->Attribute(attr); \
	if (str == NULL) str = str_notfound; \
	var = strdup(str); } while(0)
#define INFO_SET_INT(var, item)	do { \
	ele = movie->FirstChildElement(item); \
	if (ele) str = ele->GetText(); \
	else str = NULL; \
	if (str) var = atoi(str); \
	else var = -1; } while(0)

	INFO_SET_STR(p->name, "title");
	INFO_SET_STR(p->overview, "description");
	INFO_SET_STR(p->summary, "tagline");
	INFO_SET_STR(p->imdb_id, "imdbID");
	p->company = strdup(str_notfound);
	/* get the rate */
	ele = movie->FirstChildElement("IMDB_rank"); 
	if (ele) {
		char rating[32] = {0};
		str = ele->GetText();
		if (str) {
			strncpy(rating, str, 32);
			/* x10 */
			for (int i=0; i<strlen(rating); i++){
				if (rating[i] == '.') {
					rating[i] = rating[i+1];
					rating[i+1] = '.';
					break;
				}
			}
		}
		p->rate = atoi(rating);
	}
	INFO_SET_INT(p->votes, "IMDB_votes");
	INFO_SET_INT(p->year, "year");
	/* genre */
	ele = movie->FirstChildElement("genre"); 
	if ( ele ) {
		str = ele->GetText();
		if (str) {
			char text[256] = {0};
			char tmp[128] = {0};
			strncpy(text, str, 256);
			int i = 0;
			char * pc = text;
			int ic = 0;
			int len = strlen(text);
			while (i < JB_SCPR_MAX_GENRE && ic < len) {
				int k = 0;
				memset(tmp, 0, sizeof(tmp));
				while (ic < len) {
					if (text[ic] != ' ') {
						tmp[k] = text[ic];
						if (tmp[k] == '/') {
							tmp[k] = 0;
							ic++;
							break;
						}
						k++;
					}
					ic++;
				}
				p->genres[i++] = strdup(tmp);
			}
		}
	}
	/* poster */
	ele = movie->FirstChildElement("poster"); 
	if (ele) ele = ele->FirstChildElement("image"); 
	int ncover = 0, nfanart = 0;
	const char * url;
	while (ele != NULL) {
		url = ele->Attribute("url");
		if (url != NULL){
			if (ncover<JB_SCPR_MAX_IMAGE) {
				p->cover_preview[ncover] = strdup(url);
				p->cover[ncover] = strdup(url);
				ncover ++;
			}
		}
		ele = ele->NextSiblingElement("image"); 
	}
	/* fanart */
	ele = movie->FirstChildElement("fanart");
	if (ele) ele = ele->FirstChildElement("image"); 
	while (ele != NULL) {
		url = ele->Attribute("url");
		if (url != NULL){
			if (nfanart<JB_SCPR_MAX_IMAGE) {
				p->fanart_preview[ncover] = strdup(url);
				p->fanart[ncover] = strdup(url);
				nfanart ++;
			}
		}
		ele = ele->NextSiblingElement("image"); 
	}
	/* director */
	INFO_SET_STR(p->director, "director");
	/* casts */
	ele = movie->FirstChildElement("cast"); 
	if (ele) {
		str = ele->GetText();
		if (str) {
			int nactor = 0;
			char text[256] = {0};
			char tmp[128] = {0};
			strncpy(text, str, 256);
			char * pc = text;
			int ic = 0;
			int len = strlen(text);
			while (nactor < JB_SCPR_MAX_ACTOR && ic < len) {
				int k = 0;
				memset(tmp, 0, sizeof(tmp));
				while (ic < len) {
					if (text[ic] != ' ') {
						tmp[k] = text[ic];
						if (tmp[k] == ',') {
							tmp[k] = 0;
							ic++;
							break;
						}
						k++;
					}
					ic++;
				}
//				printf("tmp: %s\n", tmp);
				p->name_char[nactor] = NULL;
				p->name_actor[nactor++] = strdup(tmp);
			}
		}
	}

	printf("[ TOREC ] parse done\n");
	return 0;
}

/* to search the movie:
 * tmdb -s -l language -k keyword -o outfile 
 * */

/* to get movie info:
 * tmdb -l language -k keyword -o outfile 
 * keyword will be the id in result of search
 * */

/* return 0 when successed. */
int main(int argc, char ** argv)
{
	char language[32] = {0};
	char keyword[256] = {0};
	char output[256] = {0};
	int c;
	int ret = -1;
	bool do_search = false;
	if (argc < 5) {
		printf("%s [-s][-l en][-k keyword][-o out]\n", argv[0]);
		return -1;
	}
	while((c = getopt(argc, argv, "sl:k:o:")) != -1) {
		switch(c) {
			case 's':
				do_search = true;
				break;
			case 'l':
				strncpy(language, optarg, 32);
				break;
			case 'k':
				strncpy(keyword, optarg, 256);
				break;
			case 'o':
				strncpy(output, optarg, 256);
				break;
		}
	}
	printf("[ TOREC ] %s begin: %s\n", do_search?"SEARCH":"INFO", keyword);
#define TMPFILE	"/tmp/jukebox/torec.search.xml"
	char cmd[256];
	if (do_search)  {
		/* search movie */
		sprintf(cmd, "wget -t 1 -T 30 -nd \"http://www.torec.net/xml/xmr/movie_id_xml.asp?xmr=xtmr10tr&name=%s\" -O %s", keyword, TMPFILE);
		system(cmd);
		if (access(TMPFILE, F_OK) == 0) {
			struct SearchResult result;
			memset(&result, 0, sizeof(result));
			if (ParseXMLSearch(TMPFILE, &result) > 0) {
				ret = GenSearchResult(&result, output);
				/* releasing... */
				for (int i=0; i<result.nResults; i++) {
					if (result.results[i].name)
						free(result.results[i].name);
					if (result.results[i].id)
						free(result.results[i].id);
				}
			}
			unlink(TMPFILE);
		}
	}
	else {
		/* get movie info, and will pass the id via keywoard */
		sprintf(cmd, "wget -t 1 -T 30 -nd \"http://www.torec.net/xml/xmr/movie_info_xml.asp?xmr=xtmr10tr&id=%s\" -O %s", keyword, TMPFILE);
		system(cmd);
		if (access(TMPFILE, F_OK) == 0) {
			struct InfoResult result;
			memset(&result, 0, sizeof(result));
			if (ParseXMLInfo(TMPFILE, &result) >= 0) {
				ret = GenInfoResult(&result, output);
				/* releasing... */
#define MYFREE(x) if (x) free(x)
				MYFREE(result.name);
				MYFREE(result.overview);
				MYFREE(result.summary);
				MYFREE(result.imdb_id);
				MYFREE(result.company);
				MYFREE(result.director);
				for (int i=0; i<JB_SCPR_MAX_GENRE; i++)
					MYFREE(result.genres[i]);
				for (int i=0; i<JB_SCPR_MAX_ACTOR; i++) {
					MYFREE(result.name_actor[i]);
					MYFREE(result.name_char[i]);
				}
				for (int i=0; i<JB_SCPR_MAX_IMAGE; i++) {
					MYFREE(result.cover_preview[i]);
					MYFREE(result.cover[i]);
					MYFREE(result.fanart_preview[i]);
					MYFREE(result.fanart[i]);
				}

			}
			unlink(TMPFILE);
		}
	}
	return ret;
}


