import numpy as np
import sys

sorted = np.sort(np.loadtxt(sys.argv[1]))
yvals = np.arange(len(sorted))/float(len(sorted))
f = open(sys.argv[1] + '.cdf', 'w')
print sorted
print yvals
for (x, y) in zip(sorted, yvals):
  f.write('%f\t%f\n' % (x, y))
f.close()