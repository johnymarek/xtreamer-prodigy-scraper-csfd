#define MAX_SEARCH_RESULT	32
struct SearchResultOne {
	/* name used to indicate for user */
	char * name;
	/* year of the movie released, used to indicate for user */
	int year;
	/* id of the movie, and will be used to get the movie info */
	char * id;
};
struct SearchResult {
	int nResults;
	struct SearchResultOne results[MAX_SEARCH_RESULT];
};

#define JB_SCPR_MAX_GENRE	32
#define JB_SCPR_MAX_ACTOR	32
#define JB_SCPR_MAX_IMAGE	64
struct InfoResult {
	/* movie name */
	char * name;
	/* movie overview */
	char * overview;
	/* movie summary */
	char * summary;
	/* imdb ID */
	char * imdb_id;
	/* rate x10, eg. for 7.8/10, rate=78 */
	int rate;
	/* how many people votes */
	int votes;
	/* company name */
	char * company;
	/* names of the genre, 
	 * please remove the blank characters like space etc */
	char * genres[JB_SCPR_MAX_GENRE];
	/* money used */
	int budget;
	/* money earned */
	int revenue;
	/* year released */
	int year;
	/* name of the actor, and the character name in the movie is stored in name_char  
	 * please keep the sequence */
	char * name_actor[JB_SCPR_MAX_ACTOR];
	char * name_char[JB_SCPR_MAX_ACTOR];
	/* name of director */
	char * director;
	/* image links, used to for the menu changing covers
	 * user can pick it and then will download the full size image which will be stored in cover 
	 * please keep the sequence */
	char * cover_preview[JB_SCPR_MAX_IMAGE];
	char * cover[JB_SCPR_MAX_IMAGE];
	/* image links, used to for the menu changing fanarts
	 * user can pick it and then will download the full size image which will be stored in fanart
	 * please keep the sequence */
	char * fanart_preview[JB_SCPR_MAX_IMAGE];
	char * fanart[JB_SCPR_MAX_IMAGE];

};

int GenSearchResult(struct SearchResult *p, const char * file);
int GenInfoResult(struct InfoResult *p, const char * file);

/* language will be sent following ISO 639 
{ 
  {'en', STRING_ENGLISH}, 
  {'zh', STRING_CHINESE}, 
  {'ja', STRING_JAPANESE},
  {'es', STRING_SPANISH},
  {'fr', STRING_FRENCH},
  {'de', STRING_GERMAN}, 
  {'it', STRING_ITALIAN}, 
  {'in', STRING_INDONESIAN},
  {'ko', STRING_KOREAN}, 
  {'th', STRING_THAI},      
  {'pt', STRING_PORTUGUESE}, 
  {'nl', STRING_DUTCH},
  {'cs', STRING_CZECH},
  {'hu', STRING_HUNGARIAN},  
  {'ru', STRING_RUSSIAN},
  {'pl', STRING_POLISH},
  {'fi', STRING_FINNISH},
  {'no', STRING_NORWEGIAN},
  {'sv', STRING_SWEDISH},
  {'da', STRING_DANISH},
  {'el', STRING_GREEK},
  {'ga', STRING_IRISH},
  {'hi', STRING_HINDI},
  {'sl', STRING_SLOVENIAN},
  {'iw', STRING_HEBREW}, 
  {'he', STRING_HEBREW},  
  {'tr', STRING_TURKISH},
  {'ro', STRING_ROMANIAN},
  {'ar', STRING_ARABIC},
  {'bg', STRING_BULGARIAN},
  {'la', STRING_LATIN},
  {'lt', STRING_LITHUANIAN},
  {'lv', STRING_LATVIAN},
  {'ms', STRING_MALAY},
  {'my', STRING_BURMESE},
  {'ne', STRING_NEPALI},
  {'sk', STRING_SLOVAK}, 
  {'sq', STRING_ALBANIAN}, 
  {'sr', STRING_SERBIAN}, 
  {'uk', STRING_UKRAINIAN},
  {'vi', STRING_VIETNAMESE},
  {'hr', STRING_CROATIAN},
  {'hy', STRING_ARMENIAN},  
  {'is', STRING_ICELANDIC},  
  {'km', STRING_CAMBODIAN},
  {'et', STRING_ESTONIAN}
};

 * */
