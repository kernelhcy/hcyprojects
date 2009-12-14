
***** Welcome to use Asimo search engine! *****

This is a prototype of search engine. It's very simple, but implements the main principle of current search engine.
It is just for beginner(and myslef) to learn the principle of search engine easily.

This search engine implements several tasks followed:
	- a configurable and multi-thread web-spider
	- a bloom filter
	- a indexer deal with urls and words information
	- a simplest accidence ananlyzer, only can process English word segmentation, NOTE: it does not support Chinese now!
	- two types rankers, including content-based ranker and pagerank-based ranker
	- a simplest query class, supporting 'and' and 'or' opeartion
	- database storage with original webpage info and index info
	
Additional parts:
	- a python build-in server
	- a progress meter to display index and pagerank processes
	- a self-defined htmlparser
	
Usage:
1, enter the main directory
2, start your server by double-click myserver.py script
3, enter in your web blower's address window: http://localhost:8000/index.htm
	
Copyright:
# -------------------------------------------------------------
# Author:       Chen Zhihui (zaviichen@gmail.com)
# Affiliation:  Fudan University
# Date:         11/28/2009
# Version:      1.0
# Copyright (c) 2009 by Chen Zhihui. All Rights Reserved.
# -------------------------------------------------------------

Dirctory introduction
 / 
  |-- __init__.py: pythmon module specification
  |
  |-- config.xml:  configure xml file, if you want to using command to exec python core script, 
  |                please move this file to the core/ directory.
  |
  |-- index.db:    index database, created by index.py script. (Maynot included)
  |
  |-- original.db: original database, created by webspider.py scripy to store the original webpage info. (Maynot included)
  |
  |-- index.htm:   homepage for the server.
  |
  |-- myserver.py: simple server script.
  |
  |-- readme.txt:  this introduction text.
  |
  |-- cgi-bin/
  |       |-- search_cgi.py: search cgi script
  |       | 
  |       `-- cache.py:      cache cgi script
  |
  |-- core/   
  |       |-- __init__.py:   python module specification
  |       |                  
  |       |-- analyzer.py:   accidence ananlyzer
  |       |                  
  |       |-- bloom.py:      bloom filter
  |       |                  
  |       |-- config.py:     configure file parser
  |       |                  
  |       |-- htmlparser:    self-defined HTML parser
  |       |                  
  |       |-- index.py:      indexer
  |       |                  
  |       |-- progress.py:   progress meter in the command window
  |       |                  
  |       |-- query.py:      analyze the user's query condition and get the results
  |       |                  
  |       |-- rank.py:       ranker, include content-based ranker and pagerank-based ranker 
  |       |                  
  |       |-- secore.py:     search engine interface
  |       |                 
  |       |-- utils.py:      valuable variable-definitions and functions
  |       |                  
  |       `-- webspider.py:  configurable and multi-thread webspider
  |
  |-- template/
  |       |-- result.htm :     whole results display html template read by python cgi script
  |       |
  |       |-- result_item.htm: each result display html template
  |       |
  |       `-- asimo.gif:       search engine's logo
 
Advanced tools:
Lucene:    Apache Lucene is a high-performance, full-featured text search engine library written entirely in Java. 
           It is a technology suitable for nearly any application that requires full-text search, especially cross-platform.
           homepage: http://lucene.apache.org/
PyLucene:  PyLucene is a Python extension for accessing Java Lucene.
           PyLucene is not a Lucene port but a Python wrapper around Java Lucene. 
           homepage: http://lucene.apache.org/pylucene/

Just for fun!