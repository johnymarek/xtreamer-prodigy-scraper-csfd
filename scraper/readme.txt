i'll find all the files under plugin folder, and will treat them as the scrappers. 
when doing searching, will call like this (eg, tmdb): ./plugin/tmdb -s -l en -k "keyword" -o /tmp/jukebox/scraper/search.xml. and will use the results in search.xml.
when doing scraping, will call like this (eg, tmdb): ./plugin/tmdb -l en -k "keyword" -o /tmp/jukebox/scrapper/number.info.xml. and the keyword here will use the id we got in searching.
here is the 2 scraper i made: torec & tmdb.
