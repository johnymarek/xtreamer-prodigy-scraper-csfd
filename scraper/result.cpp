#include <stdio.h>
#include "result.h"

#if 0

<?xml version="1.0" encoding="UTF-8"?>
<searchresult>
    <movie>
        <id>19995</id>
        <name>Avatar</name>
        <year>2009</year>
    </movie>
</searchresult>

#endif
int GenSearchResult(struct SearchResult *result, const char * file)
{
	FILE * pf;
	if (result->nResults <= 0)
		return -1;
	pf = fopen(file, "w+");
	if (pf == NULL)
		return -1;

	/* write to XML file */
	fprintf(pf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(pf, "<search>\n");
	for (int i=0; i<result->nResults; i++) {
		fprintf(pf, "<movie>\n");
		fprintf(pf, "<id><![CDATA[%s]]></id>\n", (result->results[i].id));
		fprintf(pf, "<name><![CDATA[%s]]></name>\n", (result->results[i].name));
		fprintf(pf, "<year>%d</year>\n", (result->results[i].year));
		fprintf(pf, "</movie>\n");
	}
	fprintf(pf, "</search>\n");


	fclose(pf);

	return 0;
}

int GenInfoResult(struct InfoResult *result, const char * file)
{
	FILE * pf;
	pf = fopen(file, "w+");
	if (pf == NULL)
		return -1;

	/* write to XML file */
	fprintf(pf, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	fprintf(pf, "<movie>\n");
	fprintf(pf, "<title><![CDATA[%s]]></title>\n", (result->name));
	fprintf(pf, "<plot><![CDATA[%s]]></plot>\n", (result->overview));
	fprintf(pf, "<tagline><![CDATA[%s]]></tagline>\n", (result->summary));
	fprintf(pf, "<year>%d</year>\n", (result->year));
	fprintf(pf, "<id>%s</id>\n", (result->imdb_id));
	fprintf(pf, "<rating>%d</rating>\n", (result->rate));
	fprintf(pf, "<votes>%d</votes>\n", (result->votes));
	fprintf(pf, "<budget>%d</budget>\n", (result->budget));
	fprintf(pf, "<revenue>%d</revenue>\n", (result->revenue));
	fprintf(pf, "<company><![CDATA[%s]]></company>\n", (result->company));
	fprintf(pf, "<genre>\n");
	for (int i=0; i<JB_SCPR_MAX_GENRE; i++) {
		if (result->genres[i]) {
			fprintf(pf, "<name><![CDATA[%s]]></name>\n", (result->genres[i]));
		}
	}
	fprintf(pf, "</genre>\n");
	fprintf(pf, "<director>\n<name><![CDATA[%s]]></name>\n</director>\n", (result->director));
	fprintf(pf, "<actor>\n");
	for (int i=0; i<JB_SCPR_MAX_ACTOR; i++) {
		if (result->name_actor[i]) {
			const char *ch = "Unknown";
			if (result->name_char[i] != NULL)
				ch = result->name_char[i];
			fprintf(pf, "<name><![CDATA[%s]]></name>\n", result->name_actor[i]);
		}
	}
	fprintf(pf, "</actor>\n");
	fprintf(pf, "<cover>\n");
	for (int i=0; i<JB_SCPR_MAX_IMAGE; i++) {
		if (result->cover[i]) {
			if (result->cover_preview[i])
				fprintf(pf, "<name preview=\"%s\"><![CDATA[%s]]></name>\n", 
						result->cover_preview[i], (result->cover[i]));
			else
				fprintf(pf, "<name><![CDATA[%s]]></name>\n", 
						(result->cover[i]));
		}
	}
	fprintf(pf, "</cover>\n");
	fprintf(pf, "<backdrop>\n");
	for (int i=0; i<JB_SCPR_MAX_IMAGE; i++) {
		if (result->fanart[i]) {
			if (result->fanart_preview[i])
				fprintf(pf, "<name preview=\"%s\"><![CDATA[%s]]></name>\n", 
						result->fanart_preview[i], (result->fanart[i]));
			else
				fprintf(pf, "<name><![CDATA[%s]]></name>\n", 
						(result->fanart[i]));
		}
	}
	fprintf(pf, "</backdrop>\n");
	fprintf(pf, "</movie>\n");


	fclose(pf);

	return 0;
}
