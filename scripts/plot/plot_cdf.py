import numpy as np
import matplotlib.pyplot as plt
import sys
import os

def save(path, ext='png', close=True, verbose=True):
    """Save a figure from pyplot.
    Parameters
    ----------
    path : string
        The path (and filename, without the extension) to save the
        figure to.
    ext : string (default='png')
        The file extension. This must be supported by the active
        matplotlib backend (see matplotlib.backends module).  Most
        backends support 'png', 'pdf', 'ps', 'eps', and 'svg'.
    close : boolean (default=True)
        Whether to close the figure after saving.  If you want to save
        the figure multiple times (e.g., to multiple formats), you
        should NOT close it in between saves or you will have to
        re-plot it.
    verbose : boolean (default=True)
        Whether to print information about when and where the image
        has been saved.
    """
    
    # Extract the directory and filename from the given path
    directory = os.path.split(path)[0]
    filename = "%s.%s" % (os.path.split(path)[1], ext)
    if directory == '':
        directory = '.'

    # If the directory does not exist, create it
    if not os.path.exists(directory):
        os.makedirs(directory)

    # The final path to save to
    savepath = os.path.join(directory, filename)

    if verbose:
        print("Saving figure to '%s'..." % savepath),

    # Actually save the figure
    plt.savefig(savepath)
    
    # Close it
    if close:
        plt.close()

    if verbose:
        print("Done")

col = int(sys.argv[2])
resfile = sys.argv[1]
orig_data = np.loadtxt(resfile)
if isinstance(orig_data[0], np.ndarray):
    data = [x[col] for x in orig_data]
else:
    data = [x for x in orig_data]

sorted = np.sort(data)
yvals = np.arange(len(sorted))/float(len(sorted))
plt.xlabel('Latency (ns) for writes')
plt.ylabel('CDF')
plt.plot(sorted, yvals)
# plt.show()
# fig = plt.figure()
# fig.savefig(resfile + '.eps', format='eps')
save(resfile, ext='png', close=True, verbose=True)