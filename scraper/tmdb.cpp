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
	TiXmlElement* movie;
	movies = docHandle.FirstChild("OpenSearchDescription").FirstChild("movies").ToElement();
	int n = 0;
	for (movie = movies->FirstChildElement("movie"); movie != NULL; movie = movie->NextSiblingElement("movie")) {
		if (n < MAX_SEARCH_RESULT) {
			int res = 0;
			const char * str;
			TiXmlElement * item;

			item = movie->FirstChildElement("id");
			if (item) {
				str = item->GetText();
				if (str) {
					p->results[n].id = strdup(str);
					res ++;
				}
			}

			item = movie->FirstChildElement("name");
			if (item) {
				str = item->GetText();
				if (str) {
					p->results[n].name = strdup(str);
					res ++;
				}
			}

			item = movie->FirstChildElement("released");
			if (item) {
				str = item->GetText();
				if (str) {
					p->results[n].year = atoi(str);
				}
			}
			if (res != 2) {
				if (p->results[n].name)
					free(p->results[n].name);
				if (p->results[n].id)
					free(p->results[n].id);
				printf("[ TMDB ] Parse failed\n");
				continue;
			}

			n++;
		}
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
	movie = docHandle.FirstChild("OpenSearchDescription").FirstChild("movies").Child("movie", 0).ToElement();
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

	INFO_SET_STR(p->name, "name");
	INFO_SET_STR(p->overview, "overview");
	INFO_SET_STR(p->summary, "tagline");
	INFO_SET_STR(p->imdb_id, "imdb_id");
	/* get the rate */
	ele = movie->FirstChildElement("rating"); 
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
	INFO_SET_INT(p->votes, "votes");
	INFO_SET_INT(p->year, "released");
	INFO_SET_INT(p->budget, "budget");
	INFO_SET_INT(p->revenue, "revenue");
	/* just get 1 company */
	INFO_SET_STR2(p->company, "studios", "studio", "name");
	/* genre */
	ele = movie->FirstChildElement("categories"); 
	if (ele) ele = ele->FirstChildElement("category"); 
	int i = 0;
	while (ele != NULL) {
		if (strcmp(ele->Attribute("type"), "genre") == 0) {
			p->genres[i] = strdup(ele->Attribute("name"));
			i++;
			if (i== JB_SCPR_MAX_GENRE)
				break;
		}
		ele = ele->NextSiblingElement("category");
	}
	/* cover */
	ele = movie->FirstChildElement("images"); 
	if (ele) ele = ele->FirstChildElement("image"); 
	int ncover = 0, nfanart = 0;
	const char * type;
	const char * size;
	const char * url;
	while (ele != NULL) {
		type = ele->Attribute("type");
		size = ele->Attribute("size");
		url = ele->Attribute("url");
		if (size != NULL && type != NULL && url != NULL){
			if (strcmp(type, "poster") == 0) {
				/* FIXME */
				if (ncover<JB_SCPR_MAX_IMAGE) {
					if (strcmp(size, "thumb") == 0)
						p->cover_preview[ncover] = strdup(url);
					else if (strcmp(size, "cover") == 0) {
						p->cover[ncover] = strdup(url);
						ncover ++;
					}
				}
			}
			else if (strcmp(type, "backdrop") == 0) {
				/* FIXME */
				if (nfanart<JB_SCPR_MAX_IMAGE) {
					if (strcmp(size, "thumb") == 0)
						p->fanart_preview[nfanart] = strdup(url);
					else if (strcmp(size, "w1280") == 0) {
						p->fanart[nfanart] = strdup(url);
						nfanart ++;
					}
				}
			}
		}
		ele = ele->NextSiblingElement("image"); 
	}
	/* casts */
	ele = movie->FirstChildElement("cast"); 
	if (ele) ele = ele->FirstChildElement("person"); 
	int nactor = 0;
	int ndirector = 0;
	const char * job;
	const char * name;
	const char * character;
	while (ele != NULL) {
		job = ele->Attribute("job");
		name = ele->Attribute("name");
		character = ele->Attribute("character");
		if (job != NULL && name != NULL) {
			if (strcmp(job, "Director") == 0) {
				if (ndirector<1) {
					p->director = strdup(name);
					ndirector ++;
				}
			}
			else if (strcmp(job, "Actor") == 0) {
				if (nactor<JB_SCPR_MAX_ACTOR) {
					p->name_actor[nactor] = strdup(name);
					p->name_char[nactor] = strdup(character);
					nactor ++;
				}
			}
		}
		ele = ele->NextSiblingElement("person"); 
	}

	printf("parse done\n");
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
	while((c = getopt(argc, argv, "sl:k:o:")) != -1) {
		switch(c) {
			case 's':
				do_search = true;
				break;
			case 'l':
				strncpy(language, optarg, 32);
				break;
			case 'k':
				strncpy(keyword, optarg, 255);
				break;
			case 'o':
				strncpy(output, optarg, 256);
				break;
		}
	}
/*	FILE *file;
	file=fopen("/tmp/tmdb.params.log","a+");
	for (int i=0; i<argc; i++) {
		fprintf(file,"argv[%d] %s\n", i, argv[i]);
	}
	fclose(file);
*/
#define LINK	"http://api.themoviedb.org/2.1"
#define TMPFILE	"/tmp/tmdb.search.xml"
#define KEY	"cc7cffe5b64245b308fbb769fd3cf013"
//#define KEY	"54dfc66c547097ec0460ad40da9135cf"
	char cmd[256];
	if (do_search)  {
		/* search movie */
		printf("keyword: %s\n", keyword);
		sprintf(cmd, "wget -t 1 -T 30 -nd \"%s/Movie.search/%s/xml/%s/%s\" -O %s", LINK, language, KEY, keyword, TMPFILE);
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
		sprintf(cmd, "wget -t 1 -T 30 -nd \"%s/Movie.getInfo/%s/xml/%s/%s\" -O %s", LINK, language, KEY, keyword, TMPFILE);
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


