/*
 *  csfd.cpp
 *
 *  Created by longmatys <longmatys@gmail.com>
 *  Useful for retrieving movie data from czech database
 *  Cleanup(?)/Rewrite(?)/Zapleveleni(?) kmarty <gkmarty@gmail.com> (proste jsem se v tom vrtal :-) )
 */

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
#include <string>
#include <vector>
using namespace std;

 /* max error message length */
#define MAX_ERR_LENGTH 80
 /* max length of a line of text from stdin */
#define MAX_TXT_LENGTH 100000

/* Stavy pri hledani */
#define S_UNKNOWN		0
#define S_LOOK4IDNAME		1
#define S_LOOK4YEAR		2
#define S_LOOK4SINGLE_YEAR	3	// Jen pro poradek, technicky neni pouzito
#define S_LOOK4SINGLE_IDNAME	4

#define S_HOTOVO		255

/* Stavy pri ziskavani infa o filmu */
/* U dat, ktera jdou ziskat rovnou z jednoho radku, a ten radek se da najit pomerne slusne, tam je pouzit pouze I_LOOK4*
 * U viceradkovych/vicezaznamovych dat, je I_LOOK4* pouzit pro preskoceni balastu az na misto kde zacinaji data
 * a I_GETTING* je pro ziskavani radek kde se potrebna data nachazeji
 */
#define I_UNKNOWN		0
#define I_LOOK4POSTER		1
#define I_LOOK4NAME		2
#define I_GETTING_NAME		3
#define I_LOOK4GENRE		4
#define I_LOOK4YEAR		5
#define I_LOOK4DIRECTOR		6
#define I_GETTING_DIRECTOR	7
#define I_LOOK4ACTORS		8
#define I_GETTING_ACTORS	9
#define I_LOOK4COVERS		10
#define I_GETTING_COVERS	11
#define I_LOOK4OVERVIEW		12
#define I_GETTING_OVERVIEW	13
#define I_LOOK4FANART		14
#define I_GETTING_FANART	15
#define I_LOOK4RATE		16

#define I_HOTOVO		255

string RemoveString(const string s, const string start, const string end="") {
	// Oproti puvodni fci v csfd.cpp tahle nemeni vstupni retezec, ale vraci novy
	string result = s;
	int pos_b,pos_e;

	while ((pos_b=result.find(start))>-1){
		if (end.length()>0){
			if ((pos_e=result.find(end))>-1){
				result.erase(pos_b,pos_e-pos_b+end.length());
			}	
		}else
			result.erase(pos_b,start.length());
	}
	return result;
}

// Tohle je obslehnuty z http://www.cplusplus.com/faq/sequences/strings/split/#string-find_first_of
// Jen je to prebouchany na fci (aby to ten vector vracelo jako navratovou hodnotu)
struct split {
	enum empties_t { empties_ok, no_empties };
};

const vector <string> split(const string s, const string delimiters, split::empties_t empties = split::empties_ok )
{
	vector <string> result;
	size_t current;
	size_t next = -1;
	do {
		if (empties == split::no_empties) {
			next = s.find_first_not_of(delimiters, next + 1);
			if (next == string::npos) break;
			next -= 1;
		}
		current = next + 1;
		next = s.find_first_of( delimiters, current );
		result.push_back(s.substr(current, next - current ));
	} while (next != string::npos);
	return result;
}

const size_t reg_match(regex_t &regexp, string source, size_t nmatch, vector <string> &result)
{
	// &regexp = hen ten onen regexp vyrobeny pres regcomp() TODO: Muze to bejt '&regexp'?
	// source = string proti kterymu se bude regexp matchovat
	// nmatch = kolik polozek ma byt vraceno (ja vim, trochu kostrbaty). Celkem jich je o jednu vic, nulta polozka je cely retezec, ktery se s regexpem matchuje
	// &result = kam se polozky ulozej
	regmatch_t pmatch[nmatch+2];	// Tohle nevim jestli se smi, vypada to dost drze
	int i, i_max;

	i_max = nmatch+1;
	result.clear(); // Odstraneni pripadnyho bordelu
	if (regexec(&regexp, source.c_str(), i_max, pmatch, 0) == 0) {
		for (i=0 ; i < i_max ; i++) {
			if (pmatch[i].rm_eo != -1) {
				result.push_back(source.substr(pmatch[i].rm_so,pmatch[i].rm_eo-pmatch[i].rm_so));
			} else {
				// Nutne. Pokud je substring prazdny, tak se taky potrebuje prenest aby bylo zachovano poradi substringu. Viz 'i_overview_strip'
				result.push_back("");
			}
		}
		return nmatch;
	} 
	return 0;
}


// Jednoradkova hledani: jsou ve tvaru i_* (bez suffixu) a jsou pouzite pro regexp
// Viceradkova hlednani: i_*_begin a i_*_end jsou pouzity pro find(), i_*_strip jsou pouzity regexp
#define i_results_begin "<div class=\"page-content\" id=\"pg-web-film\">"
#define i_poster "<img src=\"(.*)\\?h180\" alt=\"poster\" class=\"film-poster\".*/>"
#define i_name_begin "<h1>"
#define i_name_end "</h1>"
#define i_name_strip "<h1>[[:space:]]*([^[:space:]].*[^[:space:]])[[:space:]]*</h1>"
#define i_genre "<p class=\"genre\">(.*)</p>"
#define i_year "<p class=\"origin\">[^,]*, ([0-9]{4}),"
#define i_director_begin "<h4>Režie:</h4>"
#define i_director_end "</span>"
#define i_director_strip "<a href=\".*\">(.*)</a>"
#define i_actors_begin "<h4>Hrají:</h4>"
#define i_actors_end "</span>"
#define i_actors_strip "<a href=\".*\">(.*)</a>"
#define i_covers_begin "<h3>Plakáty</h3>"
#define i_covers_end "</tr>"
#define i_covers_strip "url\\('(.*)\\?h180'\\)"
#define i_overview_begin "<h3>Obsah </h3>"
#define i_overview_end "</li>"
#define i_overview_strip "alt=\"Odrážka\"[[:space:]]*class=\"dot( hidden)?\"/>[[:space:]]*(.*)<span class=\"source( user)?\">(.*)</span>"
#define i_fanart_begin "Galerie\t<span class=\"count\">"
#define i_fanart_end "<div class=\"footer\"></div>"
#define i_fanart_strip "<div class=\"photo\" style=\".*background-image: url\\('(.*)\\?w700'\\);"
#define i_rate "<h2 class=\"average\">([0-9]+)%</h2>"
int ParseInfo(const char * html, struct InfoResult * p)
{
	string line, temp_string;
	ifstream myfile(html);
	vector <string> reg_fields;
	vector <string> fields;
	int state = I_UNKNOWN;
	int i;
	int cover_i=0;
	int fanart_i=0;

	regex_t re_poster;
	regex_t re_name_strip;
	regex_t re_genre;
	regex_t re_year;
	regex_t re_director_strip;
	regex_t re_actors_strip;
	regex_t re_covers_strip;
	regex_t re_overview_strip;
	regex_t re_fanart_strip;
	regex_t re_rate;

	if (regcomp(&re_poster, i_poster, REG_EXTENDED)) { return -1; }
	if (regcomp(&re_name_strip, i_name_strip, REG_EXTENDED)) { return -1; }
	if (regcomp(&re_genre, i_genre, REG_EXTENDED)) { return -1; }
	if (regcomp(&re_year, i_year, REG_EXTENDED)) { return -1; }
	if (regcomp(&re_director_strip, i_director_strip, REG_EXTENDED)) { return -1; }
	if (regcomp(&re_actors_strip, i_actors_strip, REG_EXTENDED)) { return -1; }
	if (regcomp(&re_covers_strip, i_covers_strip, REG_EXTENDED)) { return -1; }
	if (regcomp(&re_overview_strip, i_overview_strip, REG_EXTENDED)) { return -1; }
	if (regcomp(&re_fanart_strip, i_fanart_strip, REG_EXTENDED)) { return -1; }
	if (regcomp(&re_rate, i_rate, REG_EXTENDED)) { return -1; }

	if (myfile.is_open()) {

		while (myfile.good()) {
			getline(myfile,line);

			switch (state) {
				case I_LOOK4POSTER:
					if (reg_match(re_poster, line, 1, reg_fields)) {
						if (reg_fields[1].find("http", 0, 4) != 0)
							reg_fields[1].insert(0,"http:");
						p->cover_preview[cover_i] = strdup(reg_fields[1].c_str());
						p->cover[cover_i++] = strdup(reg_fields[1].c_str());
						state = I_LOOK4NAME;
					}
					break;
				case I_LOOK4NAME:
					if (line.find(i_name_begin) != string::npos) {
						temp_string = line;
						state = I_GETTING_NAME;
					}
					break;
				case I_GETTING_NAME:
					temp_string.append(line);
					if (line.find(i_name_end) != string::npos) {
						if (reg_match(re_name_strip, temp_string, 1, reg_fields))
							p->name = strdup(RemoveString(reg_fields[1], "<", ">").c_str());
						state = I_LOOK4GENRE;
					}
					break;
				case I_LOOK4GENRE:
					if (reg_match(re_genre, line, 1, reg_fields)) {
						fields = split(reg_fields[1], " /", split::no_empties);
						for (i = 0; i < (fields.size() < JB_SCPR_MAX_GENRE ? fields.size() : JB_SCPR_MAX_GENRE); i++)
							p->genres[i] = strdup(fields[i].c_str());
						state = I_LOOK4YEAR;
					}
					break;
				case I_LOOK4YEAR:
					if (reg_match(re_year, line, 1, reg_fields)) {
						p->year = atoi(reg_fields[1].c_str());
						state = I_LOOK4DIRECTOR;
					}
					break;
				case I_LOOK4DIRECTOR:
					if (line.find(i_director_begin) != string::npos) {
						temp_string = line;
						state = I_GETTING_DIRECTOR;
					}
					break;
				case I_GETTING_DIRECTOR:
					temp_string.append(line);
					if (line.find(i_director_end) != string::npos) {
						if (reg_match(re_director_strip, temp_string, 1, reg_fields))
							p->director=strdup(reg_fields[1].c_str());
						state = I_LOOK4ACTORS;
					}
					break;
				case I_LOOK4ACTORS:
					if (line.find(i_actors_begin) != string::npos) {
						temp_string.clear();
						state = I_GETTING_ACTORS;
					}
					break;
				case I_GETTING_ACTORS:
					if (line.find(i_actors_end) != string::npos) {
						fields = split(temp_string, ",", split::no_empties);
						for (i = 0; i < (fields.size() < JB_SCPR_MAX_ACTOR ? fields.size() : JB_SCPR_MAX_ACTOR); i++) {
							if (reg_match(re_actors_strip, fields[i], 1, reg_fields))
								p->name_actor[i] = strdup(reg_fields[1].c_str());
						}
						state = I_LOOK4COVERS;
					} else {
						temp_string.append(line);
					}
					break;
				case I_LOOK4COVERS:
					if (line.find(i_covers_begin) != string::npos)
						state = I_GETTING_COVERS;
					// Pozor, covers se vubec nemusi vyskytovat. Koukam zaroven i na zacatek overview
					if (line.find(i_overview_begin) != string::npos) {
						temp_string.clear();
						state = I_GETTING_OVERVIEW;
					}
					break;
				case I_GETTING_COVERS:
					if (line.find(i_covers_end) != string::npos) {
						state = I_LOOK4OVERVIEW;
					} else {
						if (reg_match(re_covers_strip, line, 1, reg_fields)) {
							reg_fields[1] = RemoveString(reg_fields[1], "\\");
							if (reg_fields[1].find("http", 0, 4) != 0)
								reg_fields[1].insert(0,"http:");
							if (cover_i < JB_SCPR_MAX_IMAGE) {
								p->cover_preview[cover_i] = strdup(reg_fields[1].c_str());
								p->cover[cover_i++] = strdup(reg_fields[1].c_str());
							}
						}
					}
					break;
				case I_LOOK4OVERVIEW:
					if (line.find(i_overview_begin) != string::npos) {
						temp_string.clear();
						state = I_GETTING_OVERVIEW;
					}
					break;
				case I_GETTING_OVERVIEW:
					temp_string.append(line);
					if (line.find(i_overview_end) != string::npos) {
						if (reg_match(re_overview_strip, temp_string, 4, reg_fields)) {
							temp_string = RemoveString(reg_fields[2] + reg_fields[4], "\r"); // Normalne, fakt se ^M v 'overview' objevil :-O
							temp_string = RemoveString(temp_string, "\t"); // Tohle je pro sichr, kdyz tam muze bejt ten '\r' tak se uz ani nedivim
							temp_string = RemoveString(temp_string, "<", ">"); // Dobra, odstraneni vseruznejch HTML tagu co tam nemaj co delat
							p->overview = strdup(temp_string.c_str());
							p->summary = strdup(temp_string.c_str());
						}
						state = I_LOOK4FANART;
					}
					break;
				case I_LOOK4FANART:
					if (line.find(i_fanart_begin) != string::npos)
						state = I_GETTING_FANART;
					// Pozor, fanart se nemusi vubec vyskytovat. Koukam zaroven i na zacatek ratingu
					if (reg_match(re_rate, line, 1, reg_fields)) {
						p->rate = atoi(reg_fields[1].c_str());
						state = I_HOTOVO;
					}
					break;
				case I_GETTING_FANART:
					if (line.find(i_fanart_end) != string::npos) {
						state = I_LOOK4RATE;
					} else {
						if (reg_match(re_fanart_strip, line, 1, reg_fields)) {
							reg_fields[1] = RemoveString(reg_fields[1], "\\");
							if (reg_fields[1].find("http", 0, 4) != 0)
								reg_fields[1].insert(0,"http:");
							p->fanart_preview[fanart_i] = strdup(reg_fields[1].c_str());
							p->fanart[fanart_i++] = strdup(reg_fields[1].c_str());
						}
					}
					break;
				case I_LOOK4RATE:
					if (reg_match(re_rate, line, 1, reg_fields)) {
						p->rate = atoi(reg_fields[1].c_str());
						state = I_HOTOVO;
					}
					break;
				default: // Vcetne I_UNKNOWN
					if (line.find(i_results_begin) != string::npos)
						state = I_LOOK4POSTER;
			}

			if (state == I_HOTOVO)
				break; // Dal uz neni nic zajimaveho
		}

		myfile.close();
	}

	else cout << "Unable to open file";

	regfree(&re_poster);
	regfree(&re_name_strip);
	regfree(&re_genre);
	regfree(&re_year);
	regfree(&re_director_strip);
	regfree(&re_actors_strip);
	regfree(&re_covers_strip);
	regfree(&re_overview_strip);
	regfree(&re_fanart_strip);
	regfree(&re_rate);

	printf("[ CSFD ] parse done (%d)\n", state);
	return 0;
}

#define s_results_begin "<h2 class=\"header\">Filmy</h2>"
#define s_results_ends "</ul>"
// #define interest_item "<h3 class=\"subject\"><a href=\"/film/(.*)/\" .*c1\">(.*)</a>"; // class="film c1" = presna shoda nazvu? Protoze class="film c2" obsahuje jen podobny nazev
#define interest_item "<h3 class=\"subject\"><a href=\"/film/(.*)/\" .*\">(.*)</a>"
#define interest_year "<p>.* ([0-9]{4})</p>"
#define interest_single_item "<a href=\"/film/([0-9]+-)([^/]*)/zajimavosti/"
#define interest_single_year "<p class=\"origin\">[^,]*, ([0-9]{4}),.*</p>"
int ParseSearch(const char * html, struct SearchResult * p)
{
	string line;
	ifstream myfile(html);
	vector <string> reg_fields;
	int state = S_UNKNOWN;
	int nResults = 0;

	regex_t re_interest_item;
	regex_t re_interest_single_item;
	regex_t re_interest_year;
	regex_t re_interest_single_year;

	if (regcomp(&re_interest_item, interest_item, REG_EXTENDED)) { return -1; }
	if (regcomp(&re_interest_single_item, interest_single_item, REG_EXTENDED)) { return -1; }
	if (regcomp(&re_interest_year, interest_year, REG_EXTENDED)) { return -1; }
	if (regcomp(&re_interest_single_year, interest_single_year, REG_EXTENDED)) { return -1; }

	if (myfile.is_open()) {
		while (myfile.good()) {
			getline (myfile,line);
			switch (state) {
				case S_LOOK4IDNAME:
					if (reg_match(re_interest_item, line, 2, reg_fields)) {
						p->results[nResults].id = strdup(reg_fields[1].c_str());
						p->results[nResults].name = strdup(reg_fields[2].c_str());
						state = S_LOOK4YEAR; // Dal by mel nasledovat rok filmu
					}
					break;
				case S_LOOK4YEAR:
					if (reg_match(re_interest_year, line, 1, reg_fields)) {
						// Nalezen rok filmu
						p->results[nResults++].year = atoi(reg_fields[1].c_str());
						state = S_LOOK4IDNAME; // Dal uz neni nic, jen nazev a id dalsiho filmu
					}
					break;
				case S_LOOK4SINGLE_IDNAME:
					if (reg_match(re_interest_single_item, line, 2, reg_fields)) {
						// Nalezeno 'id' a 'name' single filmu
						p->results[nResults].name = strdup(reg_fields[2].c_str());
						p->results[nResults++].id = strdup((reg_fields[1]+reg_fields[2]).c_str());
						state = S_HOTOVO; // Dal uz neni treba nic hledat
					}
					break;
				default: // Vcetne S_UNKNOWN a S_LOOK4SINGLE_YEAR
					// TODO: Spolehlive rozlisit zda jde o vysledek hledani nebo detail filmu (coz je stav kdy csfd.cz najde jen jeden film)
					//       Pak by se dalo pouzit vyhledavani nazvu filmu stejne jako v ParseInfo() = mel by spravne diakritiku a tak.
					if (line.find(s_results_begin) != string::npos) {
						state = S_LOOK4IDNAME;
					} else if (reg_match(re_interest_single_year, line, 1, reg_fields)) {
						// Nalezen rok single filmu = technicky jde o S_LOOK4SINGLE_YEAR
						p->results[nResults].year = atoi(reg_fields[1].c_str());
						state = S_LOOK4SINGLE_IDNAME;
					}
			}

			if (state == S_LOOK4IDNAME && line.find(s_results_ends) != string::npos)
				state = S_HOTOVO; // Vypada to silene, ale alespon clovek vi, ze pokud je S_HOTOVO, tak slo vsechno podle planu

			if (state == S_HOTOVO) {
				// Dal uz neni nic zajimaveho
				break;
			}
		}
		myfile.close();
	} else {
		cout << "Unable to open file";
	}

	regfree(&re_interest_item);
	regfree(&re_interest_single_item);
	regfree(&re_interest_year);
	regfree(&re_interest_single_year);
	p->nResults=nResults;

	printf("[ CSFD ] search done (%d)\n", state);

	return nResults;
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

	cout << "csfd scraper";
#ifdef SVN_REV
	cout << " (svn rev " << SVN_REV << ")";
#endif
	cout << endl;

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
		sprintf(cmd, "wget -t 1 -T 30 -nd \"%s/film/%s/galerie/\" -O %s", LINK, keyword, TMPFILE);
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
