#!/usr/bin/env python
import sys
def globalMatches(n=0):
	count=0
	for a in xrange(n):
		for b in xrange(n):
			if not a==b:
				count+=1
	return count/2
def estimateTime(count,average_time=0.1):
	seconds = count*average_time
	m,s = divmod(seconds,60)
	h,m = divmod(m,60)
	return (s,m,h)

if __name__=='__main__':
	matches = globalMatches(int(sys.argv[1]))
	print estimateTime(matches,float(sys.argv[2]))
