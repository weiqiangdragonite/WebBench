# WebBench

## Introduction

Web Bench 1.5

Web Bench is very simple tool for benchmarking WWW or proxy servers. Uses 
fork() for simulating multiple clients and can use HTTP/0.9-HTTP/1.1 requests. 
This benchmark is not very realistic, but it can test if your HTTPD can realy 
handle that many clients at once (try to run some CGIs) without taking your 
machine down. Displays pages/min and bytes/sec. Can be used in more aggressive 
mode with -f switch.

## Download

[Click me](http://home.tiscali.cz/~cz210552/webbench.html)

## Install

	# tar -zxvf webbench-1.5.tar.gz
	# cd webbench-1.5
	# make && make install

## Usage

	# webbench --help
	webbench [option]... URL
	-f|--force               Don't wait for reply from server.
	-r|--reload              Send reload request - Pragma: no-cache.
	-t|--time <sec>          Run benchmark for <sec> seconds. Default 30.
	-p|--proxy <server:port> Use proxy server for request.
	-c|--clients <n>         Run <n> HTTP clients at once. Default one.
	-9|--http09              Use HTTP/0.9 style requests.
	-1|--http10              Use HTTP/1.0 protocol.
	-2|--http11              Use HTTP/1.1 protocol.
	--get                    Use GET request method.
	--head                   Use HEAD request method.
	--options                Use OPTIONS request method.
	--trace                  Use TRACE request method.
	-?|-h|--help             This information.
	-V|--version             Display program version.


常用参数说明，-c 表示客户端数，-t 表示时间

	webbench -c 并发数 -t 运行测试时间 URL

测试实例

	# webbench -c 500 -t 30 http://127.0.0.1/phpionfo.php

测试静态图片

	# webbench -c 500 -t 30 http://127.0.0.1/test.jpg

webbench测试结果

	Webbench – Simple Web Benchmark 1.5
	Copyright (c) Radim Kolar 1997-2004, GPL Open Source Software.
	
	Benchmarking: GET http://127.0.0.1/phpionfo.php
	500 clients, running 30 sec.
	
	Speed=3230 pages/min, 11614212 bytes/sec.
	Requests: 1615 susceed, 0 failed.

分析：每秒钟响应请求数：3230 pages/min，每秒钟传输数据量11614212 bytes/sec.
