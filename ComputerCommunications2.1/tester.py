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
		file_cmpr = '{_src}\\out{_indx}_{_algo}_w{_weight}_q{_quantum}.txt'.format(_algo=test[0], _weight=test[1], _quantum=test[2], _src=FOLDER_SRC, _indx=indx+1)
		file_dst = '{_dst}\\test{_indx}_{_algo}_w{_weight}_q{_quantum}.txt'.format(_algo=test[0], _weight=test[1], _quantum=test[2], _dst=FOLDER_DST, _indx=indx+1)
		file_dst2 = '{_dst}\\test{_indx}'.format(_dst=FOLDER_DST, _indx=(indx+1))
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

