#!/usr/bin/env python
# -*- coding: utf-8 -*-
import difflib, os

###########################################################
###	NOTE:						###
###	If you are using the official testing files,	###
###	Rename them from WRR to RR.			###
###########################################################
### Ex. '..\Release\ComputerCommunications2.1.exe RR .\IO_files\inp1.txt .\our_files\test1_RR_w1_q0.txt 1 0'
###########################################################

FOLDER_SRC = '.\IO_files'
FOLDER_DST = '.\our_files'
EXECUTABLE = '..\Release\ComputerCommunications2.1.exe'
tests = [[['DRR',  1,  2],
	  ['DRR',  3,  2],
	  ['RR' ,  1,  0],
	  ['RR' ,  3,  0]],
	 [['DRR',  1, 10],
	  ['DRR',  8, 10],
	  ['RR' ,  1,  0],
	  ['RR' ,  8,  0]],
	 [['DRR',  1, 40],
	  ['DRR', 30, 40],
	  ['RR' ,  1,  0],
	  ['RR' , 30,  0]]]
GREEN = '\033[92m'
RED = '\033[91m'
ENDC = '\033[0m'

for indx in range(len(tests)):
	for test in tests[indx]:
		file_src = '{}\inp{}.txt'.format(FOLDER_SRC, indx+1)
		file_cmpr = '{src}\out{indx}_{algo}_w{weight}_q{quantum}.txt'.format(algo=test[0], weight=test[1], quantum=test[2], src=FOLDER_SRC, dst=FOLDER_DST, indx=indx+1)
		file_dst = '{dst}\test{indx}_{algo}_w{weight}_q{quantum}.txt'.format(algo=test[0], weight=test[1], quantum=test[2], src=FOLDER_SRC, dst=FOLDER_DST, indx=indx+1)
		command = '{exe} {algo} {src} {dst} {weight} {quantum}'.format(exe=EXECUTABLE, algo=test[0], src=file_src, dst=file_dst, weight=test[1], quantum=test[2])
		os.system(command)
		try:
			if difflib.SequenceMatcher(None, open(file_cmpr).read(), open(file_dst).read()).ratio() == 1.0:
				res = GREEN
			else:
				res = RED
		except:
			res = RED
		print('{}{}{}'.format(res, command, ENDC))

