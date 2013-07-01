/*
 *  csfd.cpp
 *
 *  Created by longmatys <longmatys@gmail.com>
 *  Useful for retrieving movie data from czech database
 */

 /* max error message length */
#define MAX_ERR_LENGTH 80
 /* max length of a line of text from stdin */
#define MAX_TXT_LENGTH 100000

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "result.h"
#include <iostream>
#include <fstream>
#include <cstdlib>
 /* for --ansi (see off_t in regex.h) */
#include <sys/types.h>
 /* for index(): */
#include <regex.h>
#include <string.h>
#include <string>
using namespace std;
void RemoveString(string &param, const string start, const string end=""){
	int pos_b,pos_e;
	while ((pos_b=param.find(start))>-1){
		if (end.length()>0){
			if ((pos_e=param.find(end))>-1){
				param.erase(pos_b,pos_e-pos_b+end.length());
			}	
		}else
			param.erase(pos_b,start.length());
	}
}
#define interest_name "<h1>(.*)" 
#define interest_id "<a href=\"/film/([^/]*)/zajimavosti/"
#define interest_year "<p class=\"origin\">[^,]*, ([^,]*),"
int ParseInfo(const char * html, struct InfoResult * p)
{
	bool ret;
	int err;
	const char * str_notfound = "Not Found";
	const char * str;
	char temp_string[1024];
	string line,multiline,found;
	ifstream myfile(html);
	int state=0;
	int n=0;
	int pos=0;
	int cover_i=0;
	regmatch_t pmatch[3];
//Regular expressions
	//char interest_name[]="<h1>(.*)";
	char interest_overview_help1[]="<h3>Obsah </h3>";
	char interest_overview_help2[]="</li>";
	char interest_overview[]="<p>.* (.+)</p>";
	char interest_rate[]="<h2 class=\"average\">([^%]*)%?</h2>";
	char interest_votes[]="<a id=\"rating-count-link\" rel=\"nofollow\" href=\".*\">všechna hodnocení<br /> \\((.*)\\)</a>";
	char interest_actor_help1[]="<h4>Hrají:</h4>";
	char interest_actor_help2[]="</span>";
	char interest_genres[]="<p class=\"genre\">(.*)</p>";
	char interest_cover[]="url\\('(.*)'\\)";
	char interest_cover_help1[]="<h3>Plakáty</h3>";
	char interest_cover_help2[]="</tr>";
	//char interest_year[]="<p class=\"origin\">[^,]*, ([^,]*),";
	char interest_director_help1[]="<h4>Režie:</h4>";
	char interest_director_help2[]="</span>";
	regex_t re_name;
	regex_t re_overview_help1;
	regex_t re_overview_help2;
	regex_t re_overview;
	regex_t re_rate;
	regex_t re_votes;
	regex_t re_genres;
	regex_t re_actor_help1;
	regex_t re_actor_help2;
	regex_t re_cover;
	regex_t re_cover_help1;
	regex_t re_cover_help2;
	regex_t re_director_help1;
	regex_t re_director_help2;
	regex_t re_year;
//Regular expression compilation
	if ( (err = regcomp(&re_overview_help1, interest_overview_help1, REG_EXTENDED)) != 0 ) {
		/* Only First time error handling - to save my time :)
regerror(err, &re_header, err_msg, MAX_ERR_LENGTH);
printf("Error analyzing regular expression '%s': %s.\n", interest_header, err_msg);
*/
		return -1;
	}
	if ( (err = regcomp(&re_overview_help2, interest_overview_help2, REG_EXTENDED)) != 0 ) return -1;
	if ( (err = regcomp(&re_name, interest_name, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_overview, interest_overview, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_rate, interest_rate, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_votes, interest_votes, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_genres, interest_genres, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_actor_help1, interest_actor_help1, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_actor_help2, interest_actor_help2, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_cover, interest_cover, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_cover_help1, interest_cover_help1, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_cover_help2, interest_cover_help2, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_year, interest_year, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_director_help1, interest_director_help1, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_director_help2, interest_director_help2, REG_EXTENDED)) != 0 ) return -1; 
	if (myfile.is_open())
	{
		while ( myfile.good() )
		{
			getline (myfile,line);
			if (state==0){
				//Finding movie name
				if ( (err = regexec(&re_name, line.c_str(), 2, pmatch, 0)) == 0 ){
					/* Only First time error handling - to save my time :)
					   regerror(err, &myre1, err_msg, MAX_ERR_LENGTH);
					 */
					found=line.substr(pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so).c_str();
					//remove UTF 8 spaces
					RemoveString(found,"\x9");
					RemoveString(found,"<",">");
					p->name = strdup(found.c_str());
				}else if ( err != REG_NOMATCH ) return -1; 
				//Finding section with movies
				if ( (err = regexec(&re_overview_help1, line.c_str(), 0, NULL, 0)) == 0 ) state=1;
				else if ( err != REG_NOMATCH ) return -1; 
				//Finding actors beginning
				if ( (err = regexec(&re_actor_help1, line.c_str(), 0, NULL, 0)) == 0 ) state=2;
				else if ( err != REG_NOMATCH ) return -1; 
				//Finding cover art beginning
				if ( (err = regexec(&re_cover_help1, line.c_str(), 0, NULL, 0)) == 0 ) state=3;
				else if ( err != REG_NOMATCH ) return -1; 
				//Finding director beginning
				if ( (err = regexec(&re_director_help1, line.c_str(), 0, NULL, 0)) == 0 ) state=4;
				else if ( err != REG_NOMATCH ) return -1; 
				//Finding movie rate
				if ( (err = regexec(&re_rate, line.c_str(), 2, pmatch, 0)) == 0 ){
					p->rate = atoi(line.substr(pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so).c_str());
				}else if ( err != REG_NOMATCH ) return -1; 
				//Finding movie votes
				if ( (err = regexec(&re_votes, line.c_str(), 2, pmatch, 0)) == 0 ){
					found=line.substr(pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so).c_str();
					//remove UTF 8 spaces
					RemoveString(found,"\xc2\xa0");
					p->votes = atoi(found.c_str());
				}else if ( err != REG_NOMATCH ) return -1; 
				//Finding movie year
				if ( (err = regexec(&re_year, line.c_str(), 2, pmatch, 0)) == 0 ){
					p->year = atoi(line.substr(pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so).c_str());
				}else if ( err != REG_NOMATCH ) return -1; 
				//Finding movie genres
				if ( (err = regexec(&re_genres, line.c_str(), 2, pmatch, 0)) == 0 ){
					found=line.substr(pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so).c_str();
					RemoveString(found," ");
					int i=0;
					while ((i < JB_SCPR_MAX_GENRE )and ((pos=found.find("/"))>-1)){
						p->genres[i++] = strdup(found.substr(0,pos).c_str());
						found.replace(0,pos+1,"");
					}
					if ((i < JB_SCPR_MAX_GENRE )and (found.length()>0)) p->genres[i++] = strdup(found.c_str());
				}else if ( err != REG_NOMATCH ) return -1; 
			}else if (state==1){
				//Finding movie overview end
				if ( (err = regexec(&re_overview_help2, line.c_str(), 0, NULL, 0)) == 0 ) {
					RemoveString(multiline,"\r");
					RemoveString(multiline,"<",">");
					RemoveString(multiline,"\t");
					p->overview = strdup(multiline.c_str());
					p->summary = strdup(multiline.c_str());
					state=0;
					multiline="";
				}
				else if ( err != REG_NOMATCH ) return -1; 
				else  multiline+=line;
			}else if (state==2){
				//Finding movie actors end
				if ( (err = regexec(&re_actor_help2, line.c_str(), 0, NULL, 0)) == 0 ) {
					RemoveString(multiline,"<",">");
					RemoveString(multiline,"\t");
					RemoveString(multiline,"\r");
					int i=0;
					while ((i < JB_SCPR_MAX_ACTOR )and ((pos=multiline.find(", "))>-1)){
						p->name_actor[i++] = strdup(multiline.substr(0,pos).c_str());
						multiline.replace(0,pos+2,"");
					}
					if ((i < JB_SCPR_MAX_ACTOR )and (multiline.length()>0)) p->name_actor[i++] = strdup(multiline.c_str());
					state=0;
					multiline="";
				}
				else if ( err != REG_NOMATCH ) return -1; 
				else  multiline+=line;
			}else if (state==3){
				//Finding covers section end
				if ( (err = regexec(&re_cover_help2, line.c_str(), 0, NULL, 0)) == 0 ) 
					state=0;
				else if ( err != REG_NOMATCH ) return -1; 
				else {
					if ( (err = regexec(&re_cover, line.c_str(), 2, pmatch, 0)) == 0 ){ 
						found=line.substr(pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so).c_str();
						RemoveString(found,"\\");
						if (found.find("http", 0, 4) != 0) {
							// Doplneni chybejiciho "http:" pokud chybi
							found.insert(0,"http:");
						}
						if ((cover_i < JB_SCPR_MAX_IMAGE )and (found.length()>0)){
							if (pos = found.find_last_of("?")) {
								// Plny obrazek je bez cehosi a la "?h180" na konci url. Aspon by mel byt.
								found.erase(pos,found.length());
							}
							p->cover_preview[cover_i] = strdup(found.c_str());
							p->cover[cover_i++] = strdup(found.c_str());
						}
					} else if ( err != REG_NOMATCH ) return -1; 
				}
			}else if (state==4){
				//Finding movie actors end
				if ( (err = regexec(&re_director_help2, line.c_str(), 0, NULL, 0)) == 0 ) {
					RemoveString(multiline,"<",">");
					RemoveString(multiline,"\t");
					RemoveString(multiline,"\r");
					p->director=strdup(multiline.c_str());

					state=0;
					multiline="";
				}
				else if ( err != REG_NOMATCH ) return -1; 
				else  multiline+=line;
			}


		}
		myfile.close();
	}
	else cout << "Unable to open file"; 
	regfree(&re_name);
	regfree(&re_overview_help1);
	regfree(&re_overview_help2);
	regfree(&re_overview);
	regfree(&re_rate);
	regfree(&re_votes);
	regfree(&re_genres);
	regfree(&re_actor_help1);
	regfree(&re_actor_help2);
	regfree(&re_cover);
	regfree(&re_cover_help1);
	regfree(&re_cover_help2);
	regfree(&re_year);
	regfree(&re_director_help1);
	regfree(&re_director_help2);
	printf("[ CSFD ] parse done\n");
	return 0;
}
#define STATE_UNKNOWN 0
#define STATE_MULTIPLE_START 1
#define STATE_SINGLE_TYPE 2
#define STATE_MULTIPLE_TYPE 3
int ParseSearch(const char * html, struct SearchResult * p)
{
	bool ret;
	string line,found;
	ifstream myfile(html);
	regex_t re_multiple_header,re_multiple_item,interest_single_year,re_multiple_end;
	regex_t re_single_id;
	regex_t re_single_name;
	regex_t re_single_year;
	regex_t re_multiple_type;
	int err;
	char err_msg[MAX_ERR_LENGTH];
	char interest_header[]="<h2 class=\"header\">Filmy</h2>";
	char interest_item[]="<h3 class=\"subject\"><a href=\"/film/(.*)/\" .*>(.*)</a>";
	char interest_year_s[]="<p>.* (.+)</p>";
	char interest_footer[]="</ul>";
	char movie_item[]="<li>";
	char interest_multiple_results[]="<h1>Vyhledávání</h1>";
	int state=STATE_UNKNOWN;
	int n=0;
	regmatch_t pmatch[3];
	if ( (err = regcomp(&re_multiple_header, interest_header, REG_EXTENDED)) != 0 ) { return -1; }
	if ( (err = regcomp(&re_multiple_item, interest_item, REG_EXTENDED)) != 0 ) { return -1; }
	if ( (err = regcomp(&interest_single_year, interest_year_s, REG_EXTENDED)) != 0 ) { return -1; } 
	if ( (err = regcomp(&re_multiple_end, interest_footer, REG_EXTENDED)) != 0 ) { return -1; }
	if ( (err = regcomp(&re_single_id, interest_id, REG_EXTENDED)) != 0 ) return -1;
	if ( (err = regcomp(&re_single_name, interest_name, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_single_year, interest_year, REG_EXTENDED)) != 0 ) return -1; 
	if ( (err = regcomp(&re_multiple_type, interest_multiple_results, REG_EXTENDED)) != 0 ) return -1; 
	if (myfile.is_open())
	{
		while ( myfile.good() )
		{
			getline (myfile,line);
			if (state==STATE_UNKNOWN){
//Determining search result type - single or multiple, determined by h1 header
				if ( (err = regexec(&re_multiple_type, line.c_str(), 0, NULL, 0)) == 0 ) { 
					state=STATE_MULTIPLE_TYPE; 
				} else if ( err != REG_NOMATCH ) { return -1; }
				else if ( (err = regexec(&re_single_name, line.c_str(), 2, pmatch, 0)) == 0 ) { 
					found=line.substr(pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so).c_str();
					//remove UTF 8 spaces
					RemoveString(found,"\x9");
					p->results[n].name = strdup(found.c_str());
					state=STATE_SINGLE_TYPE; 
				}else if ( err != REG_NOMATCH ) { return -1; }
			}else if (state==STATE_MULTIPLE_TYPE){
				if ( (err = regexec(&re_multiple_header, line.c_str(), 0, NULL, 0)) == 0 ) { 
					state=STATE_MULTIPLE_START; 
				} else if ( err != REG_NOMATCH ) { return -1; }
			}else if (state==STATE_MULTIPLE_START){
				//Finding section with movies
				if ( (err = regexec(&re_multiple_item, line.c_str(), 3, pmatch, 0)) == 0 ) {
					if (n < MAX_SEARCH_RESULT) {
						p->results[n].id = strdup(line.substr(pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so).c_str());
						p->results[n].name = strdup(line.substr(pmatch[2].rm_so,pmatch[2].rm_eo-pmatch[2].rm_so).c_str());
					}
				} else if ( err != REG_NOMATCH ) { return -1; }
				else if ( (err = regexec(&interest_single_year, line.c_str(), 2, pmatch, 0)) == 0 ) {
					if (n < MAX_SEARCH_RESULT) {
						p->results[n].year = atoi(line.substr(pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so).c_str());
						//Year is always last interesting
						n++;
					}
				} else if ( err != REG_NOMATCH ) { return -1; }
				else if ( (err = regexec(&re_multiple_end, line.c_str(), 0, NULL, 0)) == 0 ) {
					state=STATE_UNKNOWN;
				}
				else if ( err != REG_NOMATCH ) { return -1; }
			}else if (state==STATE_SINGLE_TYPE){
				if ( (err = regexec(&re_single_year, line.c_str(), 2, pmatch, 0)) == 0 ) { 
					p->results[n].year = atoi(line.substr(pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so).c_str());
				} else if ( err != REG_NOMATCH ) { return -1; }
				if ( (err = regexec(&re_single_id, line.c_str(), 2, pmatch, 0)) == 0 ) { 
					p->results[n].id = strdup(line.substr(pmatch[1].rm_so,pmatch[1].rm_eo-pmatch[1].rm_so).c_str());
					n++;
				} else if ( err != REG_NOMATCH ) { return -1; }
			}

		}
		myfile.close();
	}
	else cout << "Unable to open file"; 
	regfree(&re_multiple_header);
	regfree(&re_multiple_item);
	regfree(&interest_single_year);
	regfree(&re_multiple_end);
	regfree(&re_single_id);
	regfree(&re_single_year);
	regfree(&re_single_name);
	regfree(&re_multiple_type);
	p->nResults=n;
	return n;
}

int main(int argc, char *argv[]) {
	/* will contain the regular expression */
	regex_t myre;
	int err;
	char err_msg[MAX_ERR_LENGTH];
	char text[MAX_TXT_LENGTH];
	bool do_search = false;
	char language[32] = {0};
	char keyword[256] = {0};
	char output[256] = {0};
	int c;
	int ret = -1;

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
	file=fopen("/tmp/csfd.params.log","a+");
	for (int i=0; i<argc; i++) {
		fprintf(file,"argv[%d] %s\n", i, argv[i]);
	}
	fclose(file);*/
#define LINK	"http://www.csfd.cz"
#define TMPFILE	"/tmp/csfd.search.xml"
	char cmd[256];
	if (do_search)  {
		/* search movie */
		printf("keyword: %s\n", keyword);
		sprintf(cmd, "wget -t 1 -T 30 -nd \"%s/hledat/?q=%s\" -O %s", LINK, keyword, TMPFILE);
		printf("%s\n",cmd);
		system(cmd);
		if (access(TMPFILE, F_OK) == 0) {
			struct SearchResult result;
			memset(&result, 0, sizeof(result));
			if (ParseSearch(TMPFILE, &result) > 0){
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
		sprintf(cmd, "wget -t 1 -T 30 -nd \"%s/film/%s/\" -O %s", LINK, keyword, TMPFILE);
		printf("%s\n",cmd);
		system(cmd);
		if (access(TMPFILE, F_OK) == 0) {
			struct InfoResult result;
			memset(&result, 0, sizeof(result));
			if (ParseInfo(TMPFILE, &result) >= 0) {
				result.imdb_id = strdup(keyword);
				ret = GenInfoResult(&result, output);
				// releasing... 
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
